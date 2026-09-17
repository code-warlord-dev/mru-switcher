#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace mru::domain {

// Snapshot-time focus anchor, captured ONCE when a session starts and passed by
// value to scope_matches; never recomputed per window (ADR-016 __2__). Fields
// are the focused window's monitor/workspace/class at that instant plus the set
// of workspace ids shown on monitors.
//
// `monitor_id`/`workspace_id` anchor the monitor/workspace scopes to the window
// focused at snapshot time (REQ-SC-002). When nothing is focused
// (`has_focus == false`) the app scope degrades to global (REQ-SC-002); the
// anchor scopes monitor and workspace also degrade to global because there is
// no reference window to compare against. That is a deliberate, fail-open
// choice documented here until SPEC pins it; the visible scope never consults
// the anchors.
struct FocusContext {
    bool has_focus = false;
    std::uint64_t monitor_id{};
    std::uint64_t workspace_id{};
    std::string app_class;                         // class() of the focused window, if any
    std::vector<std::uint64_t> visible_workspaces; // workspace ids shown at snapshot time
};

} // namespace mru::domain