# M6-T2 docs==implementation audit (issue #46)

Date: 2026-09-19 | Branch: `docs/m6-t2-docs-impl-audit` (worktree /tmp/mru-t2, NOT pushed)
Scope: SPEC/USER/API/COMPAT/ARCHITECTURE/OBSERVABILITY vs src+include+tests.
Method: three read-only subagent passes (SPEC+ARCH / USER+API+COMPAT / OBSERVABILITY+Appendix B
byte-level) + orchestrator verification of every divergence against code.
Rule: no behavior change - only docs (i), code comments (ii), new asserts (iii);
behavior questions -> follow-up issue (iv).

## Checked OK (no action)

| # | Claim | Code |
|---|-------|------|
| 1 | 4 dispatchers via addDispatcherV2 | mru_plugin.cpp:124-131 |
| 2 | cycle grammar, next default, scope-alone, error strings | dispatch_args.cpp:28-61, test_dispatch_args.cpp |
| 3 | cycle empty -> no windows; apply InvalidTargetx2/Failed/empty (2.8); apply/cancel Idle idempotent | session_controller.cpp:39-42,57-125,133-146 |
| 4 | 11 keys names/types/defaults, PLUGIN_INIT-only, clamp [0,5000], enum fallbacks, reload (debounce->later commits, rest->next session) | config_v2.cpp:13-89,123-124, config_value.cpp, mru_plugin.cpp:195-210,378 |
| 5 | Snapshot frozen+prune+clamp, virtual selection, 1 focus/apply via gateway, restore iff valid origin | session_controller.cpp, selection.cpp |
| 6 | Scopes: scratchpad-hidden gate, byte-exact app class, degrade-to-global (monitor/workspace/app); visible no-focus -> empty (SPEC-conformant, USER.md:127 documents) | scope_predicate.cpp, test_scope_predicate.cpp |
| 7 | Border solid-only+warn-once, verbatim setprop color, -1 untouched, restore-by-value | border_highlight_ui.cpp, config_v2.* |
| 8 | Appendix B wire-exact both ways: framing, 0x-lowercase addr, JSON escapes, v:1, 64KiB cap, single-client nonblock, best-effort | overlay_protocol.cpp:10-225, overlay_socket_server.cpp, test_overlay_protocol.cpp |
| 9 | apply/cancel/status ignore extra args - docs claim bare grammar, no contradiction (tolerant) | mru_plugin.cpp:89-119 |
| 10 | SPEC 3.4 frozen payload + COMPAT IPC host-limitation note - accurate, untouched | SPEC.md:322-343, COMPAT.md:94-106 |

## Divergences fixed

| ID | Divergence | Class | Fix (commit 1 unless noted) |
|----|------------|-------|----------------|
| F-1 | OBSERVABILITY mru:status verbose line (session_id=/ui=/pending_debounce=/pruned_total=) - none exist; real output active=/index=/size=/scope=/session=/last_end= (status_format.cpp:27-35; zero hits in src/) | (i) docs-fix | OBSERVABILITY rewritten to frozen format+order+last_end enum+no-verbose |
| F-2 | status_format.hpp:13-14 informative...before 1.0 vs M6-T1 freeze | (ii) comment | Frozen/additive-only |
| F-3 | config_v2.hpp:32-33 not-yet-consumed - stale M3, M5 consumes (config_v2.cpp:123-124, mru_plugin.cpp:249-324) | (ii) comment | REQ-O-001/ADR-018 consumed |
| F-4 | API border_color type color/string - code registers String only (config_v2.hpp:23-25 deliberate) | (i) docs-fix | Type string + verbatim note |
| F-5 | USER quick-start omits external_socket while reload section requires it | (i) docs-fix | Added example line |
| F-6 | USER next-session hides immediate socket teardown (mru_plugin.cpp:195-210) | (i) clarify | Teardown sentence (COMPAT precise) |
| F-7 | ARCH 10 status row omits frozen session=/last_end=+idle-scope | (i) docs-fix | Frozen payload row |
| F-8 | ARCH 11 example omits external_socket | (i) docs-fix | Added line + REQ-O-001 |
| F-9 | ARCH 11 UI keys part of session policy snapshot - SessionPolicy (scope.hpp:9-15) has no UI fields; freeze via SessionUIBackendProxy (session_ui.cpp:9-33) | (i) docs-fix | Proxy-freeze reword |
| F-10 | ARCH 2 fullHistory() preferred MRU source - code primary is HistoryTracker.order(), compositor = seed/fallback tail (hypr_window_source.cpp:84-120, ADR-015) | (i) docs-fix | Seed/fallback reword |
| F-11 | Strict-parse test 3.4 substring-only (test_status_format.cpp:11-26) | (iii) test-add | Exact full-string EQ active+idle + suffix test (commit 2) |
| F-12 | test_status_format.cpp:10 (+ informative extras) stale comment | (ii) test comment | Frozen payload (commit 2) |

## Follow-up issue (class iv - behavior NOT changed)

- #67 lock_history_on_session=false is a no-op: SessionController::on_focus
  (session_controller.cpp:164-168) ignores events whenever Active unconditionally;
  the flag only gates tracker.set_session_locked (:49-50,140-141), so the unlocked
  tracker never receives events. SPEC REQ-H-001/REQ-S-009 imply the flag controls
  lock-in. Needs ADR: forward-to-tracker when unlocked vs SPEC-clarify. Untouched.

## Rejected (checked, NOT divergences)

- visible no-focus -> empty while monitor/workspace/app degrade to global: SPEC
  REQ-SC-002 + no-focus paragraph + USER.md:127 explicit; t_sc_02 pins degrade for
  monitor/workspace only. Conformant.
- apply/cancel/status extra-arg tolerance; hyprctl keyword non-observation;
  Lua snippet; hyprpm.toml provisional hash: internally consistent.
- Historical until-1.0 rows in agent-state/research|plans + THREAT-MODEL.md:42
  (unrelated parser roadmap): informational, kept. Live informative hits after fix:
  only those + COMPAT.md:102 (accurate history sentence). Final grep clean.

## Commits (not pushed)

1. docs(m6-t2): docs==implementation audit fixes (issue #46) - F-1..F-10
2. test(m6-t2): strict mru:status full-string asserts (issue #46) - F-11+F-12

## Tests

cmake -S . -B build/t2 -DMRU_BUILD_TESTS=ON -DMRU_BUILD_PLUGIN=OFF && cmake --build
build/t2 -j8 && ctest -> 17/17 pass (incl. updated plugin_status_format).
