# Hyprland compatibility matrix

**Status:** Living — must be filled before M2 ships a `.so`  
**Related:** SPEC REQ-H-004, ADR-005, HYPRLAND-PLUGIN-SYSTEM.md

## Policy

1. Every loadable plugin release documents a **tested Hyprland git commit** (and/or release tag).
2. Plugins **must** fail closed on header hash mismatch.
3. Internal APIs (`Desktop::History::...`, `Desktop::focusState::...`) are **adapter-private** and version-gated — not part of the user contract.
4. If an internal symbol is missing on the pinned revision, the adapter uses the **fallback** path documented below and logs once.

## Matrix

| Plugin version | Hyprland commit | Hyprland tag | CI/nest tested | Notes |
|----------------|-----------------|--------------|----------------|-------|
| 0.0.0 (docs) | — | — | n/a | M0 docs only |
| 0.2.0 | `efb50993780079460b0cbed1363e2166a2de1d9f` | `v0.56.2` | **nest tested 2026-09-15** | First `.so`; headers at `/usr/include/hyprland` (distro `hyprland` pkg) |

## Internal API expectations (adapter)

| Capability | Preferred symbol / path | Fallback if unavailable |
|------------|-------------------------|-------------------------|
| MRU seed | `Desktop::History::windowTracker()->fullHistory()` | Event-sourced list from `Event::bus()->m_events.window.active` only |
| Focus | `Desktop::focusState()->fullWindowFocus` | Document exact call used on pin; must still go through FocusGateway |
| Timers | `CEventLoopTimer` via `g_pEventLoopManager` | Document in adapter notes; must satisfy SchedulerPort |
| Config reload signal | `Event::bus()->m_events.config.reloaded` | Re-read config values only at next session if signal absent |
| Window id | Stable address used by hyprctl + generation in plugin registry | Generation always plugin-local |

## M2 Nest Smoke Results (2026-09-15)

**Host:** Hyprland v0.56.2, nested inside host Wayland session.

### Setup

```bash
# Nest config
monitor = ,1920x1080@60,auto,1
bind = ALT, TAB, mru:cycle, next
bind = ALT SHIFT, TAB, mru:cycle, prev
bindrt = ALT, ALT_L, mru:apply
bind = ALT, Escape, mru:cancel

# Windows: footA, footB, footC opened before plugin load
```

### Dispatcher Matrix

| Command | Expected | Actual |
|---------|----------|--------|
| `mru:apply` (idle) | ok (FM-12 idempotent) | ok |
| `mru:cancel` (idle) | ok (FM-13 idempotent) | ok |
| `mru:status` (idle) | active=false, size=0, scope=global | active=false index=0 size=0 scope=global session=0 last_end=none |
| `mru:cycle next` (pre-load windows) | **ok (D4 regression fix)** | ok |
| `mru:status` (active) | active=true, size=3 | active=true index=1 size=3 scope=global session=1 last_end=none |
| `mru:cycle prev` | ok | ok |
| `mru:cycle` (no args) | ok (REQ-DISP-001: next) | ok |
| `mru:cycle monitor` | ok (REQ-DISP-003: scope-only) | ok |
| `mru:cycle bogus` | error: unknown argument | error: unknown argument: bogus |
| `mru:apply` (after cycles) | focus applied, session ends | ok |
| `mru:status` (after apply) | active=false, last_end=Applied | active=false index=0 size=0 scope=global session=1 last_end=Applied |

### Invariants Verified

- [x] `mru:cycle` twice: `hyprctl activewindow -j` unchanged while tabbing (REQ-F-003)
- [x] `mru:apply` focuses exactly the selected window (T-S-02)
- [x] `mru:cancel` leaves focus unchanged (REQ-S-005)
- [x] Close selected window mid-session -> `mru:apply` -> prune/clamp or `no windows` (T-F-03/T-F-04)
- [x] Repeated `mru:cycle next`: no focus flicker, MRU order stable (T-H-01)
- [x] `plugin unload` -> no crash, no pending timer (REQ-H-008)
- [x] `mru:status` returns correct payload at all states (D7)
- [x] Scope token without direction accepted (REQ-DISP-003, D6)
- [x] Pre-load windows visible from first cycle (D4 regression fix)

### Observations

- First `dispatch` right after `plugin load` may report `Invalid dispatcher` until the post-load config reload finishes; second attempt is fine. No code change in M2 — documented here.
- Registry `by_address_` grows monotonically (closed entries not cleaned). Acceptable for M2; GC candidate for post-M3.

## Verification checklist (per release)

- [x] Hash check passes on load
- [x] `mru:cycle` / `mru:apply` / `mru:cancel` smoke
- [x] Debounce does not crash on unload
- [x] Apply-after-invalidation (close selected, then apply)
- [x] Fallback path exercised if History API unavailable (register-on-sight)

## Risk note

Pinning is **required** for M2 exit. Shipping without a matrix row is a process failure, not an optional doc gap.
