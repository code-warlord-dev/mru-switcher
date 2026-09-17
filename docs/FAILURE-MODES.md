# Failure modes and recovery

**Status:** Normative for behavior when things go wrong  
**Related:** SPEC §2, ADR-014, design-notes/apply-after-invalidation.md

For each mode: **trigger → session state → dispatcher result → UI → history/timers → test IDs**.

| ID | Trigger | Session after | Dispatcher | UI | Side effects | Tests |
|----|---------|---------------|------------|-----|--------------|-------|
| FM-01 | Selected window closed before apply | Idle | if survivor focused → success; if none → **fail** `"no windows"` (never success-noop) | `Cancelled` if no focus; `Applied` if survivor focused | prune | T-F-03, T-F-04 |
| FM-02 | All snapshot windows closed | Idle | fail `"no windows"` | `Cancelled` | clear session | T-F-04, REQ-S-006 |
| FM-03 | Focused window closes while Idle | Idle | n/a | n/a | HistoryTracker: pending debounce cancelled if that ref (REQ-H-009) | T-H-05 |
| FM-04 | Workspace/monitor disappears | Active or Idle | cycle may rebuild only on **new** session; active snapshot pruned of invalid | selection_changed if prune mid-UI optional | prune invalid refs | T-F-03 + scope tests M3 |
| FM-05 | Config reload mid-session | Active unchanged | n/a | n/a | Policy/config for **next** session only (REQ-CFG-002/003, REQ-S-009) | T-CFG-02 |
| FM-06 | Plugin unload while Active | destroyed | n/a | best-effort end not guaranteed if process eject | cancel all SchedulerPort jobs; no UAF | T-H-04 |
| FM-07 | Plugin fault/eject | process-dependent | n/a | n/a | PLUGIN_EXIT may not run; prefer registrations Hyprland tears down | manual / nest |
| FM-08 | External socket dead/corrupt (M5) | unchanged | cycle/apply still work | fallback null semantics; no crash | ignore bad peer | M5 + T-UI-01 pattern |
| FM-09 | UI backend throws/errors | unchanged | success if domain ok | REQ-UI-001 isolation | log once rate-limited | T-UI isolation |
| FM-10 | FocusGateway fails (window vanished under us) | treat as invalid apply path | same as FM-01 | Cancelled or Applied per §2.8 | no second focus | T-F-03 |
| FM-11 | Empty candidates on first cycle | Idle | fail `"no windows"` | no session_start | none | T-S-04 |
| FM-12 | apply while Idle | Idle | success no-op | none | none | idempotent apply |
| FM-13 | cancel while Idle | Idle | success no-op | none | none | idempotent cancel |
| FM-14 | Invalid dispatcher args | unchanged | fail clear error | none | none | fuzz / arg tests |
| FM-15 | Unknown scope token | Idle (no start) | fail | none | none | REQ-SC-003 |
| FM-16 | Rapid cycle stress | Active | success | selection_changed storm OK | no focus; no history update | stress manual + unit |
| FM-17 | Session timeout (if enabled later) | Idle | n/a | Cancelled | optional; **not in v0.1** unless SPEC adds | — |
| FM-18 | restore_focus_on_cancel, origin dead | Idle | success | Cancelled | no focus | T-S-06 |
| FM-19 | wrap=false at edge | Active | success | selection unchanged at edge | none | T-SEL-03 |
| FM-20 | Single candidate, start_offset second | Active | success | index 0 | min(1,len-1)=0 | unit |
| FM-21 | Border/highlight API failure (`setprop`/`getprop` error) | Active, continues without highlight | cycle/apply still success | fallback null semantics + warn-once (REQ-UI-001/002) | no abort; no stuck border | T-UI-03, nest |
| FM-22 | Abrupt plugin kill/eject while Active | process-dependent | n/a | border overrides may persist until Hyprland restart | restore-by-value mitigates graceful unload only — documented gap (R0 F10/F12, COMPAT) | manual / nest |
| FM-23 | Graceful unload mid-highlight | destroyed | n/a | `on_session_end`(Cancelled) + teardown restore before UI destroy | full clear of overrides; no UAF | nest, REQ-UI-005 |

## Invariants under failure

1. Never crash the compositor from these paths.  
2. At most one focus per apply (REQ-F-006).  
3. No focus from cycle (REQ-F-003).  
4. No timer callbacks after unload (REQ-H-008).  
5. UI failures never abort apply/cancel (REQ-UI-001).  
6. No plugin-owned stuck borders after any end path (REQ-UI-005).  
