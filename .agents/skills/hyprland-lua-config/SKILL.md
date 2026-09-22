---
name: hyprland-lua-config
description: Expert Hyprland Lua configuration, keybinds (hl.bind/hl.unbind), hyprctl, hyprpm plugin management, and Omarchy integration. Use when writing or debugging Lua configs (hyprland.lua, bindings.lua), installing plugins via hyprpm, runtime control with hyprctl, adapting for Omarchy or pure Hyprland, or handling modifier-release binds and Lua API quirks.
metadata:
  level: expert
  version: "1.0"
  domain: hyprland
---

# Hyprland Lua Config + Omarchy + hyprpm

## Overview

Configure and operate Hyprland using its modern **Lua** config surface (`hl.*` API), manage plugins with **hyprpm**, control runtime state via **hyprctl**, and correctly integrate with **Omarchy** (opinionated Arch + Hyprland distro that ships Lua configs and its own bind layer).

This skill covers both pure Hyprland installs and Omarchy. Prefer Lua over legacy hyprlang whenever the host supports it.

## When to use

- Writing or editing `hyprland.lua`, `bindings.lua`, `monitors.lua`, etc.
- Keybinds with `hl.bind` / `hl.unbind` / `o.bind` (Omarchy)
- Installing / enabling plugins via hyprpm or manual `hyprctl plugin load`
- Runtime inspection (`hyprctl clients`, `hyprctl plugin list`, `hyprctl reload`)
- Omarchy-specific unbind/rebind of stock Alt+Tab and other defaults
- Modifier-release / apply-on-release patterns that fail on Lua path
- Adapting a plugin install (e.g. mru-switcher) for both backends

## When not to use

- Pure C++ plugin internals (use hyprland-plugin skill)
- Non-Hyprland compositors
- Legacy hyprlang-only configs when Lua is available

## Non-negotiables

1. **Lua is the preferred config language** on modern Hyprland. Main entry is `~/.config/hypr/hyprland.lua`.
2. **Key strings must match exactly** (case-sensitive). `"SUPER + TAB"` ≠ `"SUPER + Tab"`.
3. **hl.bind ADDS**; it does not replace. Always `hl.unbind` (or Omarchy `o.rebind`) before rebinding a key that already exists.
4. **Modifier-release binds** (`{ release = true }` on a modifier key token) are unreliable on the Lua keybind path of many pinned builds. Prefer explicit apply key or a short `hl.timer` + `hl.is_key_down` poll when true "release Alt to commit" is required.
5. **hyprpm** is the preferred plugin distribution channel. Source + `hyprctl plugin load` is fallback.
6. **Omarchy** loads its defaults first; user files under `~/.config/hypr/` override. Stock Alt+Tab must be unbound before adding a custom cycle.

## Config layout

### Pure Hyprland

```
~/.config/hypr/
├── hyprland.lua          # entry — require / dofile modules
├── bindings.lua          # keybinds
├── monitors.lua
├── input.lua
└── conf.d/               # optional drop-ins
```

Load modules with `require("bindings")` or `dofile(os.getenv("HOME") .. "/.config/hypr/bindings.lua")`.

### Omarchy

Omarchy ships a structured tree and bootstrap:

```lua
-- typical hyprland.lua (Omarchy)
dofile((os.getenv("OMARCHY_PATH") or "/usr/share/omarchy") .. "/default/hypr/bootstrap.lua")
require("default.hypr.omarchy")
require("hypr.monitors")
require("hypr.input")
require("hypr.bindings")
require("hypr.looknfeel")
require("hypr.autostart")
```

User overrides live in the same files under `~/.config/hypr/`. Defaults load first; later requires win.

Omarchy also exposes `o.bind` / `o.rebind` helpers that wrap `hl.bind` and integrate with the menu system. Prefer them inside Omarchy for discoverability; fall back to raw `hl.*` when needed.

## Keybinds — core patterns

```lua
-- Basic
hl.bind("SUPER + Q", hl.dsp.exec_cmd("kitty"))
hl.bind("SUPER + C", hl.dsp.window.close())

-- Lua function (conditional logic)
hl.bind("SUPER + X", function()
  local w = hl.get_active_window()
  if w and w.title == "htop" then
    hl.dispatch(hl.dsp.window.float({ action = "set" }))
  else
    hl.dispatch(hl.dsp.window.float({ action = "toggle" }))
  end
end)

-- Flags
hl.bind("SUPER + Shift + L", hl.dsp.exec_cmd("swaylock"), { locked = true })

-- Unbind (exact case match required)
hl.unbind("ALT + TAB")
hl.unbind("ALT + SHIFT + TAB")
```

Omarchy rebind (removes old + adds new, prints what was replaced):

```lua
o.rebind("SUPER + F", "File manager", { launch = "nautilus" })
```

### Omarchy Alt+Tab conflict (common)

Omarchy binds Alt+Tab to its own cycle. A new bind **adds** rather than replaces:

```lua
hl.unbind("ALT + TAB")
hl.unbind("ALT + SHIFT + TAB")
-- then bind your own
hl.bind("ALT + TAB", function() return hl.plugin.mru.cycle("next") end)
```

Always unbind first when replacing stock behaviour.

### Apply-on-release / modifier-release caveats

On many Hyprland builds (including the 0.56.x pin used by mru-switcher) a release bind whose key is a **modifier token** does not fire on the Lua path:

```lua
-- SILENTLY DOES NOTHING on the Lua path of many pins
hl.bind("ALT + ALT_L", function() ... end, { release = true })
```

Workarounds that work:

1. **Explicit apply key** (recommended default):
   ```lua
   hl.bind("ALT + Return", function() return hl.plugin.mru.apply() end)
   ```

2. **Poll with hl.timer + hl.is_key_down** (true "release Alt" feel):
   - Arm a 25 ms timer on first cycle
   - Poll `hl.is_key_down("Alt_L") or hl.is_key_down("Alt_R")`
   - On release call apply and disarm
   - Keep everything on the main event loop (no background threads)

3. Event-driven alternative if available: `hl.on("input.keyboard.key", ...)` watching keycodes 64/108 (Alt_L/Alt_R) with state == 0.

Never commit on Tab-release itself — that focuses on every step and destroys frozen-list semantics.

## hyprctl — runtime control

```bash
hyprctl reload                          # reload config
hyprctl clients                         # list windows
hyprctl activewindow
hyprctl plugin list
hyprctl plugin load /absolute/path/to/plugin.so
hyprctl plugin unload /absolute/path/to/plugin.so
hyprctl dispatch mru:cycle next         # call a plugin dispatcher
hyprctl getoption general:gaps_in
hyprctl keyword general:gaps_in 5       # temporary override
```

Always use absolute paths for `plugin load`. Relative paths and `~` expansion are unreliable.

## hyprpm — plugin management

Preferred one-command install once a release pin exists:

```bash
hyprpm add https://github.com/<owner>/<plugin>
hyprpm enable <plugin-name>
hyprpm reload
```

`hyprpm.toml` in the plugin repo must declare:

- `[repository]` with name + authors
- `commit_pins` mapping tested Hyprland SHA → plugin SHA
- `[plugin-name]` with description, authors, output path, build steps

Until pins are final, fall back to source install:

```bash
mkdir -p ~/.local/src
git clone <repo> ~/.local/src/<plugin>
cd ~/.local/src/<plugin>
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release ...
cmake --build build -j
hyprctl plugin load "$HOME/.local/src/<plugin>/build/<name>.so"
```

A Hyprland upgrade is a **rebuild event**. Never assume binary compatibility across commits.

## Plugin config drop-in pattern

After install, place plugin-specific config under the plugin namespace:

```lua
-- or hyprlang equivalent under plugin:<name>:
-- keys registered only in PLUGIN_INIT
```

Example files for both backends should be shipped by the plugin (`.conf` for hyprlang, `.lua` for Lua/Omarchy). Document the exact include line:

```lua
dofile(os.getenv("HOME") .. "/.config/hypr/mru-switcher-bindings.lua")
```

## Omarchy vs pure Hyprland checklist

| Concern              | Pure Hyprland                  | Omarchy                                      |
|----------------------|--------------------------------|----------------------------------------------|
| Config language      | Lua (preferred) or hyprlang    | Lua only (structured requires)               |
| Stock Alt+Tab        | usually none / user-defined    | present — must unbind first                  |
| Bind helpers         | `hl.bind` / `hl.unbind`        | `o.bind` / `o.rebind` + raw `hl.*`           |
| Plugin load          | hyprpm or hyprctl              | same + Omarchy plugin marketplace if present |
| Reload               | `hyprctl reload`               | same                                         |
| Config entry         | `hyprland.lua`                 | bootstrap + require chain                    |

## Review / smoke checklist

- [ ] Exact case match on every unbind
- [ ] Stock binds unbound before rebinding on Omarchy
- [ ] No reliance on modifier-release binds on Lua path without a verified poll/event fallback
- [ ] Plugin loaded with absolute path; appears in `hyprctl plugin list`
- [ ] Dispatchers respond (`hyprctl dispatch ...`)
- [ ] Config reload does not leave duplicate binds
- [ ] hyprpm commit_pins match the running Hyprland revision

## References

- Hyprland wiki — Configuring / Binds / Lua utilities
- Omarchy default Hyprland tree (`/usr/share/omarchy/default/hypr/`)
- Project-specific examples (e.g. mru-switcher `examples/*.lua` / `*.conf`)
- hyprpm documentation and plugin `hyprpm.toml` contract
