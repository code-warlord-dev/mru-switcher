#pragma once

namespace mru::plugin {

// Lua dispatcher bridge (hl.plugin.mru.*). Defined in plugin_lua_bridge.cpp;
// registered in PLUGIN_INIT after the dispatchers (HIGH-5). Silent no-op on a
// non-Lua (hyprlang) config — dispatchers remain the primary path.
void register_lua_functions();

} // namespace mru::plugin
