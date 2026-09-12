---
name: hyprland-focus-mru
description: Expert focus history and MRU behavior on Hyprland including Desktop History windowTracker, focus reasons, debounce, session lock-in, and Alt-Tab style virtual selection. Use when implementing or debugging recent-windows order, focus pollution, FFM interaction, or apply-on-release switchers.
metadata:
  level: expert
  version: "1.0"
  domain: hyprland
---

# Hyprland Focus History and MRU Patterns

## Overview

Hyprland maintains focus history used by directional focus and cycle helpers. Aggressive updates (FFM, clicks, keybinds, workspace changes) make **raw** history unsuitable for classic Alt+Tab without debounce and session lock-in.

## When to use

- Building MRU / recent-window switchers
- Debugging “list jumps while holding Alt”
- Choosing seed source for ordering
- Interpreting `Desktop::eFocusReason`
- Designing apply-on-release vs focus-each-step

## Authoritative sources

Prefer (in order):

1. `Desktop::History::windowTracker()->fullHistory()` when headers expose it
2. Event-sourced list from `Event::bus()->m_events.window.active` with `(PHLWINDOW, eFocusReason)`
3. IPC `activewindow` / clients (external tools only — higher latency)

## Focus reasons (typical)

Useful categories (names may evolve with Hyprland):

- Pointer — FFM, click
- Explicit — keybind, dispatch focuswindow
- Structural — workspace change, map/unmap, group changes

For v0 MRU plugins, commit **all** focuses through debounce unless SPEC says otherwise. Advanced filters by reason are optional later.

## Debounce algorithm

```text
on window.active (Idle only):
  cancel pending timer
  schedule commit of this window after debounce_ms
on timer fire:
  move window to front of MRU list (dedupe)
```

`debounce_ms = 0` means immediate commit.

## Session lock-in

While Alt+Tab session is Active:

- Do **not** reorder MRU from focus events
- Snapshot order stays fixed (except prune closed windows)
- Prevents intermediate `movefocus` / workspace hops from scrambling the list

Unlock on apply/cancel/empty.

## Virtual selection vs live focus

| Mode | Behavior | History impact |
|------|----------|----------------|
| Focus each Tab | Real focus every step | Pollutes history; jumps workspaces |
| Virtual + apply on release | Focus once at end | Clean; matches Niri / classic desktop |

Prefer virtual selection for product-quality Alt+Tab.

## Snapshot construction

1. Take MRU-ordered identities from tracker
2. Filter by scope and validity (mapped, not hidden, not fading)
3. Freeze list
4. Initial index from `start_offset` (`second` ≈ previous window)

## Validity / prune

On close/destroy of a window in the snapshot:

- Remove identity
- Clamp selection index
- If empty → end session as cancelled

Never dereference dead window pointers; treat as invalid.

## Testing focus paths

- Rapid Tab without release — order stable
- FFM over other windows during hold — MRU unchanged until unlock
- Close selected mid-session — no crash; prune or cancel
- Multi-monitor scope — candidates only on chosen monitor

## Anti-patterns

- Calling `focusWindow` inside cycle dispatcher
- Using only `m_windows` list order as “MRU”
- Ignoring lock-in during session
- Relying on IPC for in-process plugin timing

## Related project docs

- SPEC §2.4–2.5 (focus, history)
- ADR-001, ADR-002, ADR-003
- ARCHITECTURE HistoryTracker section
