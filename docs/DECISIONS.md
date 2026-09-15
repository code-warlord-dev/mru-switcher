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

---

## ADR-015: Candidate order = plugin-owned MRU first, adapter enumeration appended

**Status:** Accepted  

**Context:**  
M2 wired `HyprlandWindowSource::candidates()` straight to `Desktop::History::windowTracker()->fullHistory()`, so snapshot order came from the compositor and the plugin-owned `HistoryTracker` (debounce + lock-in, ADR-003) had no effect on Alt+Tab. The same adapter also filtered candidates through `registry_.is_known()`, which is empty right after plugin load, so every window opened before load was invisible until focused again. Reproduced in a nested session on the pinned Hyprland (`efb5099…`, v0.56.2): three windows open, `hyprctl plugin load` → `ok`, then `hyprctl dispatch 'mru:cycle next'` → `no windows`; after opening one more window post-load the same command returns `ok`.

**Decision:**  
1. The plugin-owned `HistoryTracker` order is the **primary** candidate order (REQ-H-004a). The adapter enumerates in-scope windows only as a **fallback tail** and never reorders the tracker prefix.  
2. The adapter registers every window it enumerates (“register on sight”), so windows that existed before plugin load become known on the first snapshot build (REQ-H-004b); the `is_known` filter is removed.  
3. The ordering/merging logic lives in a Hyprland-free helper (`mru::plugin::merge_mru_order`) so it is unit-testable without the compositor (ADR-007).  
4. Adapter-side validity for M2 `global` scope = window exists, is registered, and `!isHidden()` (REQ-SNAP-002 subset); monitor/workspace/visible/class predicates are M3 and extend the same enumerate → filter → merge path.

**Consequences:**  
- Debounce and lock-in now visibly affect Alt+Tab order, which is what ADR-003 promises.  
- The first Alt+Tab after login or plugin load works with pre-existing windows.  
- M3 scope filters plug into the fallback enumeration instead of replacing the ordering model.  
- The registry keeps one entry per address with a generation counter (ADR-013) and now also tracks registration sequence for deterministic fallback ordering.

## ADR-016: M3 design gate — weak-lock identity, scope predicate, config v2

**Status:** Accepted

**Context:**  
The M2 audit (Q1–Q5 design gate, auditor-01 §14; auditor-02 "Ответы на Q1–Q5") resolved the M3 architecture before implementation. Four threads converge here:

1. **HIGH-3 (registry lifetime):** `#7` switched `WindowIdentityRegistry` to weak `PHLWINDOWREF` with `lock()`-based ABA protection and `prune_closed()`, but the *normative* validity rule was left open: "валидность identity определяется `lock()` слабой ссылки, а не флагом `closed`" (auditor-02) had to be stated as a decision, not an accident of the fix.
2. **Q1 (scope model):** scope predicates must stay in the domain, tested free of the compositor (the `merge_mru_order` pattern, ADR-015).
3. **Q2/Q3 (special workspaces + `app`):** SPEC leaves scratchpad-in-`global`/`monitor`/`workspace` undecided and is silent on byte-exact class comparison.
4. **Q4/Q5 (config surface + hyprlang V2):** full config surface now, hyprlang V2 migration before behavioral scope work.

**Decision:**

1. **Weak-lock identity is normative.** A `WindowRef` with a live weak reference (`lock()` non-null) is **valid** regardless of the `closed`/destroy flag; `resolve()` returning empty means the identity is invalid. The `closed` flag only controls entry cleanup and pruning — it never decides validity (REQ-H-003, REQ-RE-003). ABA is prevented by the generation counter (ADR-013) paired with the weak `lock()` check at the same address.
2. **Scope model:** a pure, Hyprland-free predicate `bool scope_matches(Scope, const WindowMeta&, const FocusContext&)` in the domain core (ADR-007). `WindowMeta` carries **opaque** `monitor_id`/`workspace_id` (`uint64`), `mapped`, `hidden`, and `app_class`; the domain only compares, never interprets Hyprland types. `FocusContext` (current monitor, current workspace, active workspace set) is captured **once at snapshot time** and passed by value — never recomputed per window. The adapter's single duty is `PHLWINDOW → WindowMeta`.
3. **Special workspaces (scratchpad) in all five scopes:** a window on a special workspace is a candidate in *any* scope **only while** that workspace is currently shown on a monitor — the same rule as `visible`. A hidden scratchpad is excluded from `global`, `monitor`, `workspace`, and `visible` alike. Rationale: "you see it — it's in the ring; you don't — it isn't"; the alternative "always exclude scratchpad" contradicts user expectation for Alt+Tab.
4. **`app` scope:** compares `candidate.app_class == focus.app_class` **by byte, case-sensitively** (pinned in SPEC so nobody "fixes" it to case-insensitive later). Empty focus class degrades to `global`; empty candidate class never matches. The focused window is intentionally a candidate itself (with `start_offset=second` this gives the in-app toggle).
5. **Config surface (Q4):** full `plugin:mru-switcher:` surface registered in M3; reserved-not-implemented keys (e.g. `external_socket`) are registered but **documented in USER.md and API.md as `reserved — no effect until M5`** so `hyprctl getoption` matches the README.
6. **hyprlang V2 (Q5) precedes scope work.** Sequence: (0) config-v2 migration in isolation, (1) scope-predicate (pure domain + tests), (2) scope-adapter (`WindowMeta`, remaining keys, special-workspace rule). Rationale: isolate `.so` load-time config risk (past `bad_any_cast`, #3) from behavioral changes.
7. **`m_isMapped` completes `is_candidate()`** next to `!isHidden()` so the check matches REQ-SNAP-002 exactly (the L-8 fix applied the mapped bit; M3 keeps it in the predicate path).

**Consequences:**
- Registry validity no longer depends on close-event ordering; a missed/delayed `close` cannot make a dead window valid, and a strong ref can no longer mask ABA (H-3 fixed normatively, not accidentally).
- Scope behavior is unit-testable without the compositor; only the `PHLWINDOW → WindowMeta` mapping needs nested smoke (explicitly accepted risk, tracked in issue #12-style follow-ups).
- Scratchpad/`app` semantics become deterministic and pinned in SPEC, closing the Q2/Q3 holes before user reports.
- M3 work is sequenced into three small branches (config-v2 → scope-predicate → scope-adapter), each independently reviewable/mergeable.
