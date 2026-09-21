# Modifier-hold / release-detection API on pin 0.56.2 (`efb5099`)

**Owner:** orchestrator
**Date:** 2026-09-21
**Relevant ADR:** ADR-022 (apply-on-release via Lua keybind; Tab-release recipe). This memo re-verifies
the host APIs the human proposes to codify instead: explicit-apply or a **modifier-hold poll**.
**Context:** ADR-022's Lua recipe commits apply on **Tab release** because `hl.bind(..., {release=true})`
on a modifier key never fired. The human rejects Tab-release-as-apply-on-release as a product model.
Before changing the model we verify what the pin actually exposes.

> **Primary source:** a local clone of the **exact pinned commit** `efb50993780079460b0cbed1363e2166a2de1d9f`
> (GitHub mirror `hyprwm/Hyprland`, tag `v0.56.2`; `git rev-parse HEAD` == `efb5099`). Q1–Q3 quotes are
> from that tree. Q4/Q5 quotes are from the Omarchy ecosystem at their live URLs, fetched today.

---

## Q1 — Does the Lua API expose `hl.is_key_down` on efb5099?

**Verdict — YES, it exists.** Registered into the `hl` table and implemented in this exact commit.

- Registration: `src/config/lua/bindings/LuaBindingsToplevel.cpp:552` —
  `Internal::setFn(L, "is_key_down", hlIsKeyDown);` (registered inside `registerToplevelBindings`, line 535).
  All `*Bindings` modules are attached to the `hl` global table by
  `src/config/lua/bindings/LuaBindingsRegistration.cpp:87-99` (`lua_setglobal(L, "hl")`).
- Implementation: `src/config/lua/bindings/LuaBindingsToplevel.cpp:417-459`.
  - Integer arg → validates `xkb_keycode_is_legal_x11/ext`, scans `g_pKeybindManager->m_pressedKeys`
    for `k.keycode` (`...Toplevel.cpp:421-441`).
  - String arg → `xkb_keysym_from_name(key.c_str(), XKB_KEYSYM_NO_FLAGS)` (so `"Alt_L"`, `"Alt_R"`,
    `"Super_L"`, `"Return"` etc. are legal), then scans `m_pressedKeys` for `k.keysym`
    (`...Toplevel.cpp:443-455`).
  - Returns a boolean; `Enter` gets a hint to use `"Return"` (`...Toplevel.cpp:447-450`).
  - Underlying state: `std::vector<SPressedKeyWithMods> m_pressedKeys`
    (`src/managers/KeybindManager.hpp:153`), maintained on every key press/release in
    `CKeybindManager::onKeyEvent` (`src/managers/KeybindManager.cpp:391-518`).
- Also on this pin (relevant to the poll model): the **raw key event stream is exposed to Lua** as
  `hl.on("input.keyboard.key", ...)` — `src/config/lua/LuaEventHandler.cpp:179-186` forwards
  `Event::bus()->m_events.input.keyboard.key`, `dispatch("input.keyboard.key", 3, …)` pushes
  `(keycode + 8, timeMs, state)`. Event name in `knownEvents()` at `LuaEventHandler.cpp:298`.
  This lets Lua do **event-driven** modifier-release detection, not just polling.

---

## Q2 — Does the Lua API expose a timer/scheduler (poll loop)?

**Verdict — YES. `hl.timer`** exists on this pin.

- Registration: `src/config/lua/bindings/LuaBindingsToplevel.cpp:539` —
  `Internal::setMgrFn(L, mgr, "timer", hlTimer);`
- Implementation: `hlTimer` at `LuaBindingsToplevel.cpp:466-529`. Accepts
  `hl.timer(callback, { timeout = <ms>, type = "repeat"|"oneshot" })`; wraps `CEventLoopTimer`
  (`makeShared<CEventLoopTimer>(timeoutMs, callback, nullptr)`), armed via
  `g_pEventLoopManager->addTimer(timer)` — runs on the **compositor main event loop**
  (`src/managers/eventLoop/EventLoopManager.cpp:143-163`), never a background thread (compliant with
  the no-background-thread rule). Returns a `HL.Timer` userdata object
  (`src/config/lua/objects/LuaTimer.cpp:101-108`) exposing `set_enabled(bool)`, `is_enabled()`,
  `set_timeout(ms)` (`LuaTimer.cpp:37-99`). Repeat timers re-arm themselves before the callback
  (`LuaBindingsToplevel.cpp:484-486`).

**Shippable poll recipe on the pin (community-proven, see Q4):**

```lua
local held = true
local t = hl.timer(function()
  if hl.is_key_down("Alt_L") or hl.is_key_down("Alt_R") then return end
  held = false; t:set_enabled(false); hl.plugin.mru.apply()
end, { timeout = 25, type = "repeat" })
```

---

## Q3 — In-process C++ plugin options on efb5099 (no hooks, no background threads)

### (a) Does `Event::bus()->m_events` carry a keyboard key event?

**Verdict — YES.** `src/event/EventBus.hpp:113-117`:

```cpp
struct {
    Cancellable<IKeyboard::SKeyEvent>        key;
    Event<SP<IKeyboard>, const std::string&> layout;
    Event<SP<CWLSurfaceResource>>            focus;
} keyboard;
```

- `SKeyEvent` (`src/devices/IKeyboard.hpp:35-40`): `timeMs`, `keycode`, `updateMods`,
  `WL_KEYBOARD_KEY_STATE state` (pressed/released).
- Emission happens in `CInputManager::onKeyboardKey` (`src/managers/input/InputManager.cpp:1625-1634`),
  and it is emitted **before** keybind resolution:
  `Event::bus()->m_events.input.keyboard.key.emit(event, info); if (info.cancelled) return;` then
  `g_pKeybindManager->onKeyEvent(...)`. So a plugin can observe **and optionally cancel** every key
  event ahead of `KeybindManager`.
- Reachable from a plugin without hooks: `PluginAPI.hpp` includes `../event/EventBus.hpp`
  (`src/plugins/PluginAPI.hpp:25`), and the skill's Event::bus pattern applies. A plugin can track
  Alt press/release itself in-process and commit on the Alt release event — fully event-driven,
  main-thread, no polling, no hooks.

### (b) Is `IHyprKeyListener` available on this tag?

**Verdict — NO.** `rg -ni "keylistener" src/` on the efb5099 tree returns **zero matches**. That
abstraction does not exist on the pinned revision and cannot be used.

### (c) What do `KeybindManager`/`InputManager` expose for modifier state?

**Verdict — the state exists, but is NOT in the public plugin API (and the keybind-matching path is
the thing that drops modifier releases — see "Root cause" below).**

- `CKeybindManager::getPressedKeys()`-equivalent: there is no accessor; the state is the public-ish
  member `std::vector<SPressedKeyWithMods> m_pressedKeys` (`KeybindManager.hpp:153`,
  `SPressedKeyWithMods` at `KeybindManager.hpp:84-91`, with `keysym`, `keycode`,
  `modmaskAtPressTime`, `sent`, …).
- `CInputManager::getModsFromAllKBs()` — public member (declared `InputManager.hpp:192`, between the
  `public:` at line 87 and `private:` at line 219); returns the cached `m_lastMods`
  (`InputManager.cpp:1887-1889`), refreshed on IKeyboard modifier events (`InputManager.cpp:1701,1711`).
- **However:** `g_pKeybindManager`/`g_pInputManager` are `inline UP<…>` globals in internal headers
  (`KeybindManager.hpp:191`, `InputManager.hpp:313`). Nothing in `PluginAPI.hpp` exposes them; the
  build has no `-rdynamic`/export machinery for internal compositor symbols (`CMakeLists.txt` only
  carries an LTO-vs-plugins comment). Contract-wise they are **out of the public API** — using them
  means internal headers + loading hacks, against this repo's single-FocusGateway / no-internals
  policy and the skill's "prefer public HyprlandAPI and Event::bus()".
- Modmask constants for self-tracking: `eKeyboardModifiers` (`IKeyboard.hpp:13-22`,
  `HL_MODIFIER_ALT = (1 << 3)`).

### Root cause of the ADR-022 caveat (now pinned to source)

The reason `hl.bind("ALT + ALT_L", …, {release=true})` never fires is structural, in
`CKeybindManager::handleKeybinds` (`src/managers/KeybindManager.cpp`):

1. On a key release, `CKeybindManager::onKeyEvent` snapshots **current** mods
   `const auto MODS = g_pInputManager->getModsFromAllKBs();` (`KeybindManager.cpp:376`) and matches
   binds against that **release-time** modmask first-pass: `if (!IGNORECONDITIONS && ((modmask != k->modmask && !k->ignoreMods) || …)) continue;` (`KeybindManager.cpp:652`).
2. For `ALT + ALT_L` (modmask 8), by the time the `Alt_L` release event is processed, `m_lastMods`
   has already dropped ALT → current modmask 0 ≠ 8 → the bind is filtered out before the release
   branch is even reached.
3. The release branch then additionally needs `key.modmaskAtPressTime == modmask`
   (`KeybindManager.cpp:737-746`) — again impossible for a sole-modifier release whose bit is gone.
4. Ordinary keys keep working because when *Tab* is released (modmask 8, Alt still held) the current
   modmask still equals the bind's 8.

This confirms and upgrades the ADR-022 hypothesis ("release-time effective-modmask computation
excluding a sole-modifier trigger"). It applies to **any release bind keyed on a modifier token** on
this pin — the plugin's native path and the Lua path both end in `handleKeybinds`, so the same
structural limitation applies to the hyprlang `bindrt = ALT, ALT_L` recipe on this pin
(COMPAT currently records it as "keeps working"; that row predates this source-level analysis and
deserves an empirical re-check before the 1.0 tag).

---

## Q4 — Community confirmation (Omarchy switchers)

### omarchy-altswitch (Pablo-Merino) — commits on Alt release via raw event stream

Repo: https://github.com/Pablo-Merino/omarchy-altswitch — main logic `altswitch.lua`.

- Commits on modifier release **by reading the raw `input.keyboard.key` event stream**, not a keybind:
  `altswitch.lua:139-152` (quote):
  > "Committing on ALT release cannot be a keybind. A release bind on a modifier only fires when that
  > modifier is tapped on its own; pressing TAB in between cancels it, which is exactly what every
  > switch does. So the raw key stream is read instead, where the release always shows up."
  ```lua
  local ALT_KEYCODES = { [64] = true, [108] = true }
  hl.on("input.keyboard.key", function(keycode, _, state)
    if state == 0 and altswitch.active and ALT_KEYCODES[keycode] then
      altswitch_commit()
    end
  end)
  ```
  (64 = Alt_L, 108 = Alt_R — matches the +8 keycode convention from `LuaEventHandler.cpp:183`.)
- Its README (https://github.com/Pablo-Merino/omarchy-altswitch) states the same host fact verbatim:
  > "Committing on `ALT` release cannot be a keybind. A release bind on a modifier only fires when
  > that modifier is tapped alone; pressing `TAB` in between cancels it. The raw `input.keyboard.key`
  > event stream is read instead."

### omarchy-workspace-switcher (Woogy7) — commits on modifier release via `hl.is_key_down` + `hl.timer` poll

Repo: https://github.com/Woogy7/omarchy-workspace-switcher — `bindings.example.lua` (quoted verbatim):

```lua
local function switcher_watch_release()
  ...
  switcher.timer = hl.timer(function()
    ...
    local down = false
    for _, k in ipairs(hold_keys) do if hl.is_key_down(k) then down = true end end
    if not down then
      ...
      switcher_send("commit")
    end
  end, { timeout = 25, type = "repeat" })
end
```

- README (https://github.com/Woogy7/omarchy-workspace-switcher):
  > "**release `Alt` to switch**; `Esc` cancels … The modifier release is detected compositor-side
  > (`hl.is_key_down` polled by an `hl.timer`), so there is no client-side guessing."
  > Requirements: "Omarchy 4.x (omarchy-shell), Hyprland with the Lua config".

Both are live, shipping Lua-based implementations of **exactly** the two mechanisms being proposed
(modifier-hold poll via `hl.timer`+`hl.is_key_down`; event-driven alt-release via
`hl.on("input.keyboard.key")`).

### Canonical bug ticket(s) for "release bind on a modifier never fires"

There is **no single upstream ticket** that documents the exact Lua/`ALT_L` failure; evidence is the
repo's own bisect (2026-09-20) + the two Omarchy authors' READMEs above. The canonical Hyprland issue
family on the surrounding behavior:

- hyprwm/Hyprland **#7948** (2024-09): `bindr` won't trigger if SUPER is held during release
  (https://github.com/hyprwm/Hyprland/issues/7948).
- hyprwm/Hyprland **#5292** (2024-03): release binds don't work in submaps
  (https://github.com/hyprwm/Hyprland/issues/5292); fixed by PR #6025.
- hyprwm/Hyprland **discussion #12555**: Super+Tab with Super released before Tab fires the Super-only
  release bind (rofi opens) (https://github.com/hyprwm/Hyprland/discussions/12555).
- hyprwm/Hyprland **discussion #15837** (2026-08): 0.56 regression — modifier-only release bind **fires
  after** other SUPER binds (the inverse symptom, showing how churny this area is on 0.56)
  (https://github.com/hyprwm/Hyprland/discussions/15837; refs PR #15568).

Note the tension: #15837 reports modifier-only release binds *firing too much* on 0.56, while the repo's
empirical matrix on the same 0.56 pin shows `ALT + ALT_R` *never firing*. The two resolve consistently
once the source is read: `KEY`-gesture release matches run against the *live* current modmask
(`KeybindManager.cpp:652`), while "special" handlers (global/pass/sendshortcut — `SPECIALDISPATCHER`,
the `IGNORECONDITIONS` path at `KeybindManager.cpp:643-647,650`) deliberately **ignore** modmatching on
release. #15837's case goes through a `global`-style executor; the plain `mru:apply`-style binding does
not — which is why the plugin's Lua recipe observed silence.

---

## Q5 — Stock Omarchy default Alt+Tab binds (cycle_next + bring_to_top)

**Verdict — confirmed, primary source.** Omarchy repo is `omacom/omarchy` (GitHub; basecamp/omarchy
redirects), default branch **`quattro`**, file `default/hypr/bindings/tiling.lua`:

```lua
47:o.bind("ALT + TAB", "Focus on next window", hl.dsp.window.cycle_next())
48:o.bind("ALT + SHIFT + TAB", "Focus on previous window", hl.dsp.window.cycle_next({ next = false }))
49:o.bind("ALT + TAB", "Reveal active window on top", hl.dsp.window.bring_to_top())
50:o.bind("ALT + SHIFT + TAB", "Reveal active window on top", hl.dsp.window.bring_to_top())
```

Source: https://github.com/omacom/omarchy/blob/quattro/default/hypr/bindings/tiling.lua#L47-L50
(raw: `https://raw.githubusercontent.com/omacom/omarchy/quattro/default/hypr/bindings/tiling.lua`).

This matches omarchy-altswitch's own statement (`altswitch.lua:131-132`) —
> "Omarchy binds ALT+TAB four times by default (cyclenext and bring_to_top, in both directions)",
and its `hl.unbind("ALT + TAB")` / `hl.unbind("ALT + SHIFT + TAB")` before rebinding.

---

## What can / cannot ship as-is on efb5099

| Option | On pin | Route (primary source) |
|--------|--------|------------------------|
| Explicit-apply (`mru:apply` bound to a key / dispatcher) | **Ship as-is** | Already SPEC REQ-F-002; no host dependency (ADR-022 matrix). |
| Lua **modifier-hold poll** (apply on Alt release) | **Ship as-is** | `hl.is_key_down` (`LuaBindingsToplevel.cpp:552,417`) + `hl.timer` repeat (`:539,466`) — community-proven (Woogy7). |
| Lua **event-driven** Alt release (apply on release of keycode 64/108) | **Ship as-is** | `hl.on("input.keyboard.key")` (`LuaEventHandler.cpp:179-186`) — community-proven (omarchy-altswitch). |
| Native plugin Alt-release/held detection | **Ship as-is** | `Event::bus()->m_events.input.keyboard.key` (`EventBus.hpp:114`; emitted before keybinds, `InputManager.cpp:1625-1634`) — no hooks, main-thread, event-driven. |
| `hl.bind("ALT + ALT_L", …, {release=true})` (Lua) / `bindrt = ALT, ALT_L` (hyprlang) on this pin | **Do NOT ship / re-verify** | Dies in `handleKeybinds` current-mods matching (`KeybindManager.cpp:652, 737-746`); hyprlang row in COMPAT "keeps working" pre-dates this analysis and needs an empirical re-check. |
| `IHyprKeyListener` | **Cannot ship** | Absent from the efb5099 tree (`rg → 0`). |
| Plugin reads `g_pKeybindManager->m_pressedKeys` / `g_pInputManager->getModsFromAllKBs()` | **Out of contract** | Internal inline globals (`KeybindManager.hpp:191`, `InputManager.hpp:313`); not in `PluginAPI.hpp`; internal-symbol export not set up. |
| Background-thread polling of compositor internals | **Never** | Single-threaded event loop; `hl.timer`/`CEventLoopTimer` already run on the main loop. |

**Bottom line for the product model:** the "modifier-hold poll" is fully codifiable on this pin — in
fact better, both `hl.timer`+`hl.is_key_down` (poll) and `hl.on("input.keyboard.key")`
(event-driven) are shipping in production Omarchy plugins today, and the native plugin can do it
without polling at all via `Event::bus()` key events. ADR-022's Tab-release device was a workaround
for a broken **keybind** path only; the hold/release-detection mechanisms it needs are all live.
No new ADR-driven API assumption is required, but ADR-022's rationale section should be updated to
cite this memo if the recipe changes back to Alt-release.

## Files

- Hyprland efb5099 clone (shallow, tag v0.56.2): `/tmp/opencode/hyprland`
- Omarchy default binds: `omacom/omarchy@quattro`, `default/hypr/bindings/tiling.lua`
- omarchy-altswitch: `Pablo-Merino/omarchy-altswitch@main`, `altswitch.lua`
- omarchy-workspace-switcher: `Woogy7/omarchy-workspace-switcher@main`, `bindings.example.lua`