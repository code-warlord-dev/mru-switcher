# Session State
Updated: 2026-09-22 (post-PR-104)
Human goal: continue repo work per ROADMAP; ADR-028 pulse/dim merged. РЕЛИЗ/ТЕГ ТОЛЬКО ПО ЯВНОЙ КОМАНДЕ.
Active milestone: M6 (release-prep) with ADR-028 landed; v1.0.0 tag strictly human-gated
Branch: main (742a9ad)
PR: #104 merged (squash 742a9ad) — ADR-028 full border styles pulse/dim; REQ-UI-007 revised, REQ-UI-013/014/015, T-UI-13/14/15; review REQUEST-CHANGES → 6 findings fixed; CI 8/8, ctest 19/19
Blocked: none
Next action: idle. Optional: nest smoke for pulse/dim visuals (recorded pending in REQ-TRACE deviations); then M6-T9 tag on explicit human command
SPEC focus: REQ-UI-013, REQ-UI-014, REQ-UI-015, ADR-028
Open questions: (1) upstream Lua `plugin {}` support? (2) dim visual confirmation live — unit-tested, nest pending
Last artifact: PR #104 (merged 742a9ad) — BorderStyle Pulse/Dim, HyprlandPulseTimer (wl_event_loop_add_timer), opacity/opacity_inactive dim slots, V2+sidecar keys, docs/examples/CHANGELOG
