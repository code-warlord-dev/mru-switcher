# Session State
Updated: 2026-09-23 (branch cleanup + main test)
Human goal: continue repo work per ROADMAP; ADR-028 merged + README switching guide merged; live host on dim. РЕЛИЗ/ТЕГ ТОЛЬКО ПО ЯВНОЙ КОМАНДЕ.
Active milestone: M6 (release-prep) with ADR-028 landed; v1.0.0 tag strictly human-gated
Branch: main (2d4098d)
PR: none open; #106 (README guide) + #107 (state) merged
Blocked: none
Next action: idle. Live host on `border_style = dim` (sidecar). Optional: dim/pulse nest smoke (recorded pending); then M6-T9 tag on explicit human command. Stale origin branches deleted: feat/ui-border-default (merged via #97), docs/external-review-integration (merged via #82)
SPEC focus: REQ-UI-013, REQ-UI-014, REQ-UI-015, ADR-028
Open questions: (1) upstream Lua `plugin {}` support? (2) dim live visual = indistinguishable from solid on host — diagnosis paused by human ("Оставляем dim")
Last artifact: full rebuild + ctest 19/19 + clang-format clean on main 2d4098d (2026-09-23)
