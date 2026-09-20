---
name: mru-switcher
description: Implements and reviews the Niri-style MRU Alt+Tab Hyprland plugin — snapshot sessions, apply-on-release, history lock-in, scopes, UI ports, and SPEC compliance. Use when working on mru-switcher code, dispatchers mru cycle apply cancel, SessionController, or aligning behavior with project docs.
metadata:
  level: expert
  version: "1.0"
  domain: mru-switcher
---

# MRU Switcher (Niri-style Alt+Tab)

## Overview

This skill encodes the **project contracts** for the MRU window switcher plugin. Behavior is defined by `docs/SPEC.md` and design by `docs/ARCHITECTURE.md`. Do not invent alternate session semantics without an ADR.

## When to use

- Implementing M1–M6 from ROADMAP
- Changing `mru:cycle` / `mru:apply` / `mru:cancel` behavior
- SessionController, Snapshot, Selection, HistoryTracker
- Scope filters or UI backends
- Reviewing PRs against SPEC requirement IDs

## Core invariants (must hold)

1. At most **one** active session.
2. **Snapshot** is fixed at session start (only pruning allowed).
3. **`mru:cycle` never focuses** — selection is virtual.
4. Focus only via **FocusGateway** on apply (or explicit restore-on-cancel).
5. While Active, the MRU list is **frozen** — **lock-in is mandatory and unconditional**
   (REQ-H-001/010, ADR-021; the `lock_history_on_session` key is reserved and ignored).
   A promotion still pending from the previous session is flushed at session start
   (REQ-H-011).
6. Domain layer has **no** Hyprland types (`PHLWINDOW`, `g_p*`).

## Session machine

```text
Idle --cycle(success)--> Active --apply|cancel|empty--> Idle
Active --cycle--> Active (index only)
```

### First cycle

1. Resolve scope (arg or `default_scope`)
2. Flush a pending debounced promotion (`HistoryTracker::flush_pending`, REQ-H-011) —
   committed before candidates are read so the new snapshot sees it at the head
3. Build candidate list from HistoryTracker order ∩ scope ∩ validity
4. If empty → fail dispatcher (`no windows`)
5. Create Snapshot + Selection (`start_offset` first|second)
6. UI `on_session_start`; lock-in engages unconditionally (REQ-H-001)
7. State = Active

### Subsequent cycle

- Move index with wrap policy; UI `on_selection_changed`

### Apply

- Focus selected if valid (else prune / cancel if empty)
- UI `on_session_end(Applied)`
- Unlock history; Idle

### Cancel

- Optional `restore_focus_on_cancel` → focus `session_origin`
- UI `on_session_end(Cancelled)`
- Unlock history; Idle

## Dispatcher grammar

```text
mru:cycle [next|prev] [global|monitor|workspace|visible|app]
mru:apply
mru:cancel
mru:status
```

Return proper `SDispatchResult` errors for bad args / empty candidates.

## HistoryTracker

- Seed from `Desktop::History::windowTracker()->fullHistory()` when available
- Else maintain list from `window.active`
- Debounce commits with `debounce_ms`
- Never update while a session is Active: lock-in is mandatory (REQ-H-001)
- At session start `flush_pending()` commits a still-pending promotion immediately
  (validity guard included), before the snapshot is built (REQ-H-011, ADR-021)

## Scopes

| Scope | Rule |
|-------|------|
| global | valid windows |
| monitor | current monitor |
| workspace | current workspace |
| visible | on visible workspaces |
| app | same class as focus at snapshot time |

## UI ports

Implement `UIPort` — Null, Border, External. UI failures must not break apply/cancel.

## Implementation order (ROADMAP)

1. Domain + unit tests (M1)
2. Facade + Null UI MVP (M2)
3. Scopes + config (M3)
4. Border UI (M4)
5. External protocol (M5)
6. Hardening v1.0 (M6)

## Anti-patterns

- Focusing on every Tab
- Rebuilding snapshot each cycle
- Updating MRU during Active session
- Putting `PHLWINDOW` in domain headers
- Registering config outside `PLUGIN_INIT`
- Skipping hash check

## Spec traceability

Map tests to SPEC IDs (`T-S-01`, `T-H-01`, …). Behavior changes require SPEC and/or ADR updates.

## Read first

1. `docs/SPEC.md`
2. `docs/ARCHITECTURE.md`
3. `docs/DECISIONS.md`
4. `docs/HYPRLAND-PLUGIN-SYSTEM.md`
