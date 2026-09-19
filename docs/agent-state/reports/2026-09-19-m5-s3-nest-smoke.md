# Nested-smoke report — M5-S3 external overlay socket (ROADMAP M5, SPEC §5.3/§12 Appendix B)

**Date:** 2026-09-19
**Build under test:** `/tmp/mru-nest-m5/mru-switcher.so` (final: `build-plugin-m5/mru-switcher.so` @ branch `feat/m5-external-overlay`).
**Pin:** Hyprland v0.56.2 / commit `efb50993780079460b0cbed1363e2166a2de1d9f`, aquamarine 0.15.0, Wayland nested backend.
**Nest(s):** fresh instances launched `env -u HYPRLAND_INSTANCE_SIGNATURE -u HYPRLAND_CONFIG XDG_CACHE_HOME=… Hyprland -c /tmp/mru-nest-m5/hypr-nest.conf`. Host (pid 1250, wayland-1) untouched: `plugin list` = `no plugins loaded` before, during, and after.
**Windows:** three foot terminals; per-instance addresses recorded in raw evidence (e.g. A=`0x5d8b1991bde0`, B=`0x5d8b1992c1f0`, C=`0x5d8b1993cfc0`).
**Peer:** `tools/overlay_stub.py` (reference implementation) over `/tmp/mru-nest-m5/overlay.sock`.
**Raw evidence:** `/tmp/mru-nest-m5/report/*.txt`, nest config `/tmp/mru-nest-m5/hypr-nest.conf`.

## Results

| # | Step | Expected | Actual | Verdict |
|---|------|----------|--------|---------|
| 1 | Load with `ui=external` | load OK; socket bound at/before first session | load `ok`; `getoption ui` = `external`; socket file + LISTEN present **right after load** (via the `config.reloaded` that follows load) | **PASS** |
| 2 | Peer connects before first session | stub connects, no events yet | `[stub] connected` before any `mru:cycle` | **PASS** |
| 3 | `mru:cycle` → `session_start` (REQ-O-003) | one message, full window list (addr/title/class), current index | `session_start: 3 window(s), index=1` with 3 entries marked `* [1]` | **PASS** |
| 4 | Peer `select n` → `selection` (REQ-O-004) | one `selection` per command; focus untouched (REQ-F-003/O-007) | `selection: index=0`, then `index=2`; `activewindow` unchanged throughout | **PASS** |
| 5 | Peer `apply` → `session_end(applied)` + focus (REQ-O-003/O-007) | session ends, focus moves exactly once | `session_end: reason=applied`; focus → `0x…92c1f0` | **PASS** |
| 6 | Peer `cancel` → `session_end(cancelled)`, no focus | cancel end, focus unchanged | `session_end: reason=cancelled`; focus unchanged | **PASS** |
| 7 | Dispatcher `mru:cancel` with peer attached | `session_end(cancelled)` emitted | `session_end: reason=cancelled` | **PASS** |
| 8 | `select` out of bounds (REQ-O-005/O-007) | ignored, no state/focus change, no crash | no `selection` emitted; session still Active; next `apply` used the prior index | **PASS** |
| 9 | `apply` with no prior `select` | applies current index | focus → `0x…93cfc0` (index 1) | **PASS** |
| 10 | `external_socket` → empty on reload (REQ-O-001) | socket torn down, backend degrades to null, plugin works | socket unlinked; `getoption external_socket` = `""`; `mru:cycle`/`cancel` `ok`; nest alive | **PASS** |
| 11 | Path restored on reload | socket re-bound; peer reconnects | socket file + LISTEN back; new stub connected; `session_start` + `apply` served | **PASS** |
| 12 | Unload mid-session (REQ-O-008) | socket + watches removed, no crash/leak | unload `ok`; session_end cancelled delivered; peer saw `plugin closed the connection`; socket unlinked; plugin list empty; nest alive | **PASS** |
| 13 | Repeated load/unload same binary | no crash, socket recreated each load | 2× reload cycles: alive, socket present after each load | **PASS** |

**Counts:** PASS 13 · FAIL 0

## Verdicts — ROADMAP M5 exit criteria

- **Protocol documented + plugin functional without overlay: YES.** External backend sends `session_start`/`selection`/`session_end`, accepts peer `select`/`apply`/`cancel`, honours bounds; with no peer (or no socket) session logic is unchanged (`ui=null` parity).
- **Best-effort / non-blocking: YES (by construction + unit tests).** All I/O is `O_NONBLOCK`; sends drop on backpressure; reads only on the main-loop event source. No dispatcher path touches the socket (REQ-PERF-001).
- **Fail-soft: YES.** Empty path/bind failure ⇒ `NullUI` + warn-once; mid-life path removal tears the socket down cleanly.

## Defects found and fixed during this smoke (branch-only, never on `main`)

| ID | Severity | Symptom | Root cause | Fix |
|----|----------|---------|-----------|-----|
| D3 | S1 (crash) | `PLUGIN_EXIT` SEGV in the first smoke build (`hyprlandCrashReport7048`) | `SessionUIBackendProxy` factory captured a **local** `CommandHandler` by reference; `ensure_started` move-assigned from the destroyed object (UB), then destroyed garbage on teardown | handler built by `overlay_command_handler()` and stored by value; factory captures only `PluginState` (commit `97f0d56`) |
| D4 | S2 | socket never bound at load; only appeared after the first `mru:cycle` | on this pin config values are **not yet populated during `PLUGIN_INIT`**, so `ui` read as default at `build_state` | bind also on `config.reloaded` (first reliable moment); factory remains the safety net |
| D5 | S3 | peer could connect while the backend was `null` (socket left listening after path cleared) | socket was only ever started, never stopped on config change | `try_start_overlay_socket()` stops the socket when the effective backend is not `external` or the path is empty (REQ-O-001/008) |

**Observation (not a code defect):** a second crash (`hyprlandCrashReport75967`) occurred when a **different** binary was loaded at the same `.so` path inside a process that had already loaded/unloaded the plugin, and did not reproduce on a fresh process. Likely Hyprland `dlopen` handle reuse across a changed file. Operator note: reload the plugin in a **fresh nest** when the binary changed, or load from a new path.

## Gaps / not covered

- `select` with a non-integer/oversized/malformed line was unit-tested (`test_overlay_protocol`, `test_overlay_socket_server`), not re-driven live through the reference stub (the stub only emits well-formed commands).
- Multiple concurrent peers (single-client policy) not exercised live; unit-covered (`poll_accept` drops extras).
- `mru:status` payload still unobservable via `hyprctl` on this pin (M3 precedent).
- The `wl_event_loop_add_fd` mechanism (vs the `doOnReadable` named in ADR-018/REQ-O-006) is an implementation deviation requiring SPEC/ADR follow-up — see S5.

## Raw evidence

```
/tmp/mru-nest-m5/report/00-baseline.txt          nest/instance, host plugin list, options, no socket
/tmp/mru-nest-m5/report/01-load.txt              first-build load (socket still lazy) — historical
/tmp/mru-nest-m5/report/01b-clients-new.txt      three foot windows (fresh instance)
/tmp/mru-nest-m5/report/02-session-start.txt     step 3: session_start 3 windows index=1
/tmp/mru-nest-m5/report/03-select-apply.txt      steps 4/5: selection 0/2, session_end applied
/tmp/mru-nest-m5/report/04-cancel-paths.txt      steps 6/7: peer + dispatcher cancel
/tmp/mru-nest-m5/report/05-bounds-unload.txt     steps 8/9/12: bounds, apply, unload mid-session
/tmp/mru-nest-m5/report/06-final-unload.txt      step 13: reload cycles, final unload, host untouched
```
