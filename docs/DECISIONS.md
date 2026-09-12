# Architecture Decision Records — MRU Switcher

This file records significant design decisions for the MRU window switcher plugin.  
Each ADR is immutable once accepted; superseding decisions get a new number and a link back.

---

## ADR-001: Snapshot instead of live candidate list

**Status:** Accepted  

**Context:**  
Live lists (re-querying windows on every Tab) cause the order to jump when focus or workspace state changes mid-session. Niri and classic desktop Alt+Tab freeze the list at open time.

**Decision:**  
On first `mru:cycle` of a session, build an immutable `Snapshot`. Subsequent cycles only move `Selection.index` inside that snapshot.

**Consequences:**  
- Predictable UX.  
- Simple mental model.  
- Closed windows must be pruned; empty snapshot ends the session.

---

## ADR-002: Apply-on-release (virtual selection)

**Status:** Accepted  

**Context:**  
Focusing on every Tab drags the user across workspaces/monitors and pollutes history.

**Decision:**  
Selection is virtual. Real focus is applied only in `mru:apply` (typically bound with `bindrt` on modifier release).

**Consequences:**  
- Clean history and no intermediate workspace hops.  
- Requires correct `bindrt` / release binding documentation.  
- UI must show current selection without relying on real focus (border or overlay).

---

## ADR-003: Own HistoryTracker + debounce + session lock-in

**Status:** Accepted  

**Context:**  
Hyprland updates focus history aggressively (FFM, clicks, keybinds, workspace changes). Niri uses a debounce before committing a window to the recent list. Intermediate focuses during Alt+Tab must not reorder the list.

**Decision:**  
- Maintain our own MRU list (seeded from `Desktop::History::windowTracker()->fullHistory()` when available).  
- Debounce updates with `debounce_ms`.  
- While `Session == Active`, ignore focus events for history updates (lock-in).

**Consequences:**  
- Niri-like behaviour.  
- Testable in isolation.  
- Slight divergence from compositor’s internal history is possible and accepted.

---

## ADR-004: UI as Strategy (Null / Border / External)

**Status:** Accepted  

**Context:**  
Visual feedback is not part of core switching logic. Users and distros want different UIs; tests need a no-op UI.

**Decision:**  
`UIPort` with three initial adapters: Null, BorderHighlight, ExternalOverlay (IPC).

**Consequences:**  
- Core remains headless-testable.  
- Overlay can evolve without touching session logic.  
- External process adds operational complexity (documented; optional).

---

## ADR-005: Prefer Event::bus; function hooks only as last resort

**Status:** Accepted  

**Context:**  
`registerCallbackDynamic` is deprecated. Function hooks exist only on x86_64 and break easily across commits. Official plugins (e.g. hyprfocus) already use `Event::bus()->m_events.window.active.listen(...)`.

**Decision:**  
- Primary integration: `Event::bus()`.  
- Function hooks allowed only behind an explicit feature flag and only on x86_64.

**Consequences:**  
- Better forward compatibility.  
- Some advanced interception scenarios may be harder without hooks.

---

## ADR-006: Single FocusGateway

**Status:** Accepted  

**Context:**  
Scattered `focusWindow` / `fullWindowFocus` calls make reasoning and testing hard and increase the chance of inconsistent focus reasons.

**Decision:**  
All focus changes go through one `FocusGateway` adapter.

**Consequences:**  
- Auditable focus path.  
- Easy to log / metric / harden later.

---

## ADR-007: Domain has no Hyprland types

**Status:** Accepted  

**Context:**  
Plugin ABI is not stable; internal types change. Pure logic should be unit-testable without linking Hyprland.

**Decision:**  
Domain uses only `WindowRef` and plain data. Hyprland types stay in adapters.

**Consequences:**  
- Fast unit tests.  
- Clear boundary for future ports (if any).  
- Small mapping layer cost (accepted).

---

## ADR-008: Config only in PLUGIN_INIT under `plugin:mru-switcher:`

**Status:** Accepted  

**Context:**  
Hyprland allows adding config values only during plugin init and requires the `plugin:` namespace.

**Decision:**  
Follow the API strictly; document all keys; no runtime registration of new keys.

**Consequences:**  
Compliant with current Plugin API; predictable config surface.

---

## ADR-009: Dispatcher namespaced as `mru:*`

**Status:** Accepted  

**Context:**  
Plugin dispatchers share a global namespace with core and other plugins. Collisions must be avoided.

**Decision:**  
All dispatchers use the `mru:` prefix (`mru:cycle`, `mru:apply`, `mru:cancel`, `mru:status`).

**Consequences:**  
Clear ownership; low collision risk; consistent with patterns like `hyprexpo:expo`, `hy3:*`.

---

## ADR-010: start_offset = second by default

**Status:** Accepted  

**Context:**  
Classic Alt+Tab and Niri select the *previous* window on the first press, not the current one.

**Decision:**  
Default `start_offset = second`. Configurable to `first` for users who prefer different behaviour.

**Consequences:**  
Familiar desktop behaviour out of the box; still tunable.


---

## ADR-011: Default UI is null until M4; unavailable backends fall back to null

**Status:** Accepted  

**Context:**  
M2 delivers Null UI only, but earlier drafts defaulted `ui = border`, contradicting ROADMAP.

**Decision:**  
Config default is `null`. Requesting `border` or `external` before that backend exists falls back to `null` (REQ-UI-002). M4 may change the default to `border` with a CHANGELOG entry.

**Consequences:**  
No contradiction between M2 and SPEC; users can set binds early without requiring border code.

---

## ADR-012: SchedulerPort for debounce

**Status:** Accepted  

**Context:**  
Debounce behavior was specified without an execution mechanism, risking improvised timers and unload races.

**Decision:**  
Introduce `SchedulerPort` (`schedule_after` / `cancel`) owned by adapters. HistoryTracker depends on the port only. Production: compositor main-thread timer. Tests: `FakeClock`. Single pending debounce job; cancel on replace, unload, and invalid window (REQ-H-006–009).

**Consequences:**  
M1 can implement HistoryTracker without Hyprland; M2 wires the real scheduler.

---

## ADR-013: WindowRef is address + generation

**Status:** Accepted  

**Context:**  
Raw addresses can be recycled after close; focusing by address alone risks focusing the wrong window.

**Decision:**  
`WindowRef = { address, generation }`. Adapter registry bumps generation on map/track. FocusGateway validates both fields (REQ-ID-*, REQ-F-005).

**Consequences:**  
Slightly more adapter bookkeeping; eliminates stale-address focus bugs.

---

## ADR-014: Apply-after-invalidation is prune-all then clamp index

**Status:** Accepted  

**Context:**  
SPEC required “attempt apply on new selection” without defining which index.

**Decision:**  
On apply with invalid selection: prune all invalid snapshot entries; clamp index to `min(old_i, len-1)`; focus if non-empty else Cancelled. At most one focus per apply; one UI end event (REQ-F-006/007, §2.8).

**Consequences:**  
Predictable UX; simple to test (T-F-03/04).
