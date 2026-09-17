#include "hypr_window_source.hpp"

#include <algorithm>
#include <cstdint>
#include <utility>

#include <hyprland/src/SharedDefs.hpp>
#include <hyprland/src/desktop/DesktopTypes.hpp>
#include <hyprland/src/desktop/history/WindowHistoryTracker.hpp>
#include <hyprland/src/desktop/state/FocusState.hpp>
#include <hyprland/src/desktop/view/Window.hpp>
#include <hyprland/src/output/Monitor.hpp>
#include <hyprland/src/state/MonitorState.hpp>

#include "mru/domain/scope_predicate.hpp"
#include "mru_merge.hpp"

namespace mru::plugin {
namespace {

// REQ-SNAP-002 (M2 subset): a window is a candidate when it is live, mapped,
// and not hidden (m_isMapped is the pinned-0.56.2 exposed bit; L-8). Unchanged
// by M3-S3: the REQ-SC-002a special-workspace rule is enforced by the domain gate
// inside scope_matches(), not here.
bool is_candidate(const PHLWINDOW &w) {
    return w && w->m_isMapped && !w->isHidden();
}

} // namespace

HyprlandWindowSource::HyprlandWindowSource(WindowIdentityRegistry &registry, const mru::domain::HistoryTracker &tracker)
    : registry_(registry), tracker_(tracker) {}

mru::domain::FocusContext HyprlandWindowSource::current_focus() const {
    mru::domain::FocusContext focus;

    // Single enumeration (REQ-SC-002a): the visible-workspace id set is built
    // once from every enabled monitor's active + active-special workspace, and
    // this same vector is what `visible` matches against and what `hidden` is
    // derived from below — one source of truth, no "adapter visible" vs
    // "domain expected visible" drift by construction.
    for (const auto &m : State::monitorState()->monitors()) {
        if (!m)
            continue;
        const auto active = static_cast<std::uint64_t>(m->activeWorkspaceID());
        focus.visible_workspaces.push_back(active);
        const auto special = m->activeSpecialWorkspaceID();
        if (special != WORKSPACE_INVALID)
            focus.visible_workspaces.push_back(static_cast<std::uint64_t>(special));
    }

    // Focused-window anchors for the monitor/workspace/app scopes. No focus →
    // has_focus stays false and those scopes degrade to global in the domain
    // (REQ-SC-002/002b, M3-S2 pin).
    const auto w = Desktop::focusState()->window();
    if (!w)
        return focus;

    focus.has_focus = true;
    focus.monitor_id = static_cast<std::uint64_t>(w->monitorID());
    focus.workspace_id = static_cast<std::uint64_t>(w->workspaceID());
    focus.app_class = w->m_class; // class(), NOT initialClass() (REQ-SC-002b)
    return focus;
}

mru::domain::WindowMeta HyprlandWindowSource::derive_meta(const PHLWINDOW &w,
                                                          const mru::domain::FocusContext &focus) const {
    mru::domain::WindowMeta meta;
    meta.monitor_id = static_cast<std::uint64_t>(w->monitorID());
    meta.workspace_id = static_cast<std::uint64_t>(w->workspaceID());
    meta.mapped = w->m_isMapped;
    meta.app_class = w->m_class; // byte-exact, case-sensitive (REQ-SC-002b)

    // `hidden` == "on a special workspace not currently shown": derived from the
    // ONE enumerated visible set above, so it can never disagree with the domain's
    // visible_workspaces membership test (REQ-SC-002a). Guard the unmanaged window
    // whose workspace pointer is null.
    meta.hidden = w->m_workspace && w->m_workspace->m_isSpecialWorkspace &&
                  std::find(focus.visible_workspaces.cbegin(), focus.visible_workspaces.cend(), meta.workspace_id) ==
                      focus.visible_workspaces.cend();
    return meta;
}

std::vector<mru::domain::WindowRef> HyprlandWindowSource::candidates(mru::domain::Scope scope) const {
    // Snapshot-time focus, computed once and passed by const ref to every
    // scope_matches() and derive_meta() below (ADR-016 __2__).
    const mru::domain::FocusContext focus = current_focus();

    std::vector<mru::domain::WindowRef> fallback; // newest-first enumeration
    if (const auto history = Desktop::History::windowTracker()) {
        const auto &entries = history->fullHistory(); // oldest -> newest
        fallback.reserve(entries.size());
        for (auto it = entries.rbegin(); it != entries.rend(); ++it) {
            const auto w = it->lock();
            if (!is_candidate(w))
                continue;
            const auto ref = registry_.register_window(w); // register on sight (REQ-H-004b)
            if (mru::domain::scope_matches(scope, derive_meta(w, focus), focus))
                fallback.push_back(ref);
        }
    }
    for (const auto &ref : registry_.live_refs_newest_first()) { // opened post-load, not focused yet
        const auto w = registry_.resolve(ref);
        if (w && is_candidate(w) && mru::domain::scope_matches(scope, derive_meta(w, focus), focus))
            fallback.push_back(ref);
    }

    std::vector<mru::domain::WindowRef> primary; // plugin-owned MRU (REQ-H-004a)
    for (const auto &ref : tracker_.order()) {
        const auto w = registry_.resolve(ref);
        if (w && is_candidate(w) && mru::domain::scope_matches(scope, derive_meta(w, focus), focus))
            primary.push_back(ref);
    }

    return merge_mru_order(primary, fallback);
}

bool HyprlandWindowSource::is_valid(const mru::domain::WindowRef &ref) const {
    return static_cast<bool>(registry_.resolve(ref));
}

std::optional<mru::domain::WindowRef> HyprlandWindowSource::focused() const {
    const auto w = Desktop::focusState()->window();
    if (!w)
        return std::nullopt;
    return registry_.live_ref(w); // single-slot lookup, skips closed (L-7)
}

} // namespace mru::plugin