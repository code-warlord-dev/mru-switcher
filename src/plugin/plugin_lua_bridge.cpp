#include "plugin_lua_bridge.hpp"

#include <exception>
#include <string>

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

#include <hyprland/src/plugins/PluginAPI.hpp>

#include "plugin_dispatch.hpp"
#include "plugin_internal.hpp"

namespace mru::plugin {
namespace {

// --- Lua dispatcher bridge (task_0001, Omarchy UX) ------------------------------
//
// Thin wrappers so the Lua config can call the same paths as the keybind
// dispatchers: `hl.plugin.mru.cycle(...)`, `.apply()`, `.cancel()`, `.status()`.
// Dispatcher semantics are UNCHANGED (SPEC §3, REQ-DISP-001/002/003,
// REQ-F-003): cycle never focuses, apply focuses once via FocusGateway.
//
// Error contract: Hyprland's own tester plugin maps a failed SDispatchResult to
// `lua_error` (raise) and a successful one to 0 returns. `status` is the one
// exception — SPEC §3.4 carries the human-readable payload in the error field
// of a *successful* result, so it returns that payload as a Lua string.
//
// HIGH-4: a Lua C function raising via lua_error/longjmp must not leak C++
// objects with non-trivial destructors across the jump. Each entry therefore
// runs the controller call inside guarded() (exceptions -> SDispatchResult),
// then clears any std::string BEFORE lua_error/lua_pushstring so only
// stack-owned values are live at the longjmp point.
static int lua_result_or_raise(lua_State *L, SDispatchResult r) {
    if (r.success)
        return 0;
    std::string msg = r.error.empty() ? "mru-switcher: plugin function failed" : std::move(r.error);
    const char *cmsg = msg.c_str();
    // Copy into Lua-owned storage first; then drop the C++ owner before the
    // longjmp so no non-trivial destructor is skipped.
    lua_pushstring(L, cmsg);
    msg.clear();
    msg.shrink_to_fit();
    return lua_error(L);
}

static int lua_cycle(lua_State *L) {
    const int n = lua_gettop(L);
    std::string args;
    for (int i = 1; i <= n; ++i) {
        if (!lua_isstring(L, i)) {
            // HIGH-4: drop the heap before the longjmp so no owner is skipped.
            args.clear();
            args.shrink_to_fit();
            return luaL_error(L, "hl.plugin.mru.cycle: argument %d must be a string", i);
        }
        if (i > 1)
            args += ' ';
        args += lua_tostring(L, i);
    }
    SDispatchResult r = guarded([&] { return dispatch_cycle(std::move(args)); });
    return lua_result_or_raise(L, std::move(r));
}

static int lua_apply(lua_State *L) {
    (void)L;
    SDispatchResult r = guarded([&] { return dispatch_apply({}); });
    return lua_result_or_raise(L, std::move(r));
}

static int lua_cancel(lua_State *L) {
    (void)L;
    SDispatchResult r = guarded([&] { return dispatch_cancel({}); });
    return lua_result_or_raise(L, std::move(r));
}

static int lua_status(lua_State *L) {
    SDispatchResult r = guarded([&] { return dispatch_status({}); });
    if (!r.success)
        return lua_result_or_raise(L, std::move(r));
    // SPEC §3.4: payload travels in the error field of a successful result.
    std::string payload = std::move(r.error);
    const char *cpayload = payload.c_str();
    lua_pushstring(L, cpayload ? cpayload : "");
    payload.clear();
    payload.shrink_to_fit();
    return 1;
}

} // namespace

void register_lua_functions() {
    // Removal is automatic on unload (PluginAPI docs); no explicit unregister
    // needed in PLUGIN_EXIT. Returns false when the compositor runs a
    // non-Lua (hyprlang) config — not fatal: dispatchers remain the primary
    // path, so a failure here is intentionally silent.
    (void)HyprlandAPI::addLuaFunction(PHANDLE, "mru", "cycle", &lua_cycle);
    (void)HyprlandAPI::addLuaFunction(PHANDLE, "mru", "apply", &lua_apply);
    (void)HyprlandAPI::addLuaFunction(PHANDLE, "mru", "cancel", &lua_cancel);
    (void)HyprlandAPI::addLuaFunction(PHANDLE, "mru", "status", &lua_status);
}

} // namespace mru::plugin
