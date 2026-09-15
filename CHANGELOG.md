# Changelog

All notable changes to this project are documented in this file.  
Format based on [Keep a Changelog](https://keepachangelog.com/).  
Versioning: see `docs/VERSION-MAP.md` and `AGENTS.md` §15.

## [Unreleased]

### Added

- M2 MVP plugin (PR #2): loadable `mru-switcher.so` — fail-closed hash check in `PLUGIN_INIT`, dispatchers `mru:cycle`/`mru:apply`/`mru:cancel` (addDispatcherV2), Event::bus subscriptions (window.active/close, config.reloaded), Hyprland adapters (WindowSource, FocusGateway, SchedulerPort via `CEventLoopTimer`), WindowIdentityRegistry (address+generation, ADR-013), Null UI with warn-once fallback for `border`/`external`, config surface under `plugin:mru-switcher:` incl. `start_offset` (REQ-SEL-002) and live `debounce_ms` reload
- M1 domain core (pure C++, no Hyprland): `WindowRef` (address+generation, ADR-013), `Scope`/`Direction`/`StartOffset`/`SessionPolicy`, `Snapshot` (+prune helper), `SessionController` (Idle<->Active, REQ-S-001..011), `HistoryTracker` with debounce + lock-in, `SchedulerPort`/`FakeClock`
- Domain ports (interfaces for M2 adapters): `WindowSource`, `FocusGateway` (`FocusResult`), `UIPort`
- Unit tests T-S-01..08, T-F-01..05, T-RE-01, T-H-01..05, T-SEL-01..03, T-ID-01, T-H-seed, T-CFG-02 (ctest 9/9 green)
- Plan artifact: `docs/agent-state/plans/2026-09-13-m1-domain.md`
- `mru:status` dispatcher with SPEC section 3.4 payload and `last_end` diagnostics (D7)
- ADR-015: plugin-owned candidate order, register-on-sight (D4)
- `merge_mru_order` Hyprland-free helper for candidate ordering (ADR-007/ADR-015)
- M2 closeout plan: `docs/agent-state/plans/2026-09-15-m2-closeout.md`
- M2 audit report: `docs/agent-state/reports/2026-09-15-m2-audit.md`

### Changed

- Documentation set (ARCHITECTURE, SPEC, ADRs, ROADMAP, USER, API, diagrams)
- Hyprland plugin system reference
- Agent skills (project) and AGENTS.md orchestrator contract
- VERSION-MAP, agent-state progress/session templates
- Design gate: SchedulerPort, WindowRef generation, apply-after-invalidation, UI default null
- COMPAT.md, REQ-TRACE.md (expanded), design-notes/*
- FAILURE-MODES, TRANSITION-TABLE, OBSERVABILITY, THREAT-MODEL, GOVERNANCE, SUPPORT-AND-RELEASE
- CMake/CI scaffold, hyprpm.toml template, domain placeholder
- Align API.md / ARCHITECTURE.md UI default to `null` (ADR-011)
- Normative: omitted `mru:cycle` direction = `next` (REQ-DISP-001)
- Normative: empty apply after prune always errors `no windows` (REQ-DISP-002)
- App scope uses `class` only (not initialClass)
- CI: replaced stale `plugin-flag-fails-closed` job with `plugin-guards` (D1)
- CI: domain dependency guard + plugin target/config checks
- Candidate order: plugin-owned MRU first, adapter enumeration appended (ADR-015, D4)
- `SessionController::set_policy()` refreshes policy for next session only (REQ-CFG-002, D5)
- Dispatcher grammar: scope token accepted without explicit direction (REQ-DISP-003, D6)
- Plugin tests use shared `test_framework.hpp` (NDEBUG-proof, D3)
- All source files pass clang-format 22.1.8 (D2)

### Fixed

- CI: `plugin-flag-fails-closed` job was broken after M2 target existed (D1)
- Plugin tests used `assert()` which is a no-op in Release builds — rewritten with shared test framework (D3)
- Candidate order came from compositor history instead of plugin-owned MRU (D4)
- `WindowIdentityRegistry` filtered candidates via empty `is_known()` — windows opened before plugin load were invisible (D4)
- Config reload only refreshed `debounce_ms`; `default_scope`, `start_offset`, `wrap`, etc. never updated (D5)
- `mru:cycle workspace` failed with `unknown direction: workspace` — scope token without direction now accepted (D6)
- `mru:status` documented in SPEC/USER.md/API.md but not registered as dispatcher (D7)
- STRING config read via `dataPtr()` threw `std::bad_any_cast` on hyprlang 0.6.x — fixed via `getDataStaticPtr()` (#3)
- Duplicate `### Changed` heading in changelog
- FM-01 dual success/error contract

## [0.0.0] - 2026-09-13

### Added

- Initial M0 documentation-only baseline
