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
Config default is `null`. Requesting `border` or `external` before that backend exists falls back to `null` (REQ-UI-002). M4 may change the default to `border` with a CHANGELOG entry. ADR-017 extends this: the default stays `null` in M4, and flipping it is a separate decision + CHANGELOG note, not an M4 exit criterion.

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

---

## ADR-017: Border UI (M4) and border style interface

**Status:** Accepted (design gate for M4)

**Related:** ADR-004 (UI as Strategy), ADR-011 (default null + fallback), ROADMAP M4, SPEC §5

**Context:**  
M4 requires usable visual feedback without an external process:

- Highlight the virtually selected window while Alt is held.
- Update highlight on every `mru:cycle`.
- Clear highlight on apply, cancel, and plugin unload.
- No focus flicker and no stuck borders after cancel (ROADMAP M4 exit criteria).

ADR-004 already defines `UIPort` with Null / Border / External strategies. M2–M3 ship only `NullUI`; `ui = border | external` falls back to null (ADR-011, REQ-UI-002).

We also want optional style evolution (`pulse`, `dim`, …) without blocking M4 exit or breaking the config contract. Scope for M4 is therefore:

- **A (must):** solid border highlight end-to-end.
- **B (foundation only):** style interface + config tokens; only `solid` has effect in M4. Other styles are follow-up PRs.

Constraints:

- Prefer **public** Hyprland mechanisms on the pinned revision (COMPAT: v0.56.2 / `efb5099…`).
- Domain stays free of Hyprland types (ADR-007).
- UI failures must not abort session commands (REQ-UI-001).
- No live window previews inside the plugin (ROADMAP non-goal).

**Decision:**

1. **Backend selection**

   | Config `ui` | Behaviour in M4 |
   |-------------|-----------------|
   | `null` | `NullUI` (no visual side effects) |
   | `border` | `BorderHighlightUI` |
   | `external` | Fallback to `null` + warn-once until M5 (REQ-UI-002) |

   - Default remains **`null`** for the M4 release line unless a separate CHANGELOG decision flips it to `border` (ADR-011 process).
   - Backend is chosen when building plugin state / at session policy snapshot time; reload applies to the **next** session only (REQ-S-009, REQ-CFG-002).

2. **Highlight mechanism (pinned Hyprland)**

   - Primary path: public dynamic window properties equivalent to user-facing `setprop` / window-rule dynamic effects for border colour (and optionally border size) on the resolved window identity.
   - Non-goals for primary path: function hooks; mandatory `IHyprWindowDecoration` implementation.
   - Fallback: if the primary path is unavailable or fails at runtime → behave as null for that session (or that highlight attempt), log, warn-once where appropriate. Document the exact symbols and any decoration fallback in `docs/COMPAT.md` (see R0 memo `docs/agent-state/research/2026-09-17-m4-border-api.md`).
   - Highlight **must not** change real compositor focus (REQ-F-003, REQ-UI-006).

   Concrete API binding is adapter-private and version-gated in COMPAT; this ADR only requires "public props first, fail-soft".

3. **Style interface (foundation for B)**

   - Introduce config key `border_style` with normative tokens:
     - **`solid`** — implemented in M4 (required).
     - **`pulse`**, **`dim`** — reserved names; until implemented, treat as **`solid`** (optional warn-once).
     - Any other token → **`solid`** + warn-once (REQ-UI-007).
   - Style behaviour is implemented **inside** the border UI adapter (strategy or equivalent), not in `SessionController`.
   - Adding a new implemented style later is an additive change: implement the strategy, document in SPEC/USER/CHANGELOG; no dispatcher renames.

4. **Lifecycle and restore**

   `BorderHighlightUI` implements `UIPort`:

   | Call | Required behaviour |
   |------|-------------------|
   | `on_session_start(snapshot, index)` | Resolve `snapshot[index]`; apply highlight; remember prior border state for restore |
   | `on_selection_changed(index)` | Clear previous highlight (restore that window); apply to new index |
   | `on_session_end(reason)` | Clear **all** plugin-owned overrides for the session |
   | Plugin teardown / `PLUGIN_EXIT` | Same full clear; no use-after-free of window refs |

   Rules:

   - Resolve identities via the existing registry / FocusGateway validity rules (address + generation, weak-lock). Invalid target → skip highlight, do not crash (REQ-UI-004 path + T-UI-06).
   - Store enough prior state to restore colour (and size if modified). If restore is impossible, best-effort clear + error log; **do not** fail apply/cancel.
   - At most one window carries the plugin's "selected" highlight at a time during Active.

5. **Configuration surface (M4)**

   Registered only in `PLUGIN_INIT` under `plugin:mru-switcher:` (ADR-008, hyprlang V2):

   | Key | Meaning | Default | M4 notes |
   |-----|---------|---------|----------|
   | `ui` | Backend | `null` | existing |
   | `border_style` | Style token | `solid` | only `solid` has distinct effect |
   | `border_color` | Highlight colour | documented implementation default; project default `0xffffd9a0` (hex `0xAARRGGBB`, high-visibility accent) | format accepted by pin; document in USER/API |
   | `border_size` | Optional size override | `-1` | `-1` = do not change window border size |

   Reload semantics: non-debounce keys apply to the **next** session only (REQ-UI-009).

6. **Failure policy**

   - Highlight/API/restore failure → log; continue session logic; user still gets correct focus on apply when focus path succeeds.
   - Never leave a session stuck Active because UI failed.
   - Unload mid-session: end session as today (`PluginShutdown`) **and** clear highlights.

**Consequences:**

### Positive

- M4 exit criteria achievable with a single solid style.
- Style tokens and UIPort boundary allow pulse/dim without redesign.
- Fail-soft UI matches existing null fallback culture (ADR-011).
- Domain and focus path remain testable without border code.

### Negative / risks

- Pin-specific prop API may be incomplete; R0 must record COMPAT truth.
- Incorrect restore → stuck borders (mitigated by explicit end/unload clear + nest checklist).
- Colour format / gradient support varies by Hyprland version — keep M4 defaults simple (solid colour).

### Follow-ups (not M4 exit)

- Implement `pulse` / `dim` as separate PRs + SPEC amendments.
- Optional default `ui = border` after field validation.
- M5 `ExternalOverlayUI` remains independent.

---

**Compliance mapping:**

| Topic | REQ / ADR |
|-------|-----------|
| UIPort strategy | ADR-004 |
| Default null + unavailable backend | ADR-011, REQ-UI-002 |
| No abort on UI failure | REQ-UI-001 |
| Border backend | REQ-UI-003 |
| Update on cycle / clear on end | REQ-UI-004, REQ-UI-005 |
| No focus on cycle | REQ-F-003, REQ-UI-006 |
| Style tokens | REQ-UI-007 |
| Config keys | REQ-UI-008 |
| Reload next-session | REQ-S-009, REQ-CFG-002, REQ-UI-009 |
| Identity validity / weak-lock | REQ-UI-010, ADR-013/016 |
| Public props first, COMPAT-pinned | REQ-UI-011, COMPAT |

**References:**

- `docs/SPEC.md` §5 — UI requirements (REQ-UI-001..011)
- `docs/ARCHITECTURE.md` §8 — UIPort
- `docs/COMPAT.md` — pin + border mechanism row (0.56.2 / `efb5099…`)
- `docs/agent-state/research/2026-09-17-m4-border-api.md` — R0 memo (concrete pin symbols)
- `docs/ROADMAP.md` — M4 Border UI + polish
- `include/mru/domain/ui_port.hpp` — UIPort header

---

## ADR-018: External overlay (M5) — AF_UNIX stream socket protocol

**Status:** Accepted (design gate for M5)

**Related:** ADR-004 (UI as Strategy), ADR-011 (default null + fallback), ADR-016 __5__ (`external_socket` registered/reserved), ROADMAP M5, SPEC §5 + §12 Appendix B

**Context:**  
M5 adds the optional `external` UI backend: a rich overlay (previews, search, click-to-select) that lives **outside** the compositor process. ROADMAP M5 exit criteria: protocol documented in SPEC appendix; plugin remains functional if the overlay is absent. Three constraints shape the decision:

1. **REQ-PERF-001 / REQ-PERF-003** — no blocking I/O, no socket IPC on the `mru:*` dispatcher hot path; M5 socket I/O is best-effort, non-blocking / timed.
2. **REQ-RE-001** — all controller mutations run on the compositor main thread.
3. **CI `plugin-guards`** — non-facade `src/plugin/*` files must stay Hyprland-free (ADR-007), so the socket server must be POSIX-only with the event-loop wiring in the hypr adapter.

The R0 memo (`docs/agent-state/research/2026-09-19-m5-overlay-socket-api.md`) verified that the pinned Hyprland (v0.56.2 / `efb5099`) exposes `CEventLoopManager::doOnReadable(CFileDescriptor fd, fn)` — a Wayland event-loop fd watcher (POLLIN/READABLE) that runs on the main thread and takes ownership of the fd. No timer polling needed.

**Decision:**

1. **Transport: AF_UNIX SOCK_STREAM, single client.** The plugin binds the socket at `plugin:mru-switcher:external_socket` and listens. The **first** accepted client is the overlay; further clients are closed. All fds are `O_NONBLOCK`.

   - Why stream, not datagram: natural connect/peer-lost detection, backpressure signalling for best-effort drops, and no need to track `sockaddr` per sender. One overlay peer is the intended product shape for M5.
   - Empty `external_socket` or failed `bind`/`listen` ⇒ `ui=external` behaves as `null` + warn-once that session (REQ-UI-002; same fail-soft culture as ADR-011/017).

2. **Reads are event-driven; writes are best-effort.**

   - Listener fd and the accepted client fd are registered via `doOnReadable` (main thread, R0-F1). Accept and receive happen in those callbacks, non-blocking.
   - Outgoing messages: `send()` on the `O_NONBLOCK` stream fd; on any backpressure/error (`EWOULDBLOCK`, `EAGAIN`, `EINTR`, `EPIPE`, …) the line is **dropped** and logged at debug (R0-F4). A slow, dead, or absent peer can never stall `mru:*`.
   - Line framing: newline-delimited UTF-8, one JSON object per line (SPEC Appendix B, made normative).

3. **Core/domain split (ADR-007).**

   - `mru_plugin_core` owns: `OverlayProtocol` (pure JSON encode/decode), `ExternalOverlayUI` (UIPort implementation that turns lifecycle calls into protocol lines over an abstract `OverlayTransport`), and `OverlaySocketServer` (POSIX AF_UNIX server: `socket/bind/listen/accept/send/recv`, non-blocking, line framing). All three are Hyprland-free and unit-testable without the compositor; the socket server is tested over a real loopback socket in CI.
   - `mru_plugin_hypr` owns `HyprlandOverlaySocket`: constructs the server and registers `doOnReadable` on the listener and client fds, wires a command sink to `SessionController`.
   - `PluginState` gains an `overlay_socket` member that **outlives** per-session UI backends (the `SessionUIBackendProxy` factory builds a fresh `ExternalOverlayUI` per session, like `BorderHighlightUI`). Teardown order: `ui` → `overlay_socket` → … (socket after UI, so a live backend never dereferences a destroyed transport; listener fds are closed before `PLUGIN_EXIT` returns).

4. **Peer commands are bounds-checked, never arbitrary.**

   | Peer message | Controller action |
   |--------------|-------------------|
   | `select index` | `SessionController::select_index(i)` — new domain op; Active-only and `i < size`; **virtual selection only** (REQ-F-003), never focuses |
   | `apply` | existing `SessionController::apply()` (idempotent on Idle) |
   | `cancel` | existing `SessionController::cancel()` (idempotent on Idle) |

   Unknown version/type, malformed JSON, or oversized lines are ignored and logged at debug; they must never affect session state (REQ-O-005). A hostile peer cannot focus an arbitrary window — only an in-snapshot index and apply/cancel of the current session (REQ-O-007; THREAT-MODEL gets a matching row).

5. **Fallback semantics.**

   - Backend construction fails (bind error, empty path, `ui=external` before M5 code) → `NullUI` + warn-once per plugin lifetime (REQ-UI-002).
   - Overlay peer absent or dies mid-session → session logic unaffected; outgoing lines are dropped best-effort. The next session simply re-probes the socket. This satisfies "plugin remains functional if overlay is absent" (ROADMAP M5).

6. **Config surface is unchanged.** `external_socket` was already registered (ADR-016 __5__); it now has effect. `ui` default stays **`null`** (ADR-011 process); `external` is opt-in. Reload semantics follow REQ-S-009/REQ-UI-009 (next session only).

**Consequences:**

### Positive

- REQ-PERF-001/003 honoured: zero blocking I/O on the dispatcher path; event-driven reads.
- The overlay is a real external process (any language) behind a versioned, documented protocol — previews/search stay out of the plugin (ROADMAP non-goal).
- Fail-soft fits the existing null-fallback culture (ADR-011/017).
- Domain/core stays Hyprland-free and unit-testable; the socket server gets a real loopback test in CI.
- `select_index` is a small, safely-reusable domain capability (bounds-checked, never focuses).

### Negative / risks

- STREAM + single client: a second overlay simply does not attach (documented).
- Best-effort sends can drop lines under backpressure (documented; overlay should not poll the peer path).
- Peer command timing: commands are processed on the main thread when readable; no ordering guarantee against concurrent keybinds beyond the compositor's own serialization (same as dispatchers, REQ-RE-004).
- POSIX syscalls must be guarded at the C-ABI boundary (HIGH-4 pattern) — a `send`/`recv` error must never throw into the compositor.

### Follow-ups (not M5 exit)

- Async/keyboard filtering from the overlay (search-as-you-type) — protocol extension, separate PR.
- Multiple concurrent peers or broadcast UI.
- Optional `v=2` fields (icons, monitor geometry) — additive.

---

**Compliance mapping:**

| Topic | REQ / ADR |
|-------|-----------|
| UIPort strategy | ADR-004 |
| Default null + unavailable backend fallback | ADR-011, REQ-UI-002 |
| ExternalOverlayUI as backend | REQ-O-002, REQ-O-003 |
| No abort on UI failure | REQ-UI-001 |
| Non-blocking / main-thread I/O | REQ-PERF-001/003, REQ-RE-001, REQ-O-006 |
| Peer input bounds-checked | REQ-O-004, REQ-O-007 |
| Unknown/malformed peer data ignored | REQ-O-005 |
| Teardown closes socket + listeners | REQ-O-008 |
| Reload next-session only | REQ-S-009, REQ-CFG-002, REQ-UI-009 |

**References:**

- `docs/SPEC.md` §5.3 — REQ-O-001..008; §12 Appendix B — frozen protocol
- `docs/ARCHITECTURE.md` §8 — UIPort; `include/mru/domain/ui_port.hpp`
- `docs/COMPAT.md` — pin + external mechanism row (0.56.2 / `efb5099…`)
- `docs/agent-state/research/2026-09-19-m5-overlay-socket-api.md` — R0 memo
- `docs/ROADMAP.md` — M5 External overlay

---

## ADR-019: External overlay fd watch — removable `wl_event_loop_add_fd`

**Status:** Accepted (implementation refinement of ADR-018; human design gate 2026-09-19)

**Related:** ADR-018 (External overlay — AF_UNIX stream socket protocol), ADR-007 (domain/plugin split), REQ-O-006, REQ-O-008, REQ-RE-001, REQ-PERF-001/003, ROADMAP M5, SPEC §5.3 + §12 Appendix B

**Date:** 2026-09-19

---

### Context

ADR-018 (Accepted) freezes the M5 external-overlay design:

- Transport: AF_UNIX `SOCK_STREAM`, single client, non-blocking.
- Reads event-driven on the compositor main thread.
- Writes best-effort (drop on backpressure / peer death).
- Core (`OverlaySocketServer`, protocol, `ExternalOverlayUI`) stays Hyprland-free; event-loop wiring lives in the hypr adapter.

The R0 memo (`docs/agent-state/research/2026-09-19-m5-overlay-socket-api.md`) verified that the pinned Hyprland (v0.56.2 / `efb50993780079460b0cbed1363e2166a2de1d9f`) exposes `CEventLoopManager::doOnReadable(CFileDescriptor fd, fn)`. ADR-018 and the original wording of **REQ-O-006** therefore named `doOnReadable` as the registration mechanism.

During implementation of `HyprlandOverlaySocket` it became clear that `doOnReadable`:

1. Takes ownership of the `CFileDescriptor`.
2. Does **not** return a handle that can later remove the watcher.

Consequence: when a peer disconnects (or the listener must be torn down), the fd-watch cannot be removed without leaking it until plugin unload / compositor restart. This violates the teardown contract of **REQ-O-008** (“Teardown closes socket + listeners”) and creates a long-lived resource leak on a process that is expected to run for days.

The underlying Wayland event loop (`wl_event_loop`) used by Hyprland **does** provide a removable primitive:

```c
struct wl_event_source *wl_event_loop_add_fd(
    struct wl_event_loop *loop,
    int fd,
    uint32_t mask,
    wl_event_loop_fd_func_t func,
    void *data);

void wl_event_source_remove(struct wl_event_source *source);
```

`g_pCompositor->m_wlEventLoop` is the same loop that backs `doOnReadable`. Using `wl_event_loop_add_fd` therefore preserves every property required by ADR-018 (main-thread, event-driven, non-blocking) while adding the missing removability.

This ADR records the refinement. ADR-018 itself remains immutable (Accepted); only the concrete registration API is updated.

---

### Decision

1. **Registration primitive**

   In the Hyprland adapter (`HyprlandOverlaySocket`) the listener fd and the accepted client fd **SHALL** be registered with:

   ```c
   wl_event_loop_add_fd(g_pCompositor->m_wlEventLoop, fd, WL_EVENT_READABLE, callback, this)
   ```

   and removed with:

   ```c
   wl_event_source_remove(source);
   source = nullptr;
   ```

   The adapter **MUST NOT** use `CEventLoopManager::doOnReadable` for these two fds.

2. **Ownership and lifetime**

   - `HyprlandOverlaySocket` owns the two `wl_event_source *` pointers (`listen_source_`, `client_source_`).
   - `unwatch_listener()` / `unwatch_client()` are idempotent and are called on:
     - peer HANGUP / ERROR,
     - normal peer drop (`poll_client` returns false),
     - path change / config reload that restarts the socket,
     - `stop()` / destructor / `PLUGIN_EXIT`.
   - After `unwatch_*` the corresponding source pointer is null; a subsequent `watch_*` may re-register.

3. **Failure policy (unchanged from ADR-018)**

   - If `wl_event_loop_add_fd` returns null for the listener → treat as start failure, stop the server, return false so the facade can degrade to `NullUI` (REQ-O-001 / REQ-UI-002).
   - If registration of a newly accepted client fails → drop the client immediately; do not keep an unreadable peer.
   - Callbacks never throw into the compositor (HIGH-4 / REQ-UI-001 pattern).

4. **REQ-O-006 update**

   The normative text of REQ-O-006 is amended as follows (SPEC §5.3):

   > **REQ-O-006** All socket I/O SHALL run on the compositor main thread (event-driven via the Wayland event loop — `wl_event_loop_add_fd` / `wl_event_source_remove` on the pinned revision) and SHALL be non-blocking; a slow, absent or dead peer MUST NOT stall any `mru:*` dispatcher or focus path (REQ-PERF-001/003, REQ-RE-001).

   Architecture documentation (ARCHITECTURE §8 / §11) is updated to match. ADR-018’s Decision §2 is left as historical record; this ADR is the authoritative source for the concrete API after acceptance.

5. **No change to protocol, transport or domain contracts**

   Everything else from ADR-018 remains in force:

   - AF_UNIX STREAM, single client, O_NONBLOCK,
   - best-effort sends,
   - line-framed JSON protocol (Appendix B),
   - Hyprland-free core,
   - fallback to NullUI,
   - next-session-only reload semantics.

---

### Consequences

#### Positive

- Peer disconnect and plugin teardown no longer leak fd-watches (REQ-O-008 satisfied).
- Same main-thread, event-driven, non-blocking properties that ADR-018 required.
- Explicit, removable handles make the adapter’s lifetime model obvious and unit-testable in principle (the loop itself remains a compositor object).
- Clear separation of “design intent” (ADR-018) and “pin-accurate implementation detail” (this ADR).

#### Negative / risks

- Slightly lower-level API surface (`wl_event_loop_*` instead of the Hyprland convenience wrapper). Mitigated by confining the calls to a single thin adapter file.
- Future Hyprland changes to the event-loop ownership model would require a new ADR (same risk existed with `doOnReadable`).
- Reviewers must remember that ADR-018’s original wording is superseded for the registration primitive only; the rest of the design is unchanged.

#### Follow-ups (not M5 exit)

- None required for M5 exit criteria. Optional: a short COMPAT note recording that `doOnReadable` was evaluated and rejected for lack of removability on pin `efb5099`.

---

### Compliance mapping

| Topic | REQ / ADR |
|-------|-----------|
| Event-driven main-thread I/O | REQ-O-006 (amended), REQ-RE-001, REQ-PERF-001/003 |
| Teardown removes watches + closes socket | REQ-O-008 |
| Fail-soft start / NullUI fallback | REQ-O-001, REQ-UI-002, ADR-011 |
| Domain stays Hyprland-free | ADR-007 |
| Original design intent | ADR-018 (immutable) |
| Concrete registration API | **this ADR** |

---

### References

- `docs/DECISIONS.md` — ADR-018 (Accepted)
- `docs/SPEC.md` §5.3 — REQ-O-001..008 (REQ-O-006 amended by this ADR)
- `docs/ARCHITECTURE.md` §8 / §11 — UIPort and overlay socket wiring
- `docs/COMPAT.md` — pin Hyprland v0.56.2 / `efb5099…`
- `docs/agent-state/research/2026-09-19-m5-overlay-socket-api.md` — R0 memo
- Implementation: `src/plugin/hypr/hyprland_overlay_socket.{hpp,cpp}` (M5)
- Wayland server API: `wl_event_loop_add_fd`, `wl_event_source_remove` (`wayland-server.h`)
