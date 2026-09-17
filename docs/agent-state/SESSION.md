# Session State
Updated: 2026-09-17
Human goal: M4 — Border UI + polish (usable visual feedback, no external processes)
Active milestone: M4
Branch: adr/017-border-highlight-ui
PR: docs-only ADR-017/SPEC PR (adr/017-border-highlight-ui); self-review (plugin-spec-compliance) then human approval to merge (adr/* gate, AGENTS §6.4)
Blocked: none
Next action: self-review ADR-017 + SPEC diff (plugin-spec-compliance) -> human approval to merge the adr/* PR -> then M4-S1 implementer brief (BorderHighlightUI, BorderStyle `solid`, keys border_style/border_color/border_size, tests T-UI-03..07)
State: M3 complete/released (v0.3.0). M4-R0 DONE (0.56.2 pin efb5099, memo 2026-09-17-m4-border-api.md). M4-D1 IN REVIEW — ADR-017 (Accepted) + SPEC REQ-UI-001..011 + config keys integrated on branch adr/017-border-highlight-ui (not yet merged). Mechanism = public props (`setprop active/inactive_border_color` via invokeHyprctlCommand), restore-by-value, no public unset; default `ui=null` (ADR-011/017)
SPEC focus: REQ-UI-001..011; config keys border_style (solid), border_color (0xffffd9a0), border_size (-1); REQ-S-009 / REQ-CFG-002 next-session-only reload
Open: none blocking; nest must verify restore-by-value vs `-1` empty-gradient during M4-S3 (R0 open question)
Last artifact: docs/DECISIONS.md ADR-017 (Accepted) + docs/SPEC.md §5 REQ-UI-001..011
