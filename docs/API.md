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

Prefix: `plugin:mru-switcher:`. All keys are registered in `PLUGIN_INIT` (ADR-008).

### Existing (unchanged semantics)

| Key | Type | Default | Notes |
|-----|------|---------|-------|
| `debounce_ms` | int | `400` | clamp per SPEC (REQ-CFG-004) |
| `default_scope` | enum | `global` | global \| monitor \| workspace \| visible \| app |
| `start_offset` | enum | `second` | first \| second |
| `wrap` | bool | `true` | |
| `lock_history_on_session` | bool | `true` | |
| `restore_focus_on_cancel` | bool | `false` | |
| `ui` | enum | `null` | null \| border \| external |
| `external_socket` | string | `""` | reserved until M5 |

### Added in M4

| Key | Type | Default | Notes |
|-----|------|---------|-------|
| `border_style` | string | `solid` | `solid` required; `pulse` / `dim` reserved → treated as `solid` until implemented; unknown → `solid` + one warning (REQ-UI-007) |
| `border_color` | color/string | `0xffffd9a0` | Used when `ui = border`. Documented implementation default (hex `0xAARRGGBB`); accepts formats supported by the pinned Hyprland (`rgb(...)` / `rgba(rrggbbaa)` / hex) — REQ-UI-008 |
| `border_size` | int | `-1` | `-1` = do not modify window border size; `≥ 0` may set the size for the highlighted window for the duration of the highlight |

### Reload

- `debounce_ms` — affects subsequent history commits.
- `ui`, `border_style`, `border_color`, `border_size`, and other session-policy keys — **next session only** (REQ-UI-009, REQ-CFG-002).

### Non-API notes

- The highlight is not a dispatcher; there is no `mru:highlight`.
- `mru:status` does not need to expose border state for 1.0; optional later.

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
