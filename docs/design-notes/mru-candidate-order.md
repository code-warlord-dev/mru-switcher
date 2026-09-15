# Design note — MRU candidate order

**Status:** Accepted (ADR-015)  
**Milestone:** M2 (fix) / M3 (scope extension)

## Problem

M2 built the snapshot from `Desktop::History::windowTracker()->fullHistory()` (adapter) and filtered it with
`registry_.is_known()`. Two consequences observed on the pinned Hyprland (`efb5099…`, v0.56.2) in a nested session:

1. The plugin-owned `HistoryTracker` (debounce + lock-in, ADR-003) never influenced Alt+Tab order — the
   compositor list was the real source, so REQ-H-004a (“plugin owns the semantic MRU list”) was not met.
2. The registry is empty right after `PLUGIN_INIT`, so windows opened **before** load were filtered out:
   three `foot` windows open, `hyprctl plugin load …` → `ok`, `hyprctl dispatch 'mru:cycle next'` → `no windows`.
   Opening one more window after load made the same command return `ok`.

## Model

```text
candidates(scope):
  fallback = [valid windows in scope, newest-first]     # adapter enumeration
  primary  = [tracker.order() filtered by scope+valid]  # plugin-owned MRU  (REQ-H-004a)
  return merge_mru_order(primary, fallback)             # no duplicates, primary wins
```

- `merge_mru_order` is Hyprland-free (`src/plugin/mru_merge.cpp`) so the ordering rules are unit-testable
  without linking the compositor (ADR-007) — tests `T-SNAP-01` in `tests/plugin/test_mru_merge.cpp`.
- The adapter registers every window it enumerates (“register on sight”), which also seeds the tracker at init
  and makes windows that predate the plugin load participate in the first snapshot (REQ-H-004b).
- Identity stays `address + generation` (ADR-013, REQ-ID-003): a recycled address never merges two entries.

## Fallback enumeration order (M2, global scope)

1. `fullHistory()` reversed (newest focus first), weak refs locked and filtered by `!isHidden()`.
2. Registry entries that are live and not already listed, newest registration first
   (`WindowIdentityRegistry::live_refs_newest_first()`), i.e. windows opened since load that were never focused.

Order is deterministic; never-focused windows rank after focused ones, newest open first.

## Extension point (M3)

Scope predicates (monitor / workspace / visible / app) become filters inside the fallback enumeration —
`monitorID()`, `workspaceID()` / `m_workspace`, `visibleOnMonitor()`, `m_sClass` — while the merge and the
primary-prefix rule stay unchanged. The tracker order is scope-agnostic on purpose: it is the MRU axis, not a
scope axis.

## Risks

- A window hidden by a special workspace is excluded in M2 `global` scope; if a user reports that, the
  predicate set — not the ordering model — is what changes (new ADR).
- Registry entries are keyed by address and never evicted; growth is bounded by distinct addresses seen in a
  session (tracked as an S3 backlog item).