# Progress
Updated: 2026-09-13 (M1 implementation complete on feat/m1-domain; awaiting review + MR)

## M0 Foundations
- [x] ARCHITECTURE.md
- [x] SPEC.md (v0.2 design gate)
- [x] DECISIONS.md (ADRs through ADR-014)
- [x] ROADMAP.md (M2 Null UI + pin clarified)
- [x] USER.md / API.md / DIAGRAMS.md
- [x] HYPRLAND-PLUGIN-SYSTEM.md
- [x] SECURITY.md / CONTRIBUTING.md
- [x] Project skills under `.agents/skills/`
- [x] AGENTS.md (orchestrator + token economy + state sync)
- [x] VERSION-MAP.md
- [x] agent-state/SESSION.md + PROGRESS.md
- [x] Design gate: UI default null + fallback (ADR-011)
- [x] Design gate: SchedulerPort / debounce (ADR-012 + design-notes)
- [x] Design gate: WindowRef generation (ADR-013 + design-notes)
- [x] Design gate: apply-after-invalidation (ADR-014 + design-notes)
- [x] COMPAT.md matrix template
- [x] REQ-TRACE.md
- [x] FAILURE-MODES.md, TRANSITION-TABLE.md, OBSERVABILITY.md
- [x] THREAT-MODEL.md, GOVERNANCE.md, SUPPORT-AND-RELEASE.md
- [x] CMake scaffold + CI workflow + hyprpm.toml template
- [x] Domain dependency guard in CI
- [x] Consistency pass: UI defaults, cycle grammar, FM-01, CMake PLUGIN fail-closed, REQ-TRACE expand, CI pins

## M1 Domain
- [x] Domain types (WindowRef address+generation, Snapshot, Selection, Session, Scope, Policy)
- [x] SessionController + apply-after-invalidation
- [x] HistoryTracker + SchedulerPort + FakeClock
- [x] Unit tests T-S-* / T-H-* / T-SEL-* / T-F-03/04 / T-ID-01 (ctest 5/5 green)
- [ ] Compliance review + MR merge to main

## M2 MVP plugin
- [ ] PLUGIN_INIT hash check + config registration
- [ ] Dispatchers mru:cycle / apply / cancel
- [ ] Event::bus subscriptions
- [ ] FocusGateway + Null UI
- [ ] Nested smoke (basic binds)

## M3 Scopes + config
- [ ] All scopes
- [ ] Full plugin:mru-switcher config surface
- [ ] mru:status
- [ ] Config reload behavior documented

## M4 Border UI
- [ ] BorderHighlightUI
- [ ] Clear on apply/cancel
- [ ] restore_focus_on_cancel optional path

## M5 External overlay
- [ ] Protocol draft frozen
- [ ] ExternalOverlayUI + fallback

## M6 Hardening → v1.0
- [ ] CI unit tests
- [ ] hyprpm manifest / pins
- [ ] Stress cases
- [ ] Tag v1.0.0 / contract freeze
