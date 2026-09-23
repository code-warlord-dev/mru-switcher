# Session State
Updated: 2026-09-23 (post-PR-106)
Human goal: continue repo work per ROADMAP; ADR-028 merged + README switching guide merged; live host on dim. РЕЛИЗ/ТЕГ ТОЛЬКО ПО ЯВНОЙ КОМАНДЕ.
Active milestone: M6 (release-prep) with ADR-028 landed; v1.0.0 tag strictly human-gated
Branch: main (3e5db5e)
PR: #106 merged (squash 3e5db5e) — README border-style switching guide (solid/pulse/dim, key replacement, reload semantics); CHANGELOG bullet; docs-only
Blocked: none
Next action: idle. Live host on `border_style = dim` (sidecar). Optional: dim/pulse nest smoke (recorded pending in REQ-TRACE); then M6-T9 tag on explicit human command
SPEC focus: REQ-UI-013, REQ-UI-014, REQ-UI-015, ADR-028
Open questions: (1) upstream Lua `plugin {}` support? (2) dim live visual = indistinguishable from solid on host — diagnosis paused by human ("Оставляем dim"); pending check whether ring-alpha writes land
Last artifact: PR #106 (merged 3e5db5e) — README switching guide; live host: build/mru-switcher.so (ADR-028) loaded, sidecar border_style=dim
