<p align="center">
  <img src="banner.webp" alt="MRU Switcher for Hyprland" width="100%">
</p>

<h1 align="center">MRU Window Switcher</h1>

<p align="center">
  <strong>Switch contexts. Blazing fast.</strong>
</p>

<p align="center">
  A Niri-inspired MRU Alt+Tab experience for Hyprland.
  <br>
  Built around how you actually switch between windows — not where they happen to be on the screen.
</p>

<p align="center">
  <a href="https://github.com/code-warlord-dev/mru-switcher/actions/workflows/ci.yml">
    <img src="https://img.shields.io/github/actions/workflow/status/code-warlord-dev/mru-switcher/ci.yml?branch=main&label=CI" alt="CI">
  </a>
  <a href="https://github.com/code-warlord-dev/mru-switcher/releases">
    <img src="https://img.shields.io/github/v/release/code-warlord-dev/mru-switcher" alt="Release">
  </a>
  <a href="https://github.com/code-warlord-dev/mru-switcher/blob/main/LICENSE">
    <img src="https://img.shields.io/github/license/code-warlord-dev/mru-switcher" alt="License">
  </a>
  <img src="https://img.shields.io/badge/C%2B%2B-23-00599C" alt="C++23">
  <img src="https://img.shields.io/badge/Hyprland-v0.56.2-58E1FF" alt="Hyprland v0.56.2">
</p>

---

## What is it?

**MRU Window Switcher** brings a proper, predictable Alt+Tab workflow to [Hyprland](https://hyprland.org).

It is inspired by **[Niri](https://github.com/YaLTeR/niri)**'s workflow: windows are switched by **most-recently-used (MRU) order**, the list stays stable while you hold Alt, and the actual focus changes only when you release it.

In other words:

```text
Alt + Tab
    │
    ├── Tab       → move through the MRU list
    ├── Tab       → move again
    ├── Shift+Tab → go back
    │
    └── release Alt → focus the selected window
```

No focus jumping while you browse.
No MRU history being rewritten underneath you.
No guessing which window comes next.

Just **Alt+Tab that behaves like you expect**.

## See it in action

<p align="center">
  <img src="docs/images/mru-switcher.gif" alt="MRU Window Switcher in action" width="900">
</p>

The preview shows the core interaction: the selection moves through the MRU snapshot while the real focus stays where it is until the switch is committed.

---

## Why?

Traditional window switching tends to be tied to workspace order, stack order, or the compositor's current layout.

That's not always how people think.

When you're working, the mental model is usually much simpler:

> "Take me back to the thing I was just using."

MRU Switcher keeps that context explicit.

### The interaction model

| Action                     | What happens                           |
| -------------------------- | -------------------------------------- |
| First `Alt+Tab`            | Starts an MRU switching session        |
| `Tab`                      | Selects the next window                |
| `Shift+Tab`                | Selects the previous window            |
| Release `Alt`              | Applies the selection                  |
| `Escape`                   | Cancels the session                    |
| Mouse / other focus events | Do not reorder the active MRU snapshot |

The important part is that **selection and focus are separate**.

You can browse the list without making Hyprland actually focus every intermediate window.

---

## Features

* **MRU window ordering** — switch by recent usage rather than layout position
* **Frozen session snapshot** — the candidate list stays stable while tabbing
* **Apply-on-release** — real focus changes only when the session is committed
* **History lock-in** — intermediate switching does not corrupt MRU history
* **Debounce** — short-lived focus changes do not immediately reshuffle history
* **Five scopes**

  * `global`
  * `monitor`
  * `workspace`
  * `visible`
  * `app`
* **Stable window identity** — address + generation prevents recycled window IDs from becoming stale references
* **Optional border highlight** — visually shows the currently selected window without moving real focus
* **Safe invalidation** — closing a selected window during a session is handled without crashing
* **Explicit dispatchers** — designed as a real Hyprland plugin rather than a collection of shell binds

---

## Installation

MRU Switcher is a native Hyprland plugin and currently targets **Hyprland v0.56.2**.

### hyprpm (plugin manager)

The repository ships a `hyprpm.toml` manifest with `commit_pins` for the tested
Hyprland revision (`v0.56.2` / `efb5099`), repository metadata and a build stanza
that produces `build/mru-switcher.so`:

```bash
hyprpm add https://github.com/code-warlord-dev/mru-switcher.git
hyprpm update
hyprpm list        # verify the plugin is installed
hyprpm reload      # load enabled plugins into the running compositor
```

Other available commands: `hyprpm enable|disable <name>`, `hyprpm remove <url|name|author/name>`,
`hyprpm purge-cache`. Flags such as `--no-nix`, `--no-shallow` or `-f` (force) are documented in
`hyprpm --help`. Because the manifest pins the Hyprland commit, hyprpm rebuilds the plugin against
the matching headers; a Hyprland upgrade is a rebuild event, never a silent compatibility window
(see the [compatibility matrix](docs/COMPAT.md)).

> The `commit_pins` plugin-side hash is finalized at the `v1.0.0` release tag (M6-T9).

### Build from source

Clone the repository:

```bash
git clone https://github.com/code-warlord-dev/mru-switcher.git
cd mru-switcher
```

Build the plugin:

```bash
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DMRU_BUILD_PLUGIN=ON

cmake --build build -j
```

The resulting plugin is:

```text
build/mru-switcher.so
```

Load it in Hyprland:

```bash
hyprctl plugin load /absolute/path/to/mru-switcher.so
```

Or configure it directly:

```ini
plugin = /absolute/path/to/mru-switcher.so
```

> **Compatibility matters.** Hyprland plugins are tightly coupled to compositor internals. MRU Switcher deliberately fails closed when the expected Hyprland ABI/API does not match the pinned version.

See the [compatibility matrix](docs/COMPAT.md) for the tested Hyprland revision.

---

## Quick setup

Add the following binds to `hyprland.conf`:

```ini
# Forward
bind = ALT, TAB, mru:cycle, next

# Backward
bind = ALT SHIFT, TAB, mru:cycle, prev

# Apply selection when Alt is released
bindrt = ALT, ALT_L, mru:apply

# Cancel
bind = ALT, Escape, mru:cancel
```

That's enough to get the basic workflow running.

### Optional configuration

```ini
plugin {
    mru-switcher {
        debounce_ms             = 400
        default_scope           = global
        start_offset            = second
        wrap                    = true
        ui                      = border
        border_style            = solid
        border_color            = 0xffffd9a0
        border_size             = -1
        lock_history_on_session = true
        restore_focus_on_cancel = false
    }
}
```

After changing plugin configuration:

```bash
hyprctl reload
```

Configuration changes apply to the **next** MRU session. An already active session keeps the policy it started with.

For the complete configuration reference, see [docs/USER.md](docs/USER.md).

---

## Scopes

MRU Switcher can work with different window groups depending on what you're doing:

```ini
bind = ALT, TAB, mru:cycle, next global
bind = ALT, TAB, mru:cycle, next monitor
bind = ALT, TAB, mru:cycle, next workspace
bind = ALT, TAB, mru:cycle, next visible
bind = ALT, TAB, mru:cycle, next app
```

| Scope       | Windows included                               |
| ----------- | ---------------------------------------------- |
| `global`    | All mapped windows                             |
| `monitor`   | Windows on the current monitor                 |
| `workspace` | Windows on the current workspace               |
| `visible`   | Windows on currently visible workspaces        |
| `app`       | Windows belonging to the active window's class |

If no scope is specified, `default_scope` is used.

---

## UI

The switching logic is independent from the visual presentation.

### `null`

No visual feedback.

Useful for minimal setups, testing, or users who prefer a completely keyboard-driven workflow.

### `border`

Highlights the currently selected window's border while the MRU session is active.

The important distinction is:

```text
Selected window ≠ focused window
```

The border follows the virtual selection while real focus remains unchanged until `mru:apply`.

### `external`

Drives an out-of-process overlay over an AF_UNIX socket, so a separate UI can render previews and
control the selection. It is opt-in and off by default: set `ui = external` and `external_socket`.
If no overlay is attached, switching behaves exactly as with `null`. See
[docs/USER.md](docs/USER.md) and [docs/API.md](docs/API.md) for the protocol.

---

## Dispatchers

MRU Switcher exposes a small explicit dispatcher API:

```text
mru:cycle [next|prev] [scope]
mru:apply
mru:cancel
mru:status
```

Examples:

```bash
hyprctl dispatch mru:cycle next
hyprctl dispatch mru:cycle prev
hyprctl dispatch mru:apply
hyprctl dispatch mru:cancel
```

The dispatcher surface is intentionally small. The plugin owns the switching session and its invariants rather than pushing state management into shell scripts or configuration glue.

---

## Current status

**v0.5.0**

The core MRU workflow, plugin integration, scopes, configuration surface, border UI, and the external overlay backend are implemented and verified against the pinned Hyprland release.

The current release includes:

* domain core and tests
* loadable native `.so` plugin
* MRU session management
* five switching scopes
* configuration and dispatcher API
* border highlight UI
* external overlay socket + protocol (`ui = external`)
* focus restoration on cancel
* invalidation and teardown handling
* CI builds and test coverage
* live nested-Hyprland verification

The next major direction is **hardening toward `v1.0`** (contract freeze, hyprpm distribution, stress cases).

See the [roadmap](docs/ROADMAP.md) for details.

---

## Compatibility

MRU Switcher is currently pinned and tested against:

| Component  | Version                              |
| ---------- | ------------------------------------ |
| Hyprland   | `v0.56.2`                            |
| C++        | C++23                                |
| Aquamarine | `0.15.0` in live nested verification |

The plugin checks the expected Hyprland build identity and **fails closed on a mismatch** rather than attempting to run against an unknown compositor ABI.

If Hyprland changes its internal APIs, rebuild and verify the plugin against the corresponding compatibility entry before using it.

See [docs/COMPAT.md](docs/COMPAT.md).

---

## Building and testing

The domain and plugin-core tests can be built without Hyprland headers:

```bash
cmake -S . -B build \
  -DMRU_BUILD_TESTS=ON \
  -DMRU_BUILD_PLUGIN=OFF

cmake --build build -j

ctest --test-dir build --output-on-failure
```

Building the actual plugin requires the pinned Hyprland headers:

```bash
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DMRU_BUILD_PLUGIN=ON

cmake --build build -j
```

CI performs the corresponding build and test checks.

---

## For developers

The project keeps the user-facing behaviour deliberately small while documenting the implementation in depth.

If you want to understand the internals:

1. [Plugin system](docs/HYPRLAND-PLUGIN-SYSTEM.md)
2. [Architecture](docs/ARCHITECTURE.md)
3. [Specification](docs/SPEC.md)
4. [Failure modes](docs/FAILURE-MODES.md)
5. [Architecture decisions](docs/DECISIONS.md)
6. [Requirements traceability](docs/REQ-TRACE.md)

Other useful references:

* [User Guide](docs/USER.md)
* [API Reference](docs/API.md)
* [Compatibility](docs/COMPAT.md)
* [Roadmap](docs/ROADMAP.md)
* [Contributing](docs/CONTRIBUTING.md)
* [Security](docs/SECURITY.md)
* [Changelog](CHANGELOG.md)

---

## The idea

MRU Switcher is intentionally not an attempt to make Hyprland behave like Niri.

It takes one interaction pattern that works exceptionally well:

**"Switch back to what I was using."**

…and gives it a native home inside Hyprland.

Inspired by Niri's workflow, implemented as a proper Hyprland plugin.

---

## License

MIT — see [LICENSE](LICENSE).

---

<p align="center">
  <sub>Built for people who switch contexts more often than they switch workspaces.</sub>
</p>
