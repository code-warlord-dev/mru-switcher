# MRU Window Switcher — Roadmap

**Product:** Niri-style MRU Alt+Tab for Hyprland  
**Document status:** Living  
**Last updated:** 2026-09-13

---

## Vision

Deliver a production-grade window switcher that matches (and, where useful, exceeds) Niri’s recent-windows behaviour: frozen MRU snapshot, apply-on-release, focus lock-in with debounce, multi-scope filtering, and pluggable UI — implemented as a native Hyprland plugin with a clean domain boundary and testable core.

---

## Principles

1. **Correctness before chrome** — session invariants and history lock-in ship before fancy overlays.
2. **API stability for users** — dispatcher names and config keys are contracts; break only with major version + migration notes.
3. **Hyprland-friendly** — prefer `Event::bus` and public Plugin API; hooks only behind flags.
4. **Testable domain** — pure logic has unit tests without linking the compositor.
5. **Incremental value** — each milestone is usable on its own.

---

## Milestones

### M0 — Foundations (docs + contracts)

**Status:** Done (this documentation set)

- [x] ARCHITECTURE.md
- [x] DECISIONS.md (ADRs)
- [x] USER.md
- [x] DIAGRAMS.md
- [x] SPEC.md (detailed specification)
- [x] ROADMAP.md
- [x] Public dispatcher & config contracts frozen for v0.x

**Exit criteria:** Specs reviewable; no code required.

---

### M1 — Domain + headless core

**Goal:** Pure C++ domain and session logic with unit tests.

Deliverables:

- Domain types: `WindowRef`, `Snapshot`, `Selection`, `Session`, `SessionPolicy`, `Scope`
- `SessionController` state machine (Idle ↔ Active)
- `HistoryTracker` with debounce + lock-in via `SchedulerPort` + `FakeClock` in tests
- `WindowRef` (`address` + `generation`) and validity rules in domain tests
- Apply-after-invalidation (prune → clamp → apply or cancel) unit tests
- `ScopeResolver` interface (stub compositor port for tests)
- Unit tests: cycle, wrap/no-wrap, start_offset, prune-on-close, lock-in, debounce cancel/replace, identity mismatch, empty snapshot

**Exit criteria:**

- All domain tests green without Hyprland linked
- Invariants from ARCHITECTURE §4.2 enforced in code

**Depends on:** M0

---

### M2 — Hyprland MVP plugin

**Goal:** Loadable `.so` with real focus switching; **Null UI only**.

Deliverables:

- Plugin facade: hash check, `PLUGIN_INIT` / `EXIT`
- `addDispatcherV2`: `mru:cycle`, `mru:apply`, `mru:cancel`
- `Event::bus` subscriptions: `window.active`, `window.close`
- Adapters: `HyprlandCompositorPort`, `FocusGateway`, `ConfigPort`, `SchedulerPort` (main-thread timer)
- WindowRef registry with generation (identity validation)
- Null UI; config default `ui = null`; REQ-UI-002 fallback if user sets `border` early
- Default scope: `global`
- `restore_focus_on_cancel` wired through FocusGateway (domain semantics already M1)
- Manual smoke on nested Hyprland against **pinned** Hyprland revision (`docs/COMPAT.md`)

**Exit criteria:**

- Recommended binds work end-to-end (Null UI is acceptable)
- Snapshot freezes; apply-on-release works
- History not updated during active session
- Apply-after-invalidation matches SPEC §2.8
- Debounce uses SchedulerPort; no use-after-unload of timers

**Depends on:** M1

---

### M3 — Scopes + config + status

**Goal:** Full config surface and all scopes. Scope behavior, validity, and config surface decisions are defined in **ADR-016** (weak-lock identity __1__, scope predicate __2__, special workspaces __3__, app class __4__, config keys __5__, migration sequence __6__-__7__).

Deliverables:

- Pure domain `scope_matches` predicate with `WindowMeta`/`FocusContext` (ADR-016 __2__); `is_candidate()` requires `m_isMapped` and `!isHidden()` (L-8, REQ-SNAP-002)
- Special workspace rule: window on special workspace is candidate only while shown (ADR-016 __3__, REQ-SC-002a)
- `app` scope: byte-exact class compare, empty focus → global (ADR-016 __4__, REQ-SC-002b)
- Identity validity by weak `lock()` not `closed` flag (ADR-016 __1__, REQ-ID-006)
- hyprlang V2 migration **before** scope work (ADR-016 __6__)
- Remaining `plugin:mru-switcher:*` keys (Q4, reserved keys labeled); `external_socket` registered, documented reserved (ADR-016 __5__)
- Scopes: `global` | `monitor` | `workspace` | `visible` | `app`
- `mru:status` dispatcher
- Config reload behaviour documented and tested
- USER.md validated against real behaviour

**Exit criteria:**

- Each scope has at least one automated or manual test case
- Invalid scope/args return clear `SDispatchResult` errors

**Depends on:** M2

---

### M4 — Border UI + polish

**Goal:** Usable visual feedback without external processes.

Deliverables:

- `BorderHighlightUI` (border colour / opacity via public mechanisms)
- Selection highlight updates on cycle
- Clear highlight on apply/cancel
- UI polish only for cancel path (restore-on-cancel behavior already in M1/M2)

**Exit criteria:**

- Daily-driver usable with `ui = border`
- No focus flicker or stuck borders after cancel

**Depends on:** M3

---

### M5 — External overlay (optional path)

**Goal:** Path for rich UI (previews, search) without bloating the plugin.

Deliverables:

- `ExternalOverlayUI` + socket protocol (versioned messages)
- Reference overlay stub (any language) or documented protocol only
- Graceful fallback to Null/Border if overlay dies

**Exit criteria:**

- Protocol documented in SPEC appendix
- Plugin remains functional if overlay is absent

**Depends on:** M4

---

### M6 — Hardening & v1.0

**Goal:** Production release.

Deliverables:

- CI: unit tests + nested Hyprland smoke (where feasible)
- hyprpm manifest / commit pins
- Changelog, versioning policy (semver)
- Stress cases: rapid Tab, window close mid-session, monitor disconnect
- SECURITY notes (plugin runs in-process — trust model)
- Freeze dispatcher/config contracts for 1.x

**Exit criteria:**

- Tagged `v1.0.0`
- No known session-invariant violations
- Docs match implementation

**Depends on:** M4 (M5 optional for 1.0)

---

## Out of scope (near term)

| Item | Rationale |
|------|-----------|
| Live window previews inside the plugin | Belongs to overlay / separate tool |
| Replacing Hyprland built-in focus history | We mirror + lock-in; we do not own compositor history |
| Non-C++ native plugin ABI | Not supported by Hyprland |
| Guaranteed function-hook behaviour on non-x86_64 | API limitation |
| Plugin-defined window rules | Hyprland limitation; track upstream if needed |

---

## Versioning policy

- **0.x** — contracts may change with changelog entry; prefer additive changes.
- **1.x** — dispatcher names, argument grammar, and config keys are stable; removals require major bump.
- Plugin binary is always tied to Hyprland header hash; “API version” ≠ “works on every commit without rebuild”.

---

## Risk register (summary)

| Risk | Mitigation | Milestone |
|------|------------|-----------|
| Focus history API churn | Adapter + fallback to event-sourced list | M2 |
| ABI break on Hyprland update | Hash check; hyprpm pins; rebuild docs | M2+ |
| bindrt / modifier release edge cases | Document; test left/right Alt | M2–M4 |
| Overlay process failure | Fallback UI; session logic independent | M5 |
| Scope edge cases (special WS, empty) | Explicit policy in SPEC; tests | M3 |

---

## Suggested order of work for implementers

1. Domain + tests (M1)  
2. Minimal plugin + Null UI (M2)  
3. Config + scopes (M3)  
4. Border UI (M4)  
5. Overlay protocol if needed (M5)  
6. CI, hyprpm, tag 1.0 (M6)

---

## Success metrics (qualitative)

- User can Alt+Tab across workspaces/monitors without list reordering mid-hold.
- Release of Alt always ends in a single, intentional focus change.
- Debounce prevents FFM spam from poisoning MRU.
- Plugin fails fast and loudly on header mismatch instead of crashing later.
