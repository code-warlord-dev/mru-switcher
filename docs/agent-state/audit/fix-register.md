# Fix Register — Audit findings 2026-09-15

**Source:** `docs/agent-state/audit/mru-switcher-audit-2026-09-15__auditor-01.md`, `__auditor-02.md`
**Order:** follows auditor-02 §8 (independent fixes → CI → M3 feature work).
**Rule:** nothing in M3 is implemented on top of a known blocker.

| # | Severity | Defect | File(s) | Status | PR |
|---|----------|--------|---------|--------|-----|
| B1 | BLOCKER | `teardown_state()` move-assigns `PluginState{}`; members destroyed in declaration order (scheduler first) → tracker `cancel_pending()` derefs destroyed scheduler (use-after-free, REQ-H-008) | `src/plugin/mru_plugin.cpp`, `src/plugin/mru_plugin.hpp` | | |
| B2 | BLOCKER | Focus applied before lock-in released → applied window never becomes MRU head (REQ-RE-003, breaks Alt+Tab toggle) | `src/domain/session_controller.cpp` | | |
| H3 | HIGH | Registry holds `PHLWINDOW` strong ref after close; `by_address_` grows forever; accidental ABA protection depends on strong ref | `src/plugin/hypr/identity_registry.*` | | |
| H4 | HIGH | No exception barrier at any compositor entry point (`PLUGIN_INIT`, dispatchers, listeners); exception across C ABI → terminate/UB | `src/plugin/mru_plugin.cpp` | | |
| H5 | HIGH | Dispatchers registered before `build_state()`; if state build fails, first dispatch derefs null `controller` | `src/plugin/mru_plugin.cpp` | | |
| M6 | MEDIUM | One-shot timers not removed from `CEventLoopManager`; `timers_.erase()` inside own callback (auditor could not verify pin; fix symmetric) | `src/plugin/hypr/hypr_scheduler.cpp` | | |
| M8 | MEDIUM | `apply()` reports `"no windows"` for `FocusFailed`/`InvalidSelection` too (REQ-DISP-002 only requires it for empty-after-prune) | `src/domain/session_controller.cpp` | | |
| M9 | MEDIUM | `debounce_ms` cast to `int` before clamping; truncation can bypass clamp | `src/plugin/mru_plugin.cpp` | | |
| M7 | MEDIUM | Valid `default_scope != global` silently yields `no windows`; no warn-once fallback to global | `src/plugin/mru_plugin.cpp`, `hypr_window_source.cpp` | | |
| CI | HIGH-ish | `.so` never compiled in CI; no GCC build; no ASan/UBSan on domain tests; guard regex misses `hyprutils` | `.github/workflows/ci.yml` | | |
| L-1..17 | LOW | see auditor-02 §5 | multiple | | |

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