# Fix Register — Audit findings 2026-09-15

**Source:** `docs/agent-state/audit/mru-switcher-audit-2026-09-15__auditor-01.md`, `__auditor-02.md`
**Order:** follows auditor-02 §8 (independent fixes → CI → M3 feature work).
**Rule:** nothing in M3 is implemented on top of a known blocker.

| # | Severity | Defect | File(s) | Status | PR |
|---|----------|--------|---------|--------|-----|
| B1 | BLOCKER | `teardown_state()` move-assigns `PluginState{}`; members destroyed in declaration order (scheduler first) → tracker `cancel_pending()` derefs destroyed scheduler (use-after-free, REQ-H-008) | `src/plugin/mru_plugin.cpp`, `src/plugin/mru_plugin.hpp` | ✅ | #5 | Verified in v0.2.0: explicit reverse-order teardown, `scheduler.reset()` last; L-11 (`plugin_shutdown`) added later on top of the fixed ordering |
| B2 | BLOCKER | Focus applied before lock-in released → applied window never becomes MRU head (REQ-RE-003, breaks Alt+Tab toggle) | `src/domain/session_controller.cpp` | ✅ | #6 | `complete_apply()` promotes after `end_session()` unlocks history. Domain tests T-RE-04/05 + new T-RE-06 (reentrant window.active inside focus()). Adapter-side "Hyprland emits sync active inside fullWindowFocus" NOT verified on live compositor — nested env broken on aquamarine 0.56.2 (admitted assumption) |
| H3 | HIGH | Registry holds `PHLWINDOW` strong ref after close; `by_address_` grows forever; accidental ABA protection depends on strong ref | `src/plugin/hypr/identity_registry.*` | ✅ (code) / ⏳ (Q1-Q5) | #7 | #7 switched to `PHLWINDOWREF` weak ref + `lock()` ABA check + `prune_closed()`. Remaining weak-lock identity decisions (Q1-Q5, ADR-016) are PLANNED, not shipped |
| H4 | HIGH | No exception barrier at any compositor entry point (`PLUGIN_INIT`, dispatchers, listeners); exception across C ABI → terminate/UB | `src/plugin/mru_plugin.cpp` | ✅ | #7 | `guarded()` for dispatchers, `guarded_listener()` for Event::bus, try/catch in PLUGIN_INIT/PLUGIN_EXIT. Verified in v0.2.0 |
| H5 | HIGH | Dispatchers registered before `build_state()`; if state build fails, first dispatch derefs null `controller` | `src/plugin/mru_plugin.cpp` | ✅ | #7 | `config → build_state → subscribe_events → register_dispatchers`; dispatchers also null-check `controller`. Verified in v0.2.0 |
| M6 | MEDIUM | One-shot timers not removed from `CEventLoopManager`; `timers_.erase()` inside own callback (auditor could not verify pin; fix symmetric) | `src/plugin/hypr/hypr_scheduler.cpp` | ✅ | #7 |
| M8 | MEDIUM | `apply()` reports `"no windows"` for `FocusFailed`/`InvalidSelection` too (REQ-DISP-002 only requires it for empty-after-prune) | `src/domain/session_controller.cpp` | ✅ | #7 |
| M9 | MEDIUM | `debounce_ms` cast to `int` before clamping; truncation can bypass clamp | `src/plugin/mru_plugin.cpp` | ✅ | #7 |
| M7 | MEDIUM | Valid `default_scope != global` silently yields `no windows`; no warn-once fallback to global | `src/plugin/mru_plugin.cpp`, `hypr_window_source.cpp` | ✅ | #7 |
| CI | HIGH-ish | `.so` never compiled in CI; no GCC build; no ASan/UBSan on domain tests; guard regex misses `hyprutils` | `.github/workflows/ci.yml` | ✅ | #8 |
| L-12 | LOW | Guard regex catches only `#include <hyprland`; misses `hyprutils` and quoted includes (ADR-007/domain) | `.github/workflows/ci.yml` | ✅ | #8 |
| L-13 | LOW | Version string duplicated: CMake `0.0.0` vs `PLUGIN_INIT` `"0.2.0"`; single source needed | `CMakeLists.txt`, `cmake/mru-version.hpp.in`, `src/plugin/mru_plugin.cpp` | ✅ | #8 |
| L-15 | LOW | `mise.toml` uses `cmake = "latest"` (not a pin) | `mise.toml` | ✅ | #8 |
| L-1 | LOW | `fake_clock.cpp` comment promises time-order but `advance()` runs in insertion order | `src/domain/fake_clock.cpp` | ✅ | TBA (fix/low-triage) |
| L-2 | LOW | `run_all()` declared without `inline` in a header (ODR risk on 2-TU builds) | `tests/domain/test_framework.hpp` | ✅ | TBA (fix/low-triage) |
| L-3 | LOW | `Snapshot::at()` uses throwing `vector::at()` behind a caller contract | `src/domain/snapshot.cpp` | ✅ | TBA (fix/low-triage) |
| L-4 | LOW | `PHLWINDOW` passed by value in every registry method (shared_ptr refcount on hot path) | `src/plugin/hypr/identity_registry.*` | ✅ | TBA (fix/low-triage) |
| L-5 | LOW | `resolve()` returns `std::optional<PHLWINDOW>` on an already-nullable type | `src/plugin/hypr/identity_registry.*`, `hypr_window_source.cpp`, `hypr_focus_gateway.cpp`, `mru_plugin.cpp` | ✅ | TBA (fix/low-triage) |
| L-6 | LOW | Unused `cfg_` member dead dependency in `HyprlandWindowSource` | `src/plugin/hypr/hypr_window_source.*`, `mru_plugin.cpp` | ✅ | TBA (fix/low-triage) |
| L-7 | LOW | `focused()` does `is_known()`+`last_ref()` double lookup and returns identity for closed windows | `src/plugin/hypr/identity_registry.*`, `hypr_window_source.cpp` | ✅ | TBA (fix/low-triage) |
| L-8 | LOW | `is_candidate()` missing the `m_isMapped` bit required by REQ-SNAP-002 | `src/plugin/hypr/hypr_window_source.cpp` | ✅ | TBA (fix/low-triage) |
| L-9 | LOW | `#pragma GCC diagnostic ignored "-Wdeprecated-declarations"` covers the whole TU | `src/plugin/mru_plugin.cpp` | ✅ | TBA (fix/low-triage) |
| L-10 | LOW | `cycle()` active branch derefs `snapshot_` without the null-check `on_window_invalid()` has | `src/domain/session_controller.cpp` | ✅ | TBA (fix/low-triage) |
| L-11 | LOW | `SessionEndReason::PluginShutdown` unused; `PLUGIN_EXIT` leaves an active session dangling | `src/domain/session_controller.*`, `src/plugin/mru_plugin.cpp` | ✅ | TBA (fix/low-triage) |
| L-14 | LOW | `hyprpm.toml` `authors = ["TBD"]` — hyprpm publish blocker, needs release-checklist note | `hyprpm.toml` | ✅ | TBA (fix/low-triage) |
| L-16 | LOW | README claims M0-only, "`.so` is M2", "`MRU_BUILD_PLUGIN=ON` fails configure" — all stale | `README.md` | ✅ | TBA (fix/low-triage) |
| L-17 | LOW | `merge_mru_order` O(n²) — needs a "fine for tens of windows" note, no blind optimization | `src/plugin/mru_merge.cpp` | ✅ | TBA (fix/low-triage) |

## LOW fixes — resolution notes (fix/low-triage)

| # | Resolution |
|---|------------|
| L-1 | `advance()` collects `(run_at, id)` pairs and sorts them; comment now matches implementation (time order, schedule order on ties). |
| L-2 | `inline int run_all()` in header. |
| L-3 | Contract + assert: `at()` now uses `assert(i < size())` + `operator[]`; throwing `vector::at()` removed from that path, throw is intentionally not documented (caller pre: `i < size()` in header). |
| L-4 | All `PHLWINDOW` parameters (and free `address_of`) take `const PHLWINDOW&`, covering the hot `candidates()` path. |
| L-5 | `resolve()` returns `PHLWINDOW` directly (`{}` = unresolved); all 4 call sites updated (`w && is_candidate(w)`, `static_cast<bool>(...)`, focus passes `w` directly). |
| L-6 | `cfg_` member + constructor param + `config_value.hpp` include removed from `HyprlandWindowSource`; facade call updated. |
| L-7 | New single-lookup `live_ref(const PHLWINDOW&)` (find once, skip closed) replaces `is_known()+last_ref()`; `last_ref()` unchanged for the close/destroy listeners that legally need the identity of a just-closed window. |
| L-8 | `is_candidate()` now requires `w->m_isMapped` (public member on pinned 0.56.2; there is no `mapped()` accessor). |
| L-9 | `ignore` wrapped in push/pop around only the two deprecated-API call sites (`cfg_int`/`cfg_str`, `register_config_keys`). |
| L-10 | `assert(snapshot_)` before the active-branch deref, matching the invariant `on_window_invalid()` guards. |
| L-11 | New `SessionController::plugin_shutdown()` (no-op when Idle) ends the active session with `SessionEndReason::PluginShutdown`; `teardown_state()` calls it before `controller.reset()` so `on_session_end(Cancelled)` fires, `active_`/`snapshot_` clear, and history unlocks while controller/ui/tracker are still alive. Tests: `bonus_plugin_shutdown_ends_active_session`, `bonus_plugin_shutdown_when_idle_is_noop`. |
| L-14 | `authors = ["TBD"]` kept (do not fake); comment now points at the release checklist (AGENTS.md §16.2 / ROADMAP M6) to fill authors + `commit_pins` before any tag/publish. |
| L-16 | README status rewritten: M0/M1/M2 done, M3 next, `.so` builds with `MRU_BUILD_PLUGIN=ON` against the v0.56.2 pin; dispatchers section marked implemented; roadmap table shows statuses. |
| L-17 | Short comment on the `std::find` dedup: O(n²) is fine for tens of windows; deliberately no `unordered_set`. |

**Remaining:** none from §5 except L-12/L-13/L-15 (already merged in #8). `SessionEndReason::PluginShutdown` handled by `format_status` since #7.

## Design-gate answers to consolidate in ADR-016 (after fixes)

| Q | Auditor 01 | Auditor 02 | Consolidated |
|---|-----------|-----------|--------------|
| Q1 scope model | A | A | A — pure domain predicate `scope_matches(Scope, const WindowMeta&, const FocusContext&)`; opaque `uint64` ids; adapter = thin `PHLWINDOW → WindowMeta` bridge |
| Q2 API pinning | A | A | A — pin v0.56.2 + explicit check |
| Q3 scope arch | A | A | A — predicates pure domain ops, resolver responsibilities in adapter |
| Q4 special workspace | A | A + extend to all 5 scopes | A — special ws participates only while its workspace is shown on its monitor, for **all** scopes (not just visible); plus `is_candidate()` adds `m_isMapped` |
| Q5 external_socket / hyprlang V2 | A (separate tech PR) | A with precondition CI builds `.so` first | A — V2 migration as isolated PR after CI step; `external_socket` reserved key registered in M3 with docs note |

## M3 implementation order (after fixes)

1. `ci/plugin-build` — pinned headers container, real `.so` build, ASan/UBSan, GCC
2. `feat/m3-config-v2` — hyprlang V2 migration (isolated tech risk)
3. `feat/m3-scope-predicate` — pure domain predicate + `WindowMeta`/`FocusContext` + T-SC-01/02
4. `feat/m3-scope-adapter` — build `WindowMeta`, all 5 scopes, special-workspace semantics, remaining config keys (+`external_socket` reserved)
5. ADR-016 — consolidate Q1–Q5 + special-workspace semantics + identity-validity-by-weak-lock
6. Docs: USER.md/API.md reserved note, README refresh, PROGRESS, CHANGELOG, nested smoke