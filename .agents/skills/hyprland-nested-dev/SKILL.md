---
name: hyprland-nested-dev
description: Expert nested Hyprland development workflow for plugins including debug sessions, hyprctl plugin load reload, headers install, and smoke testing binds. Use when setting up a nest, iterating on a .so, debugging crashes on load, or validating keybinds without risking the host session.
metadata:
  level: advanced
  version: "1.0"
  domain: hyprland
---

# Nested Hyprland Plugin Development

## Overview

Develop plugins inside a **nested** Hyprland instance so crashes and bad hooks do not take down the host desktop.

## When to use

- First-time plugin build environment
- Load/unload iteration (`hyprctl plugin load|unload`)
- Reproducing bindrt / modifier-release behavior
- Checking hash mismatch notifications

## Environment setup (typical)

1. Clone Hyprland sources matching the target version
2. Build debug and install headers (`make debug && sudo make installheaders` or distro equivalent)
3. Build plugin against those headers
4. Start nested Hyprland (see Hyprland Contributing / nested session docs)
5. In the nest — load the plugin by **absolute path**

```bash
hyprctl plugin load /absolute/path/to/plugin.so
hyprctl plugin unload /absolute/path/to/plugin.so
# reload one-liner
hyprctl plugin unload /abs/plugin.so ; hyprctl plugin load /abs/plugin.so
```

## Headers vs running binary

If `__hyprland_api_get_hash()` ≠ client hash:

- Rebuild plugin against the **same** sources as the nested binary
- Or rebuild nested Hyprland and headers together

Never ignore hash failure in production plugins.

## Smoke checklist for MRU-like plugins

- [ ] Plugin lists in `hyprctl plugin list`
- [ ] `hyprctl dispatch mru:cycle next` returns ok with windows open
- [ ] Recommended binds in nest config
- [ ] Release modifier triggers apply (`bindrt`)
- [ ] Escape cancels
- [ ] Unload does not leave stuck borders or hooks (if any)

## Debugging tips

- Run nested with verbose logs
- Prefer Event::bus logging over hooks while diagnosing
- Bisect: Null UI first, then border, then scopes
- Confirm single-threaded assumptions — no detached focus calls

## CI note

Full nested tests are heavy; gate unit tests on domain in CI, keep nested smoke manual or optional job.

## References

- Hyprland wiki — Contributing / development environment
- Project `docs/USER.md` manual checklist
- `docs/ROADMAP.md` M2 exit criteria
