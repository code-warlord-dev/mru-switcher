#include "mru_plugin.hpp"

#include <exception>
#include <stdexcept>
#include <string>

#include <hyprland/src/plugins/PluginAPI.hpp>
#include <hyprland/src/plugins/PluginSystem.hpp>

#include "config_v2.hpp"
#include "mru_version.hpp"
#include "plugin_dispatch.hpp"
#include "plugin_events.hpp"
#include "plugin_internal.hpp"
#include "plugin_lifecycle.hpp"
#include "plugin_lua_bridge.hpp"

HANDLE PHANDLE = nullptr;

APICALL EXPORT std::string PLUGIN_API_VERSION() {
    return HYPRLAND_API_VERSION;
}

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle) {
    PHANDLE = handle;

    try {
        if (!mru::plugin::hash_ok())
            throw std::runtime_error("mru-switcher: header hash mismatch, refusing to load");

        if (!mru::plugin::config::register_all(PHANDLE, mru::plugin::state().config_v2)) {
            // fail closed: an empty PLUGIN_DESCRIPTION_INFO does NOT unload the plugin
            // on pin 0.56.2 (loadPluginInternal has no empty-description check). The
            // only reliable fail-closed channel is an exception: loadPluginInternal
            // wraps initFunc in try/catch + setjmp and on std::exception runs
            // unloadPlugin(PLUGIN, true) and rejects the load. So PLUGIN_INIT lets a
            // std::exception propagate; the compositor's own catch is that barrier.
            throw std::runtime_error("mru-switcher: failed to register config values");
        }
        mru::plugin::build_state();            // constructs all members, seeds MRU (HIGH-5)
        mru::plugin::subscribe_events();       // HIGH-5: listeners after state exists
        mru::plugin::register_dispatchers();   // HIGH-5: dispatchers after state exists
        mru::plugin::register_lua_functions(); // task_0001: hl.plugin.mru.* (silent no-op on hyprlang config)
    } catch (const std::exception &e) {
        mru::plugin::teardown_state();
        HyprlandAPI::addNotification(PHANDLE, std::string("mru-switcher: init failed: ") + e.what(),
                                     CHyprColor{1, 0, 0, 1}, 5000);
        // HIGH-4: at PLUGIN_INIT the fail-closed barrier is deliberately the
        // *compositor's* catch in loadPluginInternal (pin efb5099): it unloads the
        // plugin and rejects the load. Rethrow instead of returning {} — the latter
        // is treated as a successful load with empty metadata. Dispatchers/listeners
        // still use guarded()/guarded_listener() and never propagate (HIGH-4).
        throw; // NOLINT: intentional C-ABI escape, caught by the compositor (verified)
    } catch (...) {
        mru::plugin::teardown_state();
        HyprlandAPI::addNotification(PHANDLE, "mru-switcher: init failed (internal error)", CHyprColor{1, 0, 0, 1},
                                     5000);
        // non-std exception: the compositor's catch only handles std::exception, so
        // devolve to the empty return (cannot reliably fail closed; do not crash).
        return {};
    }

    return {"mru-switcher", "Niri-style MRU Alt+Tab (snapshot, apply-on-release, lock-in)", "mru",
            MRU_SWITCHER_VERSION};
}

APICALL EXPORT void PLUGIN_EXIT() {
    try {
        mru::plugin::teardown_state(); // listeners first, scheduler last: no use-after-unload
    } catch (...) {
        // best-effort: swallow to satisfy C-ABI boundary (HIGH-4)
    }
    PHANDLE = nullptr;
}
