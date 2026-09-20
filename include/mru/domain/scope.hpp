#pragma once

namespace mru::domain {

enum class Scope { Global, Monitor, Workspace, Visible, App };
enum class Direction { Next, Prev };
enum class StartOffset { First, Second };

struct SessionPolicy {
    Scope default_scope = Scope::Global;
    StartOffset start_offset = StartOffset::Second;
    bool wrap = true;
    bool restore_focus_on_cancel = false;
    // NOTE: there is no history lock-in flag. Lock-in while a session is Active is
    // mandatory and unconditional (REQ-H-001/010, ADR-021); the
    // `lock_history_on_session` config key is reserved and ignored.
};

} // namespace mru::domain