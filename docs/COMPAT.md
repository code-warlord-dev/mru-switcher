# Hyprland compatibility matrix

**Status:** Living — must be filled before M2 ships a `.so`  
**Related:** SPEC REQ-H-004, ADR-005, HYPRLAND-PLUGIN-SYSTEM.md

## Policy

1. Every loadable plugin release documents a **tested Hyprland git commit** (and/or release tag).
2. Plugins **must** fail closed on header hash mismatch.
3. Internal APIs (`Desktop::History::…`, `Desktop::focusState::…`) are **adapter-private** and version-gated — not part of the user contract.
4. If an internal symbol is missing on the pinned revision, the adapter uses the **fallback** path documented below and logs once.

## Matrix (fill at M2+)

| Plugin version | Hyprland commit | Hyprland tag | CI/nest tested | Notes |
|----------------|-----------------|--------------|----------------|-------|
| 0.0.0 (docs) | — | — | n/a | M0 docs only |
| 0.2.0 (planned) | `efb50993780079460b0cbed1363e2166a2de1d9f` | `v0.56.2` | pending | First `.so`; headers at `/usr/include/hyprland` (distro `hyprland` pkg) |

## Internal API expectations (adapter)

| Capability | Preferred symbol / path | Fallback if unavailable |
|------------|-------------------------|-------------------------|
| MRU seed | `Desktop::History::windowTracker()->fullHistory()` | Event-sourced list from `Event::bus()->m_events.window.active` only |
| Focus | `Desktop::focusState()->fullWindowFocus` or compositor `focusWindow` as available on pin | Document exact call used on pin; must still go through FocusGateway |
| Timers | Main-thread timer / event loop integration available to plugins on pin | Document in M2 adapter notes; must satisfy SchedulerPort |
| Config reload signal | `Event::bus()->m_events.config.reloaded` | Re-read config values only at next session if signal absent |
| Window id | Stable address used by hyprctl + generation in plugin registry | Generation always plugin-local |

## Minimal build recipe (template)

```text
# 1. Check out Hyprland at the pinned commit from the matrix
# 2. Build Hyprland; install headers for that commit
# 3. Build this plugin against those headers
# 4. Nested session: same binary as headers
# 5. hyprctl plugin load /abs/path/plugin.so
```

Exact commands depend on distro; record the ones used for each matrix row in release notes.

## Verification before claiming a pin

- [ ] Hash check passes on load  
- [ ] `mru:cycle` / `mru:apply` / `mru:cancel` smoke  
- [ ] Debounce does not crash on unload  
- [ ] Apply-after-invalidation (close selected, then apply)  
- [ ] Fallback path exercised if History API ifdef’d out  

## Risk note

Pinning is **required** for M2 exit. Shipping without a matrix row is a process failure, not an optional doc gap.
