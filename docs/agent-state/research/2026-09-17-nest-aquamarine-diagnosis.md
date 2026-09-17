# Nest diagnosis — Hyprland v0.56.2 / aquamarine 0.15.0 (Omarchy) — 2026-09-17

**STATUS: working.** A nested Hyprland session starts, is interactive, loads the plugin, and runs
the full M2 dispatcher matrix + invariants live. No production code was changed.

**Plan:** `docs/agent-state/plans/2026-09-17-nest-aquamarine-diagnosis.md` (Tasks 1-5 by a diagnosis
agent, Task 4 live smoke by a tester agent). **Evidence root:** `/tmp/mru-nest-diag/`
(`DIGEST.md`, `report/00-env.txt`…`report/04-15-posttest.txt`). Dates: 2026-09-17 20:31-20:41 +03:00.

## Environment (measured, `report/00-env.txt`)

- `hyprland 0.56.2-2` @ commit `efb50993780079460b0cbed1363e2166a2de1d9f` (tag `v0.56.2`); ABI string
  `..._aq_0.15_hu_0.14_hg_0.5_hc_0.1_hlg_0.6`.
- **`aquamarine 0.15.0-2`** (`libaquamarine.so.14`). The brief's "aquamarine 0.56.2" was a slip:
  0.56.2 is the *Hyprland package*. Pin recorded correctly in COMPAT.md.
- Host: Wayland (`wayland-1`, pid 1230, `eDP-1` 1920x1080) — never touched.
- Plugin under test: `build/mru-switcher.so`, sha256 `29d9b82e…d954bf`, mtime 2026-09-15 20:42:21,
  size 4373056 (v0.2.0 release artifact).

## Ruled out (with evidence)

- **Host config leakage** — not a factor: `HYPRLAND_INSTANCE_SIGNATURE` / `HYPRLAND_CONFIG` unset and
  `-c /tmp/mru-nest-diag/hypr-nest.conf` explicit (`02-verify.txt`, `nest-final.log:1-3`).
- **Headless CLI** — `--headless` is absent in 0.56.2 (`nest-headless.log:1` `Unknown option`);
  headless works only via `AQ_HEADLESS=1` env (`03-headless-hyprland.log:232`). Plan risk #1 confirmed.
- **Software EGL** (`LIBGL_ALWAYS_SOFTWARE=1 GALLIUM_DRIVER=llvmpipe`) — no observable change; renderer
  is inherited from the host compositor. Not load-bearing. (This is a *headless renderer fallback*,
  not a Wayland-backend blocker — consistent with COMPAT.md's older headless note.)
- **Config parse fault** — NO (`02-verify.txt`): structure/keys parse; the only errors are the expected
  `Invalid dispatcher: mru:cycle/apply/cancel` from parse-before-plugin-load (already documented in
  COMPAT.md §Observations).
- **Range of the first failure** — bounded: `nest-baseline.log` failed *solely* on a missing config file
  (Task 1 ran before Task 2 created it), not on the compositor.

## Hypotheses — verdict table

| # | Hypothesis | Verdict | Evidence (file:line) |
|---|-----------|---------|----------------------|
| H1 | normal nested Wayland backend starts | **PASS** | `report/03-hdmi.txt` nest `wl_socket=wayland-2`, pid alive; `03-hdmi-hyprland.log:37-38` `Starting the Wayland backend!` / `Connected to a wayland compositor: Hyprland` |
| H2 | headless backend starts | **PASS (env only)** | `03-headless-hyprland.log:232` `backend: poll fd 12 for implementation headless`; `--headless` → `nest-headless.log:1` |
| H3 | software EGL path starts | **PASS** (no-op vs H1) | `report/03-soft.txt` |
| H4 | fault is config-parse | **NO** | `report/02-verify.txt` (only expected `Invalid dispatcher: mru:*`) |
| H5 | nest blocked by "broken aquamarine" | **NO — carried claim refuted** | `report/04-TASK4-digest.txt` + `04-15-posttest.txt`: full live matrix PASS |

## Root cause (one sentence)

Aquamarine's DRM backend always fails first because the host owns the seat (`seatd.sock` missing →
logind `Could not take control of session: Device or resource busy`), and aquamarine then **correctly
auto-falls back to the Wayland (nested) backend** — benign and expected for a nested session; the nest
was never broken by aquamarine.

Verbatim (≤20-line tail, `report/03-hdmi-hyprland.log:28-38`):

```text
DEBUG from aquamarine ]: [libseat] [libseat/backend/seatd.c:64] Could not connect to socket /run/seatd.sock: No such file or directory
DEBUG from aquamarine ]: [libseat] [libseat/libseat.c:76] Backend 'seatd' failed to open seat, skipping
ERR from aquamarine ]: [libseat] [libseat/backend/logind.c:309] Could not take control of session: Device or resource busy
ERR from aquamarine ]: DRM Backend failed
DEBUG from aquamarine ]: Starting the Wayland backend!
DEBUG from aquamarine ]: Connected to a wayland compositor: Hyprland
```

### Two distinct failure classes (do not conflate)

| Class | What | Where | Status |
|-------|------|-------|--------|
| **Fail-closed hash** | plugin refuses to load when `__hyprland_api_get_hash()` mismatches | plugin side, expected by design | verified by pinned source (M2), untouched here |
| **Nest startup** | compositor + backend selection + config parse | environment side | **working** (this artifact) |

No hash-mismatch event occurred in this nest — the `.so` was built against the same pinned pin, so the
fail-closed path was not exercised (it is covered by plugin tests).

## Working recipe (verified 2026-09-17)

```bash
mkdir -p /tmp/mru-nest-diag/cache
env -u HYPRLAND_INSTANCE_SIGNATURE -u HYPRLAND_CONFIG \
  XDG_CACHE_HOME=/tmp/mru-nest-diag/cache \
  Hyprland -c /tmp/mru-nest-diag/hypr-nest.conf > /tmp/mru-nest-diag/nest.log 2>&1 &
hyprctl instances -j                 # nest = wayland-2; take SIG from here (NOT `ls -t`)
hyprctl -i "$SIG" plugin load /home/code_warlord/Work/DEV/mru-switcher/build/mru-switcher.so
```

Gotchas observed: `--socket NAME` alone is rejected (`requires --wayland-fd`); `ls -t
/run/user/1000/hypr` is unreliable (stale dirs tie on mtime) → use `hyprctl instances -j`;
`XDG_CACHE_HOME` does not relocate the instance dir (logs go to `/run/user/1000/hypr/<SIG>/hyprland.log`);
expected startup noise: `wayland-1.lock` probe, xkbcomp warnings, Xwayland `could not connect to wayland server`.

## Live nest smoke — M2 re-verified (`report/04-TASK4-digest.txt`)

- Dispatcher matrix: all 11 rows PASS (incl. `cycle bogus` → exact `unknown argument: bogus`);
  `mru:status` rows are PASS-partial — see the IPC finding below.
- Invariants: REQ-F-003 (cycle x2, focus unchanged), T-S-02 apply→exact window, REQ-S-005 cancel→no focus,
  T-H-01 no flicker over 5 cycles, T-F-03/04 apply-after-close → survivor, REQ-H-008 unload → `ok`,
  `plugin list []`, 0 segv/assert. All PASS.
- **B2 adjudication: CONFIRMED live** (`04-11-b2-main.txt`, `04-11b-b2-verdict.txt`): after apply the
  applied window is `focusHistoryID 0` (the reentrant `window.active` listener fired inside the focus
  path), and an immediate cycle+apply goes to the *previous* window (Alt+Tab-like). This discharges the
  admitted assumption in `docs/agent-state/audit/fix-register.md` B2 ("Adapter-side … NOT verified on
  live compositor") and the "nested env broken" note there. (The earlier probe `04-08` was polluted by
  interleaved `mru:status`; re-run cleanly — recorded, not hidden.)
- Post-test (`04-15-posttest.txt`): only host pid 1230 / `wayland-1`; no stray nests; host plugin list `[]`.

## Impact on COMPAT.md / M3-S3 §7

- COMPAT.md §M2: matrix + invariants **re-verified live 2026-09-17** on the same pin; nest recipe and
  aquamarine version now recorded.
- **Doc defect found:** on 0.56.2, `hyprctl dispatch mru:status` prints bare `ok`; the status string is
## SPEC verdict — `mru:status` payload not observable via hyprctl

| REQ / doc line | Normative text | Observed fact | Verdict |
|-----------|-----------|---------------|---------|
| **REQ-DISP (SPEC §3.0)** | dispatchers return `SDispatchResult`; success → `{ success: true }` | `mru:status` → `ok` on success | (а) **SPEC-compliant** — SPEC does not require a success payload on the IPC surface |
| **SPEC §3.4** | "Return success; error field or notification MAY contain human-readable status"; "Exact format is **informative for v0.x**; clients must not parse strictly until 1.0" | status string is computed, but 0.56.2 IPC shows the `error` field only on failure → the `MAY` is not observable | (а) **SPEC-compliant** (`MAY` = optional); (б) **partially** — SPEC is silent on *observability*; COMPAT.md presents one unobservable mechanism as the record |
| **SPEC §195** | "`mru:status` and debug logs SHOULD expose the internal reason **when verbose**; the human-readable status string remains **non-normative until 1.0**" | no verbose channel used; release `.so` emits no mru debug lines | (а) **SPEC-compliant**; the SHOULD is satisfied only via the verbose/debug channel, which this smoke run did not use → **unverified, not violated** |
| **T-DISP-01..04** (`docs/REQ-TRACE.md:121-124`) | `cycle` argument grammar | all four exercised live, all PASS | n/a — unaffected by the payload question |
| **COMPAT.md §M2 rows 3/5/11 + "D7 payload" checklist line** | records full `active=… index=… last_end=…` payloads | not reproducible via `hyprctl` on this pin | (б) **documentation/reproducibility defect** in COMPAT.md (mechanism unspecified); **not** a plugin defect — the plugin computes the string (proven by failure-path strings surfacing, `04-05`) |

**Overall: (а) + (б) for the doc; not (в).** No SPEC contradiction exists, so **no ADR is required**.
Follow-up (small docs/test ticket, outside M3 scope): either (i) COMPAT.md records the *exact* capture
mechanism used for the payloads, or (ii) the smoke step asserts status state indirectly via
`hyprctl activewindow -j` / `focusHistoryID` (as this run did) and the payload rows are labelled
"capture mechanism unspecified". Revisit in the M4/M5 observability pass if the payload becomes
externally observable via the External UI protocol (ADR-004 / SPEC Appendix B).

## Human action needed

**None.** No sudo, no package pin change, no upstream issue URL. Optional queued item: the
`mru:status` documentation-reproducibility ticket above (docs-only).

## Not verified / not covered

- M3-S3 §7 scope-adapter live checklist: **still not runnable** — the scope adapter is not implemented
  (`debug_scope` / `external_socket` absent from `src/`); only 7 config keys are registered vs the 8 in
  SPEC §4 (`scope_enabled` → `no such option`, `04-09-options.txt`); the nest has a single monitor; the
  app byte-exact cases need the adapter. No M3-S3 box is ticked by this run.
- `mru:status` payload not observable via `hyprctl`: the dispatcher returns bare `ok` on success, and the
  status line is carried in the *success* result's `error` field, which the 0.56.2 IPC surfaces only for
  failures (`04-04-rawipc-status.txt`: `b'/dispatch mru:status' => b'ok'`; `04-05`: failure strings do
  appear). The payloads recorded in COMPAT.md §M2 are therefore **not reproducible as written via
  hyprctl**; they are kept and annotated instead (see SPEC verdict below).
- `ui=border` (M4) and `external` (M5) backends.
- Fail-closed hash path was not triggered (same-pin build).
- Release build has no plugin debug logging → B2 evidence is compositor-side (`focusHistoryID`), not
  plugin-log-side; `--headless` does not exist on this pin (headless only via `AQ_HEADLESS=1`).