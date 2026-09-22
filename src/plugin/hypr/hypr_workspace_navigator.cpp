#include "hypr_workspace_navigator.hpp"

#include <utility>

#include <hyprland/src/desktop/Workspace.hpp>
#include <hyprland/src/desktop/view/Window.hpp>

namespace mru::plugin {

HyprlandWorkspaceNavigator::HyprlandWorkspaceNavigator(WindowIdentityRegistry &registry) : registry_(registry) {}

void HyprlandWorkspaceNavigator::begin() {
    touches_.clear(); // a new session starts from a clean slate (ADR-026)
}

void HyprlandWorkspaceNavigator::ensure_visible(const mru::domain::WindowRef &ref) {
    // Same identity rules as HyprlandFocusGateway (ADR-006/013): only live,
    // generation-matched windows are actionable. Anything else is a no-op.
    const PHLWINDOW w = registry_.resolve(ref);
    if (!w)
        return;
    const PHLWORKSPACE ws = w->m_workspace;
    if (!ws)
        return;                           // special/scratchpad windows have no regular workspace to show
    const auto mon = w->m_monitor.lock(); // PHLMONITORREF -> SP<Monitor::CMonitor>
    if (!mon)
        return; // unmapped / transient window: no monitor to elevate
    if (mon->m_activeWorkspace == ws)
        return; // already visible: nothing to do (REQ-UI-012: only inactive)

    // Record the pre-elevation workspace exactly once per monitor per session so
    // a Cancelled end can restore it (ADR-026). Later selections on the same
    // monitor re-elevate freely without overwriting the capture.
    bool seen = false;
    for (const MonitorCapture &cap : touches_) {
        if (cap.monitor == mon) {
            seen = true;
            break;
        }
    }
    if (!seen) {
        const PHLWORKSPACE &active = mon->m_activeWorkspace;
        touches_.push_back(MonitorCapture{mon, active ? active->m_id : -1});
    }

    mon->changeWorkspace(ws, false, true, true); // active-workspace-only switch, no focus
}

void HyprlandWorkspaceNavigator::end(mru::domain::UIEndReason reason) {
    if (reason == mru::domain::UIEndReason::Cancelled) {
        // Restore the monitors we elevated to their recorded session-start
        // workspaces (same no-focus call). On Applied we deliberately leave views
        // as-is: the target's workspace is already active and FocusGateway focuses
        // the selected window next (ADR-026).
        for (MonitorCapture &cap : touches_) {
            if (!cap.monitor || cap.pre_elevation_id < 0)
                continue; // WORKSPACE_INVALID (only when the monitor had no ACTIVE
                          // workspace at capture time): nothing meaningful to restore
            const PHLWORKSPACE &active = cap.monitor->m_activeWorkspace;
            if (active && active->m_id != cap.pre_elevation_id)
                cap.monitor->changeWorkspace(cap.pre_elevation_id, false, true, true);
        }
    }
    touches_.clear();
}

} // namespace mru::plugin