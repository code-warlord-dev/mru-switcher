# Nested-smoke report — M4 restore-on-cancel (live matrix N1–N4, REQ-R-001/002, REQ-S-005/007/009, REQ-F-003)

**Date:** 2026-09-19
**Build under test:** `/home/code_warlord/Work/DEV/mru-switcher/build-plugin-restore/mru-switcher.so` — branch `feat/m4-restore-on-cancel` @ `bb0394205fa535d22e3c9ac1722e1f3022c554b7`, dedicated build dir configured `-G Ninja -DCMAKE_BUILD_TYPE=Release -DMRU_BUILD_PLUGIN=ON -DMRU_BUILD_TESTS=ON -DMRU_WARNINGS_AS_ERRORS=ON`, built clean (`cmake --build build-plugin-restore`); sha256 `daa5bd97189c0612b263197cf093fb462f4e1c895bb85ad259c8d21798467897`, mtime `2026-09-19 11:24:20 +0300`, 448 328 B. Loaded **by absolute path** in the nest → `ok` (hash check passed; no mismatch line in `nest.log`); `plugin list` → handle `5f2e03978200`, `Version: 0.4.0` (plugin handle unchanged across all reloads → no restart involved).
**Pin:** Hyprland v0.56.2 / commit `efb50993780079460b0cbed1363e2166a2de1d9f` (`pkg-config --modversion hyprland` = `0.56.2`; distro `hyprland 0.56.2-2`), aquamarine `0.15.0-2` (`libaquamarine.so.14`), Wayland nested backend (host Wayland session; no `--headless` flag on this pin).
**Nest:** 1 session — instance `efb50993780079460b0cbed1363e2166a2de1d9f_1789806305_1928909522` (pid 11613, `wl_socket wayland-2`), discovered **only via `hyprctl instances -j`**, launched

```bash
env -u HYPRLAND_INSTANCE_SIGNATURE -u HYPRLAND_CONFIG \
  XDG_CACHE_HOME=/tmp/mru-nest-restore/cache \
  Hyprland -c /tmp/mru-nest-restore/hypr-nest.conf > /tmp/mru-nest-restore/nest.log 2>&1 &
```

booted 11:25:05, **torn down (SIGTERM 11613, clean) 11:29:09**; +10 s re-check: pid gone, instance absent from `instances -j`, foot clients 0. Host pid **1250** / `wayland-1` untouched; host `plugin list` = `no plugins loaded` **before, during, after**.
**Windows:** three foot terminals opened **BEFORE plugin load** (ws1; nest `general { border_size = 2; col.active_border = 0xff44cc88; col.inactive_border = 0xff333333 }` ⇒ baseline getprop `ff44cc88 0deg` / `ff333333 0deg` / `2`) — `A=footA 0x5f2e02eff940`, `B=footB 0x5f2e02f3fb70`, `C=footC 0x5f2e032326e0`. A is deliberately closed inside N3 (clients afterwards: C + B).
**Channel:** config-file edit + `hyprctl -i "$SIG" reload` **only** (`hyprctl keyword` is dead on this pin — D1, COMPAT 2026-09-18). Sessions driven deterministically with `hyprctl -i "$SIG" dispatch 'mru:cycle next'` / `'mru:cancel'` (`bindrt` release is not injectable from CLI — M2/M3/M4 precedent).
**Raw evidence:** `/tmp/mru-nest-restore/report/*.txt` (list below); nest config `/tmp/mru-nest-restore/hypr-nest.conf` + final copy `report/96-nest-conf-final.txt`; full nest log `report/97-nest-log-full.txt`; helpers `h.sh`, `sweep.sh`, `aw.sh`.
**Unit baseline (same build):** `ctest --test-dir build-plugin-restore` → **12/12 passed** (T-S-09/T-S-10 live inside `domain_session_controller`). Nest evidence is the deliverable; unit run is context only.

## Results

| # | Config | Step | Expected | Actual | Verdict |
|---|--------|------|----------|--------|---------|
| **N1** | config file `ui=border`, `restore_focus_on_cancel=1` (plugin `border_size` default `-1` → not overridden) | baseline getprop A/B/C; `mru:cycle next` ×2; mid-session focus displacement (`dispatch focuswindow C`); `mru:cancel` | highlight follows selection; `activewindow` unchanged during cycles (REQ-F-003); after cancel origin A active again ∧ every previously highlighted window's props **byte-exact** baseline (no `0deg`, no empty gradient, no stuck highlight) | Shot 0 baseline: all three `active=[ff44cc88 0deg] inactive=[ff333333 0deg] size=[2]`. Shot 1 (cycle 1): C `active=[ffffd9a0 0deg] inactive=[ffffd9a0 0deg]`, A/B baseline, `active=0x5f2e02eff940 fh=0` **unchanged**. Shot 2 (cycle 2): C restored **byte-exact** (`ff44cc88 0deg`/`ff333333 0deg`/`2`), B highlighted, active **still A fh=0**. Shot 3 (displacement): `active=0x5f2e032326e0 (footC) fh=0` — origin no longer active. Shot 4 (`mru:cancel`): `active=0x5f2e02eff940 fh=0` = **origin refocused**, A/B/C all byte-exact baseline. Shot 5 (+1 s): stable, no drift | **PASS** |
| **N1b** (extension, see notes) | config file `ui=border`, `border_size=4`, `restore_focus_on_cancel=1` | baseline; cycle; displacement; cancel | size override visible during session and restored exactly; origin refocused | `getoption border_size int: 4`; shot 1: B `active=[ffffd9a0 0deg] inactive=[ffffd9a0 0deg] size=[4]` (other window baseline); shot 3 cancel: `active=0x5f2e032326e0 fh=0` (origin C), both windows `size=[2]`, colours `ff44cc88 0deg`/`ff333333 0deg` | **PASS** |
| **N2** | config file `ui=null`, `restore_focus_on_cancel=1` (via reload; `getoption ui` → `str: null`) | baseline; cycle ×2 (`activewindow` unchanged); displacement to C; `mru:cancel` | origin refocused; **ZERO** border prop deltas on all windows in every shot | Shots 0–2 (baseline, cycle 1, cycle 2): all windows `active=[ff44cc88 0deg] inactive=[ff333333 0deg] size=[2]` — **identical**, no highlight ever appears; `active=0x5f2e02eff940 fh=0` unchanged through both cycles. Shot 3 displacement → `active=0x5f2e032326e0 fh=0`. Shot 4 cancel → `active=0x5f2e02eff940 fh=0` (origin), sweep still zero-delta. Shot 5 (+1 s) stable | **PASS** |
| **N3** | config file `ui=border`, `restore_focus_on_cancel=1` | baseline; start session (`cycle next`); **`dispatch killactive` closes the ORIGIN A mid-session**; `mru:cancel` | no focus change relative to the pre-cancel active window, no crash, borders restored (REQ-R-002 / FM-18) | Shot 1: session Active, B highlighted, A active. Shot 2: `killactive` (active = A) → `clients` = `0x5f2e032326e0 footC`, `0x5f2e02f3fb70 footB` (A gone); compositor focused C (`active=0x5f2e032326e0 fh=0`); session survived the prune, B still highlighted. Shot 3 (pre-cancel reference): `active=0x5f2e032326e0 fh=0`. Shot 4 cancel: `active=0x5f2e032326e0 fh=0` — **no focus change** (origin invalid ⇒ no restore step; no crash / no focus side effect). Attribution limit: this row is *not* guard-discriminating, because a dead `WindowRef` also no-ops through the weak-lock validity path (ADR-013/016) even without the explicit `is_valid` guard; that guard is unit-discriminated by **T-S-06** (review mutation M3, not N3). Both remaining windows byte-exact baseline (B's highlight cleared). Shot 5 (+1 s) stable; nest alive, plugin still loaded, handle unchanged. `nest.log` crash scan clean | **PASS** |
| **N4** | config file `ui=border`, `restore=1` → **mid-session file edit to `restore=0` + `hyprctl reload`** | start session; reload; displacement to B; `mru:cancel` → **must STILL refocus origin (frozen, REQ-S-009)**; then NEW session (config now `restore=0`); displacement; `mru:cancel` → **must NOT refocus** | frozen policy wins for the running session; new value applies to the next session | Shot 1: session started (`restore=1`), B highlighted, origin C. Shot 2 (reload mid-session): `getoption plugin:mru-switcher:restore_focus_on_cancel` → `int: 0`, `ui` → `str: border` (supporting evidence only); session state unchanged (highlight still on B, active C). Shot 3 displacement → `active=0x5f2e02f3fb70 fh=0`. Shot 4 cancel → `active=0x5f2e032326e0 fh=0` = **origin C refocused (frozen `1` wins)**, borders byte-exact baseline. Shots 5–6: new session (`restore=0`), origin C, displaced to B (`active=0x5f2e02f3fb70 fh=0`). Shot 7 cancel → `active=0x5f2e02f3fb70 fh=0` — **no refocus (new `0` applies)**, borders baseline. Shot 8 (+1 s) stable | **PASS** |

**Verdict:** N1 **PASS**, N1b **PASS** (extension), N2 **PASS**, N3 **PASS**, N4 **PASS**. The restore leg of `restore_focus_on_cancel` is live-confirmed on pin `efb5099`, including REQ-S-009 freeze semantics and the REQ-R-002 invalid-origin no-op. No D2-style grammar corruption on any restore (byte-exact baselines in every post-cancel sweep; no `0deg`, no stuck `ffffd9a0`).

## Gaps / notes

- **T-S-10 (`non-cancel end never restores`) is intentionally OUT of this live matrix.** Discriminating it live needs the origin to stay valid while the session ends for another reason (e.g. empty snapshot with the origin outside the scope/on another monitor), which this single-monitor nest cannot stage without contriving state. Per the plan it stays **unit-only** (T-S-10 in `tests/domain/test_session_controller.cpp`, ctest green, 12/12). No weak live substitute was improvised.
- **`mru:status` payload is unreadable via `hyprctl` on this pin** (M3 precedent): `dispatch mru:status` answers `ok`; the payload travels in the error field of a successful `SDispatchResult` and `hyprctl` does not print it. One attempt recorded in `99-post-matrix.txt`. All assertions therefore use `activewindow -j` / `focusHistoryID` / `getprop`.
- **Mid-session focus displacement is part of the harness by design.** Virtual selection keeps the real focus on the origin during cycles (REQ-F-003), so a "was the origin refocused?" assertion would otherwise be vacuous. Each row displaces the real focus with `hyprctl -i "$SIG" dispatch focuswindow address:<other>` *after* the cycle assertions and *before* `mru:cancel`, and records the pre-cancel active window — making refocus / no-refocus observable. Deterministic compositor dispatch, not a synthetic keypress (`wtype` not used anywhere).
- **N3 closes the origin with `dispatch killactive`** (active window = origin during the session). The plugin pruned the snapshot and kept the session Active (two candidates left), so the observation is genuine: cancel with a dead origin ⇒ no crash, no focus side effect. **Attribution limit (review L-4):** that no-op is *also* guaranteed by weak-lock validity on a dead `WindowRef` (ADR-013/016) even without the explicit `is_valid` guard, so N3 does not discriminate the guard — the guarding evidence is unit-only, **T-S-06** (review mutation M3). N3's own scope is the no-crash / no-focus-side-effect / border-restore behaviour.
- **N1b is an addition beyond the briefed matrix** (brief's N1 clause "and `border_size` if it was overridden"; the briefed N1 config leaves plugin `border_size=-1`). It re-runs the N1 sequence once with `border_size=4` to cover the REQ-UI-008 size-override restore leg end-to-end (size 4 during session → 2 after cancel). Clearly additive; it replaces no briefed row.
- **Harness lesson (transparency): N1 attempt 1 is kept but superseded.** `report/attempt1-superseded/` holds the first N1 capture, which is **harness-invalid**: two stateful steps were issued as parallel tool calls and the `mru:cancel` raced the displacement capture (the displacement read showed the post-cancel state). Replaced by the atomic single-script run `report/10-n1-full.txt`; N2/N3/N4/N1b were each likewise driven inside one atomic script with internal sleeps, so no further races are possible.
- **Nest log noise (benign):** `ERR]: Invalid dispatcher: mru:cycle|apply|cancel` appears 4× at **boot**, before the plugin is loaded — the config binds dispatchers not yet registered (same pre-load reachability note as 2026-09-18 step 12). Also one boot-time `WARN: Failed to change process scheduling strategy` + xkbcomp virtual-modifier warnings. No crash/segfault/assert/hash-mismatch lines (`97-nest-log-full.txt`).
- **Not covered here (unchanged from 2026-09-18):** FM-22 abrupt ejection (kill -9 mid-session); per-frame perf loop; screenshot/visual verification of the governing border slot — border rendering is asserted via compositor state (`getprop`); FFM highlight-follow during a held session.
- Report format mirrors `docs/agent-state/reports/2026-09-18-m4-s3-nest-smoke.md` (evidence inlined because raw `/tmp` is ephemeral).

## Raw evidence

```
/tmp/mru-nest-restore/report/00-boot.txt                  host sig/env, instances pre-nest, host plugin list, launch cmd
/tmp/mru-nest-restore/report/01-clients-baseline.txt      3 foot windows pre-load (A/B/C addresses, classes, fh)
/tmp/mru-nest-restore/report/02-plugin-load.txt           .so sha256/mtime/size + load ok + plugin list + getoptions
/tmp/mru-nest-restore/report/03-getprop-format-probe.txt  origin focus + getprop reply format (ff44cc88 0deg)
/tmp/mru-nest-restore/report/09-focuswindow-sanity.txt    focuswindow address:… driver sanity (idle)
/tmp/mru-nest-restore/report/10-n1-full.txt               N1 atomic run: baseline/cycle1/cycle2/displacement/cancel/settle
/tmp/mru-nest-restore/report/20-n2-config.txt             N2 config-file edit (ui null) + diff
/tmp/mru-nest-restore/report/21-n2-reload.txt             N2 reload + getoptions + handle unchanged
/tmp/mru-nest-restore/report/22-n2-full.txt               N2 atomic run (zero-delta sweeps)
/tmp/mru-nest-restore/report/30-n3-config.txt             N3 config-file edit (ui border) + diff
/tmp/mru-nest-restore/report/31-n3-reload.txt             N3 reload + getoptions + clients
/tmp/mru-nest-restore/report/32-n3-full.txt               N3 atomic run (origin closed mid-session)
/tmp/mru-nest-restore/report/40-n4-full.txt               N4 atomic run (mid-session reload; frozen + new-value legs)
/tmp/mru-nest-restore/report/50-n1b-config.txt            N1b config-file edit (border_size 4) + diff
/tmp/mru-nest-restore/report/51-n1b-full.txt              N1b atomic run (size override restore)
/tmp/mru-nest-restore/report/96-nest-conf-final.txt       final nest config copy
/tmp/mru-nest-restore/report/97-nest-log-full.txt         full nest.log (crash-scan source)
/tmp/mru-nest-restore/report/98-host-during.txt           host plugin list + instances DURING matrix (untouched)
/tmp/mru-nest-restore/report/99-post-matrix.txt           plugin still loaded, mru:status attempt, crash scan, final sweep
/tmp/mru-nest-restore/report/99-teardown.txt              SIGTERM 11613, pid gone, instances, foot 0, host checks
/tmp/mru-nest-restore/report/attempt1-superseded/*        5 files: N1 attempt 1 (harness-invalid: parallel tool calls)
```

Config under test (`report/96-nest-conf-final.txt`): `monitor = ,1920x1080@60,auto,1`; `general { border_size = 2; col.active_border = 0xff44cc88; col.inactive_border = 0xff333333 }`; binds `ALT,TAB→mru:cycle next`, `ALT SHIFT,TAB→mru:cycle prev`, `bindrt ALT,ALT_L→mru:apply`, `ALT,Escape→mru:cancel`; `plugin { mru-switcher { ui = border; border_size = 4; restore_focus_on_cancel = 1 } }` (N1–N4 ran with the briefed block `ui`/`restore_focus_on_cancel` only; `border_size = 4` was added for N1b).
## Annex: inlined key evidence (raw /tmp is ephemeral)

Load + pin (from `02-plugin-load.txt`):

```text
daa5bd97189c0612b263197cf093fb462f4e1c895bb85ad259c8d21798467897  .../build-plugin-restore/mru-switcher.so
448328 bytes mtime=2026-09-19 11:24:20.878459574 +0300
--- load ---
ok
--- plugin list ---
Plugin mru-switcher by mru:
	Handle: 5f2e03978200
	Version: 0.4.0
--- getoption ui ---  str: border   set: true
--- getoption restore_focus_on_cancel ---  int: 1   set: true
--- getoption border_size ---  int: -1   set: false
```

N1 (from `10-n1-full.txt`) — origin `A=0x5f2e02eff940`, B=`0x5f2e02f3fb70`, C=`0x5f2e032326e0`; note the two cycle shots show the highlight moving C→B while `activewindow` never leaves A (REQ-F-003), C's restore after cycle 2 is byte-exact (D2 fix live), and shot 4 proves the refocus from a displaced focus:

```text
shot 0: pre-session baseline (origin A focused)
active=0x5f2e02eff940 class=footA fh=0 title=/t/mru-nest-restore
0x5f2e02eff940 active=[ff44cc88 0deg ] inactive=[ff333333 0deg ] size=[2 ]
0x5f2e032326e0 active=[ff44cc88 0deg ] inactive=[ff333333 0deg ] size=[2 ]
0x5f2e02f3fb70 active=[ff44cc88 0deg ] inactive=[ff333333 0deg ] size=[2 ]

shot 1: dispatch mru:cycle next
ok
active=0x5f2e02eff940 class=footA fh=0 title=/t/mru-nest-restore
0x5f2e02eff940 active=[ff44cc88 0deg ] inactive=[ff333333 0deg ] size=[2 ]
0x5f2e032326e0 active=[ffffd9a0 0deg ] inactive=[ffffd9a0 0deg ] size=[2 ]
0x5f2e02f3fb70 active=[ff44cc88 0deg ] inactive=[ff333333 0deg ] size=[2 ]

shot 2: dispatch mru:cycle next
ok
active=0x5f2e02eff940 class=footA fh=0 title=/t/mru-nest-restore
0x5f2e02eff940 active=[ff44cc88 0deg ] inactive=[ff333333 0deg ] size=[2 ]
0x5f2e032326e0 active=[ff44cc88 0deg ] inactive=[ff333333 0deg ] size=[2 ]
0x5f2e02f3fb70 active=[ffffd9a0 0deg ] inactive=[ffffd9a0 0deg ] size=[2 ]

shot 3: displacement: dispatch focuswindow C
ok
active=0x5f2e032326e0 class=footC fh=0 title=/t/mru-nest-restore
0x5f2e02eff940 active=[ff44cc88 0deg ] inactive=[ff333333 0deg ] size=[2 ]
0x5f2e032326e0 active=[ff44cc88 0deg ] inactive=[ff333333 0deg ] size=[2 ]
0x5f2e02f3fb70 active=[ffffd9a0 0deg ] inactive=[ffffd9a0 0deg ] size=[2 ]

shot 4: dispatch mru:cancel (restore=1 -> expect A refocused)
ok
active=0x5f2e02eff940 class=footA fh=0 title=/t/mru-nest-restore
0x5f2e02eff940 active=[ff44cc88 0deg ] inactive=[ff333333 0deg ] size=[2 ]
0x5f2e032326e0 active=[ff44cc88 0deg ] inactive=[ff333333 0deg ] size=[2 ]
0x5f2e02f3fb70 active=[ff44cc88 0deg ] inactive=[ff333333 0deg ] size=[2 ]
```


N2 (from `22-n2-full.txt`, `ui=null` after reload) — zero-delta sweeps and the refocus still happening with the border UI off:

```text
shot 1: dispatch mru:cycle next (ui=null: expect ZERO border deltas)
ok
active=0x5f2e02eff940 class=footA fh=0 title=/t/mru-nest-restore
0x5f2e02eff940 active=[ff44cc88 0deg ] inactive=[ff333333 0deg ] size=[2 ]
0x5f2e032326e0 active=[ff44cc88 0deg ] inactive=[ff333333 0deg ] size=[2 ]
0x5f2e02f3fb70 active=[ff44cc88 0deg ] inactive=[ff333333 0deg ] size=[2 ]

shot 3: displacement focuswindow C
ok
active=0x5f2e032326e0 class=footC fh=0 title=/t/mru-nest-restore

shot 4: dispatch mru:cancel (restore=1 -> expect A refocused)
ok
active=0x5f2e02eff940 class=footA fh=0 title=/t/mru-nest-restore
0x5f2e02eff940 active=[ff44cc88 0deg ] inactive=[ff333333 0deg ] size=[2 ]
0x5f2e032326e0 active=[ff44cc88 0deg ] inactive=[ff333333 0deg ] size=[2 ]
0x5f2e02f3fb70 active=[ff44cc88 0deg ] inactive=[ff333333 0deg ] size=[2 ]
```

N3 (from `32-n3-full.txt`) — origin A killed mid-session; the session survives (B still highlighted), cancel leaves the active window untouched and clears the highlight:

```text
shot 1: start session: dispatch mru:cycle next
ok
active=0x5f2e02eff940 class=footA fh=0 title=/t/mru-nest-restore
0x5f2e02f3fb70 active=[ffffd9a0 0deg ] inactive=[ffffd9a0 0deg ] size=[2 ]

shot 2: close ORIGIN mid-session: dispatch killactive (active must be A)
ok
--- clients (A must be gone) ---
0x5f2e032326e0 class=footC fh=0
0x5f2e02f3fb70 class=footB fh=1
active=0x5f2e032326e0 class=footC fh=0 title=/t/mru-nest-restore
0x5f2e032326e0 active=[ff44cc88 0deg ] inactive=[ff333333 0deg ] size=[2 ]
0x5f2e02f3fb70 active=[ffffd9a0 0deg ] inactive=[ffffd9a0 0deg ] size=[2 ]

shot 3: pre-cancel reference (active window just before cancel)
active=0x5f2e032326e0 class=footC fh=0 title=/t/mru-nest-restore

shot 4: dispatch mru:cancel (origin invalid -> expect NO focus change + borders restored)
ok
active=0x5f2e032326e0 class=footC fh=0 title=/t/mru-nest-restore
0x5f2e032326e0 active=[ff44cc88 0deg ] inactive=[ff333333 0deg ] size=[2 ]
0x5f2e02f3fb70 active=[ff44cc88 0deg ] inactive=[ff333333 0deg ] size=[2 ]
```

(shot 5, +1 s: `active=0x5f2e032326e0 … fh=0` with both windows at baseline; plugin list unchanged, handle `5f2e03978200`.)
N4 (from `40-n4-full.txt`) — mid-session reload to `restore=0`; the frozen `1` still refocuses origin C (shot 4), the next session honours `0` and does **not** refocus (shot 7):

```text
shot 1: start session: dispatch mru:cycle next
ok
active=0x5f2e032326e0 class=footC fh=0 title=/t/mru-nest-restore
0x5f2e02f3fb70 active=[ffffd9a0 0deg ] inactive=[ffffd9a0 0deg ] size=[2 ]

shot 2: CONFIG FILE EDIT restore 1 -> 0 (mid-session) + hyprctl reload
--- work/hypr-nest.conf.n3    2026-09-19 ...
+++ hypr-nest.conf     2026-09-19 ...
-        restore_focus_on_cancel = 1
+        restore_focus_on_cancel = 0
ok
--- getoption after reload (supporting evidence only) ---
int: 0
set: true
str: border
set: true
--- mid-session state after reload (policy frozen; highlight unchanged) ---
active=0x5f2e032326e0 class=footC fh=0 title=/t/mru-nest-restore
0x5f2e032326e0 active=[ff44cc88 0deg ] inactive=[ff333333 0deg ] size=[2 ]
0x5f2e02f3fb70 active=[ffffd9a0 0deg ] inactive=[ffffd9a0 0deg ] size=[2 ]

shot 3: displace focus to B (origin C no longer active)
ok
active=0x5f2e02f3fb70 class=footB fh=0 title=/t/mru-nest-restore

shot 4: FROZEN leg: dispatch mru:cancel (must STILL refocus origin C)
ok
active=0x5f2e032326e0 class=footC fh=0 title=/t/mru-nest-restore
0x5f2e032326e0 active=[ff44cc88 0deg ] inactive=[ff333333 0deg ] size=[2 ]
0x5f2e02f3fb70 active=[ff44cc88 0deg ] inactive=[ff333333 0deg ] size=[2 ]

shot 6: displace focus to B again (pre-cancel reference)
ok
active=0x5f2e02f3fb70 class=footB fh=0 title=/t/mru-nest-restore

shot 7: NEW-VALUE leg: dispatch mru:cancel (must NOT refocus C; active stays B)
ok
active=0x5f2e02f3fb70 class=footB fh=0 title=/t/mru-nest-restore
0x5f2e032326e0 active=[ff44cc88 0deg ] inactive=[ff333333 0deg ] size=[2 ]
0x5f2e02f3fb70 active=[ff44cc88 0deg ] inactive=[ff333333 0deg ] size=[2 ]
```

N1b (from `51-n1b-full.txt`) — `border_size = 4` override visible only during the session, restored to the compositor's 2 on cancel:

```text
shot 0: pre-session baseline (origin C focused; size must be 2)
0x5f2e032326e0 ... size=[2 ]
0x5f2e02f3fb70 ... size=[2 ]

shot 1: dispatch mru:cycle next (expect selected window size 4 + highlight)
ok
active=0x5f2e032326e0 class=footC fh=0 title=/t/mru-nest-restore
0x5f2e032326e0 active=[ff44cc88 0deg ] inactive=[ff333333 0deg ] size=[2 ]
0x5f2e02f3fb70 active=[ffffd9a0 0deg ] inactive=[ffffd9a0 0deg ] size=[4 ]

shot 3: dispatch mru:cancel (expect origin C + size back to 2 + colours baseline)
ok
active=0x5f2e032326e0 class=footC fh=0 title=/t/mru-nest-restore
0x5f2e032326e0 active=[ff44cc88 0deg ] inactive=[ff333333 0deg ] size=[2 ]
0x5f2e02f3fb70 active=[ff44cc88 0deg ] inactive=[ff333333 0deg ] size=[2 ]
```
Host isolation + teardown (from `98-host-during.txt`, `99-teardown.txt`, `00-boot.txt`) — host `plugin list` stays `no plugins loaded` throughout and only host pid 1250 survives:

```text
--- hyprctl instances -j (pre-nest) ---        [ host pid 1250 / wayland-1 only ]
--- host plugin list (pre-nest) ---            no plugins loaded
--- launch cmd ---
env -u HYPRLAND_INSTANCE_SIGNATURE -u HYPRLAND_CONFIG XDG_CACHE_HOME=/tmp/mru-nest-restore/cache \
  Hyprland -c /tmp/mru-nest-restore/hypr-nest.conf > /tmp/mru-nest-restore/nest.log 2>&1 &
[instances -j after boot adds]  efb50993780079460b0cbed1363e2166a2de1d9f_1789806305_1928909522  pid 11613  wayland-2

=== host check DURING matrix (2026-09-19T11:29:01+03:00) ===
--- host plugin list ---   no plugins loaded
--- host Hyprland pids ---  1250 11613        (11613 = the nest, not host)

=== teardown 2026-09-19T11:29:09+03:00 ===
nest SIG=efb50993780079460b0cbed1363e2166a2de1d9f_1789806305_1928909522 pid=11613
kill -TERM 11613
pid 11613 gone
--- instances -j (nest must be absent) ---   [ host pid 1250 only ]
--- host plugin list AFTER teardown ---   no plugins loaded
--- host Hyprland pids AFTER teardown ---   1250

--- +10s post-teardown confirm 2026-09-19T11:29:19+03:00 ---
foot processes: 0
Hyprland pids: 1250
host plugin list: no plugins loaded
host instances: efb50993780079460b0cbed1363e2166a2de1d9f_1789804326_83560980
```

Post-matrix state + crash scan (from `99-post-matrix.txt`, `97-nest-log-full.txt`):

```text
--- plugin list (still loaded) ---   Plugin mru-switcher by mru:  Handle: 5f2e03978200  Version: 0.4.0
--- mru:status attempt ---   ok            (payload not printed by hyprctl on this pin)
--- activewindow ---   active=0x5f2e032326e0 class=footC fh=0
--- sweep ---   0x5f2e032326e0 active=[ff44cc88 0deg ] inactive=[ff333333 0deg ] size=[2 ]
                0x5f2e02f3fb70 active=[ff44cc88 0deg ] inactive=[ff333333 0deg ] size=[2 ]
--- nest.log crash scan (crash|segfault|SIGSEGV|assert|terminate|hash mismatch|failed) ---
35: WARN ]: Failed to change process scheduling strategy
(no other matches)
--- nest.log boot lines (pre-load binds) ---
55: ERR ]: Invalid dispatcher: mru:cycle
56: ERR ]: Invalid dispatcher: mru:cycle
57: ERR ]: Invalid dispatcher: mru:apply
58: ERR ]: Invalid dispatcher: mru:cancel
```
