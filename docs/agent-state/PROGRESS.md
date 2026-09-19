# Progress
Updated: 2026-09-19 (M4 fully closed: optional restore_focus_on_cancel path — live nest N1–N4 PASS, T-S-09/T-S-10 regression, REQ-R-003 clarification; PR #35 open, CI pending)

## M0 Foundations
- [x] ARCHITECTURE.md
- [x] SPEC.md (v0.2 design gate)
- [x] DECISIONS.md (ADRs through ADR-015)
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
- [x] Compliance review + MR merge to main (MR !1; polish: REQ-F-008 retry, last_end_reason, pruned() unify)

## M2 MVP plugin
- [x] PLUGIN_INIT hash check + config registration
- [x] Adapters: HyprlandWindowSource, FocusGateway, SchedulerPort
- [x] Dispatchers mru:cycle / apply / cancel (addDispatcherV2, SDispatchResult)
- [x] Event::bus subscriptions (window.active/close, config.reloaded)
- [x] WindowRef registry with generation (ADR-013)
- [x] FocusGateway + Null UI (default ui=null; border/external fall back to null, warn-once)
- [x] restore_focus_on_cancel wired (policy -> SessionController)
- [x] STRING config via getDataStaticPtr (fix bad_any_cast on hyprlang 0.6.x, #3)
- [x] D1: CI plugin-guards job replaces stale fail-closed job
- [x] D2: clang-format pass on all source files
- [x] D3: plugin tests rewritten with shared framework (NDEBUG-proof)
- [x] D4: candidate order from plugin-owned MRU + register-on-sight (ADR-015)
- [x] D5: config reload refreshes SessionPolicy for next session (REQ-CFG-002)
- [x] D6: scope token without direction accepted (REQ-DISP-003)
- [x] D7: mru:status dispatcher with SPEC 3.4 payload
- [x] REQ-TRACE.md updated for M2 test IDs
- [x] CHANGELOG.md updated with all M2 closeout items
- [x] COMPAT.md: nest-tested row for 0.2.0 / v0.56.2
- [x] PROGRESS.md updated
- [x] SESSION.md updated
- [x] M2 audit report: docs/agent-state/reports/2026-09-15-m2-audit.md
- [x] Nested smoke on Hyprland v0.56.2 (efb5099) — all dispatchers + invariants verified
- [x] Live nest restored + re-verified 2026-09-17 (same pin; aquamarine 0.15.0, Wayland backend; B2 confirmed live) — research/2026-09-17-nest-aquamarine-diagnosis.md
- [x] M2 audit: BLOCKERs/HIGH/MEDIUM fixed (#5-#7)
- [x] M2 audit closeout: CI plugin-build (.so vs pinned headers, arch container) + sanitize (ASan/UBSan) + gcc jobs (#8)
- [x] L-12 guard regex (hyprutils + quoted), L-13 single-source version, L-15 mise pin (#8)
- [x] LOW triage L-1..L-17 (except L-12/13/15) + reentrant plugin_shutdown tests (#9)
- [x] Release v0.2.0 tagged (first release, M2 + audit closeout) — §16.2 via human gate

## M3 Scopes + config
- [x] All scopes (monitor, workspace, visible, app) — M3-S3 adapter filter + live nest §7
- [x] M3-S1: hyprlang V2 config API migration (issue #16)
- [x] M3-S2: pure domain scope predicate (issue #18) — WindowMeta, FocusContext, scope_matches, uniform special-ws rule, T-SC-01..04
- [x] M3-S3: scope adapter (WindowMeta mapping, remaining keys, T-SC-05 parse) — PR #21, issue #20; live nest §7 8/8 PASS
- [x] Full plugin:mru-switcher config surface — 8/8 documented SPEC §4 keys incl. `external_socket`
- [x] Config reload behavior documented + tested — USER.md §Config reload; unit T-CFG-02 + live nest `reports/2026-09-17-m3-reload-smoke.md` (REQ-CFG-002/003, REQ-S-009)
- [x] USER.md validated against real behavior (ROADMAP M3 deliverable) — `reports/2026-09-17-m3-closeout.md`

## M4 Border UI
- [x] M4-R0 research border API on pin 0.56.2 (memo, pin efb5099) — DONE
- [x] M4-D1 design gate — ADR-017 (Accepted) + SPEC §5 REQ-UI-001..011 + config keys border_style/border_color/border_size (PR #26 merged)
- [x] M4-S1: BorderHighlightUI + backend selection by config (`ui=border`) — PR #27 merged (92d84a7); SessionUIBackendProxy REQ-UI-009, runtime probe REQ-UI-002, restore-by-value R0 F10; suites t_ui_002_*/t_ui_009_*/t_ui_05d; ctest 12/12 + CI 6/6
- [x] Style interface: `BorderStyle` enum, `solid` implemented (strategy inside the border backend; `pulse`/`dim` reserved)
- [x] Clear paths: apply/cancel/unload -> full restore, no stuck borders (REQ-UI-005; unit t_ui_05*; unload-mid-session + abrupt-eject FM-22 -> M4-S3 nest)
- [x] Config keys border_style/border_color/border_size registered (REQ-UI-008; CI key-namespace guard)
- [x] M4-S3 live nest smoke + docs closeout — PR #30 merged (dfff936): smoke report committed with inlined evidence (`reports/2026-09-18-m4-s3-nest-smoke.md`, pin 0.56.2/efb5099); D1 keyword-channel limitation documented (COMPAT/USER/REQ-TRACE); D2 restore-grammar S1 fixed (normalize_capture + border_size restore-by-value) and CONFIRMED live 5/5; COMPAT row REQ-UI-011 recorded; R0 uncertainties resolved (call-string OK; getprop border_size supported). Nest: selection follow, zero focus side effects, single-focus apply, REQ-UI-009 frozen backend, unload-mid-session teardown restore, M2 regression — all CONFIRMED
- [x] restore_focus_on_cancel optional path — live nest N1–N4 PASS (origin refocus, dead-origin no-op, REQ-S-009 freeze; pin 0.56.2/efb5099), regression T-S-09/T-S-10 (mutation-checked), SPEC §6 numbered as REQ-R-003; report `reports/2026-09-19-m4-restore-on-cancel.md`, review `reports/2026-09-19-m4-restore-on-cancel-review.md`

## M5 External overlay
- [ ] Protocol draft frozen
- [ ] ExternalOverlayUI + fallback
- [ ] external_socket config key

## M6 Hardening -> v1.0
- [ ] CI unit tests
- [ ] hyprpm manifest / pins
- [ ] Stress cases
- [ ] Tag v1.0.0 / contract freeze
