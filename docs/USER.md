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

The files under `examples/` are the starting point for configuration. The
keybinding file comes in two flavours — pick the one for your config backend:

* **hyprlang** (default `hyprland.conf`): `examples/mru-switcher-bindings.conf`
* **Lua / Omarchy** (`hyprland.lua`): `examples/mru-switcher-bindings.lua`

**Source install** — the checkout keeps them at
`~/.local/src/mru-switcher/examples`:

```bash
mkdir -p ~/.config/hypr/conf.d
cp ~/.local/src/mru-switcher/examples/mru-switcher.conf ~/.config/hypr/conf.d/mru-switcher.conf
cp ~/.local/src/mru-switcher/examples/mru-switcher-bindings.conf ~/.config/hypr/conf.d/mru-switcher-bindings.conf
```

**hyprpm install** — hyprpm builds inside its own cache, so no source tree is
left on disk. Download the same files instead:

```bash
mkdir -p ~/.config/hypr/conf.d
curl -fsSL https://raw.githubusercontent.com/code-warlord-dev/mru-switcher/main/examples/mru-switcher.conf -o ~/.config/hypr/conf.d/mru-switcher.conf
curl -fsSL https://raw.githubusercontent.com/code-warlord-dev/mru-switcher/main/examples/mru-switcher-bindings.conf -o ~/.config/hypr/conf.d/mru-switcher-bindings.conf
```

For a pinned release (the `v1.0.0` pin, once published), replace `main` with the
tag so the examples match the plugin you are running.

**Lua / Omarchy** — install the Lua fragment to your hyprland config directory
(not `conf.d`; it is loaded by `dofile`, not `source`):

```bash
cp ~/.local/src/mru-switcher/examples/mru-switcher-bindings.lua ~/.config/hypr/mru-switcher-bindings.lua
# or: curl -fsSL https://raw.githubusercontent.com/code-warlord-dev/mru-switcher/main/examples/mru-switcher-bindings.lua -o ~/.config/hypr/mru-switcher-bindings.lua
```

The guided helper `scripts/setup-bindings.sh` does this for you: it autodetects
the backend (Lua config present → Lua fragment, otherwise hyprlang), refuses to
overwrite without `--force` (which saves a `.bak.<timestamp>` first), warns about
conflicting keybinds found via `hyprctl binds -j`, and writes nothing with
`--dry-run`. The plugin config helper (`install.sh --write-conf`) still only
handles the hyprlang `.conf` files.

Then, on **hyprlang**, `source` the two files from `hyprland.conf`:

```conf
# hyprland.conf
source = ~/.config/hypr/conf.d/mru-switcher.conf
source = ~/.config/hypr/conf.d/mru-switcher-bindings.conf
```

On **Lua**, make sure the fragment runs *before* the plugin adds its binds (the
example `hl.unbind("ALT + TAB")` must win). In an already-loaded `bindings.lua`:

```lua
dofile(os.getenv("HOME") .. "/.config/hypr/mru-switcher-bindings.lua")
```

Then reload (either backend):

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

Lua equivalent (Hyprland Lua config / Omarchy):

```lua
-- install examples/mru-switcher-bindings.lua and dofile it before the plugin binds;
-- equivalent inline recipe:
hl.unbind("ALT + TAB")
hl.unbind("ALT + SHIFT + TAB")
hl.bind("ALT + TAB",         function() hl.plugin.mru.cycle("next") end)
hl.bind("ALT + SHIFT + TAB", function() hl.plugin.mru.cycle("prev") end)
hl.bind("ALT + Return",      function() hl.plugin.mru.apply() end)
hl.bind("ALT + Escape",      function() hl.plugin.mru.cancel() end)
```

> **Lua caveat (host):** on the Lua keybind path, a release bind keyed on a
> *modifier* key (`hl.bind("ALT + ALT_L", …, { release = true })`, the Lua
> equivalent of `bindrt`) never fires in the pinned build (Hyprland
> v0.56.2 / efb5099) — release binds only fire reliably for ordinary keys. The
> Lua recipe therefore commits with an **explicit apply key**, `ALT + Return`:
> nothing moves until you press it, and the list stays frozen while you browse.
> If you want the literal "release Alt to apply" behaviour, install the poll
> variant `examples/mru-switcher-bindings-poll.lua` (a `hl.timer` that watches
> `hl.is_key_down("Alt_L")` / `"Alt_R"` and applies once, on logical release) —
> load one variant or the other, never both. Committing on the release of
> **Tab** is *not* recommended: it fires a real focus move on every step, so the
> frozen list stops matching what you see and the browse-then-commit workflow is
> lost. See [DECISIONS.md](DECISIONS.md) for the corrected decision.
>
> The `hl.unbind` lines are needed on Omarchy, whose default `Alt+Tab` (Focus on
> next window) would otherwise fire together with the MRU bind.

### 4. Optional config

```conf
plugin {
    mru-switcher {
        debounce_ms             = 400
        default_scope           = global   # global | monitor | workspace | visible | app
        start_offset            = second   # first | second
        wrap                    = true
        ui                      = null     # null | border | external (unavailable backends fall back to null)
        border_style            = solid    # solid only for now; pulse/dim reserved -> solid + warn-once
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
  the already-running session continues with its frozen backend.

A session that is already running is never changed mid-flight: its frozen
window list and selection policy stay exactly as they were when it started.
Finish or cancel it, then start a new session to use the updated settings.

Runtime `hyprctl keyword` changes to `plugin:mru-switcher:*` keys are **not**
observed by the plugin on Hyprland 0.56.2 — edit the config file and run
`hyprctl reload` instead (see `docs/COMPAT.md`).

---

## Behaviour

| Action | Result |
|--------|--------|
| First `Alt+Tab` | Opens a session, builds a **snapshot** of windows in MRU order, selects the second entry (previous window) by default |
| Further `Tab` / `Shift+Tab` | Moves selection inside the frozen snapshot only |
| Apply (`mru:apply`; hyprlang: Alt release via `bindrt`, pending re-verification — see [COMPAT.md](COMPAT.md); Lua: explicit `Alt+Return`, or Alt release with the optional poll variant) | Focuses the selected window and ends the session |
| `Escape` | Cancels the session without changing focus (unless `restore_focus_on_cancel` is set) |

While a session is active, intermediate focus changes (mouse, other keybinds) do **not**
reorder the list — lock-in is always on and cannot be switched off (there is a reserved,
ignored `lock_history_on_session` key for 0.x config compatibility: see [API.md](API.md)).
A focus change made just before you open a session is committed immediately, so
back-to-back switches rotate the list deterministically.

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

The status payload is frozen for 1.x: `active= index= size= scope= session= last_end=` —
additional keys may appear in minor releases; tolerate them. On Hyprland 0.56.x,
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
| `border` | Temporarily highlight the **selected** window's border while you hold Alt and cycle |
| `external` | Drive an external overlay via an AF_UNIX socket (see below). Empty/unbindable path → behaves as `null` with a one-time warning |

Default is `null`. To enable border highlight:

```conf
plugin {
    mru-switcher {
        ui           = border
        border_style = solid          # pulse/dim reserved → treated as solid + one-time warning
        border_color = rgba(33ccffee) # example — see below and API.md for accepted formats
        border_size  = -1             # -1 = do not change border width
    }
}
```

If you omit the border keys, the documented defaults are: `border_style = solid`, `border_color = 0xffffd9a0` (hex `0xAARRGGBB`), `border_size = -1` (= do not change border width). Accepted colour formats are those supported by the pinned Hyprland: `rgb(...)`, `rgba(rrggbbaa)`, or hex `0xAARRGGBB` as in the default. Full key reference: `docs/API.md`; availability on your Hyprland build: `docs/COMPAT.md`.

### Border behaviour (what you should see)

1. First `Alt+Tab` — the selected window (usually the previous one) gets the highlight colour.
2. Further `Tab` / `Shift+Tab` — highlight **moves** with the virtual selection; real focus stays put until release.
3. Release Alt (`mru:apply`) or Escape (`mru:cancel`) — highlight is **removed**.
4. Plugin unload / Hyprland exit mid-session — highlight is cleared as part of teardown.

If the border APIs are unavailable on your Hyprland build, the plugin keeps switching correctly and falls back to no highlight (same as `null`, one-time warning).

### UI settings reload

`ui` and the `border_*` keys apply to the **next** Alt+Tab session after `hyprctl reload`. An already open session keeps its original UI settings until apply/cancel.

### Updating the plugin (never overwrite the `.so` in place)

If Hyprland has already loaded `mru-switcher.so` once in this session, **do not overwrite that file
in place** (e.g. `cp rebuilt.so build/mru-switcher.so`). On the pinned Hyprland, re-loading a path
whose file was replaced with the same inode crashes the compositor (details and evidence:
`docs/COMPAT.md`; issue #55). A rebuild by `cmake` **relinks** and produces a fresh inode, and a
fresh-inode file at the same path loads cleanly — so the safe update flow is:

```bash
cmake --build ~/.local/src/mru-switcher/build -j     # relink = fresh inode at the same path
hyprctl plugin unload "$HOME/.local/src/mru-switcher/build/mru-switcher.so"  # if loaded in this session
hyprctl plugin load  "$HOME/.local/src/mru-switcher/build/mru-switcher.so"
```

Equivalently: `rm` the old file before copying a new one in, or use `mv`/rename, or load the
updated binary in a fresh Hyprland session. A normal `plugin unload` / `plugin load` of an
**unchanged** binary is safe.

### If the plugin fails to load after a Hyprland update

**What you see.** The plugin is not loaded — `mru:*` binds do nothing and the dispatchers are
unknown. The Hyprland log (`hyprctl rollinglog`, or your session journal) contains a line like:

```
[PluginSystem] mru-switcher: header hash mismatch, refusing to load
```

**What it means.** This is the plugin's protection, not a crash: the binary was built against a
different Hyprland version than the one now running. Hyprland plugins have no stable ABI, so
loading a mismatched binary could crash the compositor — the plugin refuses instead (fail-closed).

**Recover (pick one):**

* **hyprpm install** — rebuild from the pinned compatible source in one step, then reload:

  ```bash
  hyprpm update
  hyprpm reload   # or log out and back in
  ```

* **Source install** — rebuild against the new headers, then swap binaries per the
  ["Updating the plugin"](#updating-the-plugin-never-overwrite-the-so-in-place) section above
  (never overwrite the `.so` in place):

  ```bash
  cd ~/.local/src/mru-switcher
  git pull
  cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DMRU_BUILD_PLUGIN=ON
  cmake --build build -j
  hyprctl plugin unload "$HOME/.local/src/mru-switcher/build/mru-switcher.so"  # if loaded this session
  hyprctl plugin load  "$HOME/.local/src/mru-switcher/build/mru-switcher.so"
  ```

* **No time right now** — disable the plugin (`hyprpm disable mru-switcher`, or comment out its
  load line) and move on; Hyprland runs normally without it.

**Which Hyprland build is supported:** see the compatibility matrix in `docs/COMPAT.md` (it names
the tested Hyprland commit); check yours with `hyprctl version`.

### External overlay (`ui = external`)

An out-of-process overlay (any language) can render previews/search and drive the selection. The
plugin binds an AF_UNIX stream socket at `external_socket` and speaks a line-framed JSON protocol
(documented in `docs/API.md`; the wire format itself is developer-facing and lives in
`docs/SPEC.md`, Appendix B).

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

## How the plugin behaves when things go wrong

Every failure path is designed to degrade, not to crash the compositor: a dead overlay behaves like
`ui = null`, a bad config value falls back to its default with one warning, a storm of closing
windows prunes the frozen session snapshot without focus glitches. The full narratives —
["External overlay fails"](RISK-PROFILES.md#1-external-overlay-fails),
["Malicious / broken configuration"](RISK-PROFILES.md#2-malicious--broken-configuration) and
["Window-close storm during a session"](RISK-PROFILES.md#3-window-close-storm-during-a-session) —
live in [RISK-PROFILES.md](RISK-PROFILES.md); developer-facing matrices:
[FAILURE-MODES.md](FAILURE-MODES.md), [COMPAT.md](COMPAT.md).

---

## FAQ

**Why doesn’t focus move on every Tab?**  
By design (Niri / classic Alt+Tab). Focus is applied only on release so intermediate workspaces and history stay clean.

**Why doesn't history jump when I tab quickly?**  
History is always frozen while a session is running: **lock-in is mandatory and there is no setting to turn it off**. Make sure you are not mixing other focus binds that bypass the plugin during the session. Quick consecutive `cycle`/`apply` pairs now rotate the history deterministically instead of landing on the same window, so a fast A↔B toggle behaves the same whether or not you wait for `debounce_ms`.

**Special workspaces / scratchpads.**  
A scratchpad window is switchable only while it is **shown**; when hidden it is
excluded from every scope (including `global`). Shown scratchpad windows behave
like normal windows and follow the selected scope.

**Multi-monitor.**  
Use `scope = monitor` for per-monitor switching, or `global` for a single list across all outputs.

**Alt+Tab does nothing.**  
The plugin does not bind its own keys — without the keybinding install step it
loads silently and nothing fires. Check `hyprctl binds -j` lists the `mru:*`
binds (hyprlang), or that the Lua fragment is actually loaded (Lua/Omarchy).
On Omarchy you must also out-bind the default `Alt+Tab` ("Focus on next
window"): the Lua recipe does this with `hl.unbind("ALT + TAB")`; on hyprlang
use `unbind = ALT, TAB`.

**Selection does not apply when I release Alt (Lua/Omarchy).**  
Known host limitation: a release bind keyed on a *modifier* token never fires
on the Lua keybind path in the pinned build. The default Lua recipe therefore
commits with an explicit apply key — `Alt+Return` → `mru:apply` (§3): nothing
moves until you press it. For the literal "release Alt to apply" behaviour,
install the optional poll variant `examples/mru-switcher-bindings-poll.lua`
(a `hl.timer` watching `hl.is_key_down("Alt_L")` / `"Alt_R"`); load one of the
two files, never both. Committing on **Tab** release is not a fix: it fires a
real focus move on every step and breaks the frozen-list workflow (see
[DECISIONS.md](DECISIONS.md), ADR-023).

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
