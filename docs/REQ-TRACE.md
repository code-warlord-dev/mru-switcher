# Requirement → test → implementation trace

**Status:** Living  
**Rule:** Behavioral PRs update this table when they touch REQ/T IDs.

| SPEC ID | Test IDs | Implementation area (target) | Milestone |
|---------|----------|------------------------------|-----------|
| REQ-S-001 | T-S-01 | SessionController | M1 |
| REQ-S-002 | T-S-01, T-S-04 | SessionController | M1 |
| REQ-S-003 | T-S-01 | SessionController | M1 |
| REQ-S-004 | T-F-02 | SessionController + FocusGateway | M1–M2 |
| REQ-S-005 | T-S-05, T-S-06 | SessionController | M1–M2 |
| REQ-S-006 | T-F-04 | SessionController | M1 |
| REQ-S-007 | T-S-05 | SessionController | M1 |
| REQ-SNAP-* | T-S-01 | Snapshot builder | M1 |
| REQ-SEL-001/002 | T-SEL-01 | Selection | M1 |
| REQ-SEL-003/004 | T-S-01 | Selection | M1 |
| REQ-SEL-005 | T-SEL-03 | Selection | M1 |
| REQ-F-001/003 | T-F-01 | Dispatchers / Gateway | M1–M2 |
| REQ-F-002 | T-F-02 | Apply path | M2 |
| REQ-F-004–007 | T-F-03, T-F-04 | Apply-after-invalidation | M1–M2 |
| REQ-H-001 | T-H-01 | HistoryTracker | M1 |
| REQ-H-002/003 | T-H-02 | HistoryTracker + Scheduler | M1 |
| REQ-H-006–009 | T-H-03, T-H-04, T-H-05 | HistoryTracker + Scheduler | M1–M2 |
| REQ-ID-* | T-ID-01 | WindowRef + registry | M1–M2 |
| REQ-SC-* | T-SC-01 | ScopeResolver | M3 |
| REQ-UI-001–003 | T-UI-01 | UIPort | M2–M4 |
| REQ-CFG-* | — | ConfigPort | M2–M3 |
| REQ-SCH-* | T-H-02–05 | SchedulerPort | M1–M2 |

Empty implementation cells are filled as code lands.
