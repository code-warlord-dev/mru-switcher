# Public API reference (user-facing)

Stable contract for **users and external tools**. Internal C++ ports are described in ARCHITECTURE.md / SPEC.md.

---

## Dispatchers

### `mru:cycle`

```text
mru:cycle [next|prev] [global|monitor|workspace|visible|app]
```

Starts a session or moves selection.

| Argument | Required | Values |
|----------|----------|--------|
| direction | optional (default **next**) | `next`, `prev` |
| scope | no | `global`, `monitor`, `workspace`, `visible`, `app` |

### `mru:apply`

```text
mru:apply
```

Focuses the selected window and ends the session.

### `mru:cancel`

```text
mru:cancel
```

Ends the session without applying selection (see `restore_focus_on_cancel`).

### `mru:status`

```text
mru:status
```

Debug/status string (format not frozen until 1.0).

---

## Configuration keys

Prefix: `plugin:mru-switcher:`

| Key | Type | Default |
|-----|------|---------|
| `debounce_ms` | int | 400 |
| `default_scope` | string | global |
| `start_offset` | string | second |
| `wrap` | bool | true |
| `ui` | string | null |
| `lock_history_on_session` | bool | true |
| `restore_focus_on_cancel` | bool | false |
| `external_socket` | string | (empty) |

---

## Example binds

```conf
bind   = ALT, TAB,       mru:cycle, next
bind   = ALT SHIFT, TAB, mru:cycle, prev
bindrt = ALT, ALT_L,     mru:apply
bind   = ALT, Escape,    mru:cancel
```

```bash
hyprctl dispatch mru:cycle next monitor
hyprctl dispatch mru:apply
```

---

## Compatibility

- Requires rebuild for each Hyprland ABI/header hash.
- Dispatcher names are namespaced under `mru:` to avoid collisions.
