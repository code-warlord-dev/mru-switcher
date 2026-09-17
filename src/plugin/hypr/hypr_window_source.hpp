#pragma once

#include <optional>
#include <vector>

#include "identity_registry.hpp"
#include "mru/domain/focus_context.hpp"
#include "mru/domain/history_tracker.hpp"
#include "mru/domain/window_meta.hpp"
#include "mru/domain/window_source.hpp"

namespace mru::plugin {

// WindowSource adapter over the pinned compositor (ADR-007). The plugin-owned
// HistoryTracker order is the primary candidate order (REQ-H-004a, ADR-015); the
// compositor enumeration is a fallback tail and a register-on-sight seed source
// (REQ-H-004b) for windows opened before plugin load.
//
// M3-S3: every candidate path is filtered through the pure domain predicate
// `scope_matches()` (REQ-SC-002). The adapter's only job is the PHLWINDOW →
// WindowMeta translation and the single-per-snapshot FocusContext (ADR-016 __2__);
// the domain stays Hyprland-free (ADR-007).
class HyprlandWindowSource : public mru::domain::WindowSource {
  public:
    HyprlandWindowSource(WindowIdentityRegistry &registry, const mru::domain::HistoryTracker &tracker);
    ~HyprlandWindowSource() override = default;

    std::vector<mru::domain::WindowRef> candidates(mru::domain::Scope scope) const override;
    bool is_valid(const mru::domain::WindowRef &ref) const override;
    std::optional<mru::domain::WindowRef> focused() const override;

  private:
    // Snapshot-time focus anchors + the enumerated visible-workspace id set,
    // captured once per candidates() call (never recomputed per window).
    mru::domain::FocusContext current_focus() const;
    // PHLWINDOW → WindowMeta using the same FocusContext that scope_matches()
    // will see, so `hidden`/`visible_workspaces` share one source of truth.
    mru::domain::WindowMeta derive_meta(const PHLWINDOW &w, const mru::domain::FocusContext &focus) const;

    WindowIdentityRegistry &registry_;
    const mru::domain::HistoryTracker &tracker_;
};

} // namespace mru::plugin