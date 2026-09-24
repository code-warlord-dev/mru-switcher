# Progress
Updated: 2026-09-22 (late) — ADR-028 pulse/dim merged PR #104 (742a9ad); next: optional nest smoke, then M6-T9 tag — human gate only
Updated: 2026-09-22 (skills integration PR #96 + ADR-025 ui=border default PR #97 merged; next: M6-T9 tag — human gate only)
Updated: 2026-09-22 (ADR-023 merged PR #87; live-host diagnosis: Lua plugin-config dead end — reports/2026-09-22-lua-host-plugin-config-dead-end.md; next: M6-T9 tag — human gate only)

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
- [x] ADR-026 / REQ-UI-012 selection follows workspace — PR #101 merged (41b834d): `selection_follow_workspace` key (default `true`), `WorkspaceNavigator` port + `HyprlandWorkspaceNavigator` adapter (`CMonitor::changeWorkspace`, `noFocus`), BorderHighlightUI begin/ensure/end wiring, config_v2 + sidecar support, tests `t_ui_012_*`×5 + `cfg_08_*` + `sidecar_08_*`, COMPAT mechanism row + docs. Verified: ctest 19/19, nest smoke (view follows selection, focus untouched, cancel restores session-start workspace), CI 8/8

## M5 External overlay
- [x] M5-R0 research overlay/socket API on pin 0.56.2 (memo `research/2026-09-19-m5-overlay-socket-api.md`)
- [x] Protocol draft frozen — SPEC §12 Appendix B normative; SPEC §5.3 REQ-O-001..008 + T-O-01..08 (ADR-018 design gate, PR #38 e5cd2ba)
- [x] M5-D1 design gate — ADR-018 (Accepted) + ADR-019 fd-watch refinement
- [x] M5-S1 domain `SessionController::select_index` (REQ-O-004, T-O-01/02/08) — commit e85b4d3
- [x] M5-S2 `overlay_protocol` + `ExternalOverlayUI` over `OverlayTransport` (REQ-O-002/003/005, T-O-03/04/05/06) — commit 1d882cb
- [x] M5-S3 `OverlaySocketServer` + `HyprlandOverlaySocket` + `ui=external` wiring + reference stub (REQ-O-001/004/006/007/008, T-O-07) — commits 7e3d77a..4deb303
- [x] external_socket config key now has effect; empty/unbindable → null + warn-once (REQ-O-001)
- [x] ExternalOverlayUI + fallback; ADR-019 removable fd watch; docs S5 (SPEC/ARCHITECTURE/COMPAT/API/USER/CHANGELOG/REQ-TRACE)
- [x] M5-S6 PR + CI + merge to main — PR #40 squash-merged (f3906b6); review blockers fixed (REQ-O-005 debug log, no-throw callbacks); CI 6/6

## M6 Hardening -> v1.0
- [x] M6 plan written — `docs/agent-state/plans/2026-09-19-m6-hardening.md` (PR #44); 9 tickets M6-T1..T9 + backlog M6-B1
- [x] M6-T1 contract freeze for 1.x — PR #56 (f6383da): SPEC §0 freeze statement, §3.4 normative payload, COMPAT/API/USER/VERSION-MAP/CHANGELOG aligned; compliance review APPROVE
- [x] M6-T4 SECURITY release pass — PR #57 (79e20c7); compliance review APPROVE (follow-up #59: THREAT-MODEL controls gap)
- [x] M6-T5 hyprpm manifest — PR #60 (1f8bdd6): commit_pins populated (provisional plugin hash), clean-checkout build verified; finalize hash at tag (M6-T9)
- [x] M6-T7 window-close stress (domain) — PR #62 (8cecb60): `domain_stress_window_close` 16→17 tests, ctest + ASan green; nest close-storm pending
- [x] M6-T8 focus-invalidation stress (domain) — PR #63 (56a0fcf): `domain_stress_focus_invalidation`, §2.8 bounds, 500-apply storm, 17/17 green
- [x] M6-T6 rapid Tab (domain) — PR #61 (d7363c4): `t_h_08_*` 3 tests, review metadata resolved, 17/17 green; nest 200-cycle smoke pending (blocked on nest-env infra)
- [x] M6-T3 CI depth — PR #69 (b0524e3): `release-guard` job (grep+python3, ~3s) asserting ≥17 add_test, T-H-06/07/08 in REQ-TRACE, [Unreleased] non-empty, commit_pins parse; manual pre-tag nest gate checklist `reports/2026-09-19-m6-manual-nest-gate.md`; SUPPORT-AND-RELEASE release checklist points to it
- [x] M6-T2 docs==implementation audit — PR #68 (4b34952): F-1..F-12 fixes, strict mru:status full-string asserts; behaviour question split to #67 (untouched, needs ADR)
- [x] ADS-020 design gate — ADR-020 Accepted + SPEC §14 REQ-DIST-001..024 + REQ-TRACE rows + T-DIST-01..04 (PR #70, bec0bac) — human-approved input merged
- [x] DIST examples — PR #71 (ba7de47): `examples/mru-switcher.conf` (all 11 keys, inline docs, `ui=border` marked demo override) + `examples/mru-switcher-bindings.conf` (SPEC §11 binds); README rewritten as end-user document per REQ-DIST-018; USER.md Quick start hyprpm-first; T-DIST-04/REQ-DIST-019 greps PASS
- [x] DIST installer — PR #72 (c70e6b9): `scripts/install.sh` (canonical `~/.local/src` layout, pin check, ELF verify, `--write-conf` opt-in, no curl|bash); shellcheck clean; build paths CWD-independent
- [x] Pre-release triage (post-review) — all five issues closed on merged main:
  - [x] #65/#67 history semantics — ADR-021 (Accepted 2026-09-20) implemented in PR #74 (squash 8f76ffa): mandatory unconditional lock-in (REQ-H-001/010), `lock_history_on_session` reserved/ignored + warn-once, `HistoryTracker::flush_pending()` before snapshot/lock-in (REQ-H-011); T-H-10 a/b/c + T-H-11; ctest 17/17 + ASan + clang-format; spec-compliance review APPROVE
  - [x] #58 clang 22 `-Wreturn-type-c-linkage` — PR #76 (aac7a26): clang-only per-target suppression on the plugin facade; failure reproduced on main, fixed build verified with clang 22.1.8
  - [x] #59 THREAT-MODEL controls gap — PR #77 (cd3b4ce): plugin enforces socket mode 0600 (`fchmod`, non-fatal), THREAT-MODEL/SECURITY aligned; T-FUZZ-01 deterministic parse-path fuzz harness as ctest #18 incl. the sanitize job; 18/18 gcc + ASan
  - [x] #55 M6-B1 same-path reload — PR #78 (88b8135): deterministic same-inode trigger (3/3 SEGV) vs new-inode clean (3/3); operator guidance in COMPAT/USER; no plugin-side mitigation (host limitation)
- [x] Packaging hardening pass 2 — PR #75 (89a2ee4): REQ-DIST-025/026/027 added and implemented (hyprpm first-setup for shipped examples, enterprise install.sh, `installer` CI job); sandbox matrix 8/8; honest hyprpm badge
- [x] Installer any-checkout + author attribution (pre-tag 1.0, human-confirmed 2026-09-24; NOT a release) — merged PR #111 (squash 2d9662e): `install.sh` builds **any** checkout (was: exit 3 unless `~/.local/src/mru-switcher`), `--allow-non-canonical` now a deprecated no-op, exit 3 redefined as "not a mru-switcher checkout (no CMakeLists.txt)" (no renumbering); SPEC REQ-DIST-011/026(a)(d)/027 amended + ADR-020 §4 dated amendment note + REQ-TRACE T-DIST-05; CI `installer` job asserts the any-checkout `--dry-run` (exit 0, reports the custom layout) and the exit-3 refusal; author `Yuriy Tretyakov (code-warlord-dev)` in hyprpm.toml (both blocks) / LICENSE / PLUGIN_INIT / README, no email in metadata; shellcheck 0.11.0 clean, ctest 19/19, plugin .so rebuilt, clang-format clean, CI 8/8
- [x] Autoload documented + live-host autostart (2026-09-24, docs only, no code/config/dispatcher change) — the reboot symptom (`no plugins loaded`) is a host contract, not a plugin bug: `hyprpm enable` writes root-owned state (`sudo install -m644 -o 0 -g 0`, needs a real terminal; hyprpm refuses to run as root) and loads nothing, while `hyprpm reload` is root-free and is the command each login must run; Hyprland 0.56.2 Lua has no `hl.exec_once` (use `hl.on("hyprland.start", …)`). Landed: README "Part 4 — load the plugin on every login (hyprpm)" + `no plugins loaded` troubleshooting bullet; USER.md Quick-start autoload section, "If the plugin is gone after a reboot" recovery, FAQ entry; COMPAT rows for both host facts; report `docs/agent-state/reports/2026-09-24-hyprpm-autoload-and-autostart.md`. Live host: autostart hook appended to `~/.config/hypr/autostart.lua` (backup `autostart.lua.bak.20260924-090758`, `hyprctl configerrors` empty, `hyprctl reload` clean), `/var/cache/hyprpm` chowned to the user via human-approved `pkexec` (dirs user-writable for future builds; state files stay root-installed by hyprpm). **REMAINING (human, one command):** `hyprpm enable mru-switcher` from a real terminal — then `hyprpm reload -n` loads it and the autostart line keeps it loaded
- [x] Autoload completed + live verification (2026-09-24, same day) — human ran `hyprpm enable mru-switcher` from a terminal → `hyprpm list` = `enabled: true`, state file installed `root:root 0644` by hyprpm itself (as designed). Cold-start verification ×2: `hyprctl plugin unload <cache .so>` → `no plugins loaded` → `hyprpm reload -n` → `✔ Loaded mru-switcher` → `hyprctl plugin list` shows `mru-switcher 0.5.0`. Autostart hook confirmed start-only (`hyprctl reload` does **not** fire `hyprland.start`, so no double-load) and the hook file is proven live (`zapzap`/`Telegram` from the same `autostart.lua` are running, `hyprland.lua:23 require("hypr.autostart")`); Lua bridge verified in-session (`hyprctl dispatch 'hl.plugin.mru.status()'` executes — no nil-field error). Docs correction landed with it: `hyprctl dispatch mru:*` is unreachable on Lua hosts (USER/API/COMPAT/CHANGELOG). Remaining user check: after the next login/reboot `hyprctl plugin list` should list `mru-switcher` without any manual command
- [ ] M6-T9 release close-out — v1.0.0 tag (human gate): **prep DONE (PR #80, squash 321a2c6)** — `commit_pins` finalized to `320c4cb`, SPEC §9 normative rows T-H-06/07/08/10/11 + T-H-09 note, CHANGELOG/VERSION-MAP/COMPAT aligned; REMAINING: manual nest gate (T-DIST-01/02/03 live legs + fast-toggle #65 leg), then tag `v1.0.0` + GitHub Release on explicit human command only
- [x] Bindings first-class (ADR-022; docs+packaging, no plugin change) — merged PR #85 (squash 5202a42): ADR in DECISIONS.md; `examples/mru-switcher-bindings.lua` (Tab-release Lua recipe); `scripts/setup-bindings.sh` (backend detect, --force+backup, live conflict check via `hyprctl binds -j`, exit 0-6, --verbose; CI `installer` job sandbox step 8/8 lives); README/USER/API/COMPAT/CHANGELOG updated; review fixes applied (REQ-DIST-019 scrub, ADR-022 --reload note); bisect memo `research/2026-09-20-lua-modifier-release-bisect.md`; host config applied manually by human (release-on-Tab, working) — SPEC §14.6 REQ-DIST-018 no normative change (ADR-022 documented only). **Superseded in product framing by ADR-023 (PR #87) — see next row**
- [x] Alt-release model correction (ADR-023; docs+examples, no plugin change) — merged PR #87 (squash f0a58a0): Tab-release rejected as product model (breaks ADR-002 browse-then-commit); **B1** explicit `ALT+Return` apply now the default Lua recipe (`examples/mru-switcher-bindings.lua`); **B2** modifier-hold poll reference shipped (`examples/mru-switcher-bindings-poll.lua`, new: `hl.is_key_down` + `hl.timer` 25 ms on the main loop, exactly-once, cancel-safe); Omarchy unbind mandated; hyprlang `bindrt` COMPAT claim downgraded to re-verify-before-1.0; ADR-023 in DECISIONS.md + ADR-022 annotated; SPEC §11 informative note (no REQ-* touched); README/USER/API/COMPAT/CHANGELOG/setup-bindings.sh aligned (review fix-ups: USER.md behaviour table + FAQ, usage caveat); evidence memo `research/2026-09-21-modifier-hold-poll-api.md`; CI 8/8; ctest 18/18; review APPROVE after F1/F2 fixes; REMAINING (human-gated): empirical bindrt nest check on efb5099, then M6-T9 tag
- [ ] Live-host (Lua backend) plugin-config dead end — diagnosed 2026-09-22, **product fix delivered 2026-09-22**:
  report `reports/2026-09-22-lua-host-plugin-config-dead-end.md`; every channel for `plugin:mru-switcher:*` is dead
  on the Lua host — **ADR-024 sidecar** merged as PR #94 (squash 8148ea2): opt-in `~/.config/mru-switcher/config`
  (`examples/mru-switcher-sidecar.conf`), same 11 SPEC §4 keys, REQ-CFG-005, T-CFG-07 (7 cases), COMPAT/USER honesty;
  **facade split** merged as PR #92 (squash e094c81): `mru_plugin.cpp` 516→74 lines. REMAINING (human): live-host
  sidecar verification (copy example → reload → border highlight) + optional upstream Lua `plugin {}` research
- [x] Skills integration + version + cleanup — merged PR #96 (squash c485915): hyprland-lua / hyprland-lua-config / omarchy-plugin-security vendored + integrated into AGENTS.md (§3.2 default-skill rule for Lua work, §3.3-§3.6, §10) + .agents/skills/README.md; skills-lock.json committed; 17 stale build* dirs purged (canonical build/ kept); CMake PROJECT_VERSION corrected 0.4.0 → 0.5.0 (v0.5.0 tag exists in history). No REQ-*, CI 8/8
- [x] ui = border default (ADR-025, CEO decision) — merged PR #97 (squash 1efbe77): compiled + registered default `ui = border` (config_value.hpp / config_v2.cpp); `ui = null` explicit opt-out; unknown-token fallback → null unchanged (REQ-CFG-001); ADR-025 supersedes ADR-011 default clause + ADR-017 conditional; SPEC §4/§5.2/§5 M4 note/§14.10, USER/API/COMPAT/README/examples/CHANGELOG aligned; tests: cfg_06_defaults, t_ui_03_effective_backend_selection, t_ui_03_null_backend_no_border_io (explicit null cfg), sidecar_04; ctest 19/19 + CI 8/8 + clang-format clean. M4 "out of scope" flip closed
- [x] ADR-028 border styles pulse / dim (REQ-UI-013/014/015) — **merged PR #104 (squash 742a9ad)**: `pulse` (colour throb via `HyprlandPulseTimer` / `wl_event_loop_add_timer`, `pulse_period_ms` clamp [200,10000], fail-soft warn-once on schedule failure/throw), `dim` (per-window `opacity`/`opacity_inactive` via capture slots, restore-by-value, unreadable skip, `dim_alpha>=1` disables; `dimAround` rejected — no public setter); AARRGGBB-correct `darker_color` (review HIGH fix); `BorderStyleParams` + `PulseTimerPort` pure port; V2 + sidecar-09/10 + cfg-09/10 + t_ui_013/t_ui_014 + t_ui_07 update; SPEC/REQ-TRACE/COMPAT/USER/API/examples/CHANGELOG aligned; ctest 19/19, plugin .so builds, clang-format clean, CI 8/8; review REQUEST-CHANGES → 6 findings fixed. REMAINING: nest smoke for pulse/dim visuals (pending, recorded in REQ-TRACE deviations)
