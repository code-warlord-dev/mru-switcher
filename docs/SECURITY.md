# Security considerations

## Trust model

A Hyprland **plugin runs inside the compositor process** with the same privileges as Hyprland itself.  
Loading a plugin is equivalent to trusting that code with full access to the session (input, surfaces, other clients’ state as exposed by the compositor).

**Do not load plugins from untrusted sources.** Prefer building from source you control.

## This plugin’s surface

| Component | Risk notes |
|-----------|------------|
| Dispatchers | Triggered by user keybinds / hyprctl; args are strings — parse defensively |
| Event listeners | Receive compositor events; must not assume pointers remain valid forever |
| External UI socket (M5) | Local socket; treat peer as untrusted input; validate JSON; no shell-out on peer data |
| Config strings | Paths (e.g. socket) — avoid path traversal assumptions; document intended use |

## Recommendations

1. Ship and load only signed or self-built binaries matched to Hyprland hash.
2. Keep External UI optional; default to `null` or `border`.
3. Do not execute peer-provided strings as commands.
4. On header mismatch, refuse to load (already required by SPEC).

## Reporting issues

Report security-relevant bugs privately to the maintainer if the project establishes a contact; otherwise use the issue tracker with minimal exploit detail until a fix is available.

## Non-goals

This document does not claim sandboxing. In-process plugins cannot be meaningfully sandboxed without compositor-level support.
