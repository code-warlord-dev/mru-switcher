#pragma once

#include <vector>

#include <hyprland/src/desktop/DesktopTypes.hpp>
#include <hyprland/src/output/Monitor.hpp>

#include "identity_registry.hpp"
#include "mru/domain/ui_port.hpp"
#include "workspace_navigator.hpp"

namespace mru::plugin {

// ADR-026 / REQ-UI-012: makes the workspace of the virtually selected window the
// active workspace of its monitor while a border session is active, so the
// highlight is never off-screen. `CMonitor::changeWorkspace(ws, internal=false,
// noMouseMove=true, noFocus=true)` switches the ACTIVE WORKSPACE only — window and
// keyboard focus stay untouched (REQ-F-003 / REQ-UI-006; ADR-023 intact). On
// Cancelled each monitor touched this session is restored to its recorded
// pre-elevation workspace; on Applied views are left as-is (the target workspace
// is already active; FocusGateway focuses the window next). Resolution of
// WindowRef -> window/monitor/workspace uses the same WindowIdentityRegistry path
// as HyprlandFocusGateway (ADR-006). Failures are callers' business: we never
// throw; BorderHighlightUI keeps the session fail-soft (REQ-UI-001).
class HyprlandWorkspaceNavigator : public WorkspaceNavigator {
  public:
    explicit HyprlandWorkspaceNavigator(WindowIdentityRegistry &registry);

    void begin() override;
    void ensure_visible(const mru::domain::WindowRef &ref) override;
    void end(mru::domain::UIEndReason reason) override;

  private:
    // Pre-elevation active workspace id of one monitor, recorded lazily on its
    // first elevation of the session (ADR-026: only monitors actually touched).
    // The monitor is held by strong ref for the session so an unplug mid-session
    // cannot leave a dangling pointer for the Cancelled restore (review finding:
    // monitors are SP-owned on the pin; PHLMONITORREF/raw would be a use-after-free
    // risk in end(Cancelled)).
    struct MonitorCapture {
        PHLMONITOR monitor;                // strong ref keeps the monitor alive while we may restore
        WORKSPACEID pre_elevation_id = -1; // WORKSPACE_INVALID; ids are stable on the pin
    };

    WindowIdentityRegistry &registry_;
    std::vector<MonitorCapture> touches_; // cleared by begin(), re-cleared by end()
};

} // namespace mru::plugin