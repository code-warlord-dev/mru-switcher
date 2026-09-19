# M5-R0: External overlay socket API on pin 0.56.2 (`efb5099`)

**Owner:** orchestrator (direct header inspection, no nest required)
**Date:** 2026-09-19
**Relevant SPEC:** §2.11 REQ-PERF-001/003, §5.2 `external` backend, Appendix B (protocol draft)
**Relevant ADR:** ADR-004 (UI strategy), ADR-011 (default null + fallback), ADR-017 (M4 gate, `external` falls back to null until M5)

---

## Problem

M5 needs an `ExternalOverlayUI` that sends session events to a peer over a Unix socket and accepts
`select` / `apply` / `cancel` commands back from that peer — without ever blocking the compositor
thread. REQ-PERF-001 forbids blocking I/O and socket IPC on the `mru:*` dispatcher hot path;
REQ-PERF-003 requires best-effort non-blocking / timed external I/O. The pinned Hyprland revision is
`efb50993780079460b0cbed1363e2166a2de1d9f` (= v0.56.2), with headers installed at
`/usr/include/hyprland` (distro `hyprland` package, verified `pkg-config --modversion hyprland` = 0.56.2).

## Findings (all verified by reading installed headers of the pin)

### F1 — Event-driven fd watching exists on the main loop

`src/managers/eventLoop/EventLoopManager.hpp` (pin):

```cpp
// schedule function to when fd is readable (WL_EVENT_READABLE / POLLIN),
// takes ownership of fd
void doOnReadable(Hyprutils::OS::CFileDescriptor fd, std::function<void()>&& fn);
```

- Backed by `wl_event_loop_add_fd` (via `SReadableWaiter`, `onFdReadable` / `onFdReadableFail`).
- Callback runs on the compositor main thread; fd returned to non-blocking poll.
- `CFileDescriptor` is move-only, closes on destruction; waiter is auto-removed when the source
  fires/dies. No leaking fd on teardown if the waiter is dropped.
- Global instance: `g_pEventLoopManager` (`inline UP<CEventLoopManager>`).

**Verdict:** this is the "compositor main-thread, event-driven, non-blocking" mechanism the M5 socket
needs. No timer-based polling of the socket is required (`CEventLoopTimer` exists as an alternative
but is strictly worse: fixed-interval wakeups). ARCHITECTURE §14 already commits "UI stays
main-thread / event-loop safe, same as SchedulerPort".

### F2 — What the adapter must supply vs what the core may own

- `doOnReadable` / `g_pEventLoopManager` live in **Hyprland** headers → only `src/plugin/hypr/`
  (`mru_plugin_hypr`) may include them (CI `plugin-guards` job enforces: non-facade `src/plugin/*`
  files must not include `hypr*`).
- The AF_UNIX **socket server itself** (socket/bind/listen/accept/recv/send, `fcntl(O_NONBLOCK)`,
  line framing) is pure POSIX and Hyprland-free → belongs in `mru_plugin_core`, unit-testable with a
  real loopback socket in the CI `unit` job (no Hyprland required).
- The thin Hyprland layer is: construct the core server + register `doOnReadable` on the listen fd and
  the accepted client fd, forwarding readiness to the core's non-blocking `poll_accept()` /
  `poll_recv()`.

### F3 — Peer command round-trip is main-thread safe

Peer `select`/`apply`/`cancel` arrive in the `doOnReadable` callback (main thread). Forwarding them to
`SessionController` there satisfies REQ-RE-001 (all controller mutations on the compositor main
thread). `apply()`/`cancel()` already exist and are idempotent on Idle; `select` needs a small new
domain operation (bounds-checked index set) — see plan.

### F4 — Best-effort outgoing sends

`send()` on an `O_NONBLOCK` SOCK_STREAM fd returns immediately: success writes what fits, or
`EWOULDBLOCK`/`EAGAIN`/`EINTR`/`EPIPE`. Best-effort policy = on any of these, drop the line, log at
debug; never retry-spin, never block. A peer that reads slowly or dies mid-session therefore cannot
stall `mru:*` (REQ-PERF-001). Line framing: newline-delimited UTF-8, one JSON object per line
(Appendix B).

## SPEC impact

- No existing REQ changes. Appendix B becomes **normative** in the M5 design gate (ADR-018):
  - Transport: AF_UNIX **SOCK_STREAM**, single client (first accepted; others closed).
  - `external_socket` (already registered, reserved — ADR-016 __5__) becomes the bound path; empty
    path ⇒ `ui=external` behaves as `null` + warn-once (REQ-UI-002 continuation).
  - Peer absent / dies ⇒ session logic unaffected; UI messages best-effort dropped.
  - Peer NEVER controls anything beyond `select` (bounds-checked, in-snapshot) + `apply`/`cancel` of
    the current session — no arbitrary window access.
- New requirement IDs: REQ-O-001..008 (see ADR-018).
- Domain: add `SessionController::select_index(size_t)` (Active-only, bounds-checked).

## Links

- `docs/SPEC.md` §2.11 (REQ-PERF-001/003), §5.2 (backends), §12 Appendix B (draft protocol)
- `docs/DECISIONS.md` ADR-004, ADR-011, ADR-016 __5__, ADR-017
- `docs/ROADMAP.md` M5
- Pin headers: `/usr/include/hyprland/src/managers/eventLoop/{EventLoopManager,EventLoopTimer}.hpp`