# Session State
Updated: 2026-09-22
Human goal: correction stream delivered — Tab-release dropped as the Lua product recipe; explicit-apply (B1) default + modifier-hold poll (B2) reference shipped (ADR-023, PR #87). РЕЛИЗ ТОЛЬКО ПО ЯВНОЙ КОМАНДЕ
Active milestone: M6 (release-prep); v1.0.0 tag strictly human-gated (NO tag without explicit human command)
Branch: none (on main)
PR: #87 merged (squash f0a58a0, CI 8/8, review APPROVE)
Next action: idle — wait for human: (a) empirical bindrt nest check on efb5099, (b) manual nest gate (T-DIST-01/02/03 + fast-toggle #65 + bindrt leg), (c) explicit tag command
Blocked: none
State: ADR-023 on main: B1 default (examples/mru-switcher-bindings.lua), B2 poll reference (examples/mru-switcher-bindings-poll.lua), Omarchy unbind mandated, COMPAT bindrt downgraded to re-verify-before-1.0, SPEC §11 informative note only; host user config still on the human's live release-on-Tab setup — re-point to B1/poll at leisure
SPEC focus: REQ-DIST-016/017/018 (deliverability, non-normative bindings story); REQ-F-002 (apply) untouched; no normative REQ change in PR #87
Open questions: does hyprlang bindrt fire apply on Alt release on efb5099? (empirical nest check — human-gated, before 1.0)
Last artifact: https://github.com/code-warlord-dev/mru-switcher/pull/87