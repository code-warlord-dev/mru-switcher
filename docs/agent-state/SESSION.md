# Session State
Updated: 2026-09-22
Human goal: live-host border highlight works (confirmed); multi-workspace blindness fixed, merged, and confirmed live by human ("ОГОНЬ, РАБОТАЕТ"). РЕЛИЗ/ТЕГ ТОЛЬКО ПО ЯВНОЙ КОМАНДЕ.
Active milestone: M6 (release-prep); v1.0.0 tag strictly human-gated
Branch: main (41b834d)
PR: #101 merged (squash 41b834d) — ADR-026 / REQ-UI-012 selection view follows the highlighted window; CI 8/8 green, ctest 19/19, nest smoke confirmed
Blocked: none
Next action: idle. Optional follow-up: decide on multi-monitor already-visible targets (focus-monitor during hold) — explicitly out of ADR-026 scope by design; revisit only on feedback
SPEC focus: REQ-UI-012, REQ-F-003, §4 `selection_follow_workspace`, ADR-026
Open questions: (1) upstream Lua `plugin {}` support? (2) multi-monitor hold-focus movement — deferred by design, revisit on feedback
Last artifact: PR #101 (merged 41b834d) — WorkspaceNavigator port + HyprlandWorkspaceNavigator (changeWorkspace noFocus), BorderHighlightUI begin/ensure/end, config key default true, tests + docs