# Session State
Updated: 2026-09-24 (host autoload + autostart docs)
Human goal: plugin installed, loaded now, and loaded on every Hyprland start; document the path in README. РЕЛИЗ/ТЕГ ТОЛЬКО ПО ЯВНОЙ КОМАНДЕ.
Active milestone: M6 (release-prep); v1.0.0 tag strictly human-gated
Branch: main (d4b837b)
PR: #111/#112/#113/#114 all merged; none open
Blocked: none — `hyprpm enable mru-switcher` done by the human (state `enabled = true`, installed root:root 0644 by hyprpm)
Next action: idle. Autoload verified from a cold state (`hyprctl plugin unload` → `hyprpm reload -n` → `✔ Loaded mru-switcher`); the user's own check is after the next login/reboot (`hyprctl plugin list` should list `mru-switcher` with no manual command). Optional: pulse/dim nest smoke (recorded pending); then M6-T9 tag on explicit human command.
SPEC focus: REQ-DIST-018 (README as user doc), REQ-DIST-001/002 (hyprpm channel), T-DIST-01/02, REQ-CFG-005 (Lua sidecar — active: `border_style = dim`)
Open questions: (1) upstream Lua `plugin {}` support? (2) dim live visual indistinguishable from solid — paused by human ("Оставляем dim") (3) git author email stays in commit history as before (repo metadata carries none) (4) hyprpm cache dirs are now user-owned (`pkexec chown`, approved) while state files stay root-installed — upstream-intended split, revisit if a future `hyprpm` version expects otherwise
Last artifact: PR #114 squash d4b837b (dispatch-on-Lua docs correction + autoload state); CI 8/8; live `mru-switcher 0.5.0` loaded via `hyprpm reload -n`, `hyprpm list` → `enabled: true` (2026-09-24)
