# Session State
Updated: 2026-09-19
Human goal: prepare the repo for the first stable-facing release; M5 external overlay delivered
Active milestone: M5 COMPLETE — v0.5.0 release PR (tag placed on the release merge commit)
Branch: main (release PR for v0.5.0); tag v0.5.0 on the release merge commit
PR: #40 M5 MERGED; #39 assets MERGED (e51f17f); #42 README MERGED (85a3a5f); release PR = this change
Next action: M6 planning (writing-plans / to-tickets on hardening → v1.0)
Blocked: none
State: ctest 15/15 (clang debug + ASan/UBSan + gcc); nest smoke 13/13 PASS on pin 0.56.2/efb5099; CI 6/6 on every merge
SPEC focus: M5 closed (REQ-O-001..008, ADR-018/019); M6 will target contract freeze, hyprpm pins, stress
Open: known 0.5.0 limitations recorded in docs/COMPAT.md (same-path .so reload; non-cancel-end live test); M6 not started
Last artifact: v0.5.0 release PR
