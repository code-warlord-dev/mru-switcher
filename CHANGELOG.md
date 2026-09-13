# Changelog

All notable changes to this project are documented in this file.  
Format based on [Keep a Changelog](https://keepachangelog.com/).  
Versioning: see `docs/VERSION-MAP.md` and `AGENTS.md` §15.

## [Unreleased]

### Added

- Documentation set (ARCHITECTURE, SPEC, ADRs, ROADMAP, USER, API, diagrams)
- Hyprland plugin system reference
- Agent skills (project) and AGENTS.md orchestrator contract
- VERSION-MAP, agent-state progress/session templates
- Design gate: SchedulerPort, WindowRef generation, apply-after-invalidation, UI default null
- COMPAT.md, REQ-TRACE.md (expanded), design-notes/*
- FAILURE-MODES, TRANSITION-TABLE, OBSERVABILITY, THREAT-MODEL, GOVERNANCE, SUPPORT-AND-RELEASE
- CMake/CI scaffold, hyprpm.toml template, domain placeholder

### Changed

- Align API.md / ARCHITECTURE.md UI default to `null` (ADR-011)
- Normative: omitted `mru:cycle` direction = `next` (REQ-DISP-001)
- Normative: empty apply after prune always errors `no windows` (REQ-DISP-002)
- App scope uses `class` only (not initialClass)
- CMake FATAL_ERROR if `MRU_BUILD_PLUGIN=ON` before M2 target exists
- CI: ubuntu-24.04, clang-18, clang-format-18, plugin-flag fail-closed job

### Fixed

- Duplicate `### Changed` heading in changelog
- FM-01 dual success/error contract

## [0.0.0] - 2026-09-13

### Added

- Initial M0 documentation-only baseline
