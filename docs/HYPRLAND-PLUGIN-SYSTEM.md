# Hyprland Plugin System — Architecture Reference

**Status:** Reference (host compositor)  
**Audience:** Plugin authors, reviewers of MRU Switcher design  
**Related:** ARCHITECTURE.md (MRU plugin), SPEC.md, SECURITY.md

This document describes how **Hyprland’s native plugin system** works: loading, lifecycle, API surface, events, constraints, and how a plugin such as the MRU switcher maps onto it.

---

## 1. Place in the compositor

Hyprland initializes managers in three stages. The plugin system is created in **STAGE_LATE**:

| Stage | Enum | Typical contents |
|-------|------|------------------|
| 0 | `STAGE_PRIORITY` | Event loop, config, keybind manager, permissions, … |
| 1 | `STAGE_BASICINIT` | OpenGL, protocol manager, seat |
| 2 | `STAGE_LATE` | Input, renderer, XWayland, **`CPluginSystem`**, … |

Global instance: `g_pPluginSystem` (`UP<CPluginSystem>`).

A plugin is **not** a separate process. It is a shared object (`.so`) loaded with `dlopen` into the Hyprland process. Consequences:

- C++ objects and types are shared across the boundary.
- **No ABI stability** — compile with the same compiler and matching headers as the running compositor.
- Same privilege level as the compositor (see SECURITY.md).

---

## 2. Plugin load lifecycle

```text
hyprctl plugin load /absolute/path/plugin.so
        │  (or config `plugin = …` / hyprpm)
        ▼
CPluginSystem::loadPlugin(path)
        │
        ├─ Permission check (CDynamicPermissionManager)
        ├─ Reject if already loaded from same path
        ├─ dlopen(path)
        ├─ Resolve pluginAPIVersion / PLUGIN_API_VERSION
        ├─ Compare with HYPRLAND_API_VERSION
        ├─ m_allowConfigVars = true   ← config registration window
        ├─ Call pluginInit(HANDLE)
        ├─ m_allowConfigVars = false
        └─ Track CPlugin metadata in m_loadedPlugins
```

Unload:

```text
hyprctl plugin unload PATH
        │
        ▼
CPluginSystem::unloadPlugin
        ├─ Call PLUGIN_EXIT if user-initiated unload
        ├─ Remove registered dispatchers, config values, layouts, decorations
        └─ dlclose
```

Note: on **fault/eject**, `PLUGIN_EXIT` may **not** run. Do not rely on it for critical cleanup alone; prefer registrations that Hyprland tears down automatically.

---

## 3. Required exports from the plugin `.so`

| Export | Role |
|--------|------|
| `PLUGIN_API_VERSION()` / `pluginAPIVersion` | Returns API version string; must match compositor |
| `PLUGIN_INIT(HANDLE)` / `pluginInit` | Synchronous init; register features; return `PLUGIN_DESCRIPTION_INFO` |
| `PLUGIN_EXIT()` / `pluginExit` | Optional; user unload only |

`HANDLE` is the plugin’s identity for subsequent `HyprlandAPI` calls. Store it (e.g. global `PHANDLE`).

### Recommended header-hash check (wiki practice)

Inside `PLUGIN_INIT`:

```text
COMPOSITOR_HASH = __hyprland_api_get_hash()
CLIENT_HASH     = __hyprland_api_get_client_hash()
if unequal → notify + abort load
```

This catches “compiled against different sources than the running binary” better than API version alone.

### Init constraints

- Runs **synchronously** on the compositor thread.
- Blocking work or nested `hyprctl`/IPC that waits on Hyprland can deadlock or hang load.
- Config values may be added **only** while init runs (`m_allowConfigVars`).

---

## 4. Runtime metadata: `CPlugin`

Per loaded module Hyprland tracks approximately:

```text
CPlugin
  m_name, m_description, m_author, m_version, m_path
  m_handle                         // dlopen handle
  m_registeredDecorations[]
  m_registeredDispatchers[]        // names in KeybindManager
  m_registeredHyprctlCommands[]
  m_registeredAlgos[]              // layout algorithm names
  m_registeredApiValues[]          // config IValue entries
```

Used for bookkeeping and cleanup, not as a public “plugin SDK object” for third parties.

---

## 5. `HyprlandAPI` surface (by capability)

```text
                         HyprlandAPI
                              │
      ┌───────────┬───────────┼───────────┬────────────┬──────────────┐
      ▼           ▼           ▼           ▼            ▼              ▼
   Config    Dispatchers    Events     Layouts    Decorations     Hooks / misc
```

### 5.1 Configuration

| API | Notes |
|-----|--------|
| `addConfigValueV2` | **Only in `PLUGIN_INIT`**; takes `SP<Config::Values::IValue>` (typed: `Int`/`String`/`Bool`/…). Keep the SP and read through `->value()` |
| `getConfigValue` / `addConfigValue` | **V1 (deprecated; legacy backend only) — not used by mru-switcher (M3-S1)** |
| Namespace | Must be under `plugin:…` |

Invalid: registering config later at runtime; keys outside `plugin:`.

### 5.2 Dispatchers

| API | Notes |
|-----|--------|
| `addDispatcherV2(handle, name, fn)` | Preferred; `fn` → `SDispatchResult(std::string args)` |
| `addDispatcher` | Deprecated |
| `removeDispatcher` | Optional; unload usually cleans up |

Names are global in the keybind/dispatcher table → use a prefix (`mru:`, `hy3:`, …).

Invocation paths: keybinds, `hyprctl dispatch`, Lua `hl.dispatch`.

### 5.3 Events

**Deprecated:** `registerCallbackDynamic` / old string event hooks.

**Current:** `Event::bus()` (`CEventBus`), typed signals, e.g.:

```cpp
Event::bus()->m_events.window.active.listen(
    [](PHLWINDOW w, Desktop::eFocusReason reason) { /* ... */ });
```

Relevant groups (illustrative, not exhaustive):

| Group | Examples |
|-------|----------|
| `window` | `active`, `open`, `openEarly`, `close`, `destroy`, `kill`, `title`, `class_`, `fullscreen`, `updateRules`, `moveToWorkspace` |
| `workspace` | create/remove/active/move |
| `config` | `preReload`, `reloaded` |
| `keybinds` | `submap` |
| lifecycle | `start`, `exit`, `tick`, … |

Plugins may register **custom** events (`addEvent` / `CCustomEvent`) consumable from Lua via `hl.on`.

Keep listener lifetimes alive for the plugin duration (e.g. store listen handles as static/members).

### 5.4 Layout algorithms

| API | Notes |
|-----|--------|
| `addTiledAlgo` / `addFloatingAlgo` | Name + `type_info` + factory |
| `removeAlgo` | |
| `addLayout` (IHyprLayout*) | Deprecated |

Used by scrolling/hy3-style plugins; **not** required for an MRU switcher.

### 5.5 Function hooks

| API | Notes |
|-----|--------|
| `findFunctionsByName` | Prefer over hard-coded addresses |
| `createFunctionHook` | Then `hook()` / `unhook()` |

- **x86_64 only**; other architectures ignore hook attempts.
- Highest breakage rate across Hyprland updates.
- Guidelines: prefer Event::bus; hooks as last resort.

### 5.6 Other

- Window decorations: `addWindowDecoration` / remove  
- Notifications: `addNotification` / V2  
- `invokeHyprctlCommand`  
- Lua function registration  
- Custom events for Lua  

### 5.7 Private members

Optional pattern (fragile):

```cpp
#define private public
#include <hyprland/src/...>
#undef private
```

Increases coupling; avoid when public API or bus suffices.

---

## 6. Threading model

- The Wayland / compositor event loop is **single-threaded**.
- Plugin code that touches compositor state must run on that thread (dispatchers, event listeners).
- Background threads are only acceptable for fully detached work (e.g. writing a file) with **no** compositor callbacks from those threads.

### 6.1 `CEventLoopTimer` semantics (pinned: Hyprland v0.56.2 `efb5099`)

Source: header inspection (`EventLoopTimer.hpp`, `EventLoopManager.hpp`); no compositor source `.cpp` available on the pinned tag.

```cpp
CEventLoopTimer(
    std::optional<Time::steady_dur> timeout,
    std::function<void(SP<CEventLoopTimer> self, void* data)> cb_,
    void* data_);
```

- `g_pEventLoopManager->addTimer(SP<CEventLoopTimer>)` — the manager holds its own strong reference.
- `g_pEventLoopManager->removeTimer(SP<CEventLoopTimer>)` — explicit removal.
- Manager header comment: *"Note: will remove the timer if the ptr is lost"* — the manager tracks and auto-removes when the last strong ref dies.
- `cancel()` disarms the timer; the callback will not fire.

**Scheduler contract (MEDIUM-6, symmetric teardown):** the HyprlandSchedulerPort callback receives `SP<CEventLoopTimer> self` from the manager. On fire: (1) copy `self` to local, (2) `removeTimer(self)`, (3) erase bookkeeping, (4) run user callback. This keeps the timer object alive through the callback and avoids relying on the manager's implicit cleanup semantics. Verified safe: `cancel()` already calls `removeTimer` from outside the callback path, so calling it during dispatch is expected by the manager API.

---

## 7. Permissions and trust

- Loading a plugin is gated by permission policy (`CDynamicPermissionManager`) depending on Hyprland version/config.
- A loaded plugin has the same power as compositor code: input, surfaces, other clients’ state as exposed internally.
- **Never load untrusted `.so` files.** Prefer self-built artifacts pinned to a known Hyprland commit (hyprpm `commit_pins`).

---

## 8. External vs in-process extension (when *not* to use a plugin)

| Need | Prefer |
|------|--------|
| Query state, run dispatchers, react to socket2 events | IPC (`hyprctl`, `.socket.sock`, `.socket2.sock`) — any language |
| Config-time logic, light reactions | Lua (`hl.on`, binds, layout.register) |
| Snapshot + history lock-in in the same tick as focus, virtual selection, deep layout | **Native C++ plugin** |

Hyprland maintainers have rejected “plugins in arbitrary languages via D-Bus” as a substitute for in-process plugins: IPC already covers the loose-integration case.

---

## 9. Mapping: MRU Switcher → plugin system

```text
┌────────────────────── Hyprland process ──────────────────────┐
│  CKeybindManager  ──►  mru:cycle | mru:apply | mru:cancel    │
│  Event::bus         ──►  window.active / window.close        │
│  Config             ──►  plugin:mru-switcher:*               │
│                                                              │
│  ┌──────────────── mru-switcher.so ────────────────────────┐ │
│  │  L0 Facade: PLUGIN_INIT, hash check, API registration   │ │
│  │  L1 SessionController, HistoryTracker, ScopeResolver    │ │
│  │  L2 Domain (no Hyprland types)                          │ │
│  │  L3 Adapters: CompositorPort, FocusGateway, UIPort      │ │
│  └─────────────────────────────────────────────────────────┘ │
└──────────────────────────────────────────────────────────────┘
```

| Hyprland subsystem | MRU usage |
|--------------------|-----------|
| `CPluginSystem` | Load/unload |
| `addDispatcherV2` | Public `mru:*` contract |
| Config API | `plugin:mru-switcher:*` |
| `Event::bus` | MRU updates, snapshot prune |
| `Desktop::History` (when available) | Seed ordering |
| `focusWindow` / `Desktop::focusState` | Only via FocusGateway on apply |
| Function hooks | Not on the default path |
| Layout algo API | Unused (not a layout plugin) |

Design rules for MRU (see ARCHITECTURE.md / DECISIONS.md):

- Domain stays free of `PHLWINDOW` / globals.
- One active session; snapshot immutable except prune.
- No focus on `mru:cycle`; focus only on apply.
- History lock-in while session Active.

---

## 10. Author checklist

1. Export API version + init (+ optional exit).  
2. Hash-check headers in init.  
3. Register config only in init, under `plugin:`.  
4. Prefer `addDispatcherV2` and `Event::bus`.  
5. Prefix dispatcher and config names.  
6. No compositor-touching worker threads.  
7. Treat internal headers as unstable; prefer documented API + bus.  
8. Fail closed on version/hash mismatch.  
9. Document rebuild requirement for each Hyprland upgrade.  
10. For hyprpm: provide `hyprpm.toml` with build steps and optional commit pins (populated in this repo since M6-T5 — see the repository `hyprpm.toml`; pinning is a release contract per `docs/COMPAT.md`, "Risk note"; the plugin-side hash is finalized at the release tag, M6-T9).

---

## 11. References (conceptual)

- Wiki: Plugins → Getting started, Advanced, Plugin guidelines  
- Headers: `src/plugins/PluginAPI.hpp`, `PluginSystem.hpp`  
- Events: `src/event/EventBus.hpp`  
- Official plugins: `hyprwm/hyprland-plugins` (patterns for bus listen, dispatchers, config)

---

## 12. Document history

| Date | Note |
|------|------|
| 2026-09-13 | Initial reference for MRU Switcher documentation set |
| 2026-09-15 | Add §6.1: CEventLoopTimer semantics on pinned v0.56.2 (MEDIUM-6 findings) |
