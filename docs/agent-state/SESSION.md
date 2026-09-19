# Session State
Updated: 2026-09-19
Human goal: add README badges to main (done, PR #37); then deliver M5 (external overlay) fully
Active milestone: M5 EXTERNAL OVERLAY — code + docs complete on branch; M5 closes on merge to main
Branch: feat/m5-external-overlay @ 4deb303 (from main @ e5cd2ba; assets extracted to chore/assets-banner PR #39; M5 history rebased clean)
PR: M5 PR pending (this step); #38 (ADR-018 gate) MERGED; #39 (assets) open; #37 (badges) MERGED
Next action: open M5 PR → CI 6/6 → plugin-spec-compliance + code-review self-review → squash-merge → close M5 checkbox
Blocked: none
State: core ctest 15/15 (clang debug + ASan/UBSan + gcc); .so builds; plugin-guards clean; nest smoke 13/13 PASS on pin 0.56.2/efb5099 (session_start/selection/session_end, peer select/apply/cancel, bounds, empty-path degrade, unload no-leak)
Fixes during smoke: dangling handler capture (SEGV), bind-on-config.reloaded (PLUGIN_INIT values empty on pin), stop socket on non-external/empty path
SPEC focus: REQ-O-001..008, REQ-O-006 (amended, ADR-019), REQ-UI-002/009, REQ-PERF-001/003, REQ-RE-001; T-O-01..08
Open: ADR-019 accepted (human) — human gate for adr/* resolved; doOnReadable explicitly rejected; same-path binary reload crash is an operator note (fresh nest when .so changes)
Last artifact: docs/agent-state/reports/2026-09-19-m5-s3-nest-smoke.md
