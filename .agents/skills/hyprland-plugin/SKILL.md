---
name: hyprland-plugin
description: Expert Hyprland native plugin development including PluginAPI, Event bus, dispatchers V2, config under plugin namespace, hash checks, layouts, and function hooks. Use when writing, reviewing, debugging, or scaffolding a Hyprland .so plugin, PLUGIN_INIT, hyprpm, or in-process compositor extensions.
metadata:
  level: expert
  version: "1.0"
  domain: hyprland
---

# Hyprland Native Plugin Development

## Overview

Implement or review **in-process** Hyprland plugins (C++ `.so`). Plugins share the compositor address space; ABI is unstable. Prefer public `HyprlandAPI` and `Event::bus()` over internal hooks.

## When to use

- Creating or modifying a Hyprland plugin
- `PLUGIN_INIT` / `PLUGIN_EXIT` / hash mismatch issues
- Registering dispatchers, config, layouts, decorations
- Choosing Event::bus vs function hooks
- hyprpm manifests and commit pins

## When not to use

- External tools that only need IPC (`hyprctl`, socket2) — any language is fine without this skill
- Pure Lua config (`hl.on`, binds) with no `.so`

## Non-negotiables

1. **Language** — native plugins are C++ only (same compiler family as Hyprland; official builds use gcc).
2. **Hash check** in `PLUGIN_INIT` — compare `__hyprland_api_get_hash()` and `__hyprland_api_get_client_hash()`; abort on mismatch.
3. **Config** — register values **only** inside `PLUGIN_INIT`; keys under `plugin:`.
4. **Dispatchers** — use `addDispatcherV2`; return `SDispatchResult`; namespace names (`mru:`, `hy3:`).
5. **Events** — prefer `Event::bus()->m_events.*`; `registerCallbackDynamic` is deprecated.
6. **Threads** — Wayland loop is single-threaded; never touch compositor state from background threads.
7. **Hooks** — `createFunctionHook` is x86_64-only and last resort.

## Required exports

```cpp
APICALL EXPORT std::string PLUGIN_API_VERSION() { return HYPRLAND_API_VERSION; }

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle) {
    PHANDLE = handle;
    // hash check → config → dispatchers → Event::bus listeners → return {name, desc, author, version};
}

APICALL EXPORT void PLUGIN_EXIT() { /* optional user-unload cleanup */ }
```

Store `HANDLE` globally for API calls.

## Registration order (recommended)

1. Hash check (fail closed)
2. `addConfigValue*` under `plugin:name:…`
3. `addDispatcherV2`
4. `Event::bus()` subscriptions (keep listener objects alive)
5. Optional layouts / decorations / notifications
6. Return `PLUGIN_DESCRIPTION_INFO`

## Event::bus patterns

```cpp
static auto P = Event::bus()->m_events.window.active.listen(
    [](PHLWINDOW w, Desktop::eFocusReason r) { /* ... */ });
```

Common signals for window tooling — `window.active`, `window.open`, `window.close`, `window.destroy`, `config.reloaded`.

## Function hooks (avoid unless necessary)

1. `findFunctionsByName(PHANDLE, "symbol")` (cache result static)
2. `createFunctionHook(handle, addr, &hk)`
3. `hook()` / `unhook()`
4. Member hooks need `thisptr` as first argument

## Config read pattern

```cpp
static auto* const PVAL = &HyprlandAPI::getConfigValue(PHANDLE, "plugin:foo:bar")->intValue;
```

Pointer is stable after init.

## Private members

Only if required — `#define private public` around specific includes, then `#undef private`. Prefer not to.

## hyprpm

Provide `hyprpm.toml` with `output`, `build` steps, optional `commit_pins` for Hyprland revisions.

## Review checklist

- [ ] Hash check present
- [ ] No config registration outside init
- [ ] Dispatchers V2 + prefixed names
- [ ] Events via bus, listeners not dropped
- [ ] No compositor calls from non-main threads
- [ ] Hooks gated / x86_64 documented if used
- [ ] Unload path does not assume `PLUGIN_EXIT` on crash

## References

- Project docs — `docs/HYPRLAND-PLUGIN-SYSTEM.md`
- Wiki — Plugins Getting Started, Advanced, Guidelines
- Headers — `PluginAPI.hpp`, `EventBus.hpp`, `PluginSystem.hpp`
