# Session State
Updated: 2026-09-18
Human goal: M4 — Border UI + polish (released as v0.4.0)
Active milestone: M4 CLOSED — v0.4.0 released
Branch: main (f8d2eb2 = PR #32 squash; tag v0.4.0 on f8d2eb2)
PR: #32 merged (release prep); #27/#29/#30/#31/#32 all merged
Release: v0.4.0 — annotated tag + GitHub Release (notes = CHANGELOG [0.4.0] + milestone/pin line + D1/D2 highlights); hyprpm commit_pins deliberately post-M6
Next action: idle — per human direction: light polish notes, optional restore_focus_on_cancel between 0.4 and M5, or M5 only if rich UI needed; FM-22/perf = backlog, not gate
Blocked: none
State: M4 complete — M4-R0 (#25), ADR-017 + SPEC REQ-UI-001..011 (#26), M4-S1 backend (#27), M4-S3 smoke + D2 restore-grammar fix CONFIRMED live 5/5 + D1 keyword-channel docs (#30). ctest 12/12; CI 6/6. Nest evidence committed with inlined annex.
SPEC focus: REQ-UI-001..011 closed for M4; T-UI-01..07 (+ t_ui_010_*)
Open: FM-22 abrupt-eject stuck borders (documented gap); perf loop / FFM highlight-follow untested (outside matrix); live degrade leg NOT-RUNNABLE (unit-covered); restore_focus_on_cancel optional path
Last artifact: PR #30 (dfff936); PROGRESS M4 checkboxes flipped
