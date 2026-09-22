#pragma once

namespace mru::plugin {

// Plugin-owned state construction / destruction (HIGH-5: build_state -> subscribe
// -> dispatch -> lua; teardown in reverse order, listeners first, scheduler last
// so HistoryTracker::cancel_pending() still sees a live scheduler, REQ-H-008).
// Defined in plugin_lifecycle.cpp.
void build_state();
void teardown_state();

} // namespace mru::plugin
