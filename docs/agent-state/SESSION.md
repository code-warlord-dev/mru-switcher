# Session State
Updated: 2026-09-22
Human goal: facade split (PR #92) + sidecar config fix for Lua hosts (PR #94) delivered per human brief. РЕЛИЗ ТОЛЬКО ПО ЯВНОЙ КОМАНДЕ
Active milestone: M6 (release-prep); v1.0.0 tag strictly human-gated (NO tag without explicit human command)
Branch: main (8148ea2)
PR: #92 (facade split) + #94 (ADR-024 sidecar) merged; no open PRs
Blocked: none
Next action: idle — wait for human: (a) live-host sidecar verification (copy examples/mru-switcher-sidecar.conf → ~/.config/mru-switcher/config + reload → border highlight), (b) upstream Lua plugin-{} research, (c) empirical bindrt nest check, (d) M6-T9 tag on explicit command
State: mru_plugin.cpp 516→74 lines (INIT/EXIT only); sidecar overlay live on main (REQ-CFG-005/ADR-024). Live host still on hyprpm-cached .so — needs hyprpm rebuild/reload to pick up sidecar.
SPEC focus: REQ-CFG-005 (sidecar overlay); REQ-DISP-001/002/003, REQ-CFG-001/002/004
Open questions: (1) upstream Lua `plugin {}` support? (2) sidecar verified live on Omarchy host?
Last artifact: PR #94 (squash 8148ea2) — sidecar_config + T-CFG-07 + ADR-024 + COMPAT/USER honesty