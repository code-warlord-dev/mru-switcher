# Session State
Updated: 2026-09-17T22:00:00Z
Human goal: M4 — Border UI + polish (usable visual feedback, no external processes)
Active milestone: M4; step M4-R0 (research border API on pin 0.56.2) IN PROGRESS
Branch: docs/m4-r0-border-api (off main 2dc750d)
PR: none yet (R0 memo will open one); v0.3.0 released (tag f9b5dee)
Blocked: none
Next action: research subagent -> memo docs/agent-state/research/2026-09-17-m4-border-api.md; then design gate (ADR-017 or design-note) before M4-S1
State: M3 complete/released; UIPort exists (on_session_start/selection_changed/session_end); only NullUI implemented; config `ui` = null|border|external, border/external fall back to null (ADR-011, REQ-UI-002); default null until CHANGELOG decision
SPEC focus: REQ-UI-001 (fail-soft), REQ-UI-002 (fallback null + warn-once), REQ-UI-003 (default MAY switch to border in M4)
Open: mechanism choice (public setprop/window-rule vs IHyprWindowDecoration); restore-prev-color strategy; active vs inactive border (cycle must NOT focus, REQ: no focus from cycle); minimal config keys (ui + optional border_color?); ABI risk
Last artifact: docs/agent-state/SESSION.md; research pending
