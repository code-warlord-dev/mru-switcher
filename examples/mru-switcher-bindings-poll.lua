-- MRU Switcher — B2 reference: "release Alt to apply" via a poll (Lua / Omarchy)
-- =====================================================================
--
-- Copy-paste alternative to examples/mru-switcher-bindings.lua: same unbind
-- and same cycle binds, but the selection is committed when you let go of
-- Alt. Load ONE of the two files, never both (the bind set is identical).
--
-- Why a poll and not a release bind? A release bind whose key is a MODIFIER
-- token does not fire on the Lua keybind path of the pinned Hyprland
-- (v0.56.2 / commit efb5099): hl.bind("ALT + ALT_L", ..., { release = true })
-- silently does nothing there, because keybinds are matched against the
-- *release-time* modmask. hl.timer + hl.is_key_down are live on the pin, so
-- the poll is the supported Lua route to a logical "Alt released" event.
--
-- Why the poll is cheap and safe:
--   * hl.timer runs on the compositor's main event loop (CEventLoopTimer) —
--     never a background thread, so nothing races the compositor. Compliant
--     with the project's no-background-thread rule.
--   * hl.is_key_down("Alt_L"/"Alt_R") reads the compositor's own pressed-key
--     state, not a guess.
--   * one repeat timer at 25 ms, armed only between the first Tab press and
--     the Alt release (or cancel, or the next Tab press), and disarmed
--     BEFORE apply() — so apply runs exactly once and never after cancel.
--
-- Alternative community route (event-driven, no poll) — watch the raw key
-- events instead and commit on the release of either Alt:
--
--   hl.on("input.keyboard.key", function(keycode, timeMs, state) ... end)
--
-- keycode 64 = Alt_L, 108 = Alt_R (the Lua event carries xkb keycodes, i.e.
-- evdev + 8) and state == 0 means "released". Same result, no timer loop;
-- use whichever your Hyprland build exposes.
--
-- How to include it, from a Lua config (bindings.lua / hyprland.lua):
--
--   dofile(os.getenv("HOME") .. "/.config/hypr/mru-switcher-bindings-poll.lua")
--
-- or just copy the code below into your bindings.lua and delete this file.
-- After adding the line run: hyprctl reload

-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
-- All state is local to this file: nothing leaks into _G beyond the bind
-- callbacks the compositor keeps for us.
local watcher = nil   -- the single hl.timer; created once, only enabled while armed
local armed = false   -- true from a Tab cycle until Alt is released / we cancel

local function alt_down()
  return hl.is_key_down("Alt_L") or hl.is_key_down("Alt_R")
end

-- Stop watching without applying (used by cancel, and re-armed by the next Tab).
local function disarm()
  armed = false
  if watcher then watcher:set_enabled(false) end
end

-- Lazily create the one repeat timer. The callback never assumes the timer is
-- disabled for correctness: `armed` is the single source of truth and it is
-- cleared BEFORE apply(), so even a stray extra fire is a no-op and can never
-- apply twice.
local function ensure_watcher()
  if watcher then return watcher end
  watcher = hl.timer(function()
    if not armed then
      watcher:set_enabled(false)  -- idle: stay quiet until the next cycle
      return
    end
    if alt_down() then return end  -- Alt still held: keep waiting
    armed = false                  -- exactly once: no later fire can apply
    watcher:set_enabled(false)     -- clear before apply(), never after
    hl.plugin.mru.apply()          -- logical "Alt released" -> commit selection
  end, { timeout = 25, type = "repeat" })
  return watcher
end

-- Arm (or re-arm, on every Tab press) the release watcher.
local function arm()
  local t = ensure_watcher()
  armed = false        -- clear first: a fire during arming must not apply
  t:set_enabled(true)  -- (re)start the 25 ms window
  armed = true
end

-- Cycle the virtual selection, then wait for Alt to come up.
local function cycle_and_arm(dir)
  local result = hl.plugin.mru.cycle(dir)  -- raises on failure, moves no real focus
  arm()
  return result
end

-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
-- Omarchy ships its own ALT+TAB ("Focus on next window"). hl.bind ADDS a
-- duplicate instead of replacing, so unbind the defaults first, then bind.
hl.unbind("ALT + TAB")
hl.unbind("ALT + SHIFT + TAB")

hl.bind("ALT + TAB",         function() return cycle_and_arm("next") end)
hl.bind("ALT + SHIFT + TAB", function() return cycle_and_arm("prev") end)

-- Cancel with no focus change: drop the watcher FIRST, so Alt coming up (or
-- arriving late) can never race a cancel into an apply.
hl.bind("ALT + Escape", function()
  disarm()
  return hl.plugin.mru.cancel()
end)

-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
-- Prefer an explicit commit key instead of holding-and-releasing Alt? Use
-- examples/mru-switcher-bindings.lua (B1: hl.bind("ALT + Return",
-- hl.plugin.mru.apply)). On a hyprlang config use
-- examples/mru-switcher-bindings.conf.
--
-- IMPORTANT: always write a modifier as a separate token ("ALT + ALT_L",
-- never a bare "ALT_L" — a bare token binds with modmask 0 and silently never
-- matches). This file has no release bind at all, which is the point.
