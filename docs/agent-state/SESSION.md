# Session State
Updated: 2026-09-22
Human goal: live-host border highlight works (confirmed); multi-workspace blindness fixed and merged (view follows selection). РЕЛИЗ/ТЕГ ТОЛЬКО ПО ЯВНОЙ КОМАНДЕ.
Active milestone: M6 (release-prep); v1.0.0 tag strictly human-gated
Branch: main (41b834d)
PR: #101 merged (squash 41b834d) — ADR-026 / REQ-UI-012 selection view follows the highlighted window; CI 8/8 green, ctest 19/19, nest smoke confirmed
Blocked: none
Next action: human optional live-host re-check — after host swap the border now follows across hidden workspaces during Alt-hold (new build/mru-switcher.so has it); decide whether multi-monitor already-visible targets should also move focus monitor during hold (explicitly OUT of ADR-026 scope)
SPEC focus: REQ-UI-012, REQ-F-003, §4 `selection_follow_workspace`, ADR-026
Open questions: (1) upstream Lua `plugin {}` support? (2) multi-monitor hold-focus movement — deferred by design, revisit on feedback
Last artifact: PR #101 (merged 41b834d) — WorkspaceNavigator port + HyprlandWorkspaceNavigator (changeWorkspace noFocus), BorderHighlightUI begin/ensure/end, config key default true, tests + docs