# Architecture governance

## Source of truth order

1. SPEC.md  
2. DECISIONS.md (ADRs)  
3. FAILURE-MODES.md / TRANSITION-TABLE.md  
4. ARCHITECTURE.md  
5. design-notes/*  
6. COMPAT.md (pins)  
7. USER/API  

## ADR index

| ADR | Title | Status |
|-----|-------|--------|
| 001 | Snapshot not live list | Accepted |
| 002 | Apply-on-release | Accepted |
| 003 | HistoryTracker debounce + lock-in | Accepted |
| 004 | UI strategy | Accepted |
| 005 | Event::bus over hooks | Accepted |
| 006 | Single FocusGateway | Accepted |
| 007 | Domain free of Hyprland types | Accepted |
| 008 | Config only in PLUGIN_INIT | Accepted |
| 009 | mru:* dispatcher names | Accepted |
| 010 | start_offset second default | Accepted |
| 011 | UI default null until M4 | Accepted |
| 012 | SchedulerPort | Accepted |
| 013 | WindowRef generation | Accepted |
| 014 | Apply-after-invalidation clamp | Accepted |

## Definition of Done by milestone

### M1
- [ ] Domain + FakeClock + WindowRef in `src/`  
- [ ] Unit tests for T-IDs listed in REQ-TRACE for M1  
- [ ] CI job: configure, build tests, run tests, clang-format check  
- [ ] No Hyprland link required for unit tests  

### M2
- [ ] Loadable `.so` on **pinned** COMPAT row  
- [ ] FM-01–FM-03, FM-11–FM-13 smoke or unit  
- [ ] hyprpm.toml present  
- [ ] Nested smoke checklist signed off in PR  
- [ ] Logging at warn/info for session boundaries  

### M3–M6
- Per ROADMAP exit criteria + updated REQ-TRACE + CHANGELOG  

## Dependency rules (enforce in M1+)

```text
domain/        → (none of hyprland)
application/   → domain, ports
adapters/      → domain, ports, hyprland
plugin/        → application, adapters
```

Optional: clang-tidy or simple CI grep for `#include <hyprland` under `domain/`.

## Review checklist (plugin/API PR)

- [ ] SPEC IDs listed  
- [ ] ADR needed?  
- [ ] FAILURE-MODES row impacted?  
- [ ] REQ-TRACE updated  
- [ ] Hash check intact  
- [ ] No focus in cycle path  
- [ ] Timers cancelled on teardown  
- [ ] UI isolation  

## C4 (short)

- **Context:** User ↔ Hyprland ↔ MRU plugin (in-process); optional overlay process.  
- **Container:** single `.so` in compositor process.  
- **Components:** Facade, SessionController, HistoryTracker, ScopeResolver, FocusGateway, UIPort, SchedulerPort.  
- **Code:** see ARCHITECTURE.md modules.  
