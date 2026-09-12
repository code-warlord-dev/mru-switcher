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

---

## Dispatchers

| Dispatcher | Arguments | Description |
|------------|-----------|-------------|
| `mru:cycle` | `[next\|prev] [scope]` | Start or advance session |
| `mru:apply` | — | Focus selection, end session |
| `mru:cancel` | — | End session without applying |
| `mru:status` | — | Print debug info (via hyprctl) |

Example:

```bash
hyprctl dispatch mru:cycle next
hyprctl dispatch mru:apply
hyprctl dispatch mru:status
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
Included when they match the active scope. Use `workspace` or `visible` if you want to limit them.

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
- [ ] `monitor` / `workspace` / `app` scopes filter correctly
- [ ] Config reload picks up new `debounce_ms` / `default_scope`
