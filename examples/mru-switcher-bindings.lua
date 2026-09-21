-- MRU Switcher — recommended keybindings (Lua / Omarchy)
-- =====================================================================
--
-- The plugin never captures keys itself, so WITHOUT these binds Alt+Tab
-- does nothing. This fragment calls the plugin's Lua bridge (hl.plugin.mru.*),
-- which exists only when the plugin is loaded AND the config is Lua-based
-- (a hyprlang config silently has no hl.plugin table — use
-- examples/mru-switcher-bindings.conf there instead).
--
-- Two supported apply models — pick exactly one:
--
--   B1 — explicit apply key (THIS FILE, the default). Cycle with Alt+Tab,
--        commit with Alt+Return. Nothing moves until you say so: hold Alt ->
--        tap Tab as often as you like -> Return commits -> release Alt.
--
--   B2 — logical Alt release via a poll
--        (examples/mru-switcher-bindings-poll.lua). Identical cycling, but a
--        25 ms hl.timer watches hl.is_key_down("Alt_L"/"Alt_R") and commits
--        the moment you let go of Alt — the classic Niri-style feel.
--
-- Do NOT reach for apply-on-Tab-release. It commits a real focus move on
-- every Tab press, reshuffles the stack under you, and destroys the
-- frozen-list browse-then-commit workflow (ADR-001/ADR-003). It was only
-- ever a degraded host workaround, never a product model — see ADR-023.
--
-- The workaround existed because a release bind whose key is a MODIFIER token
-- never fires on the Lua keybind path of the pinned Hyprland (v0.56.2 /
-- commit efb5099): hl.bind("ALT + ALT_L", ..., { release = true }) silently
-- does nothing there. That is exactly why B2 needs a poll (hl.timer +
-- hl.is_key_down — both live on the pin) instead of a release bind.
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

-- B1: commit the selection with an explicit key. Alt+Return is the default
-- choice here; ALT + Space is an equally valid one. Check your own config
-- first: if Alt+Return (or Alt+Space) is already bound, hl.bind ADDS a
-- duplicate and both would fire — hl.unbind it, or pick the other key.
hl.bind("ALT + Return",      function() return hl.plugin.mru.apply() end)

-- Cancel the session with no focus change.
hl.bind("ALT + Escape", function() hl.plugin.mru.cancel() end)

-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
-- Want "release Alt to apply" instead of a commit key? Load
-- examples/mru-switcher-bindings-poll.lua — same unbind + cycle binds, plus
-- the 25 ms hl.timer poll. Load one file or the other, never both: the bind
-- set is identical, so you would just bind Alt+Tab twice.
--
-- On a hyprlang config use examples/mru-switcher-bindings.conf instead.
-- Host evidence for the caveat above:
-- docs/agent-state/research/2026-09-21-modifier-hold-poll-api.md.
--
-- IMPORTANT: always write the modifier as a separate token ("ALT + ALT_L",
-- never a bare "ALT_L"). A bare "ALT_L" binds with modmask 0, which never
-- matches the release-time Alt modifier (effective mods = 8) and silently
-- does nothing — the exact failure this fragment exists to avoid.