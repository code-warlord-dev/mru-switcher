# Session State
Updated: 2026-09-19
Human goal: prepare the repo for the first stable-facing release; M5 external overlay delivered
Active milestone: M6 HARDENING → v1.0 — plan merged (PR #44); issues not yet created
Branch: main @ 0980cdb; no active feature branch
PR: #40/#39/#42/#43 MERGED (v0.5.0 released, tag v0.5.0); #44 plan MERGED
Next action: await human approval to create M6 milestone + issues T1..T9/B1, then start M6-T1 (contract-freeze audit)
Blocked: M6 issue creation awaiting human approval (plan §6)
State: v0.5.0 released; ctest 15/15 (clang debug + ASan/UBSan + gcc); nest 13/13 on pin 0.56.2/efb5099; CI 6/6
SPEC focus: M5 closed; M6 = contract freeze (T1), docs==impl (T2), CI gate (T3), security (T4), hyprpm pins (T5), stress T6/T7/T8, release T9
Open: known 0.5.0 limitations in docs/COMPAT.md; M6-B1 same-path .so reload (confirm→fix-or-document)
Last artifact: docs/agent-state/plans/2026-09-19-m6-hardening.md (PR #44)
