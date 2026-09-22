#pragma once

#include <exception>
#include <string>

#include <hyprland/src/plugins/PluginAPI.hpp>

#include "mru_plugin.hpp"

// Global plugin handle, defined in mru_plugin.cpp (set in PLUGIN_INIT, cleared
// in PLUGIN_EXIT). Declared at global scope to match the definition; facade TUs
// reach it by ordinary lookup from inside namespace mru::plugin.
extern HANDLE PHANDLE;

namespace mru::plugin {

// Shared plugin-owned state. The single instance is a function-local static in
// plugin_lifecycle.cpp; every facade TU reaches it through this accessor (the
// state is stable for the plugin lifetime, HIGH-5).
PluginState &state();

// Hash check (fail closed) and config-derived session policy. Defined in
// plugin_lifecycle.cpp.
bool hash_ok();
mru::domain::SessionPolicy policy_from_config(const PluginConfig &cfg);

// Dispatcher result helpers. Defined in plugin_dispatch.cpp.
SDispatchResult ok_result();
SDispatchResult err_result(std::string error);

// HIGH-4: an exception escaping into the compositor through a C-ABI-like entry
// point is std::terminate at best, UB at worst — one malformed config line must
// not take down a user session. Every dispatcher, listener, Lua entry and peer
// command runs behind a barrier that converts exceptions into an error result /
// notification.
template <class F> SDispatchResult guarded(F &&f) noexcept {
    try {
        return f();
    } catch (const std::exception &e) {
        return err_result(std::string("internal error: ") + e.what());
    } catch (...) {
        return err_result("internal error");
    }
}

} // namespace mru::plugin
