#include "hypr_window_source.hpp"

#include <hyprland/src/desktop/DesktopTypes.hpp>
#include <hyprland/src/desktop/history/WindowHistoryTracker.hpp>
#include <hyprland/src/desktop/state/FocusState.hpp>
#include <hyprland/src/desktop/view/Window.hpp>

#include "mru_merge.hpp"

namespace mru::plugin {
namespace {

// REQ-SNAP-002 (M2 subset): a window is a candidate when it is live and visible.
bool is_candidate(const PHLWINDOW &w) {
    return w && !w->isHidden();
}

} // namespace

HyprlandWindowSource::HyprlandWindowSource(WindowIdentityRegistry &registry, const PluginConfig &cfg,
                                           const mru::domain::HistoryTracker &tracker)
    : registry_(registry), cfg_(cfg), tracker_(tracker) {}

std::vector<mru::domain::WindowRef> HyprlandWindowSource::candidates(mru::domain::Scope scope) const {
    if (scope != mru::domain::Scope::Global)
        return {}; // M2: global only; M3 adds the other scopes (ADR-015 extension point)

    std::vector<mru::domain::WindowRef> fallback; // newest-first enumeration
    if (const auto history = Desktop::History::windowTracker()) {
        const auto &entries = history->fullHistory(); // oldest -> newest
        fallback.reserve(entries.size());
        for (auto it = entries.rbegin(); it != entries.rend(); ++it) {
            const auto w = it->lock();
            if (!is_candidate(w))
                continue;
            fallback.push_back(registry_.register_window(w)); // register on sight (REQ-H-004b)
        }
    }
    for (const auto &ref : registry_.live_refs_newest_first())
        fallback.push_back(ref); // opened post-load, not focused yet

    std::vector<mru::domain::WindowRef> primary; // plugin-owned MRU (REQ-H-004a)
    for (const auto &ref : tracker_.order()) {
        if (const auto w = registry_.resolve(ref); w.has_value() && is_candidate(*w))
            primary.push_back(ref);
    }

    return merge_mru_order(primary, fallback);
}

bool HyprlandWindowSource::is_valid(const mru::domain::WindowRef &ref) const {
    return registry_.resolve(ref).has_value();
}

std::optional<mru::domain::WindowRef> HyprlandWindowSource::focused() const {
    const auto w = Desktop::focusState()->window();
    if (!w)
        return std::nullopt;
    if (!registry_.is_known(address_of(w)))
        return std::nullopt;
    return registry_.last_ref(w);
}

} // namespace mru::plugin