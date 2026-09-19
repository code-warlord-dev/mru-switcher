# M5 — External overlay (fully)

**Human decision (2026-09-19):** proposed by ROADMAP M5 "optional path" — approved **по**
Юрий Анатольевич: proceed fully. Badges README chore already landed on `main` (#37) before this plan.

**Bachelor flow:** R0 research (memo done) → D1 design gate (ADR-018 + SPEC freeze) → S1 domain →
S2 protocol + UI → S3 socket adapter + facade + reference stub → S4 verify (unit/sanitize/format/
.so/nest smoke) → S5 docs + state files → S6 review + ship. Milestone tags: `v0.5.0` per VERSION-MAP
after the milestone closes (human gate on tag, §16.2).

## Scope (M5 exit criteria, ROADMAP)

1. Protocol **frozen** in SPEC Appendix B (normative).
2. `ExternalOverlayUI` (UIPort) + best-effort socket transport; `ui=external` opt-in; default stays
   `null` (ADR-011/017).
3. Graceful fallback: backends impl absent / socket cannot bind / path empty → `null` behaviour,
   warn-once (REQ-UI-002); overlay peer absent/dies → session logic unaffected (best-effort sends).
4. Reference overlay stub `tools/overlay_stub.py` (Python 3, stdlib only) + live nest smoke.

## Design decisions (details recorded in ADR-018)

- Transport: AF_UNIX **SOCK_STREAM**; plugin binds at `plugin:mru-switcher:external_socket`;
  first accepted client is the overlay; further clients closed. All fds `O_NONBLOCK`.
- Reads: `CEventLoopManager::doOnReadable` on listener + client fds (pin 0.56.2, R0-F1). Writes:
  best-effort non-blocking `send()`, drop-on-backpressure (R0-F4). Main-thread only (REQ-RE-001).
- Wire split: core owns POSIX `OverlaySocketServer` (Hyprland-free, loopback-tested); the hypr
  adapter owns event-loop registration + command sink wiring.
- Peer commands: `select index` → `SessionController::select_index` (new, Active-only, bounds-check);
  `apply` / `cancel` → existing controller ops (idempotent). Unknown v/type/broken JSON → ignore+log.
- New normative reqs REQ-O-001..008; new domain test(s) and protocol/UI/socket tests.

## Implementation slices

| Slice | Files | Tests |
|-------|-------|-------|
| S1 domain | `include/mru/domain/session_controller.hpp`, `src/domain/session_controller.cpp` | `tests/domain/test_session_controller.cpp` (select_index) |
| S2 protocol + UI | `src/plugin/overlay_protocol.{hpp,cpp}`, `src/plugin/external_overlay_ui.{hpp,cpp}`, `src/plugin/overlay_window_info.hpp` | `tests/plugin/test_overlay_protocol.cpp`, `tests/plugin/test_external_overlay_ui.cpp` |
| S3 socket + facade | `src/plugin/overlay_socket_server.{hpp,cpp}` (core), `src/plugin/hypr/hyprland_overlay_socket.{hpp,cpp}`, facade wiring in `src/plugin/mru_plugin.cpp`, `config_value.*` UiBackend::External | `tests/plugin/test_overlay_socket_server.cpp`; nest smoke |
| S4 stub | `tools/overlay_stub.py` | live nest |
| S5 docs | SPEC App. B freeze, API.md, USER.md, COMPAT.md, REQ-TRACE.md, ARCHITECTURE §8/§11, CHANGELOG, PROGRESS, SESSION | — |

## Branches / PRs

- `adr/018-external-overlay` — ADR-018 + SPEC freeze (docs-only).
- `feat/m5-external-overlay` — S1..S5 code + tests + docs + stub; squash-merged after self-review.

## Verify order (cheap first)

1. `cmake -S . -B build-m5 -DMRU_BUILD_PLUGIN=OFF` + ctest (core + domain + plugin core tests).
2. clang-format 22 dry-run on new files.
3. Local `.so` build against installed pin headers (`MRU_BUILD_PLUGIN=ON`) — matches CI plugin-build.
4. Nested Hyprland smoke on pin `efb5099` with `tools/overlay_stub.py`.
5. CI 6/6 after PR.

## Conventions (enforced)

- ADR-007: domain + plugin core stay Hyprland-free (CI `plugin-guards`).
- New socket files under `src/plugin/` may NOT include `hypr*` headers — keep the event-loop
  registration in `src/plugin/hypr/`.
- `select_index` only moves the virtual selection (never focuses, REQ-F-003).
- External peer input is bounds-checked; no arbitrary window focus (THREAT-MODEL update in S5).