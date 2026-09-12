# Design note — WindowRef identity

**Status:** Accepted (ADR-013)  
**Milestone:** M1 (domain type) / M2 (registry)

## Problem

Hyprland window addresses can be reused. A snapshot holding only an address may focus a different client after close/reopen.

## Model

```text
WindowRef {
  uint64_t address;     // compositor identity surface used by tools
  uint64_t generation;  // plugin registry epoch for this address
}
```

Equality: both fields equal.

## Registry (adapter)

```text
on track/map(window):
  gen = ++epoch_for_address[address]  // or global monotonic paired with address
  store map address -> { generation: gen, weak/handle }

on unmap/destroy(window):
  mark dead; do not reuse generation for a new client without bump

validate(ref):
  entry = map[ref.address]
  return entry.live && entry.generation == ref.generation
```

Generation must change whenever a new logical window occupies an address.

## Snapshot / focus

- Snapshot stores `WindowRef` only.  
- FocusGateway: `validate` then focus; else invalid.  
- Domain never stores `PHLWINDOW*`.

## Tests

- T-ID-01 stale generation invalid  
- Apply path with recycled address must not focus the new client under old ref  
