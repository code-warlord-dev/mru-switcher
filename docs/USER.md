# MRU Window Switcher — User Guide

Niri-style Alt+Tab for Hyprland: most-recently-used order, frozen list while tabbing, focus applied on modifier release.

---

## Quick start

### 1. Install the plugin

**Recommended: hyprpm** — the intended workflow when you already manage
Hyprland plugins via hyprpm. Once the pinned `v1.0.0` release is published:

```bash
hyprpm add https://github.com/code-warlord-dev/mru-switcher
hyprpm enable mru-switcher
hyprpm reload
```

Until that pin exists, **build from source** into the canonical layout (no
`sudo`, everything under your home directory):

```bash
mkdir -p ~/.local/src
git clone https://github.com/code-warlord-dev/mru-switcher.git ~/.local/src/mru-switcher
cmake -S ~/.local/src/mru-switcher -B ~/.local/src/mru-switcher/build -DCMAKE_BUILD_TYPE=Release -DMRU_BUILD_PLUGIN=ON
cmake --build ~/.local/src/mru-switcher/build -j
hyprctl plugin load "$HOME/.local/src/mru-switcher/build/mru-switcher.so"
```

Alternative: the guided install helper does the same build into the canonical
path and verifies your Hyprland headers against the pin:

```bash
git clone https://github.com/code-warlord-dev/mru-switcher.git ~/.local/src/mru-switcher && cd ~/.local/src/mru-switcher && ./scripts/install.sh
```

Run `./scripts/install.sh --help` for flags.

### 2. Start from the shipped examples

The files under `examples/` are the starting point for configuration. Copy
them, then `source` them from `hyprland.conf`:

```bash
mkdir -p ~/.config/hypr/conf.d
cp ~/.local/src/mru-switcher/examples/mru-switcher.conf ~/.config/hypr/conf.d/mru-switcher.conf
cp ~/.local/src/mru-switcher/examples/mru-switcher-bindings.conf ~/.config/hypr/conf.d/mru-switcher-bindings.conf
```

```conf
# hyprland.conf
source = ~/.config/hypr/conf.d/mru-switcher.conf
source = ~/.config/hypr/conf.d/mru-switcher-bindings.conf
```

Then reload:

```bash
hyprctl reload
```

`examples/mru-switcher.conf` documents every config key inline (purpose,
default, allowed values) and sets `ui = border` — a first-run demo override of
the plugin default `null` — so you can see the selection; set it back to
`null` if you prefer no visual feedback.

### 3. Recommended binds, inline

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

### 4. Optional config

```conf
plugin {
    mru-switcher {
        debounce_ms             = 400
        default_scope           = global   # global | monitor | workspace | visible | app
        start_offset            = second   # first | second
        wrap                    = true
        ui                      = null     # null | border | external (border from M4; unavailable backends fall back to null)
        border_style            = solid    # border highlight style (M4: solid only; pulse/dim reserved -> solid + warn-once)
        border_color            = 0xffffd9a0  # border highlight colour (bright accent; verbatim setprop grammar)
        border_size             = -1       # -1 = leave border size unchanged (colour only)
        restore_focus_on_cancel = false
        external_socket         =        # AF_UNIX path; required when ui = external (empty = degrade to null)
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
  `restore_focus_on_cancel`, `ui`, `external_socket`, and the
  border-* keys) — applies to the **next** session you start. Note: switching `ui` away
  from `external` (or clearing `external_socket`) stops the socket listener at reload;
  the already-running session continues with its frozen backend (REQ-UI-009).

A session that is already running is never changed mid-flight: its frozen
window list and selection policy stay exactly as they were when it started.
Finish or cancel it, then start a new session to use the updated settings.

Runtime `hyprctl keyword` changes to `plugin:mru-switcher:*` keys are **not**
observed by the plugin on Hyprland 0.56.2 — edit the config file and run
`hyprctl reload` instead (see `docs/COMPAT.md`, M4 nest smoke 2026-09-18).

---

## Behaviour

| Action | Result |
|--------|--------|
| First `Alt+Tab` | Opens a session, builds a **snapshot** of windows in MRU order, selects the second entry (previous window) by default |
| Further `Tab` / `Shift+Tab` | Moves selection inside the frozen snapshot only |
| Release `Alt` | Focuses the selected window and ends the session |
| `Escape` | Cancels the session without changing focus (unless `restore_focus_on_cancel` is set) |

While a session is active, intermediate focus changes (mouse, other keybinds) do **not**
reorder the list — lock-in is always on and cannot be switched off (there is a reserved,
ignored `lock_history_on_session` key for 0.x config compatibility: see [API.md](API.md)).
A focus change made just before you open a session is committed immediately, so
back-to-back switches rotate the list deterministically (ADR-021).

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

The status payload is frozen for 1.x: `active= index= size= scope= session= last_end=` — SPEC §3.4,
normative since the M6-T1 contract freeze; tolerate unknown additional keys. On Hyprland 0.56.x,
`hyprctl dispatch mru:status` prints only `ok`: the plugin carries the status payload in a field
`hyprctl` surfaces only on failure, so it is not visible through IPC on this version — read it with a
libwayland dispatcher binding or from the plugin log. This is a documented host limitation
(`docs/COMPAT.md` matrix), not a plugin defect. (Failure messages do surface, e.g.
`mru-switcher: not initialized`.) External tools already assert session state indirectly:

```bash
hyprctl activewindow -j
```

---

## UI backends

| Value | Behaviour |
|-------|-----------|
| `null` | No visual feedback (logic only; useful for tests and minimal setups) |
| `border` | Temporarily highlight the **selected** window's border while you hold Alt and cycle (**M4+**) |
| `external` | Drive an external overlay via an AF_UNIX socket (**M5+**; see below). Empty/unbindable path → behaves as `null` with a one-time warning |

Default is `null`. To enable border highlight:

```conf
plugin {
    mru-switcher {
        ui           = border
        border_style = solid          # only solid is active in M4; pulse/dim reserved
        border_color = rgba(33ccffee) # example — see below and API.md for accepted formats
        border_size  = -1             # -1 = do not change border width
    }
}
```

If you omit the border keys, the documented defaults are: `border_style = solid`, `border_color = 0xffffd9a0` (hex `0xAARRGGBB`), `border_size = -1` (= do not change border width). Accepted colour formats are those supported by the pinned Hyprland: `rgb(...)`, `rgba(rrggbbaa)`, or hex `0xAARRGGBB` as in the default. Full key reference: `docs/API.md`; availability on your Hyprland build: `docs/COMPAT.md`.

### Border behaviour (what you should see)

1. First `Alt+Tab` — the selected window (usually the previous one) gets the highlight colour.
2. Further `Tab` / `Shift+Tab` — highlight **moves** with the virtual selection; real focus stays put until release (REQ-UI-006).
3. Release Alt (`mru:apply`) or Escape (`mru:cancel`) — highlight is **removed**.
4. Plugin unload / Hyprland exit mid-session — highlight is cleared as part of teardown (REQ-UI-005).

If the border APIs are unavailable on your Hyprland build, the plugin keeps switching correctly and falls back to no highlight (same as `null`, one-time warning).

### UI settings reload

`ui` and the `border_*` keys apply to the **next** Alt+Tab session after `hyprctl reload`. An already open session keeps its original UI settings until apply/cancel (REQ-UI-009, REQ-CFG-002).

### External overlay (`ui = external`, M5+)

An out-of-process overlay (any language) can render previews/search and drive the selection. The
plugin binds an AF_UNIX stream socket at `external_socket` and speaks the line-framed JSON protocol
frozen in `docs/SPEC.md` §12 Appendix B (`docs/API.md` has the message table).

```conf
plugin {
    mru-switcher {
        ui              = external
        external_socket = /run/user/1000/mru-overlay.sock
    }
}
```

- The socket is bound at load (after the initial config reload) so an overlay can connect **before**
  the first Alt+Tab. Start the overlay like any other Hyprland autostart process (e.g.
  `exec-once = ~/.local/bin/my-overlay`, with the socket path matching `external_socket`), or let it
  reconnect.
- If the overlay is absent or dies mid-session, switching still works exactly as with `ui = null`;
  outgoing messages are dropped (best-effort). The plugin never blocks on the peer.
- Clearing `external_socket` or changing `ui` away from `external` and reloading stops the listener.
- A reference peer for testing ships at `tools/overlay_stub.py`:

  ```bash
  python3 tools/overlay_stub.py /run/user/1000/mru-overlay.sock
  # then press Alt+Tab; type "s 2", "a" (apply), "c" (cancel), "q" to quit
  ```

### Border manual test checklist

- [ ] With `ui = border`, first cycle shows highlight on the selected window
- [ ] Repeated Tab moves highlight without moving real focus
- [ ] Apply (Alt release) focuses selection and removes highlight
- [ ] Cancel (Escape) removes highlight and does not leave a stuck border
- [ ] Rapid Tab does not leave multiple windows highlighted
- [ ] `len == 1` (only the focused window in scope): the active-border slot also highlights
- [ ] Unload the plugin mid-session: borders are restored
- [ ] `ui = null` restores the previous non-visual behaviour
- [ ] Invalid / closed window mid-session does not crash; highlight skips or session ends cleanly

---

## FAQ

**Why doesn’t focus move on every Tab?**  
By design (Niri / classic Alt+Tab). Focus is applied only on release so intermediate workspaces and history stay clean.

**Order jumps when I tab quickly.**  
History is always frozen while a session is running: **lock-in is mandatory and there is no setting to turn it off**. Make sure you are not mixing other focus binds that bypass the plugin during the session. Quick consecutive `cycle`/`apply` pairs now rotate the history deterministically instead of landing on the same window (ADR-021), so a fast A↔B toggle behaves the same whether or not you wait for `debounce_ms`.

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
- [ ] `restore_focus_on_cancel = 1` (config file + `hyprctl reload`): `mru:cancel` refocuses the window that was focused at session start; a dead origin is a no-op (no focus change, no crash) — [live nest confirmation](agent-state/reports/2026-09-19-m4-restore-on-cancel.md)
- [ ] `ui = external` + `external_socket` with `tools/overlay_stub.py`: Alt+Tab starts a session, Tab moves the overlay selection, `s N` jumps, `a`/`c` apply/cancel; killing the stub does not break switching ([live nest report](agent-state/reports/2026-09-19-m5-s3-nest-smoke.md))
