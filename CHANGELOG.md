# Changelog

All notable changes to this project are documented in this file.  
Format based on [Keep a Changelog](https://keepachangelog.com/).  
Versioning: see `docs/VERSION-MAP.md` and `AGENTS.md` §15.

## [Unreleased]

### Added

- M4 border UI contract (docs/design gate, no runtime behavior yet): **ADR-017 (Accepted)** — `BorderHighlightUI` with solid border highlight via public window props ("public props first, fail-soft"; concrete symbols adapter-private, recorded in `docs/COMPAT.md`; R0 memo pinned to Hyprland 0.56.2 / `efb5099`), style interface (`border_style=solid`; `pulse`/`dim` reserved), and new keys `border_color` (default **`0xffffd9a0`**) / `border_size` (default **`-1`** = leave size untouched) / `border_style` (default **`solid`**) under `plugin:mru-switcher:`; default `ui` stays **`null`**. SPEC §5 gains **REQ-UI-001..011** with tests **T-UI-03..07** continuing the T-UI series.
- M4-S1 runtime border backend (behavior; `ui = border` opt-in, default stays `null`): `BorderHighlightUI` implements `UIPort` on the R0 mechanism — per-window `setprop active/inactive_border_color` via `invokeHyprctlCommand` (public props first, ADR-017 / REQ-UI-011; concrete symbols adapter-private in `docs/COMPAT.md`). `SessionUIBackendProxy` rebuilds the backend from the **current** config at each session start and freezes it for that session, so a `hyprctl reload` changes only the next session (REQ-UI-009). A session-start runtime probe degrades the backend to null for that session with a single warn-once (REQ-UI-002); restore is **by value** — a colour is overridden only after both prior values were read back and is restored from the captured values, never a bare `-1`/`unset` on a colour slot (R0 F10, REQ-UI-005). Dispatcher grammar and `mru:status` unchanged; nest smoke (focus-follow, latency, restore probes) is M4-S3.

- M4-S3 nest smoke — docs traceability (D1): `docs/COMPAT.md` records the `hyprctl keyword` channel limitation on Hyprland 0.56.2 (keyword changes to `plugin:mru-switcher:*` keys do not reach the plugin cache; config file + `hyprctl reload` is the supported runtime channel) plus the setprop/getprop grammar asymmetry and the border_size probe correction; `docs/USER.md` "Config reload" carries the user-facing warning; `docs/REQ-TRACE.md` REQ-UI-004/005/011 rows note the live smoke (report: `docs/agent-state/reports/2026-09-18-m4-s3-nest-smoke.md`)

### Fixed

- Border restore values are normalized to the `setprop` grammar: `getprop` returns unprefixed `<hex6> <N>deg` while `setprop` accepts only `0x…`/`rgb()`/`rgba()` without suffix — restoring captured output verbatim produced empty-gradient (invisible) borders (D2, found in the M4-S3 nest smoke; fixed by `normalize_capture` in `98c0dd5`)
- `border_size` is now restored by the captured value with an `unset` fallback (R0 correction: `getprop border_size` works on this pin, per COMPAT M4 nest smoke 2026-09-18)

<!-- placeholder for post-v0.3.0 changes -->

## [0.3.0] - 2026-09-17

### Changed

- Config layer migrated from the legacy V1 config API to hyprlang V2 (`addConfigValueV2`, M3-S1): every `plugin:mru-switcher:` key is registered once in `PLUGIN_INIT` as a typed `Config::Values::*` value whose `SP` lives in `PluginState`; reads go through `mru::plugin::config::read()` (typed `value_traits` + `underlying()` contract) instead of `getConfigValue`/`dataPtr()` casts (RP-1, RP-3). Behavior is unchanged: `debounce_ms` clamp `[0,5000]`, enum fallback + once-warn, reload-next-session-only
- PLUGIN_INIT fail-closed: hash mismatch and config-registration failure now throw so the compositor's `loadPluginInternal` (pin 0.56.2) unwinds PLUGIN_INIT and unloads the plugin — an empty `PLUGIN_DESCRIPTION_INFO` alone does not unload on this pin (HIGH-4, issue #16)
- `default_scope` resolves as parsed: the MEDIUM-7 "force global until M3" shim is removed now that all five scopes are implemented by the M3-S3 adapter filter (REQ-SC-002)

### Added

- M3-S2 pure domain scope layer (issue #18): `WindowMeta` (opaque `monitor_id`/`workspace_id` `uint64`, `mapped`, `hidden`, `app_class`), snapshot-time `FocusContext` (passed by const ref, never recomputed), and stateless free function `scope_matches()` (REQ-SC-002/002a/002b, ADR-016). Uniform special-workspace rule covers all five scopes; `app` is byte-exact case-sensitive with empty-class folds (T-SC-01..04, domain tests)
- M3-S3 adapter wiring (issue #20): `HyprlandWindowSource` now translates pinned-0.56.2 `PHLWINDOW` → `WindowMeta` (`monitor_id`, `workspace_id`, `mapped`, `app_class = m_class`) and filters both candidate paths through `scope_matches()` (REQ-SC-002/002a/002b). `FocusContext` is built once per snapshot in `current_focus()`; the visible-workspace set comes from a single `State::monitorState()->monitors()` enumeration (active + active-special) and is the same vector `hidden` is derived from, so adapter/domain drift is impossible by construction. `global` remains M2-identical (drop-in)
- M3-S3 (S3-4) config surface complete: `plugin:mru-switcher:external_socket` registered (default `""`, reserved for M5, no effect until then), completing the documented 8-key SPEC §4 surface (ADR-016 __5__)

### Documentation

- USER.md validated against live behavior; config reload documented (`hyprctl reload` → next session only, active session untouched — REQ-CFG-002/003, REQ-S-009); `mru:status` IPC caveat on 0.56.x; scratchpad FAQ corrected (a hidden scratchpad is excluded from every scope)

## [0.2.0] - 2026-09-15

### Added

- M2 MVP plugin (PR #2): loadable `mru-switcher.so` — fail-closed hash check in `PLUGIN_INIT`, dispatchers `mru:cycle`/`mru:apply`/`mru:cancel` (addDispatcherV2), Event::bus subscriptions (window.active/close, config.reloaded), Hyprland adapters (WindowSource, FocusGateway, SchedulerPort via `CEventLoopTimer`), WindowIdentityRegistry (address+generation, ADR-013), Null UI with warn-once fallback for `border`/`external`, config surface under `plugin:mru-switcher:` incl. `start_offset` (REQ-SEL-002) and live `debounce_ms` reload
- M1 domain core (pure C++, no Hyprland): `WindowRef` (address+generation, ADR-013), `Scope`/`Direction`/`StartOffset`/`SessionPolicy`, `Snapshot` (+prune helper), `SessionController` (Idle<->Active, REQ-S-001..011), `HistoryTracker` with debounce + lock-in, `SchedulerPort`/`FakeClock`
- Domain ports (interfaces for M2 adapters): `WindowSource`, `FocusGateway` (`FocusResult`), `UIPort`
- Unit tests T-S-01..08, T-F-01..05, T-RE-01, T-H-01..05, T-SEL-01..03, T-ID-01, T-H-seed, T-CFG-02 (ctest 9/9 green)
- Plan artifact: `docs/agent-state/plans/2026-09-13-m1-domain.md`
- `mru:status` dispatcher with SPEC section 3.4 payload and `last_end` diagnostics (D7)
- ADR-015: plugin-owned candidate order, register-on-sight (D4)
- `merge_mru_order` Hyprland-free helper for candidate ordering (ADR-007/ADR-015)
- M2 closeout plan: `docs/agent-state/plans/2026-09-15-m2-closeout.md`
- M2 audit report: `docs/agent-state/reports/2026-09-15-m2-audit.md`

### Changed

- CI builds the real `mru-switcher.so` against pinned Hyprland headers (Arch container, `plugin-build` job); adds ASan/UBSan and GCC test jobs; extends ADR-007/domain include guards to `hyprutils` and quoted includes (L-12)
- Plugin version now comes from a single source: CMake `project(VERSION …)` → `cmake/mru-version.hpp.in` → `PLUGIN_INIT` (was hard-coded `"0.2.0"` while CMake said `0.0.0`, L-13)
- `mise.toml` pins `cmake` to a concrete version instead of `latest` (L-15)
- Documentation set (ARCHITECTURE, SPEC, ADRs, ROADMAP, USER, API, diagrams)
- Hyprland plugin system reference
- Agent skills (project) and AGENTS.md orchestrator contract
- VERSION-MAP, agent-state progress/session templates
- Design gate: SchedulerPort, WindowRef generation, apply-after-invalidation, UI default null
- COMPAT.md, REQ-TRACE.md (expanded), design-notes/*
- FAILURE-MODES, TRANSITION-TABLE, OBSERVABILITY, THREAT-MODEL, GOVERNANCE, SUPPORT-AND-RELEASE
- CMake/CI scaffold, hyprpm.toml template, domain placeholder
- Align API.md / ARCHITECTURE.md UI default to `null` (ADR-011)
- Normative: omitted `mru:cycle` direction = `next` (REQ-DISP-001)
- Normative: empty apply after prune always errors `no windows` (REQ-DISP-002)
- App scope uses `class` only (not initialClass)
- CI: replaced stale `plugin-flag-fails-closed` job with `plugin-guards` (D1)
- CI: domain dependency guard + plugin target/config checks
- Candidate order: plugin-owned MRU first, adapter enumeration appended (ADR-015, D4)
- `SessionController::set_policy()` refreshes policy for next session only (REQ-CFG-002, D5)
- Dispatcher grammar: scope token accepted without explicit direction (REQ-DISP-003, D6)
- Plugin tests use shared `test_framework.hpp` (NDEBUG-proof, D3)
- All source files pass clang-format 22.1.8 (D2)

### Fixed

- CI: `plugin-flag-fails-closed` job was broken after M2 target existed (D1)
- Plugin tests used `assert()` which is a no-op in Release builds — rewritten with shared test framework (D3)
- Candidate order came from compositor history instead of plugin-owned MRU (D4)
- `WindowIdentityRegistry` filtered candidates via empty `is_known()` — windows opened before plugin load were invisible (D4)
- Config reload only refreshed `debounce_ms`; `default_scope`, `start_offset`, `wrap`, etc. never updated (D5)
- `mru:cycle workspace` failed with `unknown direction: workspace` — scope token without direction now accepted (D6)
- `mru:status` documented in SPEC/USER.md/API.md but not registered as dispatcher (D7)
- STRING config read via `dataPtr()` threw `std::bad_any_cast` on hyprlang 0.6.x — fixed via `getDataStaticPtr()` (#3)
- **Audit BLOCKER-1 (UAF in teardown):** `PluginState` members were destroyed in declaration order with the scheduler first, so `HistoryTracker::cancel_pending()` dereferenced a destroyed scheduler. `teardown_state()` now tears down in explicit reverse order with `scheduler.reset()` last (#5)
- **Audit BLOCKER-2 (MRU head after apply):** `apply()` focused the selected window while history lock-in was still held, so the applied window never became the MRU head. `complete_apply()` now ends the session (unlocking history) before `tracker_.on_focus(applied)` (#6); reentrant `window.active` inside focus is swallowed by lock-in (T-RE-06, #10)
- **Audit HIGH-3 (registry lifetime):** `WindowIdentityRegistry` held strong `PHLWINDOW` refs and grew `by_address_` forever; switched to weak `PHLWINDOWREF` with `lock()`-based ABA protection and `prune_closed()` (#7); weak-lock identity formalized later in ADR-016
- **Audit HIGH-4 (exception barrier):** no `try/catch` at compositor entry points (`PLUGIN_INIT`, dispatchers, Event::bus listeners) — exceptions across the C ABI terminate the compositor. Added `guarded()`/`guarded_listener()` and enclosed `PLUGIN_INIT`/`PLUGIN_EXIT` (#7)
- **Audit HIGH-5 (dispatcher ordering):** dispatchers were registered before `build_state()`; a failed state build left `controller` null at first dispatch. Now `config → build_state → subscribe_events → register_dispatchers`, with dispatchers null-checking `controller` (#7)
- Duplicate `### Changed` heading in changelog
- FM-01 dual success/error contract
- `PLUGIN_EXIT` now ends an active session before teardown — UI receives `on_session_end(Cancelled)`, state is cleared, and history unlocks; `SessionEndReason::PluginShutdown` is finally used (L-11)
- `Snapshot::at()` no longer throws into the compositor — bounds are a documented caller contract enforced by assert (L-3)
- `cycle()` asserts the active-snapshot invariant instead of silently dereferencing a possibly-null snapshot (L-10)
- `focused()` uses a single registry lookup and no longer returns the identity of a closed window (L-7)
- `is_candidate()` now requires the mapped bit (`m_isMapped`) per REQ-SNAP-002 "mapped, not hidden, not fading" (L-8)
- `resolve()` returns `PHLWINDOW` directly instead of `optional<PHLWINDOW>`, and `PHLWINDOW` is passed by `const&` in all registry methods (L-4/L-5)
- Removed the unused `cfg_` dead dependency from `HyprlandWindowSource` (L-6)
- `FakeClock::advance()` runs due jobs in `(run_at, id)` order, matching its documented contract (L-1)
- The `-Wdeprecated-declarations` suppression in the facade is now scoped push/pop around only the legacy config-API call sites (L-9)

## [0.0.0] - 2026-09-13

### Added

- Initial M0 documentation-only baseline
