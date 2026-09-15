# Requirement → test → implementation trace

**Status:** Living — update on every behavioral PR  
**Rule:** Every `REQ-*` in SPEC should appear here.

> **M2 status (2026-09-15):** M1 domain + M2 plugin landed. Closeout defects D1–D7 fixed. All rows marked `M2` are done; M3 rows remain.

| SPEC ID | Test IDs | Implementation area (target) | Milestone |
|---------|----------|------------------------------|-----------|
| REQ-S-001 | T-S-01 | SessionController | M1 |
| REQ-S-002 | T-S-01, T-S-04 | SessionController | M1 |
| REQ-S-003 | T-S-01 | SessionController | M1 |
| REQ-S-004 | T-F-02, T-S-02 | SessionController + FocusGateway | M2 |
| REQ-S-005 | T-S-03, T-S-05, T-S-06 | SessionController | M2 |
| REQ-S-006 | T-F-04 | SessionController | M1 |
| REQ-S-007 | T-S-05 | SessionController | M1 |
| REQ-S-008 | T-S-01, observability | session_id monotonic | M2 |
| REQ-S-009 | T-CFG-02 | immutable SessionPolicy | M2 |
| REQ-S-010 | T-S-07 | ignore scope override when Active | M2 |
| REQ-S-011 | — | no session timeout | M1 |
| REQ-SNAP-001 | T-S-01 | Snapshot / History order | M1 |
| REQ-SNAP-001a | T-SNAP-01 (merge_mru_merge) | plugin-owned MRU first (ADR-015) | M2 |
| REQ-SNAP-002 | T-SC-01, nest | Snapshot + validity filter | M2 (global) |
| REQ-SNAP-003 | T-S-01 | Snapshot immutability | M1 |
| REQ-SNAP-004 | T-F-03 | prune + clamp | M1 |
| REQ-SEL-001 | T-SEL-01, T-S-08 | start_offset second | M1 |
| REQ-SEL-002 | T-S-08 | start_offset first | M2 |
| REQ-SEL-003 | T-SEL-02 | next wrap | M1 |
| REQ-SEL-004 | T-SEL-02 | prev wrap | M1 |
| REQ-SEL-005 | T-SEL-01 | wrap=false clamp | M1 |
| REQ-F-001 | T-F-01 | FocusGateway only | M2 |
| REQ-F-002 | T-F-02, T-S-02 | apply path | M2 |
| REQ-F-003 | T-F-01 | cycle no focus | M2 |
| REQ-F-004 | T-F-03 | apply-after-invalidation | M2 |
| REQ-F-005 | T-ID-01 | validate generation | M2 |
| REQ-F-006 | T-F-02, T-F-03 | single focus per apply | M2 |
| REQ-F-007 | T-F-03, T-F-04, T-F-05 | single UI end | M2 |
| REQ-F-008 | T-F-05 | FocusResult InvalidTarget/Failed | M2 |
| REQ-F-009 | T-S-02, T-S-03, T-F-05, mru:status | SessionEndReason | M2 |
| REQ-H-001 | T-H-01, bonus_focus_during_active | lock-in | M2 |
| REQ-H-002 | T-H-02 | debounce schedule | M1 |
| REQ-H-003 | T-H-02 | debounce commit | M1 |
| REQ-H-004 | integration, nest | History seed | M2 |
| REQ-H-004a | ADR-015, nest | plugin-owned MRU source | M2 |
| REQ-H-004b | T-H-seed, nest | register on sight | M2 |
| REQ-H-004c | T-H-seed | at most one entry per identity | M2 |
| REQ-H-005 | — | reason filter future | future |
| REQ-H-006 | T-H-03 | single pending job | M1 |
| REQ-H-007 | T-H-02 | FakeClock / main thread | M2 |
| REQ-H-008 | T-H-04 | cancel on unload | M2 |
| REQ-H-009 | T-H-05 | invalid before fire | M1 |
| REQ-ID-001 | T-ID-01 | WindowRef type | M1 |
| REQ-ID-002 | T-ID-01 | generation bump | M2 |
| REQ-ID-003 | T-ID-01, T-merge-03 | equality | M2 |
| REQ-ID-004 | T-ID-01 | snapshot stores ref | M1 |
| REQ-ID-005 | T-ID-01 | validate | M2 |
| REQ-ID-006 | T-ID-02 | weak-ref lock() validity (ADR-016) | M2 |
| REQ-SC-001 | T-SC-01 | scope resolve | M3 |
| REQ-SC-002 global | T-SC-01, nest | ScopeResolver | M2 (nest) |
| REQ-SC-002 monitor | T-SC-01 | ScopeResolver | M3 |
| REQ-SC-002 workspace | T-SC-01 | ScopeResolver | M3 |
| REQ-SC-002 visible | T-SC-01 | ScopeResolver | M3 |
| REQ-SC-002 app | T-SC-02 | **class** only | M3 |
| REQ-SC-002a | T-SC-03 | special ws only while shown (ADR-016) | M3 |
| REQ-SC-002b | T-SC-04 | byte-exact class compare (ADR-016) | M3 |
| REQ-SC-003 | T-SC-03 | unknown scope fail | M3 |
| REQ-DISP-001 | T-DISP-01, nest | omitted direction = next | M2 |
| REQ-DISP-002 | T-F-04 | empty apply -> no windows | M2 |
| REQ-DISP-003 | T-DISP-03, nest | scope without direction | M2 |
| REQ-CFG-001 | T-CFG-01 | invalid enum fallback | M2 |
| REQ-CFG-002 | T-CFG-02 | reload next session only | M2 |
| REQ-CFG-003 | T-CFG-02 | ui next session | M3 |
| REQ-CFG-004 | T-CFG-03 | debounce_ms clamp to [0,5000] | M2 |
| REQ-UI-001 | T-UI-02 | UI isolation | M4 |
| REQ-UI-002 | T-UI-01 | backend -> null | M2 |
| REQ-UI-003 | T-UI-01 | M2 null default | M2 |
| REQ-R-001 | T-S-05 | restore on cancel | M2 |
| REQ-R-002 | T-S-06 | origin invalid | M2 |
| REQ-HL-001 | nest, CI guards | hash check | M2 |
| REQ-HL-002 | nest | addDispatcherV2 | M2 |
| REQ-HL-003 | nest | Event::bus | M2 |
| REQ-HL-004 | review | hooks off | M2 |
| REQ-HL-005 | review | no extra threads | M2 |
| REQ-HL-006 | CI guards, nest | plugin: config | M2 |
| REQ-ERR-001 | T-ERR-01 | exception -> result | M2 |
| REQ-ERR-002 | T-F-03 | dead prune | M2 |
| REQ-ERR-003 | nest | fail closed init | M2 |
| REQ-SCH-001 | M2 adapter | production timer (CEventLoopTimer) | M2 |
| REQ-SCH-002 | T-H-02 | FakeClock | M1 |
| REQ-SCH-003 | T-H-03 | job replace | M1 |
| REQ-RE-001–004 | T-RE-01, nest | reentrancy / apply+active | M2 |
| REQ-PERF-001–004 | review | hot path constraints | M2 |

## Test ID index

| Test ID | Intent | Milestone |
|---------|--------|-----------|
| T-S-01 | snapshot freeze on cycle | M1 |
| T-S-02 | apply focuses and ends | M2 |
| T-S-03 | cancel without apply focus | M1 |
| T-S-04 | empty candidates fail cycle | M1 |
| T-S-05 | restore_focus_on_cancel valid | M2 |
| T-S-06 | restore origin invalid | M2 |
| T-S-07 | Active cycle ignores new scope token | M2 |
| T-S-08 | start_offset first selects slot 0 | M2 |
| T-SEL-01 | wrap false clamp at edges | M1 |
| T-SEL-02 | wrap next/prev | M1 |
| T-SEL-03 | wrap false clamp | M1 |
| T-H-01 | lock-in | M1 |
| T-H-02 | debounce quiet period | M1 |
| T-H-03 | replace pending | M1 |
| T-H-04 | cancel on destroy | M1 |
| T-H-05 | invalid before fire | M1 |
| T-H-seed | init seed from compositor or empty | M2 |
| T-F-01 | cycle no FocusGateway | M1 |
| T-F-02 | apply one focus | M1 |
| T-F-03 | prune/clamp/apply (a/b/c) | M2 |
| T-F-04 | empty -> error no windows | M1 |
| T-F-05 | FocusResult InvalidTarget/Failed (a/b/c) | M2 |
| T-ID-01 | stale generation | M1 |
| T-DISP-01 | omitted direction = next | M2 |
| T-DISP-02 | direction and scope | M2 |
| T-DISP-03 | scope only (REQ-DISP-003) | M2 |
| T-DISP-04 | invalid args | M2 |
| T-CFG-01 | enum fallback | M2 |
| T-CFG-02 | reload next session only | M2 |
| T-CFG-03 | debounce_ms clamp to [0,5000] | M2 |
| T-CFG-04 | ui backend parse | M2 |
| T-CFG-05 | start_offset parse | M2 |
| T-CFG-06 | config defaults | M2 |
| T-UI-01 | backend fallback | M2 |
| T-UI-02 | UI throw isolated | M4 |
| T-ERR-01 | exception mapping | M2 |
| T-RE-01 | apply + synthetic active does not reopen | M2 |
| T-merge-01 | primary first, fallback appended | M2 |
| T-merge-02 | dedup keeps first occurrence | M2 |
| T-merge-03 | generation is part of identity | M2 |
| T-merge-04 | empty and single sources | M2 |
| T-status-01 | active session status string | M2 |
| T-status-02 | idle session with last_end | M2 |
| T-status-03 | scope names stable | M2 |
| bonus_focus_during_active | lock-in ignores focus events | M2 |
| bonus_on_window_invalid_prunes | FM-04 prune + clamp | M2 |
| bonus_on_window_invalid_empties | REQ-S-006 empty -> Cancelled | M2 |
| bonus_apply_cancel_idempotent | FM-12/13 idempotent no-op | M2 |
