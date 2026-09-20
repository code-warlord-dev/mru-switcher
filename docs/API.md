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

Debug/status string. Payload format is **frozen in `docs/SPEC.md` §3.4** (stable for 1.x):
`active=… index=… size=… scope=… session=… last_end=…`; additional keys may appear in minor releases
(tolerate them). On Hyprland 0.56.x `hyprctl dispatch` shows only `ok` — see `docs/COMPAT.md`.

---

## Configuration keys

Prefix: `plugin:mru-switcher:`. All keys are registered in `PLUGIN_INIT`.

### Existing (unchanged semantics)

| Key | Type | Default | Notes |
|-----|------|---------|-------|
| `debounce_ms` | int | `400` | clamped to `[0, 5000]` ms per SPEC |
| `default_scope` | enum | `global` | global \| monitor \| workspace \| visible \| app |
| `start_offset` | enum | `second` | first \| second |
| `wrap` | bool | `true` | |
| `lock_history_on_session` | bool | `true` | **Reserved / ignored**: locking the window list while a session is active is mandatory. The key stays registered so 0.x configs keep parsing; a `false` value only triggers one warning notification per plugin lifetime. Removal is a 2.0 candidate |
| `restore_focus_on_cancel` | bool | `false` | |
| `ui` | enum | `null` | null \| border \| external |
| `external_socket` | string | `""` | AF_UNIX path for `ui = external`. Empty or unbindable → behaves as `null` + one warning |

### Added for border highlight (`ui = border`)

| Key | Type | Default | Notes |
|-----|------|---------|-------|
| `border_style` | string | `solid` | `solid` required; `pulse` / `dim` reserved → treated as `solid` until implemented; unknown → `solid` + one warning |
| `border_color` | string | `0xffffd9a0` | Used when `ui = border`. Documented implementation default (verbatim `setprop` colour grammar: hex `0xAARRGGBB` / `rgb(...)` / `rgba(rrggbbbaa)`; intentionally registered as `String`, not `Color`) |
| `border_size` | int | `-1` | `-1` = do not modify window border size; `≥ 0` may set the size for the highlighted window for the duration of the highlight |

### External overlay protocol (`ui = external`)

`ui = external` makes the plugin bind an AF_UNIX stream socket at `external_socket` and speak a
line-framed JSON protocol (normative wire format: `docs/SPEC.md`, Appendix B). The plugin never
blocks on the
peer: messages are best-effort and session/focus behaviour is identical to `ui = null` when no peer
is attached. A reference peer is provided at `tools/overlay_stub.py`.

| Direction | Message | Fields |
|-----------|---------|--------|
| plugin → peer | `session_start` | `windows[]` (`addr`, `title`, `class`), `index` |
| plugin → peer | `selection` | `index` |
| plugin → peer | `session_end` | `reason` = `applied` \| `cancelled` |
| peer → plugin | `select` | `index` (virtual selection only; out-of-range ignored) |
| peer → plugin | `apply` | — |
| peer → plugin | `cancel` | — |

Envelope: one JSON object per `\n`, `"v": 1`, `"type"`. The decoder accepts a tolerant subset of JSON —
arbitrary whitespace and extra unknown JSON keys are tolerated — while unknown `v`/`type`, malformed
JSON, missing required fields, and oversized lines are ignored. `addr` is `0x` + lowercase hex (same
value as `hyprctl clients`).

### Reload

- `debounce_ms` — affects subsequent history commits.
- `ui`, `border_style`, `border_color`, `border_size`, and other session-policy keys — **next session only**.

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

## Lua bridge (`hl.plugin.mru.*`)

Thin wrappers over the same paths as the dispatchers above — same grammar and
semantics (direction defaults to `next`, `cycle` never focuses). Registered in
`PLUGIN_INIT` via `HyprlandAPI::addLuaFunction`; removal is automatic on unload.
On a non-Lua (hyprlang) config the registration silently no-ops and the
dispatchers remain the primary path.

```lua
hl.plugin.mru.cycle("next")            -- or ("prev"), ("next", "global"), ("workspace"), ()
hl.plugin.mru.apply()
hl.plugin.mru.cancel()
local s = hl.plugin.mru.status()       -- same payload as mru:status (§3.4)
```

Errors raise a Lua error (e.g. unknown scope token); `status` returns the
`active=… index=… size=… scope=… session=… last_end=…` string. Omarchy-style — the working recipe (host caveat: release binds on a modifier
key do not fire on the Lua path, so apply commits on **Tab** release, not on
Alt release — a host limitation (see docs/DECISIONS.md); `hl.plugin.mru.*` is the typed Lua bridge and
preferred over `hl.dispatch("mru:…")`, which reaches the same dispatchers but
returns nothing and surfaces errors as strings):

```lua
hl.unbind("ALT + TAB")
hl.unbind("ALT + SHIFT + TAB")
hl.bind("ALT + TAB",         function() hl.plugin.mru.cycle("next") end)
hl.bind("ALT + SHIFT + TAB", function() hl.plugin.mru.cycle("prev") end)
hl.bind("ALT + TAB",         function() hl.plugin.mru.apply() end, { release = true })
hl.bind("ALT + SHIFT + TAB", function() hl.plugin.mru.apply() end, { release = true })
hl.bind("ALT + Escape",      function() hl.plugin.mru.cancel() end)
```

This exact fragment ships as `examples/mru-switcher-bindings.lua`.

---

## Compatibility

- Requires rebuild for each Hyprland ABI/header hash.
- Dispatcher names are namespaced under `mru:` to avoid collisions.
