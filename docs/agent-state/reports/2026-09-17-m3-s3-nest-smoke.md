# Nested-smoke report — M3-S3 scope adapter (§7 checklist, issue #20)

**Date:** 2026-09-17
**Plan:** `docs/agent-state/plans/2026-09-17-m3-s3-scope-adapter.md` §7 (lines 158-186)
**Build under test:** `/home/code_warlord/Work/DEV/mru-switcher/build-plugin/mru-switcher.so` (M3-S3 HEAD `eafc0aa`; loaded OK, hash check passed; plugin list reports `Version: 0.2.0` as printed string — version string not bumped, cosmetic only)
**Pin:** Hyprland v0.56.2 / aquamarine 0.15.0 (COMPAT.md §"Nest recipe"), Wayland backend (DRM fallback noise expected)
**Nest sessions:** 3 runs (run-1 `…1789672639`, run-2 `…1789672935`, run-3 `…1789673373`); final nest killed, host pid 1245 / wayland-1 untouched, host `plugin list` = `no plugins loaded`
**Raw evidence:** `/tmp/mru-nest-m3/report/*.txt` (paths per row below), nest config `/tmp/mru-nest-m3/hypr-nest.conf`, env `/tmp/mru-nest-m3/env`, log `/tmp/mru-nest-m3/nest.log`

## Results

| # | Step | Expected | Actual | Verdict |
|---|------|----------|--------|---------|
| 1 | Load + config surface | `plugin load` OK (hash); 8 `plugin:mru-switcher:*` keys populating `getoption`, defaults per SPEC §4 | Loaded OK. All 8 keys present individually: `debounce_ms=int 400`, `default_scope=str global`, `start_offset=str second`, `wrap=int 1`, `ui=str null`, `lock_history_on_session=int 1`, `restore_focus_on_cancel=int 0`, `external_socket=str ""` (empty). Note: `hyprctl getoption plugin:mru-switcher:*` wildcard is NOT supported on 0.56.2 (`no such option`) → enumerated the 8 documented keys individually. | **PASS** |
| 2 | Scratchpad shown | Scratchpad window (foot in `special:special`, shown + focused) is a candidate in every scope (global/monitor/workspace/visible/app) — REQ-SC-002a "shown" leg | All 5 scopes: apply-on-release focused S=special `0x56ef500ace40` multiple times (global ×12, monitor ×12, workspace ×16, visible ×5, app ×16 of 16 iterations) → S ∈ every candidate list | **PASS** |
| 3 | Scratchpad hidden | S (special hidden) absent from all five candidate lists — REQ-SC-002a "hidden" leg | 0/16 iterations in every scope focused S; global/monitor/workspace/visible = exactly the 3 normal ws1 windows, app = exactly the 2 `foot` windows (anchor foot) | **PASS** |
| 4 | 2 monitors / 2+ workspaces | `monitor`→mon-A only, `workspace`→ws1 only, `visible`→shown wss only, `global`→all | A@ws1 / B@ws1 (mon0), C@ws2 (mon1), D@ws3 (mon0), focus A. `global`={A,B,C,D} (4/4), `monitor`={A,B,D} (mon0 only), `workspace`={A,B} (ws1 only), `visible`={A,B,C} (W1-ws1 + H1-ws2 only). `output create headless` **worked** → cross-monitor fully exercised, not NOT-RUNNABLE | **PASS** |
| 5 | app byte-exact | Two same-class + one different-class; case-variant does NOT match — REQ-SC-002b | A=foot, B=Firefox, C=foot, D=Gmail, E=`Foot` (case-variant). `app` (anchor foot) = exactly {A,C}; Firefox, Gmail, and case-variant `Foot` excluded | **PASS** |
| 6 | Focus-less degrade | monitor/workspace/app behave as global; `visible` still filters | Per-session re-null (empty ws4, `activewindow`={}); `monitor`==`workspace`==`app`==`global` = same 5-window set; `visible` = {C} = the only window on a currently-shown workspace | **PASS** |
| 7 | T-SC-05 behavior | `mru:cycle bogus` → failure naming the unknown scope token; session NOT started — REQ-SC-003 | `mru:cycle bogus` → `unknown scope token: bogus`; `mru:cycle next bogus` → same; `mru:cycle next workspace extra` → `too many arguments` (distinct). Post-error `mru:apply` is a no-op (focus unchanged) → idle, session not started | **PASS** |
| 8 | M2 drop-in regression | Default cycle/apply/cancel on plain windows; MRU-first order, pre-load windows visible, apply focuses once, no cycle focus side-effect — issue #20 Goal, REQ-F-003, ADR-015 | Default `mru:cycle`×2: `activewindow` unchanged (no side effect). Order: focus D then E (0.7 s gaps) → fresh session apply focused second-of-MRU = D exactly. Apply once: focus stable at t+0.5 s and t+1.1 s. `mru:cancel` mid-session: focus unchanged. Pre-load run: 3 windows opened BEFORE `plugin load`; first cycle post-load OK and all 3 pre-load windows reachable via `global` (run-3, evidence 09-*) | **PASS** |

**Counts:** PASS 8 · FAIL 0 · NOT-RUNNABLE 0

## Verdicts

- **H-1 — scratchpad "shown" leg (REQ-SC-002a): PASS.** S shown+focused ∈ candidates of all five scopes (step 2); S hidden ⇒ excluded from all five (step 3). `window_meta.hidden` ↔ `visible_workspaces` share one source (adapter single-enumeration) and behaved correctly live.
- **M-1 — live M2 drop-in: PASS.** Default `mru:cycle`/`mru:apply`/`mru:cancel` behavior identical to M2 matrix: no cycle focus side-effect, MRU-first selection, apply-once, cancel-noop, pre-load windows visible from the first cycle.

## Gaps / not covered

- **Second monitor: fully covered** — `hyprctl output create headless` works on this pin (Wayland backend); cross-monitor rows in step 4 exercised (monitor/workspace/visible/global all matched plan). If the pin had refused headless output, step 4's cross-monitor half would have been NOT-RUNNABLE; it did not.
- **Disabled monitor scenario: NOT-RUNNABLE for this run** — §7 contains no disabled-monitor test row; disabling the host-facing output inside the nest (e.g. `output WAYLAND-1 disable`) would drop the primary backend surface and was deliberately not attempted. Worth an explicit row when a disabled-monitor scope row lands in the plan.
- **M-2** (milestone-two scope semantics): exercised live as step 8 (PASS); no separate M-2-only config/timeout path in §7.
- **`mru:status` payload**: not observable via `hyprctl` on this pin (COMPAT.md IPC note, known 0.56.2 limitation — not a plugin defect). All membership/idle assertions were made indirectly via `activewindow -j` after apply-on-release and focus-stability checks.
- **Observability nuance recorded:** rapid-fire `mru:cycle`→`mru:apply` without pauses coalesces the 400 ms history debounce, so the *visible pool* of a given snapshot shifts toward recently-focused windows. With ≥0.6 s pauses every scope assertion reproduced exactly per spec (evidence 03-dry, 06b, 08, 09). This is a test-tooling artifact, not a plugin defect.

## Raw evidence

```
/tmp/mru-nest-m3/report/01-config-surface.txt    step 1  (8 keys, defaults)
/tmp/mru-nest-m3/report/02a-scratch-shown-{global,monitor,workspace,visible,app}.txt  step 2 (apply→activewindow per scope)
/tmp/mru-nest-m3/report/02b-slow-global.txt      step 2/8 (slow-commit global control)
/tmp/mru-nest-m3/report/03-dry-global.txt        step 2/8 (clean 3-window global control)
/tmp/mru-nest-m3/report/03a-scratch-hidden-{...,app}.txt          step 3
/tmp/mru-nest-m3/report/04a-multi-{global,monitor,workspace,visible}.txt  step 4
/tmp/mru-nest-m3/report/05a-app-byteexact-app.txt               step 5
/tmp/mru-nest-m3/report/06a-focusless-{global,...}.txt          step 6 (first pass)
/tmp/mru-nest-m3/report/06b-focusless2-{global,...}.txt         step 6 (per-session re-null, canonical)
/tmp/mru-nest-m3/report/07-scope-token-error.txt step 7 (dispatch errors)
/tmp/mru-nest-m3/report/07-idle-check.txt        step 7 (idle/no-session proof)
/tmp/mru-nest-m3/report/08-m2-dropin.txt         step 8 (side-effect/order/apply-once/cancel)
/tmp/mru-nest-m3/report/09-preload-windows.txt   step 8 (pre-load windows visible from first cycle)
```