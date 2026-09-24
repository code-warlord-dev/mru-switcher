# Session State
Updated: 2026-09-24 (v1.0.0 tagged + released)
Human goal: v1.0.0 shipped (Variant A single-pin). Next: host reboot check + optional pulse/dim by-eye.
Active milestone: M6 DONE — v1.0.0 released
Branch: main (aee883d)
PR: #117 (prep) + #118 (pin + nest report) merged; none open
Blocked: none
Next action: idle. Host still runs cached 0.5.0 .so (hyprpm update rebuilt but `[ERR] removePluginRepo` left stale cache; reload reloaded old .so) — needs terminal `hyprpm update` retry or reboot; after that `hyprctl plugin list` should show 1.0.0. Pulse/dim visual by-eye still pending (headless nest).
SPEC focus: M6-T9 close-out
Open questions: (1) upstream Lua `plugin {}` support? (2) dim live visual by-eye (3) hyprpm removePluginRepo ERR on update — retry from terminal
Last artifact: tag v1.0.0 on aee883d + GitHub Release https://github.com/code-warlord-dev/mru-switcher/releases/tag/v1.0.0 (2026-09-24)
