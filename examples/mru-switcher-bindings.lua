-- MRU Switcher — recommended keybindings (Lua / Omarchy)
-- =====================================================================
--
-- The plugin never captures keys itself, so WITHOUT these binds Alt+Tab
-- does nothing. This fragment calls the plugin's Lua bridge (hl.plugin.mru.*),
-- which exists only when the plugin is loaded AND the config is Lua-based
-- (a hyprlang config silently has no hl.plugin table — use
-- examples/mru-switcher-bindings.conf there instead).
--
-- How to include it, from a Lua config (bindings.lua / hyprland.lua):
--
--   dofile(os.getenv("HOME") .. "/.config/hypr/mru-switcher-bindings.lua")
--
-- or just copy the hl.bind lines below into your bindings.lua and delete
-- this file. After adding the line run: hyprctl reload

-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
-- Omarchy ships its own ALT+TAB ("Focus on next window"). hl.bind ADDS a
-- duplicate instead of replacing, so unbind the defaults first, then bind.
hl.unbind("ALT + TAB")
hl.unbind("ALT + SHIFT + TAB")

-- Cycle forward / backward through the frozen MRU list on Tab press.
-- cycle() moves the *virtual* selection only; real focus does not move.
hl.bind("ALT + TAB",         function() return hl.plugin.mru.cycle("next") end)
hl.bind("ALT + SHIFT + TAB", function() return hl.plugin.mru.cycle("prev") end)

-- Apply the selection when Tab is released (Niri-style apply-on-release).
-- NOTE (ADR-022): release binds whose key is a MODIFIER — hyprlang
-- "bindrt = ALT, ALT_L, mru:apply" or Lua "ALT + ALT_L" with release=true —
-- do not fire on Hyprland efb5099's Lua keybind path. Tab is an ordinary key
-- and its release fires reliably, so Tab-release commits the selection.
-- Result: hold Alt -> tap Tab -> release Tab -> focus moves once.
hl.bind("ALT + TAB",         function() return hl.plugin.mru.apply() end, { release = true, description = "MRU apply (Alt+Tab release)" })
hl.bind("ALT + SHIFT + TAB", function() return hl.plugin.mru.apply() end, { release = true, description = "MRU apply prev (Alt+Shift+Tab release)" })

-- Cancel the session with no focus change.
hl.bind("ALT + Escape", function() hl.plugin.mru.cancel() end)

-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
-- Note: if a future Hyprland build makes modifier-key release binds fire
-- again, you can revert to the original apply-on-Alt-release form:
--
--   hl.bind("ALT + ALT_L", function() return hl.plugin.mru.apply() end, { release = true })
--   hl.bind("ALT + ALT_R", function() return hl.plugin.mru.apply() end, { release = true })
--
-- IMPORTANT: always write the modifier as a separate token ("ALT + ALT_L",
-- never a bare "ALT_L"). A bare "ALT_L" binds with modmask 0, which never
-- matches the release-time Alt modifier (effective mods = 8) and silently
-- does nothing — the exact failure this fragment exists to avoid.