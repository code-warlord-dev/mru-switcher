# Design note — SchedulerPort and debounce

**Status:** Accepted (ADR-012)  
**Milestone:** M1 (FakeClock) / M2 (compositor timer)

## Problem

Debounce requires delayed execution, cancellation, main-thread safety, and clean unload. Specifying only “wait N ms” leaves implementers to invent timers unsafely.

## Interface

```text
using JobId = uint64_t; // 0 = none

struct SchedulerPort {
  // Schedule callback after delay_ms. Returns job id.
  // Callback MUST run on the logical main/compositor thread.
  JobId schedule_after(uint32_t delay_ms, std::function<void()> cb);

  // Cancel if pending; no-op if already fired or unknown.
  void cancel(JobId id);
};
```

## HistoryTracker rules

1. On focus while Idle: `cancel(pending_)`; `pending_ = schedule_after(debounce_ms, commit)`.
2. `commit` captures `WindowRef` by value; at fire, re-check validity; if invalid, no MRU update.
3. On lock-in Active / unload / destructor: `cancel(pending_)`; `pending_ = 0`.
4. At most one pending job.

## FakeClock (tests)

```text
FakeClock : SchedulerPort
  advance(ms)  // moves time; runs due callbacks sync in order
  now()
```

Unit tests:

- T-H-02 debounce quiet period  
- T-H-03 replace pending  
- T-H-04 cancel on destroy  
- T-H-05 invalid before fire  

## Production adapter (M2)

- Use the pinned Hyprland/compositor mechanism for delayed main-thread callbacks (exact API recorded in COMPAT.md for the pin).
- Never `std::thread::sleep` on the Wayland thread.
- Never run domain commits from a detached thread without marshalling.

## Unload

`PLUGIN_EXIT` and adapter teardown call `cancel` on all jobs owned by the plugin before destroying HistoryTracker.
