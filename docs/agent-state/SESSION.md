# Session State
Updated: 2026-09-18
Human goal: M4 — Border UI + polish (usable visual feedback, no external processes)
Active milestone: M4
Branch: fix/m4-s3-border-restore-grammar (98c0dd5 D2 fix + 75a735f D1 docs)
PR: #27 merged (M4-S1, squash 92d84a7); #29 merged (state sync, 79bc630); PR for this branch pending
Next action: push -> PR -> CI -> self-merge -> M4 closeout (PROGRESS checkboxes, VERSION-MAP, FM-22 gap note)
Blocked: none
State: M3 done (v0.3.0). M4-S1 merged. M4-S3 nest smoke DONE (pin 0.56.2/efb5099): PASS 5 / FAIL 6 / NOT-RUNNABLE 1 -> D1 keyword-channel limitation (docs-only, resolved: COMPAT/USER/REQ-TRACE) + D2 restore-grammar corruption (S1, fixed: normalize_capture + border_size restore-by-value; ctest 12/12 green). Compliance review: READY after fixups -> nest report committed with inlined key evidence.
SPEC focus: REQ-UI-001..011; T-UI-01..07 (+ t_ui_010_*); REQ-CFG-002/003; ADR-017
Open: FM-22 abrupt-eject stuck borders (documented gap); perf loop + FFM highlight-follow untested (outside 12-row matrix); live degrade leg NOT-RUNNABLE (unit-covered)
Last artifact: docs/agent-state/reports/2026-09-18-m4-s3-nest-smoke.md
