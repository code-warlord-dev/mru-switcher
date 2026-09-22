# Session State
Updated: 2026-09-22
Human goal: live-host border highlight works (confirmed by human); now fix multi-workspace blindness — selection view follows the highlighted window. РЕЛИЗ/ТЕГ ТОЛЬКО ПО ЯВНОЙ КОМАНДЕ.
Active milestone: M6 (release-prep); v1.0.0 tag strictly human-gated
Branch: feat/adr-026-selection-follow-workspace (worktree contains ADR-026 implementation, uncommitted)
PR: none yet — preparing PR for ADR-026 / REQ-UI-012 (view-follow)
Blocked: none
Next action: commit feature → push → PR → self-review → CI → squash-merge; then live-host note for human (border now follows workspace during hold)
SPEC focus: REQ-UI-012, REQ-UI-006, REQ-F-003, REQ-CFG-002, §4 `selection_follow_workspace`; ADR-026
Open questions: (1) upstream Lua `plugin {}` support? (2) after merge: human live-host perception of workspace flips during Alt-hold (multi-monitor already-visible targets NOT elevated by design)
Last artifact: ADR-026 + SPEC §4/§5/§9 edits + implementation (WorkspaceNavigator port, HyprlandWorkspaceNavigator adapter via CMonitor::changeWorkspace(ws,false,true,true), BorderHighlightUI begin/ensure/end, config key default true, sidecar, tests t_ui_012_*×5 + cfg_08* + sidecar_08*, docs) — build+ctest 19/19, nest smoke: view follows selection, focus untouched, cancel restores session-start workspace