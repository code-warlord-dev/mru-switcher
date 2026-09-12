---
name: cpp-plugin-architecture
description: Expert hexagonal or ports-and-adapters architecture for C++ in-process compositor plugins with pure domain, adapters, and testable cores. Use when structuring Hyprland or similar plugins, separating domain from PHLWINDOW globals, designing FocusGateway, or writing unit tests without linking the compositor.
metadata:
  level: expert
  version: "1.0"
  domain: architecture
---

# C++ In-Process Plugin Architecture

## Overview

Compositor plugins that mix domain logic with `g_pCompositor` and raw `PHLWINDOW` become untestable and brittle under ABI churn. Use a **strict boundary** — pure domain + ports + adapters.

## When to use

- Structuring a new Hyprland plugin beyond a single `main.cpp`
- Introducing SessionController / services with unit tests
- Refactoring tangled plugin code
- Defining UI or compositor ports

## Layer rules

```text
L0  Plugin facade     PLUGIN_INIT, HyprlandAPI, Event::bus wiring
L1  Application       use-cases / controllers (session, tracking)
L2  Domain            pure types + invariants (no compositor headers)
L3  Adapters          implement ports using Hyprland / OS / UI
```

**Dependency direction:** L0/L1 → L2; L3 implements interfaces declared for L1/L2.  
Domain **never** includes Hyprland headers.

## Ports (interfaces)

Typical ports for window tooling:

| Port | Responsibility |
|------|----------------|
| `CompositorPort` | list windows, monitor/workspace context, subscribe lifecycle |
| `FocusGateway` | **single** path to apply focus |
| `ConfigPort` | typed access to plugin config |
| `UIPort` | session_start / selection_changed / session_end |
| `ClockPort` | virtual clock for debounce tests |

Keep interfaces small. Prefer return values over throwing across the facade boundary; map to `SDispatchResult` at L0.

## Domain patterns

- **Value objects** — `WindowRef` (stable id string/address), `Scope`, `Selection`
- **Entities / aggregates** — `Snapshot`, `Session`
- **Invariants enforced in domain methods** — e.g. index always in range or session ends
- **No singletons** in domain; inject trackers/policies

## FocusGateway discipline

All focus side effects go through one adapter method. Benefits:

- Auditable apply path
- Easy to assert in tests (mock gateway)
- Consistent focus reason if API allows

## Testing strategy

| Layer | How |
|-------|-----|
| Domain | Unit tests, no Hyprland link |
| History debounce | Fake `ClockPort` |
| Session machine | Table-driven tests for cycle/apply/cancel |
| Adapters | Nested Hyprland integration / manual |

## File layout (suggested)

```text
include/mru/domain/...
include/mru/ports/...
src/domain/...
src/application/...
src/adapters/hyprland/...
src/adapters/ui/...
src/plugin/main.cpp
tests/domain/...
```

## Anti-patterns

- `PHLWINDOW` in domain headers
- Calling `focusWindow` from three different files
- Static global session state mutated from event and dispatcher without a controller
- UI logic embedded in dispatcher parsers
- Tests that require a live compositor for pure policy checks

## Fit to MRU project

Follow `docs/ARCHITECTURE.md` modules and ADR-006 / ADR-007. SPEC remains normative for behavior.
