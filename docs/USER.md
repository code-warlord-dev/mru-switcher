# MRU Window Switcher — User Guide

Niri-style Alt+Tab for Hyprland: most-recently-used order, frozen list while tabbing, focus applied on modifier release.

---

## Quick start

### 1. Load the plugin

```conf
# hyprland.conf (or via hyprpm)
plugin = /path/to/mru-switcher.so
# or
exec-once = hyprpm reload -n
```

### 2. Recommended binds

```conf
# Cycle forward / backward
bind   = ALT, TAB,       mru:cycle, next
bind   = ALT SHIFT, TAB, mru:cycle, prev

# Apply selection when Alt is released
bindrt = ALT, ALT_L,     mru:apply
# bindrt = ALT, ALT_R,   mru:apply   # if you use right Alt

# Cancel
bind   = ALT, Escape,    mru:cancel
```

Lua equivalent (Hyprland Lua config):

```lua
hl.bind("ALT + TAB",        function() hl.dispatch("mru:cycle next") end)
hl.bind("ALT + SHIFT + TAB", function() hl.dispatch("mru:cycle prev") end)
hl.bind("ALT_L",            function() hl.dispatch("mru:apply") end, { release = true })
hl.bind("ALT + Escape",     function() hl.dispatch("mru:cancel") end)
```

### 3. Optional config

```conf
plugin {
    mru-switcher {
        debounce_ms             = 400
        default_scope           = global   # global | monitor | workspace | visible | app
        start_offset            = second   # first | second
        wrap                    = true
        ui                      = null     # null | border | external (border from M4; earlier falls back to null)
        lock_history_on_session = true
        restore_focus_on_cancel = false
    }
}
```

---

## Config reload

Settings are read once when the plugin loads. After editing the
`plugin:mru-switcher` block in `hyprland.conf`, run:

```bash
hyprctl reload
```

and the plugin re-reads its settings (via the `config.reloaded` event):

- `debounce_ms` — applies to the MRU updates that happen **after** the reload.
- Everything else (`default_scope`, `start_offset`, `wrap`,
  `lock_history_on_session`, `restore_focus_on_cancel`, `ui`) — applies to the
  **next** session you start.

A session that is already running is never changed mid-flight: its frozen
window list and selection policy stay exactly as they were when it started.
Finish or cancel it, then start a new session to use the updated settings.

---

## Behaviour

| Action | Result |
|--------|--------|
| First `Alt+Tab` | Opens a session, builds a **snapshot** of windows in MRU order, selects the second entry (previous window) by default |
| Further `Tab` / `Shift+Tab` | Moves selection inside the frozen snapshot only |
| Release `Alt` | Focuses the selected window and ends the session |
| `Escape` | Cancels the session without changing focus (unless `restore_focus_on_cancel` is set) |

While a session is active, intermediate focus changes (mouse, other keybinds) do **not** reorder the list (lock-in).

---

## Scopes

Pass a scope as the second argument to `mru:cycle`:

```conf
bind = ALT, TAB, mru:cycle, next global
bind = ALT, TAB, mru:cycle, next monitor
bind = ALT, TAB, mru:cycle, next workspace
bind = ALT, TAB, mru:cycle, next visible
bind = ALT, TAB, mru:cycle, next app
```

If omitted, `default_scope` from config is used.

| Scope | Meaning |
|-------|---------|
| `global` | All mapped windows |
| `monitor` | Windows on the current monitor |
| `workspace` | Windows on the current workspace |
| `visible` | Windows on currently visible workspaces |
| `app` | Windows with the same class as the active one |

When no window is focused, `monitor`, `workspace`, and `app` fall back to `global` behavior; `visible` still shows only currently visible workspaces. A scope token that is not one of the five above is rejected with an `unknown scope token` error and no session starts.

---

## Dispatchers

| Dispatcher | Arguments | Description |
|------------|-----------|-------------|
| `mru:cycle` | `[next\|prev] [scope]` | Start or advance session |
| `mru:apply` | — | Focus selection, end session |
| `mru:cancel` | — | End session without applying |
| `mru:status` | — | Report debug state (prints just `ok` via `hyprctl` on 0.56.x — see note below) |

Example:

```bash
hyprctl dispatch mru:cycle next
hyprctl dispatch mru:apply
```

On Hyprland 0.56.x, `hyprctl dispatch mru:status` prints only `ok`: the plugin
carries the status payload in a field `hyprctl` surfaces only on failure, so it
is not visible through IPC on this version. (Failure messages do surface, e.g.
`mru-switcher: not initialized`.) External tools already assert session state
indirectly:

```bash
hyprctl activewindow -j
```

---

## UI backends

| Value | Behaviour |
|-------|-----------|
| `null` | No visual feedback (logic only; useful for tests) |
| `border` | Temporarily highlight the selected window (from **M4**; before M4 falls back to null) |
| `external` | Drive an external overlay process via socket (**M5**; before M5 falls back to null) |

---

## FAQ

**Why doesn’t focus move on every Tab?**  
By design (Niri / classic Alt+Tab). Focus is applied only on release so intermediate workspaces and history stay clean.

**Order jumps when I tab quickly.**  
Ensure `lock_history_on_session = true` and that you are not mixing other focus binds that bypass the plugin during the session.

**Special workspaces / scratchpads.**  
A scratchpad window is switchable only while it is **shown**; when hidden it is
excluded from every scope (including `global`). Shown scratchpad windows behave
like normal windows and follow the selected scope.

**Multi-monitor.**  
Use `scope = monitor` for per-monitor switching, or `global` for a single list across all outputs.

**Plugin fails to load after Hyprland update.**  
Recompile against the new headers. The plugin aborts on hash mismatch to avoid crashes.

---

## Manual test checklist

- [ ] First Alt+Tab selects previous window
- [ ] Repeated Tab cycles frozen list without reordering
- [ ] Release Alt focuses selected window
- [ ] Escape cancels
- [ ] FFM / mouse focus during session does not change list order
- [ ] Closing the selected window prunes snapshot or ends session
- [ ] Scratchpad shown → switchable; hidden → excluded from every scope ([live nest §2–3](agent-state/reports/2026-09-17-m3-s3-nest-smoke.md))
- [ ] `monitor` / `workspace` / `visible` / `app` scopes filter correctly ([live nest §4–5](agent-state/reports/2026-09-17-m3-s3-nest-smoke.md))
- [ ] Config reload: new `debounce_ms` / `default_scope` apply to the **next** session only; the active session keeps its frozen policy ([T-CFG-02](../tests/domain/test_session_controller.cpp))
