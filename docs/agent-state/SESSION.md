# Session State
Updated: 2026-09-24 (host autoload + autostart docs)
Human goal: plugin installed, loaded now, and loaded on every Hyprland start; document the path in README. РЕЛИЗ/ТЕГ ТОЛЬКО ПО ЯВНОЙ КОМАНДЕ.
Active milestone: M6 (release-prep); v1.0.0 tag strictly human-gated
Branch: main (50b7aa7) + PR in flight (`docs/dispatch-lua-note`)
PR: #111/#112/#113 merged; PR open (dispatch-on-Lua docs correction + state)
Blocked: none — `hyprpm enable mru-switcher` done by the human (state `enabled = true`, installed root:root 0644 by hyprpm)
Next action: idle. Autoload verified from a cold state (`hyprctl plugin unload` → `hyprpm reload -n` → `✔ Loaded mru-switcher`); the user's own check is after the next login/reboot (`hyprctl plugin list` should list `mru-switcher` with no manual command). Optional: pulse/dim nest smoke (recorded pending); then M6-T9 tag on explicit human command.
SPEC focus: REQ-DIST-018 (README as user doc), REQ-DIST-001/002 (hyprpm channel), T-DIST-01/02, REQ-CFG-005 (Lua sidecar — active: `border_style = dim`)
Open questions: (1) upstream Lua `plugin {}` support? (2) dim live visual indistinguishable from solid — paused by human ("Оставляем dim") (3) git author email stays in commit history as before (repo metadata carries none) (4) hyprpm cache dirs are now user-owned (`pkexec chown`, approved) while state files stay root-installed — upstream-intended split, revisit if a future `hyprpm` version expects otherwise
Last artifact: `docs/agent-state/reports/2026-09-24-hyprpm-autoload-and-autostart.md` + cold-start evidence: unload → `no plugins loaded` → `hyprpm reload -n` → `✔ Loaded mru-switcher`; `hyprpm list` → `enabled: true`; `hyprctl plugin list` → `mru-switcher 0.5.0` (2026-09-24)
