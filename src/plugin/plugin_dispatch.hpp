#pragma once

#include <string>

#include <hyprland/src/plugins/PluginAPI.hpp>

namespace mru::plugin {

// Dispatcher entry points (REQ-DISP-001/002/003, REQ-F-003). Defined in
// plugin_dispatch.cpp; registered with the compositor by register_dispatchers()
// (HIGH-5: only after build_state(), HIGH-4: every entry runs behind guarded()).
SDispatchResult dispatch_cycle(std::string args);
SDispatchResult dispatch_apply(std::string args);
SDispatchResult dispatch_cancel(std::string args);
SDispatchResult dispatch_status(std::string args);
void register_dispatchers();

} // namespace mru::plugin
