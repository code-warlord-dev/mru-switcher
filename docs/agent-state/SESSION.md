# Session State
Updated: 2026-09-22
Human goal: live-host border highlight must work on Lua/Omarchy. РЕЛИЗ/ТЕГ ТОЛЬКО ПО ЯВНОЙ КОМАНДЕ; PR → main обязателен (запрос человека).
Active milestone: M6 (release-prep); v1.0.0 tag strictly human-gated
Branch: fix/lua-host-border-and-ui-default (rebase planned on main b3ff7ca #96-#98)
PR: #99 (to be updated after rebase; old base 20de82c not mergeable)
Blocked: none
Next action: PR #99 must be rebuilt onto main b3ff7ca — drop duplicated ADR-025/default/docs (already in main via #97), keep unique delta: (1) HyprctlBorderPropIo::set → in-process g_pKeybindManager->m_dispatchers["setprop"] (Lua dispatch shim broke unquoted setprop; nest-verified: mru:cycle writes 0xffffd9a0, cancel restores); (2) register_all returns optional reason + unload-first hint (Lua name-collision on re-load). Docs: CHANGELOG Fixed x2, USER FAQ x2, COMPAT rows x2 + row 49 + M4 mechanism. Then human live-host swap: unload /var/cache/hyprpm/code_warlord/mru-switcher/mru-switcher.so → load ~/.local/src/mru-switcher/build/mru-switcher.so.
SPEC focus: REQ-UI-002/003/011; REQ-CFG-002/005; ADR-024/025 (on main)
Open questions: (1) upstream Lua `plugin {}` support? (2) live swap done by human on Omarchy host?
Last artifact: nest verification of border fix (nest3.log); ctest green at 20de82c; PR #99 opened then superseded by #96-#98 landing