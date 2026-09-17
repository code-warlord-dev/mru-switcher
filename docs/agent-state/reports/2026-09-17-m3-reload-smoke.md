# Nested-smoke report — M3 config reload (REQ-CFG-002/003, REQ-S-009)

**Date:** 2026-09-17
**Scope:** Live config reload behavior of the MRU switcher in a nested Hyprland (0.56.2): baseline config surface, global-scope default, active-session immutability on reload, next-session-only application of `default_scope` / `debounce_ms`, ui swap NOT-RUNNABLE by design (M4/M5).
**Build under test:** `/home/code_warlord/Work/DEV/mru-switcher/build-plugin/mru-switcher.so` (Version string `0.2.0`, cosmetic per prior reports); loaded OK, hash check passed.
**Pin:** Hyprland v0.56.2 / aquamarine 0.15.0 (COMPAT.md §"Nest recipe"), Wayland backend (DRM fallback noise expected; `Invalid dispatcher: mru:*` at parse time is the documented startup-order noise — COMPAT.md §Observations). Nest SIG `…1789674599_1050402639` (pid 76458, wayland-2), host pid 1245 / wayland-1.
**Nest config:** `/tmp/mru-nest-reload/hypr-nest.conf` (no host includes; `default_scope = global`, `debounce_ms = 400`, binds ALT+TAB `mru:cycle` / bindrt ALT_L `mru:apply` / ALT+Escape `mru:cancel`).
**Raw evidence:** `/tmp/mru-nest-reload/report/*.txt`, env `/tmp/mru-nest-reload/env`, log `/tmp/mru-nest-reload/nest.log`.
**End state:** nest killed; host pid 1245 alive (`ps`), only instance, `hyprctl plugin list` = `no plugins loaded`.

## Results

| # | Step | Expected | Actual | Verdict |
|---|------|----------|--------|---------|
| 1 | Baseline config surface | `getoption plugin:mru-switcher:default_scope` → `global`; `…:debounce_ms` → `400` (SPEC §4 defaults) | `str: global`, `int: 400` (both `set: true`) | **PASS** |
| 2 | Default scope = global reaches ws2 | A@ws1 / B@ws2, session from ws1 → `mru:apply` focuses B@ws2 (REQ-SC-001, REQ-S-009 scope leg) | cycle mid-session focus unchanged (A@ws1, no side-effect), apply → `activewindow` = `mruB` (pid 77201, ws 2) | **PASS** |
| 3 | Active session + config reload | Edit `default_scope = workspace` mid-session, `hyprctl reload` → `getoption`→`workspace`; **active session untouched** (frozen snapshot/selection, REQ-CFG-002/REQ-S-009) | `getoption` after reload = `workspace`; mid-session focus still A@ws1; apply on the still-active session focused **B@ws2** (the pre-reload global selection) — a rewritten workspace-scope session could only have focused A@ws1 | **PASS** |
| 4 | New session uses reloaded scope | End/apply the session, start a **new** one on ws1 → only ws1 windows candidates; B@ws2 inaccessible for THIS session | `getoption` = `workspace`; new session apply → `mruA`@ws1 (single candidate), did NOT jump to B@ws2 | **PASS** |
| 5 | `debounce_ms` reload | Edit `debounce_ms = 1000`, reload, `getoption` → `1000` (applies to post-reload Idle/MRU focus debounce, REQ-CFG-002) | `int: 400` → `int: 1000` after reload | **PASS** |
| 6 | `ui` backend swap on reload | REQ-CFG-003: switch backend for **next** session — M4/M5 territory | Only `ui=null` registered (`str: null`, `set: false`); no border/external backend exists pre-M4/M5 | **NOT-RUNNABLE** by design |

**Counts:** PASS 5 · FAIL 0 · NOT-RUNNABLE 1

## Verdicts

- **REQ-CFG-002 (reload refreshes values; applied to next session + post-reload debounce): PASS.** `hyprctl reload` after mid-session config edits flipped both `getoption` values immediately (405-step 03b, 05b) while the live session kept its frozen policy (apply → B@ws2, step 3), and the very next session used the new `default_scope=workspace` (step 4 — only ws1 candidates). Post-reload debounce re-read evidenced via `getoption` → `1000` per the test brief's minimum bar (step 5).
- **REQ-CFG-003 (ui backend switch for next session): NOT-RUNNABLE by design.** Only `ui=null` exists on this milestone; `border`/`external` are M4/M5. The reload *listener* re-reads `ui` into config (verified by prior m3-s3 review), but no live backend swap can be exercised yet. No defect observed on the `null` path.
- **REQ-S-009 (SessionPolicy snapshot at session start; reload MUST NOT mutate active session): PASS.** The mid-reload apply is the decisive proof: selection stayed on the pre-reload global-scope target (B@ws2). A reload that rewrote the active policy would have rebuilt the snapshot to `{A}` and applied A@ws1.

## Notes / gaps

- **Exec quirk:** unquoted `hyprctl dispatch exec foot -a mruA` hung without spawning windows (returned `ok`, no window); quoted `dispatch exec "foot -a mruA"` works. Evidence files from step 2 used the quoted form. Test-tooling only, not a plugin defect.
- **Mid-session activewindow** captured before and after reload in 03a/03b shows continuous focus A@ws1 → session neither restarted nor rebuilt.
- **`mru:status` payload** not observable via `hyprctl` on this pin (COMPAT.md IPC note); all membership/freeze assertions done indirectly via `activewindow -j`.
- **Timing discipline:** ≥1.2 s pauses after focus changes so the 400 ms history debounce commits deterministically (same artifact as prior smoke report); rapid-fire runs would shift the visible pool toward recent focuses without indicating a plugin defect.
- Startup `Invalid dispatcher: mru:*` errors in the nest log are the documented pre-load parse noise; after `plugin load` + the config-reload re-resolve the dispatchers work (all steps above executed fine).

## Raw evidence

```
/tmp/mru-nest-reload/report/01-baseline.txt           step 1  (8-key surface, defaults)
/tmp/mru-nest-reload/report/02-global-baseline.txt    step 2  (global: apply → B@ws2)
/tmp/mru-nest-reload/report/03a-pre-reload.txt        step 3  (session Active, scope=global)
/tmp/mru-nest-reload/report/03b-after-reload.txt      step 3  (getoption=workspace, frozen apply → B@ws2)
/tmp/mru-nest-reload/report/04-new-session-workspace-scope.txt  step 4 (new session → A@ws1 only)
/tmp/mru-nest-reload/report/05a-before.txt            step 5  (debounce_ms=400)
/tmp/mru-nest-reload/report/05b-after.txt             step 5  (debounce_ms=1000)
/tmp/mru-nest-reload/report/06-ui-not-runnable.txt    step 6  (ui=null only → NOT-RUNNABLE)
/tmp/mru-nest-reload/env                              nest SIG
/tmp/mru-nest-reload/hypr-nest.conf                   nest config (as-mutated across steps)
/tmp/mru-nest-reload/nest.log                         nest stdout log
```