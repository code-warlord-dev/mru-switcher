# Session State
Updated: 2026-09-24 (host autoload + autostart docs)
Human goal: plugin installed, loaded now, and loaded on every Hyprland start; document the path in README. РЕЛИЗ/ТЕГ ТОЛЬКО ПО ЯВНОЙ КОМАНДЕ.
Active milestone: M6 (release-prep); v1.0.0 tag strictly human-gated
Branch: main (2d9662e) + docs PR in flight (`docs/hyprpm-autoload`)
PR: #111/#112 merged; docs PR open (autoload/autostart)
Blocked: `hyprpm enable mru-switcher` needs the human's sudo password in a real terminal (hyprpm installs root-owned state via `sudo install -o 0 -g 0` and refuses to run as root; non-interactive sudo fails) → `hyprpm list` still shows `enabled: false`
Next action: human runs `hyprpm enable mru-switcher` (normal terminal) → then `hyprpm reload -n` → verify `hyprctl plugin list` shows mru-switcher. Everything else for the autoload goal is done: autostart hook in `~/.config/hypr/autostart.lua`, docs + COMPAT rows landed.
SPEC focus: REQ-DIST-018 (README as user doc), REQ-DIST-001/002 (hyprpm channel), T-DIST-01/02, REQ-CFG-005 (Lua sidecar — active: `border_style = dim`)
Open questions: (1) upstream Lua `plugin {}` support? (2) dim live visual indistinguishable from solid — paused by human ("Оставляем dim") (3) git author email stays in commit history as before (repo metadata carries none)
Last artifact: `docs/agent-state/reports/2026-09-24-hyprpm-autoload-and-autostart.md`; host evidence: journal `pam_unix(sudo:auth): conversation failed` for hyprpm's internal sudo; source `hyprpm/src/helpers/Sys.cpp` + `DataState.cpp:30,37`; live `hyprctl plugin list` shows mru-switcher 0.5.0 (manually loaded this session)
