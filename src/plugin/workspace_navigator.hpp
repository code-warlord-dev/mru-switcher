#pragma once

#include "mru/domain/ui_port.hpp"
#include "mru/domain/window_ref.hpp"

namespace mru::plugin {

// Output port that keeps the selected window's workspace visible on its monitor
// while a border session is active (ADR-026 / REQ-UI-012). Implemented by
// HyprlandWorkspaceNavigator in the Hyprland adapter; kept here as a
// Hyprland-free interface so BorderHighlightUI is unit-testable (ADR-007,
// cpp-plugin-architecture). Lifecycle is owned by BorderHighlightUI: begin() on
// session start, ensure_visible() whenever a target is actually highlighted,
// end(reason) on session end. Elevation must never focus a window
// (REQ-F-003 / REQ-UI-006) and failures must fail soft (REQ-UI-001).
class WorkspaceNavigator {
  public:
    virtual ~WorkspaceNavigator() = default;

    // Session start: forget the previous session's touches. Per-monitor
    // pre-elevation state is recorded lazily, on the first elevation of each
    // monitor the session actually touches.
    virtual void begin() = 0;
    // Makes the workspace of `ref` the active workspace of its monitor when it
    // is not already active. No window or keyboard focus; no-op when `ref`
    // cannot be resolved or its workspace is already visible.
    virtual void ensure_visible(const mru::domain::WindowRef &ref) = 0;
    // Session end: on Cancelled restore each touched monitor to its recorded
    // pre-elevation workspace; on Applied leave views as-is (the target's
    // workspace is already active and FocusGateway focuses the window next).
    virtual void end(mru::domain::UIEndReason reason) = 0;
};

// Shared no-op navigator: the `selection_follow_workspace = false` backend
// (ADR-026: exact legacy off-screen highlight) and the default constructor
// argument of BorderHighlightUI for tests that do not exercise elevation.
// Never touches the compositor.
class NullWorkspaceNavigator : public WorkspaceNavigator {
  public:
    void begin() override {}
    void ensure_visible(const mru::domain::WindowRef &) override {}
    void end(mru::domain::UIEndReason) override {}
};

inline WorkspaceNavigator &null_workspace_navigator() {
    static NullWorkspaceNavigator instance;
    return instance;
}

} // namespace mru::plugin