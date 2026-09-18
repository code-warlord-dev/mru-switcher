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
| 0.2.0 | `efb50993780079460b0cbed1363e2166a2de1d9f` | `v0.56.2` | **nest tested 2026-09-15, re-verified 2026-09-17** | First `.so`; headers at `/usr/include/hyprland` (distro `hyprland` pkg). Nest deps: **aquamarine `0.15.0`** (`libaquamarine.so.14`), Wayland backend. Live re-verify: `docs/agent-state/research/2026-09-17-nest-aquamarine-diagnosis.md` |

## Internal API expectations (adapter)

| Capability | Preferred symbol / path | Fallback if unavailable |
|------------|-------------------------|-------------------------|
| MRU seed | `Desktop::History::windowTracker()->fullHistory()` | Event-sourced list from `Event::bus()->m_events.window.active` only |
| Focus | `Desktop::focusState()->fullWindowFocus` | Document exact call used on pin; must still go through FocusGateway |
| Timers | `CEventLoopTimer` via `g_pEventLoopManager` | Document in adapter notes; must satisfy SchedulerPort |
| Config reload signal | `Event::bus()->m_events.config.reloaded` | Re-read config values only at next session if signal absent |
| Window id | Stable address used by hyprctl + generation in plugin registry | Generation always plugin-local |
| Border highlight (M4, `ui=border`) | Per-window `setprop address:0x<ptr> active_border_color <color>` / `inactive_border_color <color>` via `HyprlandAPI::invokeHyprctlCommand` (public; in-process hyprctl router, `PRIORITY_SET_PROP`; no focus side effect) | Unavailable/failed → `NullUI` + warn-once (REQ-UI-002); `IHyprWindowDecoration` documented fallback only

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

### Nest recipe (verified 2026-09-17, Hyprland v0.56.2 / aquamarine 0.15.0)

```bash
mkdir -p /tmp/mru-nest-diag/cache
env -u HYPRLAND_INSTANCE_SIGNATURE -u HYPRLAND_CONFIG \
  XDG_CACHE_HOME=/tmp/mru-nest-diag/cache \
  Hyprland -c /path/to/hypr-nest.conf > nest.log 2>&1 &
hyprctl instances -j          # read the nest SIG + wl_socket (usually wayland-2) from here
hyprctl -i "$SIG" plugin load /abs/path/to/mru-switcher.so
```

- **Isolation:** unset `HYPRLAND_INSTANCE_SIGNATURE` / `HYPRLAND_CONFIG` so the nest cannot inherit the
  host config; pass `-c` explicitly. `XDG_CACHE_HOME` isolates the cache but **not** the instance dir —
  logs land in `/run/user/<uid>/hypr/<SIG>/hyprland.log`.
- **Discover the nest by `hyprctl instances -j`**, *not* `ls -t /run/user/<uid>/hypr`. The latter is
  unreliable: stale instance dirs from earlier runs tie on mtime and you can pick the wrong SIG
  (`/tmp/mru-nest-diag/env` uses `ls -t`, which is why the recipe here supersedes it).
- **DRM/seat note:** the nest's aquamarine tries DRM first and always fails (`seatd.sock` missing;
  logind `Device or resource busy` because the host owns the seat), then **falls back to the Wayland
  backend**. `DRM Backend failed` in a nest log is expected and benign — not a nest blocker.
- `--socket NAME` alone is rejected (needs `--wayland-fd`); there is no `--headless` CLI flag on this
  pin — headless only via `AQ_HEADLESS=1`.
- Expected startup noise: `Invalid dispatcher: mru:*` at parse time (plugin not yet loaded),
  `wayland-1.lock` probe warning, xkbcomp warnings, Xwayland `could not connect to wayland server`.
- Never point `plugin load` at the host session; keep `WAYLAND_DISPLAY`/`SIG` scoped to the nest.

### Live re-verify 2026-09-17

Re-ran the matrix + invariants below against the same pin (`0.2.0` / `efb5099` / `v0.56.2`), aquamarine
`0.15.0`, Wayland backend: **all rows PASS**, invariants PASS, B2 adjudicated live (applied window
becomes MRU head → Alt+Tab-like). Evidence: `/tmp/mru-nest-diag/report/04-*`; artifact:
`docs/agent-state/research/2026-09-17-nest-aquamarine-diagnosis.md`. **Not covered by this run:**
M3-S3 §7 scope adapter (not implemented), multi-monitor, `ui=border`/`external`.

### Dispatcher Matrix

> **IPC note (0.56.2, verified 2026-09-17) — `mru:status` payload is not observable via `hyprctl`.**
> `hyprctl dispatch mru:status` prints bare `ok`. The status string is carried in the *success*
> result's `error` field, and Hyprland 0.56.2's IPC surfaces that field only on **failure** (raw
> socket `.socket.sock` behaves identically). Therefore the `Actual` payloads recorded for
> `mru:status` below are **not reproducible as written via `hyprctl`** on this pin — they were captured
> some other way (notification/log/patched client) or recorded aspirationally. The rows are **kept**,
> not deleted; treat them as "capture mechanism unspecified". The plugin itself computes the string
> (failure-path strings do surface), so this is a **documentation/reproducibility defect, not a plugin
> defect**; SPEC §3.4 makes the format informative and non-parsable until 1.0, so this is not a SPEC
> violation and needs no ADR. Details + verdict table:
> `docs/agent-state/research/2026-09-17-nest-aquamarine-diagnosis.md` (§SPEC verdict).
> For smoke runs, assert status state indirectly via `hyprctl activewindow -j` / `focusHistoryID`.

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
- [x] `mru:status` returns correct payload at all states (D7) — ⚠ payload **not observable via `hyprctl` on 0.56.2** (see IPC note above); state verified indirectly 2026-09-17
- [x] Scope token without direction accepted (REQ-DISP-003, D6)
- [x] Pre-load windows visible from first cycle (D4 regression fix)

### Observations

- **Startup config-parse errors (red error frame):** when the nest config already binds `mru:*` dispatchers
  (`bind = ALT, TAB, mru:cycle` …) and the plugin is then loaded manually with `hyprctl plugin load`, the
  **initial** config parse logs `Invalid dispatcher: mru:cycle/apply/cancel` and Hyprland shows the red
  error frame. This is startup-order noise, **not** a plugin defect: the plugin is not yet registered at
  parse time. After `plugin load` the built-in `config.reloaded` re-resolves the binds and the dispatchers
  work normally (all matrix commands above pass). Avoided automatically on a hyprpm install, where the
  plugin loads before the config is parsed. No code change in M2 — documented here.
- First `dispatch` right after `plugin load` may report `Invalid dispatcher` until the post-load config reload finishes; second attempt is fine. No code change in M2 — documented here.
- Registry `by_address_` grows monotonically (closed entries not cleaned). Acceptable for M2; GC candidate for post-M3.

## M4 Border UI — mechanism (R0, pin `efb5099`, v0.56.2)

**Source of truth:** `docs/agent-state/research/2026-09-17-m4-border-api.md` (R0 memo). Every mechanism claim cites the pin `efb50993780079460b0cbed1363e2166a2de1d9f` (= v0.56.2), not `main`.

- **Primary mechanism (public props first, ADR-017 / REQ-UI-011):** per-window `setprop address:0x<ptr> active_border_color <color>` / `inactive_border_color <color>` via `HyprlandAPI::invokeHyprctlCommand(...)` — the same synchronous in-process hyprctl router the IPC socket uses (`g_pHyprCtl->makeDynamicCall`), on the calling (main) thread, `PRIORITY_SET_PROP`. Wins above window rules/groups; **no focus side effect** (REQ-F-003, REQ-UI-006). Window selector `address:0x<ptr>` (lowercase hex) matches the plugin's `WindowRef.address`.
- **Restore (no public `unset`):** there is no public reset for border props at this pin; `-1` yields an empty gradient = **invisible border**, not the user's colour. Therefore restore is **by value**: capture the effective colour via `getprop` before overriding, write it back through the same `setprop` path on selection change / session end / unload. `teardown_state()` runs the restore before the UI backend is destroyed.
- **Fallback:** decoration path (`IHyprWindowDecoration`, borders-plus-plus pattern) is a **documented fallback only**, not the M4 implementation. If the primary path fails at runtime → behave as `null` + warn-once (REQ-UI-002).

### M4 nest smoke row (planned — `ui=border`)

Full 9-step recipe in the R0 memo (“Open questions / proposed live nest experiment”); headline checks:

| Check | Expected |
|-------|----------|
| `ui=border`, `border_color` default, cycle ×3 | only the selection window's border highlighted; `hyprctl activewindow -j` unchanged (REQ-F-003) |
| Restore probe (getprop before/after; `-1` vs user colour) | decides restore-by-value vs `-1` reset (R0 open question F10) |
| Apply / cancel / Escape | exactly one focus; all borders restored (no stuck) — M4 exit |
| `plugin unload` mid-session | no crash; borders restored (REQ-UI-005) |
| Bogus prop name in a test build | warn-once + null behaviour; apply/cancel unaffected (REQ-UI-001/002) |

### M4 nest smoke (2026-09-18) — recorded

Full report: `docs/agent-state/reports/2026-09-18-m4-s3-nest-smoke.md`; raw evidence `/tmp/mru-nest-m4/report/`. Two mechanism findings recorded against pin `efb5099` (= v0.56.2):

1. **setprop/getprop grammar asymmetry (D2).** `getprop … active_border_color` returns unprefixed `<hex6> <N>deg` (e.g. `ff44cc88 0deg`), while `setprop` accepts ONLY `0x…`/`rgb()`/`rgba()` **without** the angle suffix; unprefixed/angle-suffixed values parse to an **empty gradient** (invisible border). Restoring the captured `getprop` output verbatim therefore corrupted borders (smoke step 4; F10's empty-gradient hazard confirmed live on the restore path). Fixed in `fix/m4-s3-border-restore-grammar` (commit `98c0dd5`): `normalize_capture` in `BorderHighlightUI` rewrites every captured reply into a setprop-safe form. Acceptance map: `/tmp/mru-nest-m4/report/02g-grammar-map.txt`.
2. **`hyprctl keyword` channel limitation (D1 — COMPAT/USER).** `hyprctl keyword plugin:mru-switcher:<key> <value>` is accepted by hyprlang (getoption reflects it) but never reaches the plugin's cached config on this pin: `st.config` refreshes only on the `config.reloaded` event, which a config-file edit + full `hyprctl reload` fires and a keyword change does not (discriminators: `/tmp/mru-nest-m4/report/02e-configfile-reload.txt`, `02f-keyword-discriminator.txt`; plugin handle unchanged across reload — no restart involved). **Config file + `hyprctl reload` is the supported runtime channel.** This matches SPEC REQ-CFG-002/003 (refresh on `config.reloaded`) — no SPEC change; user-facing note in `docs/USER.md` "Config reload".
3. **R0 memo correction (border_size).** `getprop … border_size` IS supported on this pin and returns the effective integer (probe: `/tmp/mru-nest-m4/report/09-border-size-probe.txt`); after a `setprop`-carried `unset` the effective size is unchanged, so size restore is by captured value with an `unset` fallback (`border_size = -1` default keeps the size untouched, REQ-UI-008).

## Verification checklist (per release)

- [x] Hash check passes on load
- [x] `mru:cycle` / `mru:apply` / `mru:cancel` smoke
- [x] Debounce does not crash on unload
- [x] Apply-after-invalidation (close selected, then apply)
- [x] Fallback path exercised if History API unavailable (register-on-sight)

Re-verified live **2026-09-17**: full dispatcher matrix + invariants re-run on this pin; B2 (applied
window becomes MRU head) confirmed via `focusHistoryID`. Evidence: `/tmp/mru-nest-diag/report/04-*`.
See `docs/agent-state/research/2026-09-17-nest-aquamarine-diagnosis.md`.

## Risk note

Pinning is **required** for M2 exit. Shipping without a matrix row is a process failure, not an optional doc gap.
