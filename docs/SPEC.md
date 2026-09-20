# MRU Window Switcher — Technical Specification

**Version:** 0.3 (M6 contract audit)  
**Status:** Normative; **contract freeze for 1.x declared in §0 (M6-T1)**
**Document type:** Normative  
**Related:** ARCHITECTURE.md, DECISIONS.md, USER.md, ROADMAP.md, COMPAT.md

This document is the detailed contract for behaviour, interfaces, configuration, and error handling.  
Where this SPEC conflicts with informal docs, **SPEC wins** until an ADR updates it.

---

## 0. Contract freeze for 1.x (M6-T1)

Declared in M6-T1 (issue #50) after a line-by-line audit of §2–§4 against `src/plugin/`, `src/domain/`, and
`tests/`. No semantic gaps were found; this section changes no behaviour — it freezes what already ships.

**Frozen for 1.x:**

1. **Dispatcher names + grammar** (§3): `mru:cycle [next|prev] [global|monitor|workspace|visible|app]`,
   `mru:apply`, `mru:cancel`, `mru:status` — registered via `addDispatcherV2`. Omitted direction = `next`
   (REQ-DISP-001); a scope token without an explicit direction is accepted (REQ-DISP-003); unknown tokens fail
   with a clear error string (REQ-SC-003).
2. **`mru:status` payload** (§3.4): the `key=value` format there is **normative**; new keys may be appended
   without a breaking change (additive).
3. **Configuration keys** (§4): every registered key under `plugin:mru-switcher:` — 11 total:
   `debounce_ms`, `default_scope`, `start_offset`, `wrap`, `ui`, `border_style`, `border_color`,
   `border_size`, `lock_history_on_session`, `restore_focus_on_cancel`, `external_socket` — names, types,
   defaults, and reload semantics (REQ-CFG-001..004, REQ-S-009, REQ-UI-009) are frozen. **Except:** the
   `lock_history_on_session` value is **reserved and ignored** (lock-in while Active is mandatory — REQ-H-010,
   ADR-021); the key stays registered for 1.x, and its removal is a 2.0 candidate.
4. **Snapshot / apply / restore semantics** (§2): snapshot frozen at first cycle, prune only (REQ-SNAP-003/004);
   virtual selection — `mru:cycle` never focuses (REQ-F-003); exactly one focus per `mru:apply` through
   FocusGateway (REQ-F-006) with apply-after-invalidation per §2.8; lock-in + debounce (REQ-H-001..003,
   REQ-H-011, ADR-001..003, ADR-021); restore on an explicit `mru:cancel` only (REQ-R-003).
5. **External UI protocol** (§12 Appendix B, protocol version `v=1`): frozen as shipped in M5.

**Versioning rule (semver):** in 1.x, breaking any frozen contract above — a dispatcher rename, a grammar
restriction, a key rename/type/default change, or a semantics change — requires a **major** bump (2.0.0).
Additive changes (a new optional dispatcher argument, an appended `mru:status` key, a new config key) are
minor. See `docs/VERSION-MAP.md` and AGENTS.md §15. During the remainder of 0.x a frozen contract may still
change only with a CHANGELOG entry, and every such change must be reconciled before `v1.0.0`.

---

## 1. Definitions

| Term | Definition |
|------|------------|
| **Session** | Period from first successful `mru:cycle` until `apply`, `cancel`, or forced end |
| **Snapshot** | Immutable ordered list of window identities captured at session start |
| **Selection** | Index into the current Snapshot identifying the candidate to apply |
| **MRU list** | Plugin-maintained most-recently-used ordering of windows |
| **Lock-in** | While session is Active, focus events do not update the MRU list |
| **Debounce** | Delay after focus before a window is committed to the MRU list |
| **Scope** | Filter applied when building a Snapshot |
| **Apply** | Action that focuses the selected window and ends the session |
| **Window identity** | Value `WindowRef` — see §2.7; not a raw pointer alone |
| **WindowRef** | `{ address: uint64, generation: uint64 }` as observed by the adapter at capture time |
| **SchedulerPort** | Abstraction for debounce timers; real impl uses compositor event loop; tests use FakeClock |
| **UI backend** | Concrete `UIPort` implementation selected by config `ui` |
| **Border highlight** | Temporary visual override of window border attributes for the current selection while a session is Active |
| **Border style** | Named behaviour of `BorderHighlightUI` (`solid`, reserved `pulse` / `dim`, …) |
| **UIEndReason** | Coarse end signal to UI: `Applied` \| `Cancelled` (see REQ-F-009 mapping) |

---

## 2. Behavioural requirements

### 2.1 Session lifecycle

**REQ-S-001** The plugin SHALL support exactly one active session at a time (global to the plugin instance).

**REQ-S-002** A session SHALL start on the first `mru:cycle` command when state is Idle and at least one candidate window exists after scope filtering.

**REQ-S-003** While state is Active, further `mru:cycle` commands SHALL only change Selection within the existing Snapshot (no rebuild).

**REQ-S-004** `mru:apply` SHALL focus the currently selected window (if still valid) and transition to Idle.

**REQ-S-005** `mru:cancel` SHALL transition to Idle without focusing the selection (unless `restore_focus_on_cancel` applies — see §6).

**REQ-S-006** If the Snapshot becomes empty (all windows closed/invalid), the session SHALL end as Cancelled.

**REQ-S-007** Starting a session SHALL record the then-current focused window as `session_origin` for optional restore.

**REQ-S-008** Each session SHALL receive a monotonic `session_id` (uint64, plugin lifetime) for diagnostics and logs.

**REQ-S-009** At session start the controller SHALL snapshot **SessionPolicy** (effective scope, wrap, start_offset, restore_focus_on_cancel, ui backend choice for this session). Runtime config reload MUST NOT mutate the active session's policy; new values apply to the **next** session only. (History lock-in is not part of the policy: it is mandatory while Active — REQ-H-001, ADR-021.)

**REQ-S-010** While Active, a `mru:cycle` that includes a **scope** token differing from the session policy scope MUST **ignore** the override and continue with the existing Snapshot (MUST NOT rebuild). Implementations MAY log at debug. (Ergonomic default: switching keybinds mid-hold does not abort the session.)

**REQ-S-011** Automatic session timeout is **out of scope** for v0.x and 1.0. No implicit apply/cancel by timer is permitted.

### 2.2 Snapshot construction

**REQ-SNAP-001** Snapshot order SHALL be MRU-first (most recently committed window first), using HistoryTracker state at construction time.

**REQ-SNAP-001a** Candidate enumeration SHALL return the plugin-owned MRU order first and other in-scope valid windows after it, without duplicating identities (ADR-015).

**REQ-SNAP-002** Snapshot SHALL contain only windows that pass the effective Scope filter and validity checks (mapped, not hidden, not fading out — exact criteria in adapter).

**REQ-SNAP-003** Snapshot SHALL be immutable for the lifetime of the session except for **pruning** of identities that become invalid.

**REQ-SNAP-004** After pruning, Selection index SHALL be clamped to a valid range; if empty → REQ-S-006.

### 2.3 Selection and start_offset

**REQ-SEL-001** `start_offset = second` (default): initial index is `min(1, len-1)` when `len >= 1`.

**REQ-SEL-002** `start_offset = first`: initial index is `0`.

**REQ-SEL-003** `mru:cycle next` SHALL set `index = (index + 1) % len` when `wrap = true`.

**REQ-SEL-004** `mru:cycle prev` SHALL set `index = (index + len - 1) % len` when `wrap = true`.

**REQ-SEL-005** When `wrap = false`, next/prev SHALL clamp at ends and NOT wrap; dispatcher result remains success.

### 2.4 Focus application

**REQ-F-001** Real compositor focus SHALL change only through FocusGateway.

**REQ-F-002** Focus SHALL be applied on `mru:apply` and on policy paths explicitly defined as apply (e.g. modifier release bound to `mru:apply`).

**REQ-F-003** Intermediate `mru:cycle` invocations MUST NOT focus windows (virtual selection only).

**REQ-F-004** If the selected identity is invalid at apply time, the plugin SHALL run **Apply-After-Invalidation** (§2.8). It SHALL NOT crash.

**REQ-F-005** FocusGateway SHALL resolve `WindowRef` to a live window only if address **and** generation still match the adapter’s registry; otherwise the ref is invalid.

### 2.5 History and lock-in

**REQ-H-001** HistoryTracker SHALL NOT update MRU ordering while a session is Active — lock-in is **mandatory** and unconditional (ADR-021). While Idle, ordering updates per REQ-H-002/003.

**REQ-H-002** When Idle, a focus event SHALL start/reset a debounce timer of `debounce_ms` milliseconds.

**REQ-H-003** Only after the timer fires without newer focus events SHALL the window be committed to the MRU list.

**REQ-H-004** Preferred seed/order source: `Desktop::History::windowTracker()->fullHistory()` when available; otherwise event-sourced list.

**REQ-H-004a** The plugin **owns** the semantic MRU list used for Alt+Tab. Compositor history is seed/reconciliation only when available, not a live source of truth during the session.

**REQ-H-004b** On plugin init, HistoryTracker SHALL attempt to seed from the compositor history source; if unavailable or empty, start empty and populate from subsequent focus events. Windows discovered while enumerating candidates SHALL be registered on sight so that windows opened before plugin load participate from the first snapshot (ADR-015).

**REQ-H-004c** History contains at most one entry per currently known live `WindowRef` identity; destroyed identities are removed (no separate max-length config required for v1).

**REQ-H-005** Focus reasons MAY be used to ignore pure pointer-enter noise if configured in a future revision; v0.1 commits all focus events subject to debounce.

**REQ-H-006** Debounce SHALL be implemented via `SchedulerPort` (§2.9). Only one pending debounce job exists at a time; a new focus event cancels/replaces the previous pending job.

**REQ-H-007** Debounce callbacks SHALL run on the compositor/main thread (or be marshalled there). Domain tests use `FakeClock` that advances time and drains due jobs synchronously.

**REQ-H-008** On plugin unload, session end, or destruction of HistoryTracker, all pending debounce jobs SHALL be cancelled and MUST NOT run after teardown.

**REQ-H-009** If the window referenced by a pending debounce job becomes invalid before fire, the job SHALL be cancelled and MUST NOT commit to the MRU list.

**REQ-H-010** The `lock_history_on_session` config key is **reserved**: it SHALL remain registered under `plugin:mru-switcher:` and its value SHALL NOT affect behaviour (lock-in is mandatory per REQ-H-001). The plugin MAY emit a warn-once notification when the key is set to a non-default value, pointing at the documentation. The key SHALL NOT be advertised as a working option in `examples/`, README, or USER.md. (ADR-021)

**REQ-H-011** At session start, a **pending** debounced focus commit SHALL be flushed — cancelled and committed immediately, subject to the REQ-H-009 validity guard — **before** lock-in engages, so that consecutive sessions separated by less than `debounce_ms` still observe the previously applied window in MRU order. With no pending job the operation is a no-op; no pending job may survive into the Active state (REQ-H-006/008). (ADR-021)

### 2.6 Scopes

**REQ-SC-001** Effective scope = argument to `mru:cycle` if present and valid; else `default_scope`.

**REQ-SC-002** Scope semantics:

| Scope | Include window if |
|-------|-------------------|
| `global` | Valid (mapped, not hidden, not fading) AND window is not on a hidden special workspace |
| `monitor` | Valid AND on current monitor AND window is not on a hidden special workspace |
| `workspace` | Valid AND on current workspace AND window is not on a hidden special workspace |
| `visible` | Valid AND on a currently visible workspace (member of the visible workspace set) |
| `app` | Valid AND same **`class`** (not `initialClass`) as the focused window at snapshot time; if no focused window, behave as `global` |

With **no focused window at snapshot time**, `monitor` and `workspace` SHALL behave as `global` (there is no reference window to anchor them to). (ADR-016, M3-S2)

**REQ-SC-002a** Special workspaces (scratchpad): a window on a special workspace is a candidate in **any** scope **only while** that special workspace is currently shown on a monitor (i.e. it is a member of the visible workspace set at snapshot time). A hidden special workspace excludes its windows from `global`, `monitor`, `workspace`, and `visible` alike. "You see it — it is in the ring; you don't — it is not." (ADR-016 __3__)

**REQ-SC-002b** `app` class comparison is **byte-exact and case-sensitive** (`candidate.m_class == focus.m_class`); it SHALL NOT be made case-insensitive later without a new ADR + SPEC change. A focus with an empty class degrades to `global`; a candidate with an empty class never matches. The focused window is itself a candidate (intentional: with `start_offset=second` it yields the in-app toggle). (ADR-016 __4__)

**REQ-SC-003** Unknown scope token SHALL cause `mru:cycle` to fail with a clear error string (session not started).


### 2.7 Window identity (`WindowRef`)

**REQ-ID-001** `WindowRef` SHALL contain at least:
- `address` — compositor window address (as used by hyprctl / internal id)
- `generation` — monotonic counter or epoch from the adapter registry for that address slot

**REQ-ID-002** When a window is mapped/tracked, the adapter assigns or bumps `generation` so that a closed window’s address reused by a later window does **not** compare equal to the old `WindowRef`.

**REQ-ID-003** Equality of two refs requires both `address` and `generation` to match.

**REQ-ID-004** Snapshot entries store `WindowRef`, not raw `PHLWINDOW*`. Adapters may cache weak pointers only as an optimization and MUST re-validate via REQ-F-005 before focus.

**REQ-ID-005** Validity check for a ref: adapter finds a live mapped window with the same address **and** the same generation; otherwise invalid.

**REQ-ID-006** Identity validity is decided by the **weak reference `lock()`**, not by a `closed`/destroy flag: a ref whose cached weak ref resolves (`.lock()` non-null) with a matching generation is valid **regardless of any arrival/delivery of a `close` event**; an empty `lock()` makes the identity invalid even if the `closed` flag never fired. (ADR-016 __1__) The `closed` flag only drives entry cleanup and pruning — it never overrides weak-lock validity.

### 2.8 Apply-after-invalidation

When `mru:apply` runs and the current selection’s `WindowRef` is invalid:

1. **Prune** all invalid entries from the Snapshot (stable relative order of survivors).
2. If Snapshot is empty → end session as **Cancelled** (UI `Cancelled`); dispatcher MUST return `{ success: false, error: "no windows" }` if nothing was focused.
3. If Snapshot non-empty:
   - Let `i` be the selection index **before** prune.
   - After prune, set index to `min(i, len-1)` (clamp toward the same slot / previous neighbor). **Do not** scan arbitrarily far; a single clamp after full prune is enough.
   - Validate the new selection; if somehow still invalid, repeat prune+clamp once more; if still empty → Cancelled as in step 2.
   - Focus the resolved window via FocusGateway; end session as **Applied** (UI `Applied`).

**REQ-F-006** At most one successful focus occurs per `mru:apply` call.

**REQ-F-007** UI receives exactly one `on_session_end`: `Applied` if focus ran, `Cancelled` if not.

**REQ-F-008** FocusGateway SHALL return a structured **FocusResult** to the controller (not a bare bool). The result MUST distinguish at least:

| FocusResult | Meaning | Controller action |
|-------------|---------|-------------------|
| `Applied` | Focus was issued successfully to the resolved live window | End session as applied (UI `Applied`, internal reason `Applied`) |
| `InvalidTarget` | `WindowRef` failed validation (stale generation, missing, unmapped) | Treat as invalid selection: continue **Apply-After-Invalidation** (§2.8) or end with no focus |
| `Failed` | Focus API rejected or failed unexpectedly after a valid target was resolved | End session without a successful focus; UI `Cancelled`; internal reason `FocusFailed`; log at error |

Domain tests may use a mock FocusGateway that returns these values without Hyprland.

**REQ-F-009** Internally the controller SHALL record a **SessionEndReason** for diagnostics, status, and logs. UIPort continues to receive only the coarse UI reason (`Applied` | `Cancelled`). Mapping:

| SessionEndReason | UI `on_session_end` | Typical trigger |
|------------------|---------------------|-----------------|
| `Applied` | Applied | Successful focus on apply |
| `UserCancel` | Cancelled | `mru:cancel` / user abort |
| `NoWindows` | Cancelled | Snapshot empty (after prune or on start failure path) |
| `InvalidSelection` | Cancelled | Apply path exhausted invalid targets without focus |
| `FocusFailed` | Cancelled | FocusResult `Failed` |
| `PluginShutdown` | Cancelled | Unload / teardown while Active |

`mru:status` and debug logs SHOULD expose the internal reason when verbose; the human-readable status string is frozen in §3.4 (normative since the M6-T1 contract freeze, §0).

### 2.9 SchedulerPort (debounce)

Logical interface:

```text
cancel(job_id)
job_id = schedule_after(delay_ms, callback)
```

**REQ-SCH-001** Production adapter schedules on the Hyprland/compositor event loop (or equivalent main-thread timer API available to the plugin). Exact Hyprland symbol is adapter-private and version-pinned in COMPAT.

**REQ-SCH-002** `FakeClock` in tests: `advance(ms)` fires all due callbacks synchronously on the calling thread.

**REQ-SCH-003** HistoryTracker holds at most one `job_id` for debounce; schedule replaces previous.

---

### 2.10 Reentrancy and event ordering

**REQ-RE-001** All domain and SessionController mutations run on the compositor main thread (no worker threads touching controller state).

**REQ-RE-002** A single dispatcher invocation (`cycle` / `apply` / `cancel`) is **logically atomic** with respect to session state: it finishes its state transition before processing further dispatcher calls ordered by Hyprland after it.

**REQ-RE-003** Events emitted as a consequence of `FocusGateway` focus during `apply` (e.g. `window.active`) MUST NOT reopen or mutate the session that is already terminating; HistoryTracker applies lock-in/Idle rules based on post-transition state (typically Idle after apply completes).

**REQ-RE-004** Nested or overlapping dispatcher entry during an in-progress transition is ordered by the compositor; the plugin MUST NOT assume concurrent mutation of one session. Prefer finishing the current transition, then handling the next dispatcher.

### 2.11 Performance and non-functional constraints

**REQ-PERF-001** Dispatchers and event listeners MUST NOT block the compositor thread: no blocking I/O, no `sleep`, no process spawn, no `hyprctl`/socket IPC on the `mru:cycle` / `mru:apply` / `mru:cancel` hot path.

**REQ-PERF-002** After Snapshot construction, advancing selection (`cycle`) is **O(1)** index arithmetic. Snapshot build and prune are **O(n)** in snapshot/candidate size.

**REQ-PERF-003** UIPort implementations MUST NOT block the controller on external I/O (M5 socket is best-effort, non-blocking / timed).

**REQ-PERF-004** No allocations beyond the unavoidable are required on the cycle hot path; do not re-enumerate all compositor windows on every cycle (resolve candidates at snapshot time).


## 3. Dispatcher specification

All dispatchers registered via `HyprlandAPI::addDispatcherV2`.

### 3.0 Dispatcher common rules

**REQ-DISP-001** For `mru:cycle`, if direction token is omitted, treat as `next`.

**REQ-DISP-002** When apply ends with no focus (empty after prune), result is always `{ success: false, error: "no windows" }` (not success no-op).

### 3.1 `mru:cycle`

**Grammar:**

```text
mru:cycle [next|prev] [global|monitor|workspace|visible|app]
```

- **Direction:** if omitted, MUST default to `next` (normative). Binds SHOULD still pass an explicit direction for clarity.
- Scope optional (§2.6).

**REQ-DISP-003** A scope token MAY be passed without an explicit direction token; direction then defaults to `next` (REQ-DISP-001). Any token that is neither a known direction nor a known scope SHALL fail with a clear error.

**Results:**

| Condition | `SDispatchResult` |
|-----------|-------------------|
| Success | `{ success: true }` |
| No candidates | `{ success: false, error: "no windows" }` |
| Invalid args | `{ success: false, error: "..." }` |

**Side effects:**

- Idle + success → create Snapshot, set Selection, notify UI `on_session_start`
- Active + success → update Selection, notify UI `on_selection_changed`

### 3.2 `mru:apply`

**Grammar:** `mru:apply` (no args)

**Behaviour:** REQ-F-002, REQ-S-004; UI `on_session_end(Applied)`; unlock history.

**Idempotent:** If already Idle, success no-op.

### 3.3 `mru:cancel`

**Grammar:** `mru:cancel`

**Behaviour:** REQ-S-005; UI `on_session_end(Cancelled)`; unlock history.

**Idempotent:** If already Idle, success no-op.

### 3.4 `mru:status`

**Grammar:** `mru:status`

**Behaviour:** Return success; the error field of the successful result MAY carry the human-readable status:

```text
active=true index=2 size=5 scope=global
```

**Frozen payload (1.x):** `active=`, `index=`, `size=`, `scope=` plus the additive `session=`
(monotonic `session_id`, REQ-S-008) and `last_end=` (internal `SessionEndReason`, REQ-F-009 —
`Applied | UserCancel | NoWindows | InvalidSelection | FocusFailed | PluginShutdown`, or `none`),
in that order, as produced by the plugin on 0.56.2. **Normative since the M6-T1 contract freeze (§0):**
tools may parse the `key=value` pairs strictly; unknown additional keys MAY appear in minor releases
and MUST be tolerated. When no session is active: `active=false index=0 size=0`, and `scope` is the
effective `default_scope`. `session=` is the monotonic `session_id` (REQ-S-008) — the id of the
active session while one is running, and of the most recently started session while idle;
`last_end=` is `none` until the first session has ended, then the reason the most recent session
ended (REQ-F-009). Transport visibility is a host matter — on the pinned Hyprland 0.56.2
`hyprctl dispatch` prints only `ok` and does not surface this payload (`docs/COMPAT.md` matrix row);
a libwayland dispatcher binding or the plugin log shows it.

---

## 4. Configuration specification

All keys under `plugin:mru-switcher:`.  
Registered only in `PLUGIN_INIT`. Types follow Hyprland config value types used by the plugin API.

**Frozen surface (1.x, §0):** names, types, defaults, and reload semantics of all keys below are stable
in 1.x; a change to any of them requires a major version bump. Additive new keys are minor releases.

| Key | Type | Default | Description |
|-----|------|---------|-------------|
| `debounce_ms` | int | `400` | Debounce before MRU commit (ms); `0` = immediate; clamp to **[0, 5000]** (REQ-CFG-004) |
| `default_scope` | string | `global` | One of: `global`, `monitor`, `workspace`, `visible`, `app` |
| `start_offset` | string | `second` | `first` \| `second` |
| `wrap` | bool | `true` | Wrap selection at ends |
| `ui` | string | `null` | `null` \| `border` \| `external` — see REQ-UI-002 |
| `border_style` | string/enum | `solid` | Border highlight style; M4: only `solid` has effect; unknown/reserved (`pulse`, `dim`, …) → `solid` + warn-once (REQ-UI-007) |
| `border_color` | color/string | `0xffffd9a0` | Border highlight colour — documented implementation default (hex `0xAARRGGBB`); format as accepted by the pinned Hyprland; documented in USER/API (REQ-UI-008) |
| `border_size` | int | `-1` | Border highlight size; `-1` = do not touch window border size (colour only) (REQ-UI-008) |
| `lock_history_on_session` | bool | `true` | **Reserved — value ignored.** Lock-in while a session is Active is mandatory (REQ-H-001/010, ADR-021). Kept registered for 0.x config compatibility; non-default values trigger a warn-once notification. Removal is a 2.0 candidate |
| `restore_focus_on_cancel` | bool | `false` | On cancel, focus `session_origin` if still valid |
| `external_socket` | string | (empty) | Path for the External UI protocol (M5, ADR-018): AF_UNIX stream socket bound by the plugin. Empty path or bind failure ⇒ `ui = external` behaves as `null` + warn-once (REQ-O-001) |

**REQ-CFG-001** Invalid string enums SHOULD fall back to default and MAY notify once.

**REQ-CFG-002** Values are read at `PLUGIN_INIT`. On Hyprland `config.reloaded` (Event::bus), the plugin SHOULD refresh cached config pointers/values used by controllers. Active session policy is **not** retroactively rewritten mid-session (snapshot/selection unchanged); new settings apply to the **next** session and to debounce delay for subsequent Idle focuses.

**REQ-CFG-003** Changing `ui` at reload switches backend for the next session start; current session may keep the backend already used for that session.

**REQ-CFG-004** Numeric `debounce_ms` outside `[0, 5000]` SHALL be clamped into range; implementations MAY log once at warn when clamping.

---

## 5. UI port specification

### 5.1 Interface (logical)

```text
on_session_start(snapshot, selection)
on_selection_changed(selection)
on_session_end(reason: Applied | Cancelled)  # UI; internal SessionEndReason is richer (REQ-F-009)
```

### 5.2 Backends

| Backend | Requirements |
|---------|--------------|
| `null` | No compositor side effects |
| `border` | Visible highlight of selected window; cleared on session end; MUST NOT leave permanent rule damage |
| `external` | Best-effort notify via AF_UNIX socket (M5, ADR-018); session logic MUST work if peer absent |

UI provisioning follows ADR-004 / ADR-017; the config default stays `null` in M4 (ADR-011) and concrete border symbols are adapter-private, recorded in `docs/COMPAT.md` (REQ-UI-011).

**REQ-UI-001** Failures inside any `UIPort` implementation SHALL NOT abort `mru:cycle`, `mru:apply`, or `mru:cancel`, and SHALL NOT leave the session state machine in an undefined state. Session transitions remain driven by the controller and FocusGateway.

**REQ-UI-002** If the configured `ui` backend is not implemented or cannot be constructed, the plugin SHALL use `NullUI` and MAY emit at most one warning for that condition per plugin lifetime (or per config change that re-selects the backend). (`border` before M4 and `external` before M5 are instances of this rule.)

**REQ-UI-003** When effective policy `ui = border`, the plugin SHALL attach `BorderHighlightUI` as the session UIPort implementation for that session.

**REQ-UI-004** `BorderHighlightUI` SHALL, on `on_session_start` and on each `on_selection_changed`, ensure that only the window corresponding to the current selection index carries the plugin’s selection highlight (previous highlighted window cleared first).

**REQ-UI-005** On `on_session_end` (any `UIEndReason`) and on plugin unload/teardown while highlights may exist, the plugin SHALL remove all border overrides it applied. After a successful clear path there SHALL be no stuck borders attributable to the plugin.

**REQ-UI-006** Highlight updates MUST NOT apply real compositor focus. Focus changes remain solely via FocusGateway on apply (and restore-on-cancel policy paths). Consistent with REQ-F-003.

**REQ-UI-007** Config `border_style`:

- `solid` — required implementation in M4.
- Reserved tokens `pulse`, `dim` (and any later documented tokens not yet implemented) SHALL behave as `solid` until a SPEC/ADR revision implements them; implementations MAY warn once.
- Unknown tokens SHALL behave as `solid` and SHOULD warn once.

**REQ-UI-008** The following config keys SHALL be registered under `plugin:mru-switcher:` in M4 (in addition to existing keys):

| Key | Semantics |
|-----|-----------|
| `border_style` | Style token; default `solid` |
| `border_color` | Colour used for the selection highlight; default is implementation-defined but MUST be documented in USER.md / API.md. The project’s documented implementation default is `0xffffd9a0` (hex `0xAARRGGBB`). |
| `border_size` | Integer border size override; default `-1` means **do not** change the window’s border size |

**REQ-UI-009** Changes to `ui`, `border_style`, `border_color`, and `border_size` via config reload SHALL apply only to sessions started **after** the reload (snapshot in SessionPolicy / equivalent). An Active session keeps the UI behaviour chosen at its start.

**REQ-UI-010** `BorderHighlightUI` SHALL resolve `WindowRef` through the same validity rules as focus (address + generation / weak-lock, ADR-013 / ADR-016). If the target is invalid, highlight for that index is skipped; the session continues.

**REQ-UI-011** Prefer public compositor/plugin APIs for border mutation on the pinned Hyprland revision. Exact symbols are adapter-private and recorded in `docs/COMPAT.md`. Function hooks are not required for M4 compliance.

M4 UI out of scope: live window previews inside the plugin; full behaviour of `pulse` / `dim` (reserved only); the M5 external overlay protocol; changing the default `ui` from `null` to `border` (optional product decision + CHANGELOG, not required by REQ-UI-*).

### 5.3 External overlay (M5)

ME-5 implements the `external` backend (ADR-018): an out-of-process overlay fed by the plugin over an
AF_UNIX stream socket, with peer-issued selection commands. The protocol is frozen in §12 Appendix B.

**REQ-O-001** If the effective backend is `external` but `external_socket` is empty or the socket
cannot be bound, the plugin SHALL use `NullUI` behaviour for that session and MAY emit at most one
warning per plugin lifetime (REQ-UI-002 continuation).

**REQ-O-002** On session start with `ui = external` and a successfully bound socket, the plugin SHALL
install `ExternalOverlayUI` as the session UIPort. The absence, slowness, or death of a peer MUST NOT
change session logic: outgoing messages are best-effort (REQ-PERF-003) and MAY be dropped under
backpressure; the session and its focus behaviour MUST remain identical to `ui = null`.

**REQ-O-003** `ExternalOverlayUI` SHALL emit exactly one protocol message per UIPort lifecycle call
in the following mapping:

| UIPort call | Message (§12 Appendix B) |
|-------------|--------------------------|
| `on_session_start(snapshot, index)` | `session_start` with the full window list (`addr`, `title`, `class`) and `index` |
| `on_selection_changed(index)` | `selection` with `index` |
| `on_session_end(reason)` | `session_end` with `reason` = `applied` \| `cancelled` (UIEndReason mapping, REQ-F-009) |

**REQ-O-004** Peer commands SHALL map to controller operations as follows, all deduced on the
compositor main thread and only while data is readable (REQ-RE-001):

| Peer message | Effect |
|--------------|--------|
| `select` with index `i` | `SessionController::select_index(i)` — Active-only, `i < snapshot.size()`, **virtual selection only**; ignored otherwise |
| `apply` | `SessionController::apply()` (idempotent on Idle) |
| `cancel` | `SessionController::cancel()` (idempotent on Idle) |

**REQ-O-005** Peer data with an unknown version, unknown `type`, malformed JSON, or an oversized/
invalid line SHALL be ignored and logged at debug; it MUST NOT change session state or focus.

**REQ-O-006** All socket I/O SHALL run on the compositor main thread (event-driven via the Wayland
event loop — `wl_event_loop_add_fd` / `wl_event_source_remove` on the pinned revision, ADR-019;
`doOnReadable` was evaluated and rejected for the listener/client fds because it returns no handle
to remove the watcher) and SHALL be non-blocking; a slow, absent or dead peer MUST NOT stall any
`mru:*` dispatcher or focus path (REQ-PERF-001/003, REQ-RE-001).

**REQ-O-007** Peer input SHALL NOT grant ability to focus an arbitrary window: `select` only moves the
virtual selection within the existing Snapshot (REQ-F-003) and `apply`/`cancel` reuse the existing,
bounds-safe controller paths. No new focus pathway exists for the peer.

**REQ-O-008** On plugin teardown / `PLUGIN_EXIT` (or socket teardown), all socket fds (listener, client,
waiter) SHALL be closed and any pending read callbacks SHALL be deregistered so no callback runs after
unload and no fd is leaked.

---

## 6. Optional restore on cancel

When `restore_focus_on_cancel = true`:

**REQ-R-001** On cancel, if `session_origin` is still valid, FocusGateway SHALL focus it.

**REQ-R-002** If invalid, no focus change beyond ending the session.

**REQ-R-003** Restore applies to an explicit `mru:cancel` only. A session ended for any other reason (REQ-S-006 empty snapshot / `NoWindows`, plugin shutdown, `apply`) SHALL NOT move focus to `session_origin`.

---

## 7. Hyprland integration requirements

**REQ-HL-001** `PLUGIN_INIT` SHALL compare `__hyprland_api_get_hash()` and `__hyprland_api_get_client_hash()` and abort load on mismatch (notification + exception/return failure per Plugin API practice).

**REQ-HL-002** Dispatchers SHALL be registered with `addDispatcherV2`.

**REQ-HL-003** Focus and window events SHOULD use `Event::bus()` (`window.active`, `window.close`, …).

**REQ-HL-004** Function hooks, if any, SHALL be optional, x86_64-only, and disabled by default.

**REQ-HL-005** Plugin MUST NOT spawn threads that touch compositor state.

**REQ-HL-006** Config values MUST use the `plugin:` namespace.

---

## 8. Error handling and safety

**REQ-ERR-001** No uncaught exceptions across dispatcher boundaries if the API expects `SDispatchResult` (catch and convert to error).

**REQ-ERR-002** Null/dead window pointers MUST be treated as invalid and pruned.

**REQ-ERR-003** Plugin load failure MUST leave Hyprland usable (no partial hooks left active — prefer init order that registers hooks only after success path).

---

## 9. Testing requirements (normative for M1+)

| ID | Case |
|----|------|
| T-S-01 | First cycle creates session; second cycle does not rebuild snapshot order |
| T-S-02 | apply focuses selected and ends session |
| T-S-03 | cancel ends session without apply focus |
| T-SEL-01 | start_offset second selects index 1 when len≥2 |
| T-SEL-02 | wrap true cycles 0 after last |
| T-H-01 | focus during Active does not change MRU order |
| T-H-02 | debounce: rapid focuses only commit last after quiet period |
| T-SC-01 | each scope filters as specified |
| T-F-01 | cycle never calls FocusGateway |
| T-F-02 | apply calls FocusGateway exactly once when selection valid |
| T-F-03 | apply with invalid selection: prune, clamp, apply survivor or cancel empty |
| T-F-04 | apply never focuses when snapshot empty after prune |
| T-ID-01 | WindowRef with stale generation fails validity |
| T-H-03 | second focus before debounce fires cancels first pending commit |
| T-H-04 | pending debounce cancelled on unload / tracker destroy |
| T-H-05 | invalid window before debounce fire does not commit |
| T-SEL-03 | wrap false clamps at ends |
| T-S-04 | empty candidate list fails cycle without Active session |
| T-S-05 | restore_focus_on_cancel focuses session_origin when valid |
| T-S-06 | restore_focus_on_cancel no-ops when origin invalid |
| T-UI-01 | unknown/unavailable ui backend falls back to null |
| T-UI-02 | UI exception is isolated and never aborts apply/cancel |
| T-UI-03 | `ui=null` -> no border side effects (domain/controller mock UI) |
| T-UI-04 | selection change -> previous cleared, new highlighted (adapter mock or nest) |
| T-UI-05 | apply/cancel/unload -> no stuck highlight |
| T-UI-06 | invalid WindowRef on highlight path -> no crash, session continues |
| T-UI-07 | unknown `border_style` -> solid + no abort |
| T-DISP-01 | omitted cycle direction equals next |
| T-F-05 | FocusResult InvalidTarget and Failed paths |
| T-S-07 | Active cycle ignores different scope token |
| T-RE-01 | apply then synthetic active does not reopen session |
| T-SC-02 | app scope matches class only |
| T-SC-03 | special workspace window is candidate only while its workspace is shown (ADR-016 __3__) |
| T-SC-04 | app class compare is byte-exact and case-sensitive; empty focus class degrades to global (ADR-016 __4__) |
| T-SC-05 | unknown scope token fails `mru:cycle` with a clear error string (REQ-SC-003; parse/dispatch layer, M3-S3) |
| T-ID-02 | ref validity is decided by weak-ref lock(), not the closed flag (REQ-ID-006, ADR-016 __1__) |
| T-S-09 | policy refresh mid-session does not change the frozen restore flag (REQ-S-009) |
| T-S-10 | non-cancel session end (NoWindows / plugin shutdown) never moves focus (REQ-S-006, REQ-R-003) |
| T-O-01 | `select_index` in an Active session moves the virtual selection and notifies UI (REQ-O-004, REQ-F-003) |
| T-O-02 | `select_index` out of range / Idle is a no-op without focus (REQ-O-004) |
| T-O-03 | `session_start`/`selection`/`session_end` serialize per Appendix B incl. JSON escaping of `title`/`class` (REQ-O-003) |
| T-O-04 | peer `select`/`apply`/`cancel` parse; `apply`/`cancel` map to controller ops (REQ-O-004) |
| T-O-05 | unknown version/type, broken JSON, oversized line → ignored, session unaffected (REQ-O-005) |
| T-O-06 | `ExternalOverlayUI` with absent/failing transport does not abort the session (REQ-O-002, REQ-UI-001) |
| T-O-07 | AF_UNIX server: accept first client, line framing, non-blocking send/recv (REQ-O-006; loopback test) |
| T-O-08 | peer `apply`/`cancel` at Idle are safe idempotent no-ops (covers REQ-O-004 idle leg) |

T-UI-03..07 continue the T-UI series begun in M2 (T-UI-01/02).

---

## 10. Non-requirements (v0.1 / v1.0)

- Live thumbnails / screencopy inside the plugin  
- Search-as-you-type filter (candidate for overlay protocol)  
- Per-monitor parallel sessions  
- Persistence of MRU list across Hyprland restarts  
- Compatibility with non-Hyprland compositors  

---

## 11. Appendix A — Recommended keybind mapping

```conf
bind   = ALT, TAB,       mru:cycle, next
bind   = ALT SHIFT, TAB, mru:cycle, prev
bindrt = ALT, ALT_L,     mru:apply
bind   = ALT, Escape,    mru:cancel
```

Modifier release MUST be bound by the user (or documented wrapper); the plugin does not hook raw XKB itself in v0.1.

---

## 12. Appendix B — External UI protocol (normative, M5)

**Transport:** AF_UNIX **SOCK_STREAM** socket bound by the plugin at
`plugin:mru-switcher:external_socket`. The plugin listens; the **first** accepted client is the
overlay (later clients are closed). Framing: newline-delimited UTF-8, **one JSON object per line**,
no embedded newlines in strings (escaped as `\n`). All I/O is non-blocking and best-effort
(REQ-O-002, REQ-O-006).

**Plugin → peer** (one message per UIPort lifecycle call, REQ-O-003):

```json
{"v":1,"type":"session_start","windows":[{"addr":"0x1a2b3c","title":"term","class":"foot"},{"addr":"0x4d5e6f","title":"editor","class":"neovide"}],"index":1}
{"v":1,"type":"selection","index":2}
{"v":1,"type":"session_end","reason":"applied"}
```

- `addr`: lowercase hexadecimal, `0x`-prefixed, of `WindowRef.address`.
- `title` / `class`: JSON-escaped strings; missing/empty metadata is serialized as `""`.
- `index`: integer index into the `session_start` window list (same meaning as `selection`).
- `session_end.reason`: `applied` (UIEndReason Applied) | `cancelled` (UIEndReason Cancelled).

**Peer → plugin** (optional control, REQ-O-004):

```json
{"v":1,"type":"select","index":3}
{"v":1,"type":"apply"}
{"v":1,"type":"cancel"}
```

- `select` index is bounds-checked against the session snapshot; out-of-range/Idle is a no-op.
- Unknown version, unknown `type`, malformed JSON, and oversized/incomplete lines are ignored and
  logged at debug (REQ-O-005).

Version `1` is normative for M5 and stays stable for the 0.x line; a future `v=2` is additive.

---

## 13. Appendix C — Traceability

| REQ block | ADR / Arch |
|-----------|------------|
| Snapshot / selection | ADR-001, ADR-002, ADR-010 |
| History lock-in / debounce | ADR-003 |
| UI port | ADR-004, ADR-017 |
| Event::bus / hooks | ADR-005 |
| FocusGateway | ADR-006 |
| Domain purity | ADR-007 |
| Config namespace | ADR-008 |
| Dispatcher names | ADR-009 |

---

## 14. Distribution and installation (ADR-020)

**Related:** ADR-020, ROADMAP M6, VERSION-MAP 1.0.0, `hyprpm.toml`, `docs/USER.md`, README.md

This section freezes the **user-facing installation and packaging contracts** for the 1.0 release line (ADR-020).  
It does not change dispatcher names, config keys, or session semantics (those are under the M6-T1 contract freeze).  
It defines what "installable product" means for end users and for Omarchy/Hyprland daily drivers.

### 14.1 Installation channels

**REQ-DIST-001** The project SHALL support exactly two first-class installation channels for 1.0:

1. **hyprpm** (recommended)
2. **Source build** under the canonical path defined in REQ-DIST-007

**REQ-DIST-002** hyprpm SHALL be presented as the primary/recommended path in README and USER.md.  
Manual `hyprctl plugin load` remains supported and documented, but is not the primary story.

**REQ-DIST-003** Until the v1.0.0 tag finalises `commit_pins` in `hyprpm.toml`, documentation MUST NOT claim that hyprpm installation is production-complete. It MAY describe the commands as the preferred path once the 1.0 pin is published.

### 14.2 hyprpm contract

**REQ-DIST-004** `hyprpm.toml` SHALL contain:

- a valid `[repository]` block (name, authors),
- `commit_pins` mapping the tested Hyprland revision to the plugin commit of the release tag,
- a `[mru-switcher]` (or equivalent) block with `description`, `authors`, `output`, and a `build` stanza that produces `build/mru-switcher.so`.

**REQ-DIST-005** The recommended user commands for hyprpm (once pins are final) SHALL be documented as:

```bash
hyprpm add https://github.com/code-warlord-dev/mru-switcher
hyprpm enable mru-switcher
hyprpm reload
```

(or a verified chained equivalent). Documentation MUST state any extra step (`hyprpm update`, etc.) that is required on the pinned Hyprland version.

**REQ-DIST-006** Omarchy users SHALL be told, near the top of the installation section, that hyprpm is the intended workflow when they already manage Hyprland plugins via hyprpm.

### 14.3 Canonical source layout

**REQ-DIST-007** Source installations SHALL use the following layout:

```text
~/.local/src/mru-switcher/                 # git working tree
~/.local/src/mru-switcher/build/mru-switcher.so
```

**REQ-DIST-008** User-facing documentation SHALL NOT use placeholder paths such as `/absolute/path/to/...` or `/path/to/mru-switcher.so`. All examples MUST use the canonical layout or `$HOME`-based expansion.

**REQ-DIST-009** The documented source build sequence SHALL be copy-pasteable and free of `sudo` for a normal user install into `~/.local/src`.

**REQ-DIST-010** Manual load after a source build SHALL be documented as:

```bash
hyprctl plugin load "$HOME/.local/src/mru-switcher/build/mru-switcher.so"
```

(or the equivalent `plugin =` line using the same absolute path).

### 14.4 Installer script (optional)

**REQ-DIST-011** If `scripts/install.sh` is shipped, it SHALL:

- target the canonical layout of REQ-DIST-007,
- detect or accept the Hyprland version / headers,
- configure and build with the same flags used in `hyprpm.toml` / CI,
- verify that `build/mru-switcher.so` exists after the build,
- print a human-readable summary (source path, plugin path, detected Hyprland version, next steps for keybindings),
- fail with actionable messages on missing toolchain, missing headers, or obvious pin mismatch.

**REQ-DIST-012** `curl | bash` SHALL NOT be documented as the primary installation method.  
Preferred documented invocation for the script (if present):

```bash
git clone https://github.com/code-warlord-dev/mru-switcher.git ~/.local/src/mru-switcher
cd ~/.local/src/mru-switcher
./scripts/install.sh
```

**REQ-DIST-013** The installer SHALL NOT silently modify the user’s `hyprland.conf` unless the user passes an explicit opt-in flag. It MAY write or print a fragment under `~/.config/hypr/conf.d/` when requested.

**REQ-DIST-026** If `scripts/install.sh` is shipped, it SHALL additionally:

- **(a) canonical layout** — work in the canonical layout of REQ-DIST-007 by default, and, when run outside it, either refuse with an explicit error naming both remedies (use the canonical checkout, or opt in explicitly) or accept an explicit opt-out (`--dir <path>` / `--allow-non-canonical`). A silent warning is not sufficient.
- **(b) interface** — support `--dry-run` (change nothing, print the full plan, exit 0), `--quiet` / `--verbose`, and `--jobs N`; reject unknown options and stray positional arguments with a non-zero exit code and a `--help` hint.
- **(c) `--write-conf`** — write the shipped examples (`examples/*.conf`) **verbatim** into `${XDG_CONFIG_HOME:-$HOME/.config}/hypr/conf.d/` (no duplicate fragment maintained inline in the script), never overwrite an existing file without `--force`, and with `--force` save a `.bak` backup before replacing it.
- **(d) exit status and summary** — return documented exit codes (0 on success; distinct non-zero codes for usage errors, refused non-canonical layouts, preflight failures, build failures, and refused overwrites), report the failing location on unexpected errors, and print a final summary with the source path, plugin path, detected Hyprland version, and next steps.

**REQ-DIST-027** If the installer is checked in CI, `scripts/install.sh` SHALL be validated at minimum with `bash -n`, `shellcheck` (when available on the runner) and `--dry-run` in a clean temporary directory; a negative case (unknown option) MUST assert a non-zero exit code.

### 14.5 Examples

**REQ-DIST-014** The repository SHALL ship:

```text
examples/mru-switcher.conf
examples/mru-switcher-bindings.conf
```

**REQ-DIST-015** `examples/mru-switcher.conf` SHALL:

- be a complete, loadable `plugin { mru-switcher { … } }` block,
- document every public config key inline (purpose, default, allowed values, short recommendation),
- avoid internal requirement IDs and adapter/implementation jargon,
- be intended for copy into `~/.config/hypr/conf.d/mru-switcher.conf` (or equivalent include path).

**REQ-DIST-016** `examples/mru-switcher-bindings.conf` SHALL contain only the recommended keybindings:

- `mru:cycle next` / `prev`
- `bindrt` apply on Alt release
- cancel (`mru:cancel`)

**REQ-DIST-017** README and USER.md SHALL direct new users to the files under `examples/` as the starting point for configuration, not only to a minimal inline snippet.

**REQ-DIST-025** Documentation SHALL describe how to obtain the shipped `examples/` files for **both** installation channels of REQ-DIST-001, with real, copy-pasteable commands and no placeholder paths (REQ-DIST-008):

- **source** — copy the files from the canonical checkout (`~/.local/src/mru-switcher/examples/…`);
- **hyprpm** — download the same files from the repository (raw `main` and/or the release tag), or copy them from a local checkout when one exists.

Documentation MUST NOT reference a location that the channel does not guarantee (for example an assumed hyprpm plugin-cache directory) unless that location is verified for the documented setup.

### 14.6 README as user documentation

**REQ-DIST-018** README SHALL be structured primarily for end users. Required high-level order:

1. Title / banner  
2. Badges (see REQ-DIST-021)  
3. Positioning (Hyprland + Omarchy; Niri as interaction inspiration)  
4. What it is (one screen)  
5. Short demo (GIF)  
6. Why it exists  
7. Features (user language)  
8. Installation (hyprpm first, then source)  
9. First setup (examples + source lines)  
10. Configuration overview  
11. Scopes / UI (short)  
12. Troubleshooting  
13. Compatibility / pin  
14. “For developers” → links into `docs/`

**REQ-DIST-019** README body SHALL NOT mix SPEC requirement identifiers, ADR numbers, or internal adapter layer names into the main user narrative. Those belong under `docs/`.

**REQ-DIST-020** Deep architecture, threat model, REQ-TRACE, plugin ABI, and design history remain under `docs/` and are linked, not duplicated, from README.

### 14.7 Positioning and badges

**REQ-DIST-021** README header badges SHOULD be limited to a small set that answers “what is it, for what, how ready”:

- CI  
- Release (version)  
- License  
- Linux  
- Hyprland  
- Omarchy  
- hyprpm  
- C++23  

Stars, forks, downloads, codecov, CMake, clang, and “Wayland” as a standalone badge are out of scope for the default header.

**REQ-DIST-022** README SHALL state early that the project is built for Hyprland and especially for Omarchy, and that the interaction model is inspired by Niri’s recent-windows workflow. These are two different claims and MUST NOT be collapsed into one.

### 14.8 Configuration fragments and includes

**REQ-DIST-023** Documentation SHALL show a concrete include pattern, for example:

```conf
source = ~/.config/hypr/conf.d/mru-switcher.conf
source = ~/.config/hypr/conf.d/mru-switcher-bindings.conf
```

after the user has copied the example files.

**REQ-DIST-024** The project MUST NOT require the user to hand-edit a `plugin = /some/path.so` line as the primary documented path when hyprpm is available and pins are final.

### 14.9 Non-unit checks (M6 packaging)

- **T-DIST-01** — Fresh clone → documented source build → `hyprctl plugin load` succeeds on the pinned Hyprland (nest or host).  
- **T-DIST-02** — `hyprpm add` + enable + reload loads the plugin on the pinned revision after the 1.0 tag.  
- **T-DIST-03** — `examples/mru-switcher.conf` is accepted by hyprlang (no unknown keys, defaults load).  
- **T-DIST-04** — README and USER.md contain zero occurrences of the substring `/path/to/` or `/absolute/path` in user commands.
- **T-DIST-05** — `scripts/install.sh` passes `bash -n`, `shellcheck` (when available) and `--dry-run` in a clean temp directory, and an unknown option exits non-zero (CI `installer` job).  

### 14.10 Out of scope for this section

- Changing default `ui` from `null` (ADR-011).  
- Adding new dispatchers or config keys.  
- Guaranteeing hyprpm behaviour on untested Hyprland commits.  
- Windows / non-Linux packaging.  
- Automated GUI installer.
