# Hyprland compatibility matrix

**Status:** Living — must be filled before M2 ships a `.so`  
**Related:** SPEC REQ-H-004, ADR-005, HYPRLAND-PLUGIN-SYSTEM.md

## Policy

1. Every loadable plugin release documents a **tested Hyprland git commit** (and/or release tag).
2. Plugins **must** fail closed on header hash mismatch.
3. Internal APIs (`Desktop::History::...`, `Desktop::focusState::...`) are **adapter-private** and version-gated — not part of the user contract.
4. If an internal symbol is missing on the pinned revision, the adapter uses the **fallback** path documented below and logs once.

## Known host limitations

| Limitation | Evidence | Impact | Mitigation |
|------------|----------|--------|------------|
| On the **Lua keybind path**, release binds on a *modifier* key never fire | 2026-09-20 bisect on `efb5099`/`v0.56.2` host (`docs/agent-state/research/2026-09-20-lua-modifier-release-bisect.md`): `hl.bind("ALT + ALT_R"/"ALT + ALT_L", …, {release=true})` → no callback (modmask 8); ordinary keys with the same modmask fire (`F9` release modmask 0 — fires; `"ALT + TAB"` release modmask 8 — fires) | Apply-on-release silently broken for the documented `hl.bind("ALT_L", {release=true})` recipe; the hyprlang `bindrt = ALT, ALT_L` path on the same pin keeps working | ADR-022: Lua recipe applies on **Tab** release (`examples/mru-switcher-bindings.lua`); hyprlang recipe unchanged |

## Matrix

| Plugin version | Hyprland commit | Hyprland tag | CI/nest tested | Notes |
|----------------|-----------------|--------------|----------------|-------|
| 0.0.0 (docs) | — | — | n/a | M0 docs only |
| 0.2.0 | `efb50993780079460b0cbed1363e2166a2de1d9f` | `v0.56.2` | **nest tested 2026-09-15, re-verified 2026-09-17** | First `.so`; headers at `/usr/include/hyprland` (distro `hyprland` pkg). Nest deps: **aquamarine `0.15.0`** (`libaquamarine.so.14`), Wayland backend. Live re-verify: `docs/agent-state/research/2026-09-17-nest-aquamarine-diagnosis.md` |
| 0.5.0 → 1.0 | `efb50993780079460b0cbed1363e2166a2de1d9f` | `v0.56.2` | **nest tested 2026-09-15..19 (M3–M5 smokes)** | **IPC caveat — host limitation (M6-T1):** `hyprctl dispatch` prints only `ok`; the `mru:status` payload (SPEC §3.4) rides in the error field of a *successful* dispatch result, which `hyprctl` surfaces only on failure — read it via a libwayland dispatcher binding or the plugin log. Host limitation, not a plugin defect; failure-path errors (e.g. `mru-switcher: not initialized`) do surface through `hyprctl` |
| 1.0.0 (pending tag) | `efb50993780079460b0cbed1363e2166a2de1d9f` | `v0.56.2` | hyprpm install smoke pending release tag (M6-T9); **stress smokes PASS 2026-09-19** (see §Stress evidence) | hyprpm distribution via `hyprpm.toml`: `commit_pins` populated (Hyprland `efb5099` → plugin `320c4cb`, finalized in the M6-T9 prep pass — re-verify against the tagged commit), repository metadata + build stanza verified by a clean-checkout build (M6-T5, issue #47) |
| 1.0.0 (pending tag) | `5c9377c15f85c50648f35ca5a213754f95b93ca0` | `v0.56.1` | **untested** — declared in `hyprpm.toml` `commit_pins` for users on this compositor, no nest run on this revision | Declared-but-untested older pin (multi-pin pass, M6-T9 prep follow-up); only `v0.56.2` is nest-verified; plugin-side hash re-verified at the tagged commit |
| 1.0.0 (pending tag) | `36b2e0cfe0c6094dbc47bd42a437431315bb3087` | `v0.56.0` | **untested** — declared in `hyprpm.toml` `commit_pins` for users on this compositor, no nest run on this revision | Declared-but-untested older pin (multi-pin pass, M6-T9 prep follow-up); only `v0.56.2` is nest-verified; plugin-side hash re-verified at the tagged commit |
| 1.0.0 (pending tag) | `af923e30d1d24f1f4a4f5cb8308065173c1d9539` | `v0.55.0` | **untested** — declared in `hyprpm.toml` `commit_pins` for users on this compositor, no nest run on this revision | Declared-but-untested older pin (multi-pin pass, M6-T9 prep follow-up); only `v0.56.2` is nest-verified; plugin-side hash re-verified at the tagged commit |
| 1.0.0 (pending tag) | `0002f148c9a4fe421a9d33c0faa5528cdc411e62` | `v0.54.0` | **untested** — declared in `hyprpm.toml` `commit_pins` for users on this compositor, no nest run on this revision | Declared-but-untested older pin (multi-pin pass, M6-T9 prep follow-up); only `v0.56.2` is nest-verified; plugin-side hash re-verified at the tagged commit |

### Stress evidence (M6, pin v0.56.2)

- **Rapid Tab (T-H-08, issue #52):** `docs/agent-state/reports/2026-09-19-m6-rapid-tab-smoke.md` — 200× `mru:cycle next` in **0.89s**, zero errors, focus immobile during cycles, exactly one focus change on `mru:apply`; 200 cycles + 4 apply checkpoints in **1.09s**; no-side-effect invariant + manual-focus history rotation + clean teardown all PASS. Live behavior matches the domain hammer (`t_h_08_*`, 1000 cycles, zero focus, exact-once-apply).
- **Window-close storm (T-H-06, issue #53):** `docs/agent-state/reports/2026-09-19-m6-window-close-smoke.md` — kill 1 mid-session window → apply lands exactly once on the MRU neighbor; kill all-but-one → focus on survivor; kill all mid-session → graceful end (`status ok`, nest + plugin alive, no crash); fresh-windows final session + clean teardown all PASS (SPEC §2.6/§2.8, REQ-S-006).
- **Monitor disconnect (T-H-07, issue #51):** `docs/agent-state/reports/2026-09-19-m6-monitor-disconnect-smoke.md` — 2nd monitor feasible **only** via `hyprctl output create headless` (config `monitor=` lines ignored on this pin); `output remove` mid-`scope=monitor`-session → apply lands once on the MRU neighbor, no crash; scope=monitor positive PASS. Compositor migrates windows to the surviving monitor (no ghosts).
- **Observation (CLOSED by ADR-021, M6):** back-to-back `cycle→apply` sessions used to re-land on the same window when chained within the 400 ms debounce window (issue #65) — the promotion of the applied window was scheduled through the debounce and then cancelled by lock-in at the next session start. ADR-021 makes lock-in mandatory and flushes a still-pending promotion at session start (REQ-H-011), so chained sessions rotate deterministically (A↔B); real held-Alt rapid-Tab (single session) was never affected. Regressions: domain `t_h_10_*`, `t_h_11_*`.

## Internal API expectations (adapter)

| Capability | Preferred symbol / path | Fallback if unavailable |
|------------|-------------------------|-------------------------|
| MRU seed | `Desktop::History::windowTracker()->fullHistory()` | Event-sourced list from `Event::bus()->m_events.window.active` only |
| Focus | `Desktop::focusState()->fullWindowFocus` | Document exact call used on pin; must still go through FocusGateway |
| Timers | `CEventLoopTimer` via `g_pEventLoopManager` | Document in adapter notes; must satisfy SchedulerPort |
| Config reload signal | `Event::bus()->m_events.config.reloaded` | Re-read config values only at next session if signal absent |
| Window id | Stable address used by hyprctl + generation in plugin registry | Generation always plugin-local |
| Border highlight (M4, `ui=border`) | Per-window `setprop address:0x<ptr> active_border_color <color>` / `inactive_border_color <color>` via `HyprlandAPI::invokeHyprctlCommand` (public; in-process hyprctl router, `PRIORITY_SET_PROP`; no focus side effect) | Unavailable/failed → `NullUI` + warn-once (REQ-UI-002); `IHyprWindowDecoration` documented fallback only
| Overlay fd watch (M5, `ui=external`) | `wl_event_loop_add_fd(g_pCompositor->m_wlEventLoop, fd, WL_EVENT_READABLE, cb, this)` + `wl_event_source_remove(source)` (`wayland-server.h`; removable; ADR-019) | `CEventLoopManager::doOnReadable` **rejected** — takes ownership of the fd and returns no removable handle, so a disconnected peer's watch would leak until unload (REQ-O-008). Unavailable/failed → `NullUI` + warn-once (REQ-O-001) |

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
> defect**; the SPEC §3.4 format was informative before the M6-T1 contract freeze and is
> **normative now** (SPEC §0 / §3.4); the visibility caveat remains a **host** limitation, not a SPEC
> violation, a contract gap, or an ADR trigger. Details + verdict table:
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

### M4 restore-on-cancel live confirmation (2026-09-19) — recorded

Full report: `docs/agent-state/reports/2026-09-19-m4-restore-on-cancel.md`; raw evidence `/tmp/mru-nest-restore/report/`; build `build-plugin-restore/mru-switcher.so` (branch `feat/m4-restore-on-cancel` @ `bb03942`, sha256 `daa5bd97189c0612b263197cf093fb462f4e1c895bb85ad259c8d21798467897`, 448 328 B) on pin `efb5099` (v0.56.2, aquamarine 0.15.0, Wayland nested, single monitor). Channel: config file + `hyprctl reload` only (D1).

| Row | Config | Verdict | One-line evidence |
|-----|--------|---------|-------------------|
| N1 | `ui=border`, `restore=1` | **PASS** | cycle ×2 left `activewindow` on the origin (REQ-F-003) while the highlight followed C→B; `mru:cancel` after a forced focus displacement refocused the origin and every window's `active_border_color`/`inactive_border_color`/`border_size` were byte-exact the pre-session baseline — no `0deg`, no empty gradient, no stuck highlight (D2 fix confirmed live) |
| N2 | `ui=null`, `restore=1` | **PASS** | zero border prop deltas in every shot; cancel (from a displaced focus) refocused the origin |
| N3 | `ui=border`, `restore=1`, origin closed mid-session | **PASS** | origin killed via `killactive`; session survived the prune (highlight stayed), `mru:cancel` changed no focus and borders were restored; no crash, plugin still loaded. Scope of this row: no crash / no focus side effect with a dead origin — the dead-origin no-op is *also* what weak-lock validity guarantees (ADR-013/016), so the explicit `is_valid` guard itself is unit-discriminated by **T-S-06** (review mutation M3), not by this live row |
| N4 | `ui=border`, `restore=1` → mid-session file edit to `restore=0` + `hyprctl reload` | **PASS** | cancel **still** refocused the origin (frozen policy, REQ-S-009) while `getoption plugin:mru-switcher:restore_focus_on_cancel` already read `int: 0`; a NEW session then cancelled **without** refocusing (new value applies to the next session) |
| N1b (extra, additive) | `ui=border`, `border_size=4`, `restore=1` | **PASS** | size override `4` visible during the session, restored to the compositor's `2` after cancel (REQ-UI-008 restore leg) |

**Boundary note:** the non-cancel-end leg (T-S-10 — "a session ended for any other reason never moves focus") stays **unit-only**. Discriminating it live requires the origin to remain valid while the session ends for a different reason (e.g. the origin outside the session scope / on a second monitor), which this single-monitor nest cannot stage honestly; no weak live substitute was improvised. Covered by T-S-10 in `tests/domain/test_session_controller.cpp` (ctest 12/12 on this build).

## M5 external overlay socket — mechanism (pin `efb5099`, v0.56.2)

**Source of truth:** `docs/agent-state/research/2026-09-19-m5-overlay-socket-api.md` (R0 memo) + ADR-018/ADR-019. Socket I/O runs on the compositor main thread through the Wayland event loop; no blocking work on the `mru:*` dispatcher path (REQ-PERF-001/003).

- **Registration:** listener + first accepted client fd are watched with `wl_event_loop_add_fd(g_pCompositor->m_wlEventLoop, …)` and removed with `wl_event_source_remove(...)` (ADR-019).
- **Rejected primitive:** `CEventLoopManager::doOnReadable` was verified present on the pin but **not usable** here — it consumes the `CFileDescriptor` and exposes no handle to cancel the waiter, so a peer disconnect could not be cleaned up. See ADR-019.
- **Transport:** AF_UNIX `SOCK_STREAM`, single client, `O_NONBLOCK`, `MSG_NOSIGNAL`; send is best-effort (drop on `EAGAIN`/`EWOULDBLOCK`/`EPIPE`/error).
- **Config bind timing:** on this pin the registered config values are not populated during `PLUGIN_INIT`; the socket is therefore bound on the `config.reloaded` that follows load (and again, idempotently, at session start). Clearing `external_socket` or switching `ui` away from `external` tears the listener down (REQ-O-001).
- **Reload vs active session:** a `config.reloaded` that switches `ui` away from `external` (or clears the path) stops the listener immediately; the frozen in-session `ExternalOverlayUI` then sends best-effort to no peer and behaves as `ui = null` (REQ-O-002/REQ-UI-009).
- **Protocol:** frozen in SPEC §12 Appendix B; reference peer `tools/overlay_stub.py`.

### M5 nest smoke (2026-09-19) — recorded

Full report: `docs/agent-state/reports/2026-09-19-m5-s3-nest-smoke.md`; raw evidence `/tmp/mru-nest-m5/report/`. Build `build-plugin-m5/mru-switcher.so`, pin `efb5099` (v0.56.2, aquamarine 0.15.0, Wayland nested). 13/13 rows PASS: load/bind before first session, peer `session_start`/`selection`/`session_end`, peer `select`/`apply`/`cancel`, out-of-bounds `select` ignored, empty-path degrade, path restore, unload mid-session (no crash/leak), repeated reload cycles. Three branch-only defects were found and fixed during the smoke (dangling handler capture → SEGV; lazy bind; socket left listening after path cleared).

### Known limitations (0.5.0)

- **Reloading a changed `.so` from the same path in the same Hyprland process crashes the
  compositor when the file is overwritten **in place** (same inode) — M6-B1 repro (issue #55).**
  Sharpened trigger: the M5-era "intermittent" crash is **deterministic (3/3)** when the `.so` at the
  already-used path is replaced by `cp` in place (same inode) between `plugin unload` and
  `plugin load` — byte content is irrelevant (a same-bytes overwrite crashed too). Replacing the file
  via a **new inode** (`rm` + `cp`, atomic rename) reloads cleanly (3/3). Crash site: `dlsym` inside
  `CPluginSystem::loadPluginInternal` on `dlopen` of the same-inode-rewritten file (glibc keeps the
  unloaded object keyed by path/(dev,ino) and revisits stale symbol-table state). Repro evidence:
  `docs/agent-state/reports/2026-09-20-m6-b1-reload-repro.md` (6 runs, 3 specified changed-binary
  cycles + 3 controls, crash reports preserved). **Operator guidance:** never overwrite
  `mru-switcher.so` in place — install updates via `mv`/rename (new inode) or a new path, and prefer
  a fresh compositor (or full `hyprctl`-level restart of the session) for upgrades. A normal
  `load`/`unload` cycle of an **unchanged** binary stays clean (M5 control, 13/13).
- **Non-cancel session end is unit-only live.** Discriminating "a session ended for any other reason
  never moves focus" live requires a second monitor / out-of-scope origin, which the single-monitor
  nest cannot stage honestly; covered by `t_s_10_*` (see the M4 boundary note above).
- The three M5 smoke defects (dangling overlay command-handler capture, lazy socket bind, listener
  surviving path clear) were **fixed on the M5 branch** and are not open issues in 0.5.0.

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

**Pinning is a release contract.** Every plugin release (0.x row and 1.x alike) ships against a
documented primary Hyprland commit — `efb50993780079460b0cbed1363e2166a2de1d9f` (v0.56.2) for v1.0.0 — and
the same pins are machine-readable in `hyprpm.toml` under `commit_pins` (Hyprland SHA → plugin SHA).
Older pins (`v0.56.1` / `v0.56.0` / `v0.55.0` / `v0.54.0`) are declared-but-untested (matrix rows above);
only `v0.56.2` carries nest evidence.
The plugin-side hashes are finalized in the multi-pin pass (M6-T9 prep follow-up; annotated in the
manifest) and MUST be re-verified against the tagged commit before the v1.0.0 release is published.
The plugin fails closed on header-hash
mismatch (§Policy), so an unpinned rebuild against a newer Hyprland is a rebuild event, not a silent
compatibility window.
