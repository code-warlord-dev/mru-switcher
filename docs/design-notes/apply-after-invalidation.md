# Design note — Apply after invalidation

**Status:** Accepted (ADR-014, SPEC §2.8)  
**Milestone:** M1 unit tests / M2 integration

## Algorithm

```text
apply():
  if Idle: return ok no-op
  prune invalid refs from snapshot (stable order)
  if empty:
    end Cancelled
    return error "no windows"  // preferred
  index = min(index, len-1)
  if still invalid: prune again; if empty → Cancelled
  FocusGateway.focus(snapshot[index])  // exactly once
  end Applied
```

## UI

Exactly one `on_session_end` per apply attempt: `Applied` or `Cancelled`.

## Non-goals

- Scanning forward through the list looking for “next valid” without pruning (would differ from clamp-after-prune).
- Focusing multiple windows in one apply.

## Tests

- T-F-03, T-F-04  
- Close selected mid-session then apply  
- Close all then apply  
