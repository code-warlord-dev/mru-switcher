# Nested-smoke report — M4-S3 border highlight UI (§7 checklist, ROADMAP M4)

**Date:** 2026-09-18
**Build under test:** `/home/code_warlord/Work/DEV/mru-switcher/build-plugin/mru-switcher.so` — main `79bc630ef432bae56f1c3801ee2aa510b727f927`, rebuilt clean (`cmake --build build-plugin`); sha256 `6069ca69625855f76e85ce726027126a6f1974b2f0323795a366df58f251fe8e`, mtime `2026-09-18 06:09:51 +0300`, 439 880 B. Plugin list prints `Version: 0.2.0` (string not bumped — cosmetic, same note as M3).
**Pin:** Hyprland v0.56.2 / commit `efb50993780079460b0cbed1363e2166a2de1d9f`, aquamarine 0.15.0, Wayland nested backend (DRM fallback noise benign per 2026-09-17 diagnosis memo; no `--headless` flag on this pin).
**Nest:** 1 session — instance `efb5099…_1789701260_79750329` (pid 29680), launched `env -u HYPRLAND_INSTANCE_SIGNATURE -u HYPRLAND_CONFIG Hyprland -c /tmp/mru-nest-m4/hypr-nest.conf`; booted 06:14, torn down (SIGTERM, clean) 06:42. Host pid 1248 / wayland-1 untouched; host `plugin list` = `no plugins loaded` before, during, and after.
**Windows:** three foot terminals opened BEFORE plugin load — `A=0x5e8564a78350`, `B=0x5e85647c6b10`, `C=0x5e8564a97310` (ws1; config borders: active `0xff44cc88`, inactive `0xff333333`, size 2).
**Raw evidence:** `/tmp/mru-nest-m4/report/*.txt`; nest config `/tmp/mru-nest-m4/hypr-nest.conf`; log `/tmp/mru-nest-m4/nest.log`; helpers `hypr.sh`, `sweep.sh`, `rawipc.py`, `clients.py`.

## Results

| # | Step | Expected | Actual | Verdict |
|---|------|----------|--------|---------|
| 1 | Load sanity `ui=null` | load OK (hash, same pin); cycle/apply/cancel work; NO border prop changes (REQ-UI-003 null leg) | Loaded OK, `ui` getoption `null`. cycle→ok (focus unchanged), cancel→ok, 2nd cycle+apply→ok, apply focused `0x…a78350` (fh 2→0). All sweeps: every window `active=ff44cc88 0deg inactive=ff333333 0deg` during AND after sessions | **PASS** |
| 2 | ui=border probe leg (R0 key) — briefed path `hyprctl keyword ui border`, first session | BOTH slots of selected window → `ffffd9a0` | keyword accepted (`ok`, getoption `border`) but cycle produced ZERO border changes on any window (02b). Classification: manual `dispatch setprop address:… 0xffffd9a0` → `ok` + getprop `ffffd9a0 0deg` (compositor grammar fine, 02b); the plugin's exact byte strings `/getprop …`/`/dispatch setprop …` work verbatim over the raw IPC socket (02d); pin source: `invokeHyprctlCommand` prepends `/` itself (`PluginAPI.cpp:59–64`), `makeDynamicCall`→`getReply` routes it (`HyprCtl.cpp:1118`) — **API spelling correct, NOT the pre-declared R0 uncertainty**. Discriminator: config-file `ui = border` + `hyprctl reload` → highlight DOES appear (`ffffd9a0` both slots, 02e). ⇒ **D1: `hyprctl keyword` changes never reach the plugin's cached config** (plugin `st.config` refreshes on `config.reloaded`; file reload fires it, keyword does not; plugin handle unchanged across reload `5e8564aa2f10` — no restart involved). Probe leg itself behaves as designed: raw format `ff44cc88 0deg` = non-empty, no error tokens; bad-selector reply `window not found` matches the `looks_like_error` classifier (02a) | **FAIL** (D1; mechanism proven sound via config-file path) |
| 3 | Selection follow (REQ-UI-004) | cycle → NEW window highlighted; PREVIOUS restored to captured prior colors, exact | Run A (03): cycle1 → B highlighted `ffffd9a0` both slots; cycle2 → C highlighted, B cleared — follow mechanics correct. BUT B's restore = `0deg`/`0deg` (empty gradient = **invisible border**), not the pre-session `ff123456`. Root cause **D2**: getprop returns unprefixed `<hex6> <N>deg`; plugin restores that string verbatim via setprop; grammar map (02g) proves setprop accepts ONLY `0x…`/`rgb()` without suffix — angle-suffixed/unprefixed forms parse to EMPTY gradient. Highlight writes work only because the config string is 0x-prefixed | **FAIL** (restore half; follow half PASS) |
| 4 | Restore-by-value with custom color (R0 F10 live) | B set `0xff123456` (+inactive), made selected target, restored to EXACTLY `0xff123456` | B before session: `ff123456 0deg` both slots (0x-form round-trips byte-exact). B selected (highlight `ffffd9a0`), selection moved → B = `0deg`/`0deg` (03). Captured value destroyed by the D2 getprop→setprop round-trip. F10's hazard (empty gradient = invisible border) **confirmed live** — on the restore path, not the capture path | **FAIL** (D2) |
| 5 | Focus side-effect (REQ-UI-006) | `activewindow` unchanged during cycles; apply focuses once; no flicker | fh map before/during/after 2 cycles (05-07): focused window keeps `focusHistoryID=0`, no re-order = no focus event from setprop/getprop. Apply → focus changes exactly once, stable at t+0.5 s and t+1.1 s. (Apply driven via dispatcher — bindrt release not injectable from CLI; M2/M3 precedent) | **PASS** |
| 6 | Cancel path (REQ-UI-005) | mid-session `mru:cancel` → selected window's both slots restored to captured values; no stuck borders | Cancel executes, session ends, highlight color never persists (letter of "no stuck highlight borders" holds in every sweep). BUT the previously-selected windows are left `0deg`/`0deg` — restore writes empty gradient (D2). 02e tail proves corruption on the very first config-file session's cancel already | **FAIL** (D2) |
| 7 | Apply path | end via apply → borders fully restored | Apply works (focus correct); border sweep after apply (05-07): the two previously-selected windows = `0deg`/`0deg` (D2); never-selected windows intact | **FAIL** (D2) |
| 8 | Graceful unload mid-session | borders restored on teardown; no crash | `plugin unload` mid-session → `ok`, plugin list empty, nest alive, `activewindow` fine. Teardown restore RAN (highlight not leaked) but wrote the same corrupt value: B = `0deg`/`0deg` after unload (08) | **FAIL** (D2 on teardown restore; no-crash/no-leak legs PASS) |
| 9 | border_size optional leg | selected window size = N during session, override dropped after end | Config `border_size = 4` + reload → during session: B `border_size=4`, others `2` (09-border-size-session); after cancel: all `2` — `setprop … border_size unset` accepted and effective. **COMPAT intel: `getprop … border_size` IS supported on this pin** (effective int; 09-border-size-probe) — refutes the code comment / R0 claim "pin has no border_size getprop"; restore could read back instead of `unset` | **PASS** |
| 10 | REQ-UI-009 mid-session reload | active session keeps frozen backend/color; new session uses new color | Briefed `hyprctl keyword plugin:mru-switcher:border_color …` is unobservable to the plugin (D1) — verified via the working channel instead: explicit old color in config (`ffffd9a0`), session 1 highlights `ffffd9a0`; MID-SESSION config-file edit to `ffff00ff` (inotify, no explicit reload) → highlight still `ffffd9a0` (frozen backend ✓); cancel; new session → `ffff00ff` (10b). Attempt 1 (10-config-reload-ui.txt) was harness-invalid (no `border_color` key in config; compiled-in default highlighted) — superseded, kept for the record | **PASS** (config-file channel; keyword channel blocked by D1) |
| 11 | REQ-UI-002 degrade leg (live) | live API failure → null + warn-once | NOT-RUNNABLE live: simulating a real getprop/setprop failure requires source modification (forbidden for tester); unit tests cover the parse/runtime fallback (t_ui_002_*). Side observation: D1-blocked sessions degraded silently like a probe-skip — fail-soft held (no crash, apply/cancel fine), though that path was the validator skip (REQ-UI-010), not a probe failure | **NOT-RUNNABLE** |
| 12 | Regression sweep (ui=border active) | default grammar + M2 invariants intact | `mru:cycle bogus` → `unknown scope token: bogus`, no session, focus unchanged; `mru:cycle next workspace` → ok + highlight; `mru:cycle prev` → ok, focus untouched during; cancel clean. Pre-load windows reachable: all 3 (opened before `plugin load`) were selected across RUN A/B/step-10 sessions | **PASS** |

**Counts:** PASS 5 · FAIL 6 (steps 2, 3, 4, 6, 7, 8 — two root causes D1/D2) · NOT-RUNNABLE 1

## Verdicts — ROADMAP M4 exit criteria

- **"Daily-driver usable with ui=border": NO.** Two defects:
  - **D1 (config propagation):** `hyprctl keyword plugin:mru-switcher:*` never reaches the plugin's cached config on this pin — ui/border toggles only take effect via config-file change (+ reload/inotify). A user who sets `ui = border` once in hyprland.conf works, but the briefed live-toggle path is dead and any keyword-based tweaking is silently ignored.
  - **D2 (restore-by-value corrupts):** every window that loses the selection (cycle-away, cancel, apply, unload) ends with BOTH color slots = empty gradient (`getprop` → bare `0deg`): **invisible border**. In real use, every selected-then-deselected window loses its border entirely. This is the R0 F10 hazard realized through the getprop→setprop round-trip.
- **"No stuck borders after cancel": letter YES / spirit NO.** The highlight color `ffffd9a0` never persisted past any end path (checked in every sweep). But cancel/apply/unload leave *destroyed* (invisible) borders instead of restored ones — restore correctness, not highlight leakage, is broken (D2).
- **Solid style works** (REQ-UI-001/004-follow/006/007/009/010 legs exercised all behaved), mechanism selection (setprop both slots, priority over rules) confirmed live. **pulse/dim out of scope** ✓ (not present; unknown-token→solid is unit-covered). **border_size optional** ✓ and restore-via-`unset` works.

## R0 memo (`2026-09-17-m4-border-api.md`) — uncertainties RESOLVED vs open

| R0 item | Status after M4-S3 |
|---|---|
| Call-string spelling `dispatch` vs `dispatch/` | **RESOLVED — plain `dispatch`/`getprop` is correct.** Pin source: API prepends `/` itself (`PluginAPI.cpp:59–64`); byte-exact raw-socket replay of the plugin's strings works (02d); compositor router exonerated (`getReply`, `HyprCtl.cpp:1118`). NOT the cause of any failure. |
| getprop output format for color slots | **RESOLVED — `<hex6 AARRGGBB> <N>deg`** (no `0x` prefix), e.g. `ff44cc88 0deg`. Non-empty, no error tokens → `looks_like_error` classification is safe for real values; error replies verified as literal `window not found` / `Invalid…` shapes (02a). |
| setprop accepted color grammar (implicit in F2/F3) | **RESOLVED (new)** — accepts `0xAARRGGBB` and `rgb(...)` WITHOUT angle suffix; unprefixed hex and ANY angle-suffixed form → **empty gradient** (02g). getprop output is therefore NOT setprop-compatible verbatim. |
| F10 "empty gradient = invisible border; restore-by-value mandatory" | **CONFIRMED LIVE** — but restore must NORMALIZE the captured value (parse hex, re-emit `0x…`, drop `deg`); verbatim echo destroys the border (D2). |
| "pin has no border_size getprop" (code comment / R0) | **REFUTED** — `getprop … border_size` returns the effective int (2 global; 4 while overridden) (09-*). Size restore could read back+re-set instead of `unset` (both work). |
| `setprop … border_size unset` restore | **RESOLVED** — accepted, override dropped, global size applies again (09-border-size-session). |
| F4/F5/F6/F8/F9 (slots, priority over rules, sync repaint, verb routing, address selector) | **Consistent with observations** (both-slot overrides; file/keyword priority behavior; address selectors; immediate sweep visibility). Per-frame perf budget (F6, 100× loop) not in the 12-row matrix — untested. |
| **NEW uncertainties opened** | (a) D1 — hyprlang `keyword` vs plugin config cache on this pin (does `config.reloaded` not fire for keyword, or does the cached read path miss it?); (b) exact live trigger split between probe-degrade (would notify) and validator-skip (silent) — both fail-soft; only the validator-skip path was exercised live. |

## Gaps / not covered

- **FM-22 abrupt ejection (kill -9 compositor mid-session):** per brief, NOT attempted — documented gap; would compound D2 (no teardown restore at all).
- **Step 11 live degrade:** NOT-RUNNABLE (would require a source edit; unit-covered). No live evidence about the warn-once notification itself.
- **Step 10 attempt 1** (10-config-reload-ui.txt) is harness-invalid (color change never present in config; compiled-in default highlighted in session 1) — superseded by attempt 3 (10b); kept as raw evidence of the misstep.
- **mru:status payload** remains unobservable via hyprctl on this pin (M3 precedent); session-state assertions used getprop/activewindow/focusHistoryID only.
- **Border rendering asserted via compositor state** (`getprop`), not screenshots; the governing-slot choice (active vs inactive, F4) was not visually verified — getprop exposes stored props, not the shader's pick.
- **Perf loop (R0 recipe step 8, 100× cycle)** and **mid-session FFM highlight-follow (R0 step 5)** were outside the 12-row matrix — untested here.
- After D2, "no error" post-restore sweeps are only meaningful for windows never selected; PASS sweeps (steps 1, 9-size, 10, 12) never had a previously-selected window with a custom value in play except where noted.

## Raw evidence

```
/tmp/mru-nest-m4/report/00-boot.txt                    boot: host sig env, host plugin list empty, launch cmd
/tmp/mru-nest-m4/report/01-clients-baseline.txt        3 foot windows pre-load (addresses A/B/C)
/tmp/mru-nest-m4/report/01-plugin-load.txt             plugin load ok + plugin list (handle 5e8564aa2f10)
/tmp/mru-nest-m4/report/01-null-ui.txt                 step 1: ui=null sessions, sweeps unchanged
/tmp/mru-nest-m4/report/02a-getprop-format-before.txt  raw getprop format pre-session + error reply shapes
/tmp/mru-nest-m4/report/02b-highlight-first-session.txt step 2 keyword path: no highlight + manual grammar proof
/tmp/mru-nest-m4/report/02d-rawipc-discriminator.txt   plugin's exact byte strings over raw IPC socket (all ok)
/tmp/mru-nest-m4/report/02e-configfile-reload.txt      config-file ui=border + reload: highlight appears (KEY)
/tmp/mru-nest-m4/report/02f-keyword-discriminator.txt  keyword null/border both ignored (D1 proof); post-cancel 0deg
/tmp/mru-nest-m4/report/02g-grammar-map.txt            D2: setprop color grammar acceptance map
/tmp/mru-nest-m4/report/03-selection-follow.txt        RUN A: steps 3+4+6 (follow, custom restore, cancel)
/tmp/mru-nest-m4/report/05-07-focus-apply.txt          steps 5+7: fh stability, single focus, post-apply sweep
/tmp/mru-nest-m4/report/08-unload-mid-session.txt      step 8: unload mid-session, teardown restore, nest alive
/tmp/mru-nest-m4/report/09-border-size-probe.txt       border_size getprop support + unset grammar (COMPAT)
/tmp/mru-nest-m4/report/09-border-size-session.txt     step 9: size 4 during session, dropped after cancel
/tmp/mru-nest-m4/report/10-config-reload-ui.txt        step 10 attempt 1 (INVALID harness — superseded)
/tmp/mru-nest-m4/report/10b-config-reload-final.txt    step 10 attempt 3: frozen backend mid-session, new color next
/tmp/mru-nest-m4/report/12-regression.txt              step 12: bogus/next workspace/prev, pre-load reachability
/tmp/mru-nest-m4/report/99-teardown.txt                nest SIGTERM, processes gone, host plugin list empty
```

## Annex: inlined key evidence (raw /tmp is ephemeral)

From `02e-configfile-reload.txt` (D1 discriminator, config-file channel WORKS; last line also shows D2 corruption — prior selection restored to empty gradient):

```text
--- getoption ui after reload ---
str: border
set: true
--- getprop sweep DURING session (ui=border via config file + hyprctl reload) ---
  0x5e85647c6b10 active=ff44cc88 0deg inactive=ff333333 0deg
  0x5e8564a78350 active=ff44cc88 0deg inactive=ff333333 0deg
  0x5e8564a97310 active=ffffd9a0 0deg inactive=ffffd9a0 0deg
  0x5e8564a97310 active=0deg    inactive=0deg      <- D2: verbatim restore -> empty gradient
```

From `02f-keyword-discriminator.txt` (D1: hyprlang accepts `keyword`, plugin cache ignores it — highlight persists after `keyword ui null`):

```text
--- keyword ui null ---
ok
--- getoption (hyprlang says null) ---
str: null
set: true
--- cycle (highlight STILL appears -> plugin st.config stale on keyword) ---
  0x5e8564a97310 active=ffffd9a0 0deg inactive=ffffd9a0 0deg
```

From `02g-grammar-map.txt` (D2 grammar acceptance map):

```text
setprop [ff44cc88]          -> getprop: 0deg           (rejected -> empty gradient)
setprop [0xff44cc88]        -> getprop: ff44cc88 0deg  (accepted)
setprop [ff44cc88 0deg]     -> getprop: 0deg           (rejected -> empty gradient)
setprop [rgb(44cc88)]       -> getprop: ff44cc88 0deg  (accepted)
```

From `09-border-size-probe.txt` (R0 correction: size IS readable on this pin):

```text
getprop border_size -> integer echo (e.g. 2); setprop border_size 4 during session -> 4; "unset" drop -> ok
```
