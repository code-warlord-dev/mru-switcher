#pragma once

namespace mru::domain {

enum class Scope { Global, Monitor, Workspace, Visible, App };
enum class Direction { Next, Prev };
enum class StartOffset { First, Second };

struct SessionPolicy {
  Scope default_scope = Scope::Global;
  StartOffset start_offset = StartOffset::Second;
  bool wrap = true;
  bool lock_history_on_session = true;
  bool restore_focus_on_cancel = false;
};

} // namespace mru::domain