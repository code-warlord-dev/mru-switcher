# Changelog

All notable changes to this project are documented in this file.  
Format based on [Keep a Changelog](https://keepachangelog.com/).  
Versioning: see `docs/VERSION-MAP.md` and `AGENTS.md` §15.

## [Unreleased]

### Added

- **Keybinding delivery is a first-class install step (ADR-022), fixing the Lua/Omarchy
  apply-on-release failure:** the Lua bridge recipe that shipped in `task_0001` used
  `hl.bind("ALT_L"/"ALT_R", …, {release=true})` (plus `hl.dispatch`), and on the pinned
  Hyprland (v0.56.2 / efb5099) that combination never fires — release binds on a **modifier**
  key are dropped by the Lua keybind path (hyprlang `bindrt` on the same pin works). Root cause
  proved empirically with a bisect matrix (ordinary keys F9 / `ALT + TAB` release work;
  `ALT + ALT_R` / `ALT + ALT_L` release do not). New `examples/mru-switcher-bindings.lua` —
  the working Lua recipe via the real `hl.plugin.mru.*` bridge: unbind the Omarchy defaults,
  cycle/prev on `ALT + TAB` press, apply-on-release committed on **Tab** release (modmask 8
  rebuild), cancel on `ALT + Escape`. New opt-in `scripts/setup-bindings.sh`: autodetects the
  config backend (`hyprland.lua` present → Lua fragment, otherwise hyprlang), writes only its own
  `mru-switcher-*` files under the Hyprland config directory, refuses to overwrite without
  `--force` (backs up as `<file>.bak.<timestamp>`), live conflict-check via `hyprctl binds -j`
  (python3, `--no-python` falls back to a static hint), `--dry-run` writes nothing, exit codes
  0–6 documented in `--help`; installer job in CI gains a sandbox step for the script. README and
  USER.md installation sections now make the keybindings a mandatory, explicit step (with the
  honest warning that without them the plugin loads but Alt+Tab does nothing), with separate
  recipes for hyprlang and Lua/Omarchy; API.md Lua-bridge example corrected; Troubleshooting/FAQ
  gain entries for "Alt+Tab does nothing" and the modifier-release caveat; `examples/
  mru-switcher-bindings.conf` documents why the Lua recipe exists. Docs + packaging only — no
  domain/plugin behavior change.

- **Lua dispatcher bridge `hl.plugin.mru.*` (task_0001, Omarchy UX):** `cycle` / `apply` /
  `cancel` / `status` registered in `PLUGIN_INIT` via `HyprlandAPI::addLuaFunction`
  (namespace `mru`) as thin wrappers over the existing `dispatch_*` paths — dispatcher
  grammar and semantics unchanged (SPEC §3, REQ-DISP-001/002/003, REQ-F-003: cycle never
  focuses). Failed results raise a Lua error; `status` returns the frozen §3.4 payload
  string. Silent no-op on non-Lua (hyprlang) configs; removal automatic on unload.
  Documented in `docs/API.md` (Lua bridge section).

- **External docs review integrated (user/dev split + risk narratives):** README / USER.md / API.md
  scrubbed of internal requirement IDs (`REQ-*`), ADR references and milestone markers (M4/M5/M6) —
  user-facing docs now describe outcomes and syntax; all traceability remains authoritative in
  `docs/SPEC.md` / `docs/REQ-TRACE.md` (nothing removed from the developer surface). New
  `docs/RISK-PROFILES.md`: three narrative risk profiles — external overlay failure (FM-08),
  malicious/broken configuration, window-close storm — each tracing trigger → guards → degradation
  → user-visible result → pinned tests; `docs/FAILURE-MODES.md` FM-08 row links to the profile as
  narrative entry point (matrix stays the reference); USER.md gains a user-facing "If the plugin
  fails to load after a Hyprland update" recovery guide (real log line, `hyprpm update` / source
  rebuild / disable paths, linked from README Troubleshooting); release checklist gains an explicit
  artifacts item (`.so` + `sha256sums.txt` + built-against Hyprland commit note). Review source:
  `docs/agent-state/research/2026-09-20-external-docs-review.md`. Docs-only — no behavior change

- **Pending MRU promotion is flushed at session start (M6, ADR-021, REQ-H-011):** new domain operation `HistoryTracker::flush_pending()` — `SessionController::begin_session` cancels a still-pending debounce job and commits its window immediately (through the REQ-H-009 validity guard) **before** the candidate list is built and before lock-in engages, so back-to-back `cycle`/`apply` pairs rotate the history deterministically (A↔B) instead of depending on whether `debounce_ms` elapsed. Tests **T-H-10** (`t_h_10_start_flushes_pending_promotion_immediately`, `t_h_10_flush_does_not_commit_invalid_pending_window`, `t_h_10_flush_without_pending_is_a_noop`) and **T-H-11** (`t_h_11_chained_applies_rotate_without_clock_advance`, issue #65 regression; negative control verified: the test fails with the flush removed)

- **Source installer `scripts/install.sh` (M6 packaging, ADR-020 §4):** optional secondary helper for the source channel (ticket D2) — targets the canonical layout (SPEC REQ-DIST-007), detects Hyprland headers via `pkg-config --modversion hyprland` and verifies them against the pinned version (`docs/COMPAT.md` v0.56.2; override via `--hyprland-version` / `MRU_HYPRLAND_PIN`), and fails on toolchain gaps, missing headers, wrong arch, or pin skew; configures + builds with the exact `hyprpm.toml`/CI flags (`cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DMRU_BUILD_PLUGIN=ON` + `cmake --build build -j`); verifies `build/mru-switcher.so` is an ELF shared object; prints a success summary (source path, plugin path, detected Hyprland, recommended keybindings). `curl | bash` is not documented anywhere — the invocation is `git clone … ~/.local/src/mru-switcher && cd … && ./scripts/install.sh`; the script never edits `hyprland.conf` and only writes a fragment under `~/.config/hypr/conf.d/` when `--write-conf` is passed. No code/domain/plugin change; no README/USER/`examples/` edits (separate ticket D2a). Satisfies REQ-DIST-011..013 (T-DIST-01)

- **Examples + user-facing packaging (M6, ADR-020 / ticket D2):** `examples/mru-switcher.conf`
  (complete, loadable `plugin { mru-switcher { … } }` block — every one of the 11 registered config
  keys documented inline with purpose, default, allowed values, and a recommendation; `ui = border`
  as a clearly-marked first-run demo override of the code default `null`) and
  `examples/mru-switcher-bindings.conf` (only the 4 recommended binds: `mru:cycle next`/`prev`,
  `bindrt` apply-on-Alt-release, `mru:cancel`); README restructured as an end-user document per
  REQ-DIST-018 order with the bounded badge set (REQ-DIST-021), separate Hyprland+Omarchy / Niri
  positioning claims (REQ-DIST-022), hyprpm-first installation phrased per REQ-DIST-003, canonical
  `~/.local/src/mru-switcher` source layout (REQ-DIST-007/008/010), and examples-based first setup
  (REQ-DIST-017/023/024); USER.md Quick start switched to hyprpm-first with the canonical source
  path and `examples/` pointer, placeholder paths removed (REQ-DIST-014..024, T-DIST-03/04);
  README/USER also document the optional guided installer per REQ-DIST-012 (exact preferred
  invocation `clone → cd → ./scripts/install.sh`; no `curl | bash`) and the pre-tag nest gate gains
  a T-DIST-03 examples-load step.
  Docs/config-only — no dispatcher, config-key, or code change; `scripts/install.sh` itself is a
  separate ticket (D2b)

- **CI release guard (M6-T3, issue #49):** new `release-guard` job (ubuntu-24.04, grep+python3 only, ~30s) asserting the ctest matrix still holds ≥17 binaries, REQ-TRACE covers T-H-06/07/08, `CHANGELOG [Unreleased]` is non-empty, and `hyprpm.toml commit_pins` parses non-empty; nested smoke stays a manual pre-tag gate (`docs/agent-state/reports/2026-09-19-m6-manual-nest-gate.md`)

- **hyprpm distribution manifest (M6-T5, issue #47):** `hyprpm.toml` now carries `commit_pins` (`efb50993780079460b0cbed1363e2166a2de1d9f` = Hyprland v0.56.2 → plugin hash; the plugin-side value is provisional until the `v1.0.0` tag, M6-T9), repository metadata and a build stanza verified by a clean-checkout build producing `build/mru-switcher.so` (REQ-H-004); `docs/COMPAT.md` gains the v1.0.0 matrix row ("hyprpm install smoke pending release tag") and an explicit "pinning is a release contract" statement, `docs/VERSION-MAP.md` fills the 1.0.0 Hyprland pin, README install section gains hyprpm instructions, `docs/HYPRLAND-PLUGIN-SYSTEM.md` §10 references the populated pins

- **Contract freeze for 1.x (M6-T1, issue #50):** SPEC §0 freezes the dispatcher names + grammar, all 11 registered config keys under `plugin:mru-switcher:` (names / types / defaults / reload semantics, incl. the ticket's 10: `ui`, `external_socket`, `start_offset`, `wrap`, `debounce_ms`, `default_scope`, `restore_focus_on_cancel`, `border_style`, `border_color`, `border_size`), and the snapshot / apply / restore semantics as **stable for 1.x**; breaking any of them requires a major bump (semver, AGENTS §15 / VERSION-MAP)

- **ADR-020 + SPEC §14 distribution/installation (M6 packaging, ADR-020):** new ADR-020 (Accepted, human acceptance 2026-09-20) freezes the installation/distribution contracts — hyprpm as primary channel, canonical source layout `~/.local/src/mru-switcher`, optional `scripts/install.sh`, `examples/` as first-class user assets, README as user doc + bounded badges, no placeholder paths in user-facing commands (REQ-DIST-001..024; non-unit checks T-DIST-01..04); SPEC gains §14 "Distribution and installation" with REQ-DIST-001..024 verbatim and ADR-020 in the header; REQ-TRACE gains the REQ-DIST rows and the T-DIST index. Docs-only; no dispatcher/config/code change. `examples/`, `install.sh`, and the README/USER rewrite are a separate follow-up ticket (D2)

- **M6-T4 security follow-up (issue #59):** the external-overlay AF_UNIX socket file is now forced to mode **`0600`** via `fchmod` on the bound listener fd in `OverlaySocketServer::start` (non-fatal on syscall failure — the socket still binds and degrades per REQ-O-001; directory `0700` reworded as operator responsibility in THREAT-MODEL/SECURITY, no new config keys) and a new **parse-path fuzz harness** `tests/fuzz/fuzz_overlay_protocol.cpp` (`T-FUZZ-01`: bounded, deterministic generated + mutated inputs over `overlay_protocol::parse_command`, `LLVMFuzzerTestOneInput` entry + standalone `main()`, no libFuzzer dependency) registered as a ctest so it runs under every CI job incl. the ASan/UBSan `sanitize` job; SPEC §9 + REQ-TRACE updated. No behavior change to session/apply/cancel, dispatchers, or config (REQ-O-001/006 unaffected)

### Changed

- **`lock_history_on_session` is reserved and ignored (M6, ADR-021, REQ-H-010):** lock-in while a session is Active is mandatory — `SessionPolicy` loses the flag, `SessionController` locks unconditionally at session start and unlocks at session end. The key stays registered under `plugin:mru-switcher:` (all 11 keys remain frozen for 1.x) and is still read, but a `false` value only produces one warn-once notification per plugin lifetime pointing at the docs; it is no longer advertised in `examples/mru-switcher.conf`, README, or USER.md (API.md keeps a single "reserved / ignored" row). Removal is a 2.0 candidate
- SPEC §3.4 `mru:status` payload is now **normative** (was "informative, do not parse until 1.0"): `active= index= size= scope= session= last_end=` frozen for 1.x; new keys may be appended without a breaking change; `last_end` carries the internal `SessionEndReason` (REQ-F-009). Documentation only — dispatcher behaviour unchanged
- `docs/COMPAT.md`: the 0.56.2 IPC caveat documented as a host limitation in the matrix — `hyprctl dispatch` prints only `ok`, so the `mru:status` payload is observable via a libwayland dispatcher binding or the plugin log, not via `hyprctl` (host limitation, not a plugin defect)
- `docs/API.md` / `docs/USER.md`: `mru:status` wording aligned with the frozen payload (no semantic change)
- Security release pass (M6-T4, docs-only, no behavior change): `docs/SECURITY.md` rebuilt to release grade — 5-row attack-surface table (in-process `.so`, fail-closed hash check in `PLUGIN_INIT`, opt-in AF_UNIX overlay socket with peer-input validation/bounds and no shell-out, hyprlang config under `plugin:mru-switcher:`, Appendix B protocol framing) with threat vector + measure per row; explicit distribution-trust advisory (build from pinned source or checksum-verified artifacts; never load prebuilt `.so` from untrusted remotes); private security-reporting section; explicit non-goals (no sandboxing, no capability model); cross-links to `THREAT-MODEL.md` / `OBSERVABILITY.md` / `SUPPORT-AND-RELEASE.md` instead of duplication

### Fixed

- **Chained sessions re-landed on the same window (#65, ADR-021):** with the previous debounce behaviour a fast cluster of `mru:cycle → mru:apply` pairs could land on the same target every time, because the applied window's promotion was still pending when the next session started and was cancelled by lock-in. The pending promotion is now flushed at session start (REQ-H-011), so consecutive sessions rotate the MRU order deterministically, independent of `debounce_ms` timing. Regression: `t_h_11_chained_applies_rotate_without_clock_advance` (verified to fail with the flush removed). Live nest re-verification of the fast-toggle case on the pin stays part of the pre-tag smoke gate


- **Docs==implementation audit (M6-T2, issue #46):** `docs/OBSERVABILITY.md` `mru:status` section rewritten to the real frozen payload (`active= index= size= scope= session= last_end=`, no `verbose` arg, no `pending_debounce`/`pruned_total`/`session_id=` keys); `docs/ARCHITECTURE.md` §10 status row + §11 config example fixed (frozen payload, `external_socket` line, UI-backend freeze via `SessionUIBackendProxy` not `SessionPolicy`, compositor history = seed/fallback per ADR-015); `docs/API.md` `border_color` type corrected to `string` (verbatim `setprop` grammar, not `Color`); `docs/USER.md` quick-start gains `external_socket` + socket-teardown-on-reload note; stale `status_format.hpp` / `config_v2.hpp` comments flipped to freeze/M5 reality; follow-up #67 filed for `lock_history_on_session=false` no-op (behavior question, ADR needed — not changed here)

- **Stress coverage (M6-T6/T7/T8, issues #52/#53/#51):** domain binaries `t_h_08_*` (rapid Tab hammer: 1000-cycle wrap, interleave burst, wrap=false clamp), `T-H-06` (window-close stress: prune/clamp, NoWindows drain, dead origin, 300-step LCG), `T-H-07` (§2.8 bounds: InvalidTarget×2 → InvalidSelection, Failed-definitive, 500-apply storm, monitor drain) — ctest 17/17 + ASan/UBSan green; live nest smokes on pin v0.56.2 all pass (T-H-08: 200 cycles in 0.89s, exact-once-apply; T-H-06: close-storm matrix; T-H-07: 2-monitor disconnect; `docs/agent-state/reports/2026-09-19-m6-*.md`)

- **Local build with system clang 22.x (issue #58):** clang's `-Wreturn-type-c-linkage` (promoted to error by `-Werror`) rejected the `extern "C"` PluginAPI facade in `mru_plugin.cpp` — `PLUGIN_API_VERSION()` returns `std::string` and `PLUGIN_INIT` returns `PLUGIN_DESCRIPTION_INFO`, which is required by the Hyprland PluginAPI contract. gcc and CI were unaffected. The diagnostic now is silenced per-target (clang only) on the `mru-switcher` plugin: no behavior/ABI change, domain and tests untouched (CMakeLists.txt)

- **Same-path `.so` reload crash pinned down (M6-B1, issue #55):** nested repro (6 runs on pin v0.56.2) — overwriting the already-loaded plugin path **in place** (same inode) between `plugin unload` and `plugin load` crashes Hyprland **deterministically** (3/3 SEGV inside `dlsym`/`loadPluginInternal`; byte content irrelevant — a same-bytes overwrite crashed too), while replacement via a **new inode** (`rm`+`cp`, `mv`/rename; `cmake` relink also produces a fresh inode) reloads cleanly (3/3); unchanged-binary load/unload stays clean (M5 control, 13/13). Plugin-side code is not implicated — no speculative mitigation was attempted. Evidence: `docs/agent-state/reports/2026-09-20-m6-b1-reload-repro.md`; `docs/COMPAT.md` Known limitations sharpened with the trigger + operator guidance; `docs/USER.md` "Updating the plugin" section documents the safe update flow

- **Release-prep docs pass (M6-T9 prep, issue #54):** T-H-06/07/08/10/11 promoted into the normative SPEC §9 testing table (parity with `docs/REQ-TRACE.md`; T-H-09 intentionally unused — numbering stable, no renumbering); `hyprpm.toml` `commit_pins` plugin-side hash finalized to main `320c4cb` (no longer provisional; re-verify against the tagged commit); VERSION-MAP 1.0.0 row and COMPAT 1.0.0 row pin annotation updated. Docs/metadata only — no behavior change; the `v1.0.0` tag itself remains explicitly gated on a human release command

### Tests

- **Strict `mru:status` format test (M6-T2, issue #46):** `tests/plugin/test_status_format.cpp` now asserts exact full-string payload (`active= index= size= scope= session= last_end=` in order) for active + idle cases, plus additive-suffix tolerance per SPEC §3.4

## [0.5.0] - 2026-09-19

### Added

- M5 external overlay backend (`ui = external`, opt-in): the plugin binds an AF_UNIX stream socket at `plugin:mru-switcher:external_socket` and speaks the frozen line-framed JSON protocol (SPEC §12 Appendix B) so an out-of-process overlay can render the window list and drive the selection. New domain op `SessionController::select_index` (bounds-checked, Active-only, virtual selection — never focuses), Hyprland-free core (`overlay_protocol`, `external_overlay_ui`, `overlay_socket_server`), Hyprland fd-watch adapter, and a reference peer `tools/overlay_stub.py` (REQ-O-001..008, T-O-01..08)
- M5 design refinement **ADR-019** (Accepted): overlay listener/client fds use removable `wl_event_loop_add_fd` / `wl_event_source_remove` instead of `doOnReadable`, which returns no handle to cancel a disconnected peer's watch (REQ-O-006 amended in SPEC §5.3; ARCHITECTURE §8/§11 and COMPAT updated)
- Domain regression coverage for the optional restore path: **T-S-09** (`t_s_09_restore_flag_frozen_mid_session`) — a `hyprctl reload` of `restore_focus_on_cancel` mid-session does not change the frozen flag; the next session applies the new value (REQ-S-009, REQ-R-001) — and **T-S-10** (`t_s_10_empty_snapshot_never_restores`, `t_s_10_plugin_shutdown_never_restores`) — a session ending for a non-cancel reason (empty snapshot / `NoWindows`, plugin shutdown) never moves focus, even when the session origin stays valid (REQ-S-005/006, REQ-R-003)
- Restore-on-cancel leg confirmed live in a nested session (pin v0.56.2 / `efb5099`): `mru:cancel` refocuses the session origin and a dead origin is a no-op — live nest confirmation (see `docs/agent-state/reports/2026-09-19-m4-restore-on-cancel.md`)
- M5 nest smoke (pin v0.56.2 / `efb5099`, 13/13 PASS): socket bound before the first session, peer `session_start`/`selection`/`session_end`, peer `select`/`apply`/`cancel`, out-of-bounds select ignored, empty-path degrade, path restore, unload mid-session without crash/leak (report: `docs/agent-state/reports/2026-09-19-m5-s3-nest-smoke.md`)

### Changed

- SPEC §6 clarification (documentation only, **no behaviour change**), now numbered **REQ-R-003**: restore applies to an explicit `mru:cancel` only — a session ended for any other reason (REQ-S-006 empty snapshot, plugin shutdown, apply) never moves focus to `session_origin`

## [0.4.0] - 2026-09-18

### Added

- M4 border UI contract (docs/design gate, no runtime behavior yet): **ADR-017 (Accepted)** — `BorderHighlightUI` with solid border highlight via public window props ("public props first, fail-soft"; concrete symbols adapter-private, recorded in `docs/COMPAT.md`; R0 memo pinned to Hyprland 0.56.2 / `efb5099`), style interface (`border_style=solid`; `pulse`/`dim` reserved), and new keys `border_color` (default **`0xffffd9a0`**) / `border_size` (default **`-1`** = leave size untouched) / `border_style` (default **`solid`**) under `plugin:mru-switcher:`; default `ui` stays **`null`**. SPEC §5 gains **REQ-UI-001..011** with tests **T-UI-03..07** continuing the T-UI series.
- M4-S1 runtime border backend (behavior; `ui = border` opt-in, default stays `null`): `BorderHighlightUI` implements `UIPort` on the R0 mechanism — per-window `setprop active/inactive_border_color` via `invokeHyprctlCommand` (public props first, ADR-017 / REQ-UI-011; concrete symbols adapter-private in `docs/COMPAT.md`). `SessionUIBackendProxy` rebuilds the backend from the **current** config at each session start and freezes it for that session, so a `hyprctl reload` changes only the next session (REQ-UI-009). A session-start runtime probe degrades the backend to null for that session with a single warn-once (REQ-UI-002); restore is **by value** — a colour is overridden only after both prior values were read back and is restored from the captured values, never a bare `-1`/`unset` on a colour slot (R0 F10, REQ-UI-005). Dispatcher grammar and `mru:status` unchanged; nest smoke (focus-follow, latency, restore probes) is M4-S3.

- M4-S3 nest smoke — docs traceability (D1): `docs/COMPAT.md` records the `hyprctl keyword` channel limitation on Hyprland 0.56.2 (keyword changes to `plugin:mru-switcher:*` keys do not reach the plugin cache; config file + `hyprctl reload` is the supported runtime channel) plus the setprop/getprop grammar asymmetry and the border_size probe correction; `docs/USER.md` "Config reload" carries the user-facing warning; `docs/REQ-TRACE.md` REQ-UI-004/005/011 rows note the live smoke (report: `docs/agent-state/reports/2026-09-18-m4-s3-nest-smoke.md`)

### Fixed

- Border restore values are normalized to the `setprop` grammar: `getprop` returns unprefixed `<hex6> <N>deg` while `setprop` accepts only `0x…`/`rgb()`/`rgba()` without suffix — restoring captured output verbatim produced empty-gradient (invisible) borders (D2, found in the M4-S3 nest smoke; fixed by `normalize_capture` in `98c0dd5`)
- `border_size` is now restored by the captured value with an `unset` fallback (R0 correction: `getprop border_size` works on this pin, per COMPAT M4 nest smoke 2026-09-18)

### Changed

- Plugin version string now reports **0.4.0** via the CMake single source (`project(VERSION)`, audit L-13); previously the `.so` kept reporting the stale M2 string 0.2.0

## [0.3.0] - 2026-09-17

### Changed

- Config layer migrated from the legacy V1 config API to hyprlang V2 (`addConfigValueV2`, M3-S1): every `plugin:mru-switcher:` key is registered once in `PLUGIN_INIT` as a typed `Config::Values::*` value whose `SP` lives in `PluginState`; reads go through `mru::plugin::config::read()` (typed `value_traits` + `underlying()` contract) instead of `getConfigValue`/`dataPtr()` casts (RP-1, RP-3). Behavior is unchanged: `debounce_ms` clamp `[0,5000]`, enum fallback + once-warn, reload-next-session-only
- PLUGIN_INIT fail-closed: hash mismatch and config-registration failure now throw so the compositor's `loadPluginInternal` (pin 0.56.2) unwinds PLUGIN_INIT and unloads the plugin — an empty `PLUGIN_DESCRIPTION_INFO` alone does not unload on this pin (HIGH-4, issue #16)
- `default_scope` resolves as parsed: the MEDIUM-7 "force global until M3" shim is removed now that all five scopes are implemented by the M3-S3 adapter filter (REQ-SC-002)

### Added

- M3-S2 pure domain scope layer (issue #18): `WindowMeta` (opaque `monitor_id`/`workspace_id` `uint64`, `mapped`, `hidden`, `app_class`), snapshot-time `FocusContext` (passed by const ref, never recomputed), and stateless free function `scope_matches()` (REQ-SC-002/002a/002b, ADR-016). Uniform special-workspace rule covers all five scopes; `app` is byte-exact case-sensitive with empty-class folds (T-SC-01..04, domain tests)
- M3-S3 adapter wiring (issue #20): `HyprlandWindowSource` now translates pinned-0.56.2 `PHLWINDOW` → `WindowMeta` (`monitor_id`, `workspace_id`, `mapped`, `app_class = m_class`) and filters both candidate paths through `scope_matches()` (REQ-SC-002/002a/002b). `FocusContext` is built once per snapshot in `current_focus()`; the visible-workspace set comes from a single `State::monitorState()->monitors()` enumeration (active + active-special) and is the same vector `hidden` is derived from, so adapter/domain drift is impossible by construction. `global` remains M2-identical (drop-in)
- M3-S3 (S3-4) config surface complete: `plugin:mru-switcher:external_socket` registered (default `""`, reserved for M5, no effect until then), completing the documented 8-key SPEC §4 surface (ADR-016 __5__)

### Documentation

- USER.md validated against live behavior; config reload documented (`hyprctl reload` → next session only, active session untouched — REQ-CFG-002/003, REQ-S-009); `mru:status` IPC caveat on 0.56.x; scratchpad FAQ corrected (a hidden scratchpad is excluded from every scope)

## [0.2.0] - 2026-09-15

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

- CI builds the real `mru-switcher.so` against pinned Hyprland headers (Arch container, `plugin-build` job); adds ASan/UBSan and GCC test jobs; extends ADR-007/domain include guards to `hyprutils` and quoted includes (L-12)
- Plugin version now comes from a single source: CMake `project(VERSION …)` → `cmake/mru-version.hpp.in` → `PLUGIN_INIT` (was hard-coded `"0.2.0"` while CMake said `0.0.0`, L-13)
- `mise.toml` pins `cmake` to a concrete version instead of `latest` (L-15)
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
- **Audit BLOCKER-1 (UAF in teardown):** `PluginState` members were destroyed in declaration order with the scheduler first, so `HistoryTracker::cancel_pending()` dereferenced a destroyed scheduler. `teardown_state()` now tears down in explicit reverse order with `scheduler.reset()` last (#5)
- **Audit BLOCKER-2 (MRU head after apply):** `apply()` focused the selected window while history lock-in was still held, so the applied window never became the MRU head. `complete_apply()` now ends the session (unlocking history) before `tracker_.on_focus(applied)` (#6); reentrant `window.active` inside focus is swallowed by lock-in (T-RE-06, #10)
- **Audit HIGH-3 (registry lifetime):** `WindowIdentityRegistry` held strong `PHLWINDOW` refs and grew `by_address_` forever; switched to weak `PHLWINDOWREF` with `lock()`-based ABA protection and `prune_closed()` (#7); weak-lock identity formalized later in ADR-016
- **Audit HIGH-4 (exception barrier):** no `try/catch` at compositor entry points (`PLUGIN_INIT`, dispatchers, Event::bus listeners) — exceptions across the C ABI terminate the compositor. Added `guarded()`/`guarded_listener()` and enclosed `PLUGIN_INIT`/`PLUGIN_EXIT` (#7)
- **Audit HIGH-5 (dispatcher ordering):** dispatchers were registered before `build_state()`; a failed state build left `controller` null at first dispatch. Now `config → build_state → subscribe_events → register_dispatchers`, with dispatchers null-checking `controller` (#7)
- Duplicate `### Changed` heading in changelog
- FM-01 dual success/error contract
- `PLUGIN_EXIT` now ends an active session before teardown — UI receives `on_session_end(Cancelled)`, state is cleared, and history unlocks; `SessionEndReason::PluginShutdown` is finally used (L-11)
- `Snapshot::at()` no longer throws into the compositor — bounds are a documented caller contract enforced by assert (L-3)
- `cycle()` asserts the active-snapshot invariant instead of silently dereferencing a possibly-null snapshot (L-10)
- `focused()` uses a single registry lookup and no longer returns the identity of a closed window (L-7)
- `is_candidate()` now requires the mapped bit (`m_isMapped`) per REQ-SNAP-002 "mapped, not hidden, not fading" (L-8)
- `resolve()` returns `PHLWINDOW` directly instead of `optional<PHLWINDOW>`, and `PHLWINDOW` is passed by `const&` in all registry methods (L-4/L-5)
- Removed the unused `cfg_` dead dependency from `HyprlandWindowSource` (L-6)
- `FakeClock::advance()` runs due jobs in `(run_at, id)` order, matching its documented contract (L-1)
- The `-Wdeprecated-declarations` suppression in the facade is now scoped push/pop around only the legacy config-API call sites (L-9)

## [0.0.0] - 2026-09-13

### Added

- Initial M0 documentation-only baseline
