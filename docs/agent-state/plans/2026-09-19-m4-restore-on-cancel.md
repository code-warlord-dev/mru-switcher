# M4 closeout — restore_focus_on_cancel optional path (nest confirmation + regression)

> Planner artifact (orchestrator/worker model, AGENTS.md §3.4). Executors: read SPEC §6 + `docs/TRANSITION-TABLE.md` before editing.

**Goal:** Close the last M4 item — the optional `restore_focus_on_cancel` path is already implemented on `main` (domain + wiring; T-S-05/T-S-06 green, ctest 12/12) but its restore leg was **never confirmed live** (the 2026-09-18 12-row nest matrix ran with the default `restore=0`) and freeze / non-cancel-end regression tests are missing.
**Spec:** `docs/SPEC.md` §2.1 (REQ-S-005/007/009), §6 (REQ-R-001/002); `docs/TRANSITION-TABLE.md`; `docs/FAILURE-MODES.md` (FM-02/13/18).
**Branch:** `feat/m4-restore-on-cancel` (already checked out).

## 1. Scope / out-of-scope

| In scope | Out of scope |
|---|---|
| Live nest confirmation of the restore leg (N1–N4) | Any dispatcher/grammar change — `mru:cycle\|apply\|cancel` frozen (REQ-S-005) |
| Unit regression T-S-09 (policy freeze) + T-S-10 (non-cancel end never restores) | New config keys — the 11-key SPEC §4 surface is complete |
| SPEC §6 clarifying sentence + SPEC §9 / REQ-TRACE / CHANGELOG / COMPAT / README / USER rows | Flipping defaults `ui=null` / `restore_focus_on_cancel=false` (product decision, not required) |
| — | `pulse`/`dim`, M5 overlay, FM-22 abrupt-eject mitigation |

**No ADR required.** Behaviour is already normative in two frozen sources: SPEC §6 (REQ-R-001/002 scope restore to `mru:cancel`) and `TRANSITION-TABLE.md` (row `Active \| cancel \| restore config \| optional focus origin`; row `prune_event` has **no** restore action). This change adds verification + regression coverage + a *clarifying* restatement — no design shift (per `plugin-spec-compliance`, ADR is for design shifts).

## 2. REQ → test → file mapping

| REQ | Test ID | Test file | Assertion focus |
|---|---|---|---|
| REQ-R-001 | T-S-05 (exists), **T-S-09**, **T-S-10** | tests/domain/test_session_controller.cpp | restore only on explicit cancel |
| REQ-R-002 | T-S-06 (exists) | same | invalid origin → no focus |
| REQ-S-005 | T-S-03 (exists), **T-S-10** | same | cancel ≠ other end paths |
| REQ-S-007 | T-S-05 (exists) | same | `session_origin` captured at session start |
| REQ-S-009 | T-CFG-02 (exists), **T-S-09** | same | policy frozen mid-session; next session takes new value |
| REQ-F-003 | T-F-01 (exists), **N1** | test + nest | cycle never focuses |
| REQ-H-001 | T-H-01, bonus_focus_during_active (exist), **N1/N3** | same | lock-in; origin focus only after unlock |

## 3. Exact code/test changes (minimal)

### 3.1 New unit tests — `tests/domain/test_session_controller.cpp` (append before `} // namespace`, line 621)

Fixture/top-of-file already provides `Fixture(SessionPolicy)`, `set_valid(ref,bool)`, `candidates({...})`; fakes `MockWindowSource.validity / focused_result`, `MockFocusGateway.focused` (test_fakes.hpp:25–73).

**T-S-09 — `TEST(t_s_09_restore_flag_frozen_mid_session)`** (mirror `t_cfg_02`, line 589):
```
Fixture f(policy{restore_focus_on_cancel = true});
f.candidates({ref(10), ref(20)}); f.source.focused_result = {ref(10)};   // origin
CHECK(f.sc.cycle(Direction::Next).ok);
SessionPolicy reloaded; reloaded.restore_focus_on_cancel = false;
f.sc.set_policy(reloaded);                                               // refresh mid-session
CHECK(f.sc.cancel().ok);
CHECK(f.fg.focused == std::vector<WindowRef>{ref(10)});  // frozen=true -> origin refocused
CHECK(f.sc.cycle(Direction::Next).ok);                   // NEXT session
f.source.focused_result = {ref(10)};
CHECK(f.sc.cancel().ok);
CHECK(f.fg.focused.size() == 1);                         // new value applied, no 2nd focus
```

**T-S-10 — non-cancel end never restores** (two `TEST`s under one SPEC ID):
- `TEST(t_s_10_empty_snapshot_never_restores)` — REQ-S-006/NoWindows: `Fixture f(policy{restore=true})`; `candidates({ref(10)})`; `focused_result={ref(10)}`; cycle; `set_valid(ref(10),false); f.sc.on_window_invalid(ref(10));` → `CHECK(f.fg.focused.empty()); CHECK(f.sc.last_end_reason()==SessionEndReason::NoWindows);` (FM-02).
- `TEST(t_s_10_plugin_shutdown_never_restores)` — extends `bonus_plugin_shutdown_ends_active_session` (line 545): same policy; cycle; `f.sc.plugin_shutdown();` → `CHECK(f.fg.focused.empty()); CHECK(last_end_reason()==PluginShutdown);`.

`cancel()` is the **only** restore site (session_controller.cpp:103–115); `end_session()` never focuses — so these are characterization/regression tests, expected green on first run. If red: real bug → stop the PR, fix code (do not edit the test).

### 3.2 SPEC §6 — one clarifying sentence (documentation, **no behaviour change**)
Append after REQ-R-002: *"Restore applies to an explicit `mru:cancel` only; a session ended for any other reason (REQ-S-006 empty snapshot / `NoWindows`, plugin shutdown, apply) never moves focus."* — listed in the PR body as a docs clarification.

### 3.3 Docs rows (same PR)

| File | Edit |
|---|---|
| SPEC.md §9 | add `T-S-09` (policy freeze covers restore flag), `T-S-10` (non-cancel end never restores) |
| REQ-TRACE.md | REQ-R-001 → +T-S-09, T-S-10; REQ-R-002 → +T-S-10; REQ-S-005 → +T-S-09; REQ-S-009 → +T-S-09; Test-ID index → +T-S-09/T-S-10 |
| CHANGELOG [Unreleased] | Added: T-S-09/T-S-10 + nest confirmation of restore leg; Changed: SPEC §6 clarification |
| COMPAT.md M4 section | recorded nest row N1–N4 (pin `efb5099`) + note that T-S-10 stays unit-only |
| README.md (~line 48) | "Optional next" → drop the now-done `restore_focus_on_cancel`; leave M5 overlay |
| USER.md | add `restore_focus_on_cancel` line to manual checklist (~237) — only if wording absent (binds table line 96 already covers it) |

## 4. Nest matrix N1–N4 (live, narrow, deterministic)

**Pin:** Hyprland v0.56.2 `efb50993780079460b0cbed1363e2166a2de1d9f`, aquamarine 0.15.0, Wayland nested backend (DRM fallback noise benign). Harness = `hyprland-nested-dev`; reuse `/tmp/mru-nest-m4/` scripts/config.
**Channel:** config-file edit + `hyprctl reload` **only** — `hyprctl keyword` is dead on this pin (D1, COMPAT).
**Capture:** `getprop` border sweep + `hyprctl activewindow -j` (`focusHistoryID`), pre-session baseline stored per window.

| # | Config | Steps | Pass condition |
|---|---|---|---|
| N1 | `ui=border, restore_focus_on_cancel=1` | 3 foot windows; baseline getprop; cycle ×2 | highlight follow + `activewindow` unchanged during cycles (REQ-F-003); `mru:cancel` → origin refocused ∧ every previously-highlighted window's props **exactly equal** baseline (no `0deg`, no stuck) |
| N2 | `ui=null, restore_focus_on_cancel=1` | cycle, then `mru:cancel` | origin refocused; ZERO border prop deltas on all windows |
| N3 | `ui=border, restore_focus_on_cancel=1` | close the **origin** window mid-session, then `mru:cancel` | no focus change, no crash, borders restored (REQ-R-002 / FM-18) |
| N4 | `ui=border, restore=1` | start session → config-file edit to `restore=0` + `hyprctl reload` → `mru:cancel` | **STILL** refocuses origin (frozen, REQ-S-009); next session `cancel` → no refocus |

**OUT of the live matrix (documented):** the non-cancel-end leg (T-S-10) cannot be discriminated live without a second monitor/scope trick — stated unit-only in COMPAT + nest report, covered by T-S-10.

### 3.1a Orchestrator acceptance correction — T-S-10 must discriminate

The 3.1 pseudocode for T-S-10 makes the origin **invalid** (origin = the only candidate = `ref(10)`, which is then closed), so a hypothetical buggy "restore on NoWindows end" would be masked by the validity guard — the test would pass for the wrong reason (that is T-S-06 territory). Required design instead: **keep the origin valid but out of the snapshot**, so any focus call on a non-cancel end path is observable.

```
TEST(t_s_10_empty_snapshot_never_restores):          // REQ-S-006/NoWindows
  SessionPolicy p; p.restore_focus_on_cancel = true;
  Fixture f(p);
  f.candidates({ref(10)});            // snapshot = [10]
  f.set_valid(ref(99), true);         // origin VALID but NOT a candidate
  f.source.focused_result = {ref(99)}; // session_origin = 99
  (void)f.sc.cycle(Direction::Next);   // Active
  f.set_valid(ref(10), false);
  f.sc.on_window_invalid(ref(10));     // snapshot empties -> end NoWindows
  CHECK(f.fg.focused.empty());                  // no restore on a non-cancel end
  CHECK(f.sc.last_end_reason() == SessionEndReason::NoWindows);
  CHECK(!f.sc.is_active());
  CHECK(f.ui.ends.size() == 1 && f.ui.ends[0] == UIEndReason::Cancelled);

TEST(t_s_10_plugin_shutdown_never_restores):         // PluginShutdown
  same policy; candidates({ref(10)}); set_valid(ref(99), true);
  source.focused_result = {ref(99)}; cycle;
  f.sc.plugin_shutdown();
  CHECK(f.fg.focused.empty());
  CHECK(f.sc.last_end_reason() == SessionEndReason::PluginShutdown);
```

Same principle for **T-S-09**: assert both `fg.focused` contents and `ui.ends` (`Cancelled`, `Cancelled`) so the frozen-flag leg cannot pass by accident.

## 5. DoD / merge gates (AGENTS.md §6.2) + PR shape + risks

- [ ] SPEC-compliant (SPEC §6 sentence + §9 IDs land in the same PR)
- [ ] No focus from `mru:cycle` (N1 + T-F-01); domain free of Hyprland types; single FocusGateway
- [ ] Hash check intact in `PLUGIN_INIT`; no new config keys
- [ ] T-S-09/T-S-10 registered in SPEC §9 + REQ-TRACE; ctest green (12/12 → 14/14)
- [ ] Nest report at `docs/agent-state/reports/2026-09-19-m4-restore-on-cancel.md`
- [ ] CHANGELOG / COMPAT / USER / README rows updated
- [ ] SESSION.md + PROGRESS.md M4 checkbox closed; PR body `Fixes #<n>`

**PR shape:** branch `feat/m4-restore-on-cancel` off `main`; commits — `test(domain): freeze restore flag mid-session (T-S-09)`, `test(domain): non-cancel end never restores (T-S-10)`, `docs(spec): clarify restore scope for cancel`, `docs(nest): M4 restore-on-cancel live confirmation`, `docs(state): M4 closeout`. PR body: Summary / SPEC IDs (REQ-R-001/002, REQ-S-005/007/009, REQ-F-003, REQ-H-001; T-S-09/10) / no-ADR justification (§1) / test plan (ctest + N1–N4) / docs-clarification note.

**Risks:** refocus observable only via `activewindow`/`focusHistoryID` (`mru:status` unreadable on this pin) → keep both; N4 needs two reloads in one nest (harness proven in 10b); D1 → never use `keyword`; FM-22 abrupt-eject gap unchanged (documented); T-S-09/10 red ⇒ real bug, stop.
