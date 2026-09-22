# Session State
Updated: 2026-09-22
Human goal: live-host border highlight must work on Lua/Omarchy. РЕЛИЗ/ТЕГ ТОЛЬКО ПО ЯВНОЙ КОМАНДЕ.
Active milestone: M6 (release-prep); v1.0.0 tag strictly human-gated
Branch: main (27d9ead)
PR: #99 merged (squash 27d9ead) — Lua-host border fix + load diagnostic; CI 8/8 green, ctest 19/19
Blocked: none
Next action: human live-host swap — unload /var/cache/hyprpm/code_warlord/mru-switcher/mru-switcher.so (old build) → load ~/.local/src/mru-switcher/build/mru-switcher.so (build/mru-switcher.so now has the setprop fix). hyprpm enabled=false: reboot won't auto-reload. Then human verifies border highlight on Alt+Tab hold (expected color 0xffffd9a0).
SPEC focus: REQ-UI-002/003/011; REQ-CFG-002; ADR-017 §2 (adapter-private binding in COMPAT)
Open questions: (1) upstream Lua `plugin {}` support? (2) live swap + highlight verified by human?
Last artifact: PR #99 (merged 27d9ead) — hyprctl_border_prop_io.cpp set() via g_pKeybindManager->m_dispatchers["setprop"]; register_all optional<> diagnostic; CHANGELOG/USER/COMPAT rows; nest proof of 0xffffd9a0 write+restore