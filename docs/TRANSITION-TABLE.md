# SessionController transition table

**Status:** Normative companion to SPEC §2.1  
**Related:** ADR-001, ADR-002

States: `Idle`, `Active`.

| State | Event | Guards | Actions | Next |
|-------|-------|--------|---------|------|
| Idle | `cycle(dir, scope?)` | candidates ≥ 1 | build Snapshot; Selection via start_offset; session_origin; session_id++; UI start; lock history if configured | Active |
| Idle | `cycle` | candidates = 0 | none | Idle (fail) |
| Idle | `apply` | — | none | Idle (ok no-op) |
| Idle | `cancel` | — | none | Idle (ok no-op) |
| Idle | `focus_event` | lock rules | debounce schedule via SchedulerPort | Idle |
| Idle | `window_invalid` | pending debounce that ref | cancel job | Idle |
| Active | `cycle(dir)` | len ≥ 1 | move index (wrap policy); UI selection_changed | Active |
| Active | `cycle` | len = 0 | should not happen; end Cancelled | Idle |
| Active | `apply` | selection valid | FocusGateway once; UI Applied; unlock history; cancel debounce | Idle |
| Active | `apply` | selection invalid | §2.8 prune/clamp; focus or Cancelled | Idle |
| Active | `cancel` | restore config | optional focus origin; UI Cancelled; unlock | Idle |
| Active | `prune_event` | some invalid | prune; clamp index; if empty Cancelled→Idle | Active or Idle |
| Active | `focus_event` | lock_in | **ignore** for MRU | Active |
| Active | `unload` | — | cancel timers; drop session | (destroyed) |

## Preconditions / postconditions

**cycle (start):**  
Pre: Idle. Post: Active ⇒ snapshot.len ≥ 1 ∧ 0 ≤ index < len ∧ UI start called.

**apply:**  
Pre: any. Post: Idle ∧ (focus_count ≤ 1) ∧ (UI end exactly once if was Active).

**Invariant continuous:**  
¬(Active ∧ snapshot contains known-invalid without prune opportunity) — prune on close events and on apply.
