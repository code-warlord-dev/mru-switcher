# Requirement → test → implementation trace

**Status:** Living — update on every behavioral PR  
**Rule:** Every `REQ-*` in SPEC should appear here.

| SPEC ID | Test IDs | Implementation area (target) | Milestone |
|---------|----------|------------------------------|-----------|
| REQ-S-001 | T-S-01 | SessionController | M1 |
| REQ-S-002 | T-S-01, T-S-04 | SessionController | M1 |
| REQ-S-003 | T-S-01 | SessionController | M1 |
| REQ-S-004 | T-F-02, T-S-02 | SessionController + FocusGateway | M1–M2 |
| REQ-S-005 | T-S-03, T-S-05, T-S-06 | SessionController | M1–M2 |
| REQ-S-006 | T-F-04 | SessionController | M1 |
| REQ-S-007 | T-S-05 | SessionController | M1 |
| REQ-SNAP-001 | T-S-01 | Snapshot / History order | M1 |
| REQ-SNAP-002 | T-SC-01 | Snapshot + validity | M1–M3 |
| REQ-SNAP-003 | T-S-01 | Snapshot immutability | M1 |
| REQ-SNAP-004 | T-F-03 | prune + clamp | M1 |
| REQ-SEL-001 | T-SEL-01 | start_offset second | M1 |
| REQ-SEL-002 | T-SEL-01 | start_offset first | M1 |
| REQ-SEL-003 | T-SEL-02 | next wrap | M1 |
| REQ-SEL-004 | T-SEL-02 | prev wrap | M1 |
| REQ-SEL-005 | T-SEL-03 | wrap=false | M1 |
| REQ-F-001 | T-F-01 | FocusGateway only | M1–M2 |
| REQ-F-002 | T-F-02, T-S-02 | apply path | M2 |
| REQ-F-003 | T-F-01 | cycle no focus | M1–M2 |
| REQ-F-004 | T-F-03 | apply-after-invalidation | M1–M2 |
| REQ-F-005 | T-ID-01 | validate generation | M1–M2 |
| REQ-F-006 | T-F-02, T-F-03 | single focus per apply | M1 |
| REQ-F-007 | T-F-03, T-F-04 | single UI end | M1 |
| REQ-H-001 | T-H-01 | lock-in | M1 |
| REQ-H-002 | T-H-02 | debounce schedule | M1 |
| REQ-H-003 | T-H-02 | debounce commit | M1 |
| REQ-H-004 | integration | History seed | M2 |
| REQ-H-005 | — | reason filter future | future |
| REQ-H-006 | T-H-03 | single pending job | M1 |
| REQ-H-007 | T-H-02 | FakeClock / main thread | M1–M2 |
| REQ-H-008 | T-H-04 | cancel on unload | M1–M2 |
| REQ-H-009 | T-H-05 | invalid before fire | M1 |
| REQ-ID-001 | T-ID-01 | WindowRef type | M1 |
| REQ-ID-002 | T-ID-01 | generation bump | M2 |
| REQ-ID-003 | T-ID-01 | equality | M1 |
| REQ-ID-004 | T-ID-01 | snapshot stores ref | M1 |
| REQ-ID-005 | T-ID-01 | validate | M1–M2 |
| REQ-SC-001 | T-SC-01 | scope resolve | M3 |
| REQ-SC-002 global | T-SC-01 | ScopeResolver | M3 |
| REQ-SC-002 monitor | T-SC-01 | ScopeResolver | M3 |
| REQ-SC-002 workspace | T-SC-01 | ScopeResolver | M3 |
| REQ-SC-002 visible | T-SC-01 | ScopeResolver | M3 |
| REQ-SC-002 app | T-SC-02 | **class** only | M3 |
| REQ-SC-003 | T-SC-03 | unknown scope fail | M3 |
| REQ-DISP-001 | T-DISP-01 | omitted direction = next | M1–M2 |
| REQ-DISP-002 | T-F-04 | empty apply → no windows | M1–M2 |
| REQ-CFG-001 | T-CFG-01 | invalid enum fallback | M2–M3 |
| REQ-CFG-002 | T-CFG-02 | reload next session | M3 |
| REQ-CFG-003 | T-CFG-02 | ui next session | M3 |
| REQ-UI-001 | T-UI-02 | UI isolation | M2–M4 |
| REQ-UI-002 | T-UI-01 | backend → null | M2 |
| REQ-UI-003 | T-UI-01 | M2 null default | M2 |
| REQ-R-001 | T-S-05 | restore on cancel | M1–M2 |
| REQ-R-002 | T-S-06 | origin invalid | M1–M2 |
| REQ-HL-001 | nest | hash check | M2 |
| REQ-HL-002 | nest | addDispatcherV2 | M2 |
| REQ-HL-003 | nest | Event::bus | M2 |
| REQ-HL-004 | review | hooks off | M2 |
| REQ-HL-005 | review | no extra threads | M2 |
| REQ-HL-006 | review | plugin: config | M2 |
| REQ-ERR-001 | T-ERR-01 | exception → result | M2 |
| REQ-ERR-002 | T-F-03 | dead prune | M1–M2 |
| REQ-ERR-003 | nest | fail closed init | M2 |
| REQ-SCH-001 | M2 | production timer | M2 |
| REQ-SCH-002 | T-H-02 | FakeClock | M1 |
| REQ-SCH-003 | T-H-03 | job replace | M1 |

## Test ID index

| Test ID | Intent | Milestone |
|---------|--------|-----------|
| T-S-01 | snapshot freeze on cycle | M1 |
| T-S-02 | apply focuses and ends | M1–M2 |
| T-S-03 | cancel without apply focus | M1 |
| T-S-04 | empty candidates fail cycle | M1 |
| T-S-05 | restore_focus_on_cancel valid | M1–M2 |
| T-S-06 | restore origin invalid | M1–M2 |
| T-SEL-01 | start_offset second | M1 |
| T-SEL-02 | wrap next/prev | M1 |
| T-SEL-03 | wrap false clamp | M1 |
| T-H-01 | lock-in | M1 |
| T-H-02 | debounce quiet period | M1 |
| T-H-03 | replace pending | M1 |
| T-H-04 | cancel on destroy | M1 |
| T-H-05 | invalid before fire | M1 |
| T-F-01 | cycle no FocusGateway | M1 |
| T-F-02 | apply one focus | M1 |
| T-F-03 | prune/clamp/apply | M1 |
| T-F-04 | empty → error no windows | M1 |
| T-ID-01 | stale generation | M1 |
| T-DISP-01 | omitted direction = next | M1 |
| T-SC-01 | scopes filter | M3 |
| T-SC-02 | app uses class | M3 |
| T-SC-03 | unknown scope | M3 |
| T-CFG-01 | enum fallback | M3 |
| T-CFG-02 | reload semantics | M3 |
| T-UI-01 | backend fallback | M2 |
| T-UI-02 | UI throw isolated | M4 |
| T-ERR-01 | exception mapping | M2 |
