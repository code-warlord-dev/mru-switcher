# Review — M4 closeout: restore-on-cancel (`feat/m4-restore-on-cancel`, issue #34)

Date: 2026-09-19
Branch: `feat/m4-restore-on-cancel` (HEAD `5a8f941`) vs `main` (`8230cc8`; merge-base = `8230cc8`)
Reviewed by: reviewer subagent (skills: `plugin-spec-compliance`, `code-review`, `mru-switcher`, `cpp-plugin-architecture`)
Scope: `git diff main..HEAD` — 4 commits, 8 files: `CHANGELOG.md`, `README.md`, `docs/SPEC.md`, `docs/REQ-TRACE.md`, `docs/USER.md`, `docs/COMPAT.md`, `docs/agent-state/reports/2026-09-19-m4-restore-on-cancel.md` (new), `tests/domain/test_session_controller.cpp`. **No `src/` or `include/` change.** Read-only on tracked files; mutation testing done in a throwaway worktree (`/tmp/mru-mutation`); no nest run (out of scope for this review).

## 0. Verification performed (this review)

- **Mutation check (PART A, mandatory)** in `git worktree add /tmp/mru-mutation feat/m4-restore-on-cancel` — 6 mutations, each rebuilt with `cmake -S . -B build -G Ninja -DMRU_BUILD_TESTS=ON -DMRU_BUILD_PLUGIN=OFF -DMRU_WARNINGS_AS_ERRORS=OFF` and run via the `domain_test_session_controller` binary (`ctest` counts binaries, not cases). Table in §1.
- **Real repo (read-only)**: `cmake --build build -j` → OK; `ctest --test-dir build` → **12/12 PASS**.
- **CI-equivalent builds** (scratch copy): clang 22.1.8 Debug, `MRU_WARNINGS_AS_ERRORS=ON`, PLUGIN=OFF → build OK, **12/12 PASS**; ASan+UBSan (`-fsanitize=address,undefined`) → build OK, **12/12 PASS**.
- **clang-format**: 22.1.8, `--dry-run -Werror` on `tests/domain/test_session_controller.cpp` → **clean** (exit 0).
- **CI static guards run locally** on the real repo: `domain-deps` (no `hyprland`/`hyprutils` include under `src/domain` + `include/mru/domain`) → **OK**; `plugin-guards` (V2 registration confined to `config_v2.{cpp,hpp}`/`mru_plugin.cpp`, all keys under `plugin:mru-switcher:`, plugin core Hyprland-free, no V1 config API / `->ptr()`) → **OK**.
- **Evidence chain**: `git diff --name-only bb03942..HEAD` = `docs/COMPAT.md`, `docs/agent-state/reports/2026-09-19-m4-restore-on-cancel.md` → **no src/include/tests path**, so the nest evidence recorded at `bb03942` describes the reviewed source behaviour (and `src/` is byte-identical to `main` anyway).
- **Raw nest artifacts are still on disk** (`/tmp/mru-nest-restore/report/`, 19 files + `attempt1-superseded/`): load file sha256 `daa5bd97…8497`, 448 328 B, mtime `2026-09-19 11:24:20` **matches** `build-plugin-restore/mru-switcher.so` on disk and the committed report/COMPAT; crash scan of `97-nest-log-full.txt` → 0 hits for `crash|segfault|SIGSEGV|assert|terminate|hash mismatch`; the N3/N4 raw transcripts match the report’s inlined annex, including the pre-cancel reference shots and the mid-session config diff.
- **Nest: not run** (forbidden for this review). The live verdicts below are an audit of the recorded evidence, not a re-execution (U-1).

## 1. Mutation check — is the new test code discriminating?

Every mutation applied to `src/domain/session_controller.cpp` **in the scratch worktree only**, then rebuilt and run.

| # | Mutation (smallest src/domain edit) | Expected RED | Observed | Verdict |
|---|---|---|---|---|
| M1 | `cancel()`: `const bool restore = snapshot_policy_.restore_focus_on_cancel` → `policy_.restore_focus_on_cancel` (live policy instead of the frozen snapshot) | `t_s_09` | `[FAIL] test_session_controller.cpp:636 CHECK(f.fg.focused == std::vector<WindowRef>{ref(10)})`, `:644 EQ(size,1u): 0 != 1` → `t_s_09_restore_flag_frozen_mid_session` RED; T-S-10 stayed GREEN (path untouched) | ✅ discriminating |
| M2 | `end_session()`: restore-origin on any non-apply end — `if (reason != Applied && snapshot_policy_.restore_focus_on_cancel && origin && source_.is_valid(*origin)) fg_.focus(*origin);` | both `t_s_10` | `:665 CHECK(f.fg.focused.empty())` → `t_s_10_empty_snapshot_never_restores` RED; `:684` → `t_s_10_plugin_shutdown_never_restores` RED (collateral: `t_s_05` :143, `t_s_09` :636/:644) | ✅ discriminating |
| M3 | `cancel()`: drop `&& source_.is_valid(*origin)` | `t_s_06` | `:160 CHECK(f.fg.focused.empty())` → `t_s_06_restore_invalid_origin_noop` RED | ✅ discriminating |
| M4 | control — no mutation, same binary/build dir as M1–M3 | all green | `domain_test_session_controller`: 32/32 PASS, 0 FAIL; `ctest` **12/12 PASS** | ✅ control green |
| M5 | *(bonus)* `set_policy()` becomes a no-op (`(void)policy;`) — reload never reaches the policy | `t_s_09` leg 2 | `:644 EQ(size,1u): 2 != 1` → RED (also `t_cfg_02` RED) | ✅ leg 2 discriminates |
| M6 | *(bonus)* `cancel()`: `restore = true` unconditionally (always restore) | `t_s_09` leg 2 | leg 1 PASSES, leg 2 FAILS at `:644` (`2 != 1`) → overall RED | ✅ both legs discriminate independently |

**Conclusion:** no test passes for the wrong reason, and no expected-RED mutation stayed green. Specifically: T-S-09 catches a live-policy leak (M1), a lost reload (M5) and a hard-coded restore (M6); T-S-10 is red under a buggy non-cancel restore **while the origin stays valid and outside the snapshot** (M2) — exactly the §3.1a orchestrator correction; the earlier pseudo-code design (origin = the only candidate, then closed) would have been masked by the validity guard and stayed green. T-S-06 remains the guard’s discriminator (M3). **No HIGH finding on test quality.**

## 2. Specification / traceability mapping (PART B)

| SPEC ID | Behaviour touched | Test row (SPEC §9) | REQ-TRACE row | Milestone/evidence column | Verdict |
|---|---|---|---|---|---|
| REQ-R-001 | restore valid `session_origin` on cancel | T-S-05 (exists) | updated: `T-S-05, T-S-09, T-S-10` | M2; live N1/N2/N4 (COMPAT + report) | ✅ accurate |
| REQ-R-002 | invalid origin → no focus | T-S-06 (exists) | updated: `T-S-06, T-S-10` | M2; live N3 (see L-4) | ⚠️ T-S-10 edge over-claimed — **L-2** |
| REQ-S-005 | cancel ≠ other end paths | T-S-03, T-S-05 | updated: `T-S-03, T-S-05, T-S-06, T-S-10` | M2; live N1–N4 | ✅ |
| REQ-S-006 | empty snapshot → end Cancelled | **new row T-S-10** cites REQ-S-006 | **row unchanged: `T-F-04` only** | M1; T-S-10 unit-only (documented) | ⚠️ missing T-S-10 — **L-1** |
| REQ-S-007 | `session_origin` captured at start | T-S-05 | `T-S-05` (unchanged) | M1; N1/N3 | ✅ |
| REQ-S-009 | policy frozen at session start, reload → next session | **new row T-S-09** cites REQ-S-009 | updated: `T-CFG-02, T-S-09` | M2; live N4 frozen+new-value legs | ✅ |
| REQ-F-003 | cycle never focuses | T-F-01 | `T-F-01` (unchanged) | M2; live N1/N2 (`activewindow` never leaves the origin while the highlight follows) | ✅ live evidence lives in COMPAT, not REQ-TRACE (acceptable; N1 is not a test ID) |
| REQ-F-006 / REQ-F-007 | one focus per apply / one UI end | T-F-02/03/04/05 | unchanged | M2; unit | ✅ no behaviour touched; the new tests also assert a single UI end |
| REQ-H-001 | lock-in | T-H-01, `bonus_focus_during_active` | unchanged | M2; live N1/N3 | ✅ |

- **SPEC §6 addition is a clarification, not a behaviour change — CONFIRMED.** `src/` is untouched (`git diff main..HEAD --stat` has no src/include) and the code already behaves as the sentence states: the only `fg_.focus` outside `apply` is in `cancel()` (`session_controller.cpp:111-112`); `end_session()` — used by every other end reason — never focuses. The sentence matches `TRANSITION-TABLE.md` (`Active | cancel | restore config | optional focus origin` = yes; `prune_event` row = no restore; `unload` row = no restore; `apply` row = focus the selection only).
- **“No ADR required” is defensible — CONFIRMED.** No normative behaviour changed and no design shift was introduced (docs-only sentence + tests). Two normative sources already stated the rule (SPEC §6 REQ-R-001/002 scope + TRANSITION-TABLE rows). ADR-013/014/016/017 (weak-ref validity, clamp, registry, border UI) are untouched. The only candidate for a decision record would be “restore is cancel-only across all end paths”, which is not a design shift — so no ADR is required; numbering the sentence (L-3) is the cheaper fix.
- **REQ-TRACE test-ID index rows** `T-S-09`/`T-S-10` carry milestone `M2`, consistent with the file’s convention (milestone of the owning requirement; cf. `T-S-05..T-S-08` = M2, `T-UI-03..07` = M4 for M4 requirements). Checked; no issue.
- **Docs consistency / links:** `docs/USER.md:238` relative link resolves; `CHANGELOG.md:12` and `docs/COMPAT.md:165` paths resolve; `README.md` “Optional next” no longer lists the restore path. No dangling links.

## 3. Standards axis

- Hard rules checked (repo standards): clang-format 22.1.8 clean on the touched test file; domain Hyprland-free (guard grep OK; `tests/domain` includes only `mru/domain/*` + local fakes); hash check intact (`mru_plugin.cpp:302` unchanged, no src diff); config keys namespaced/registered only in init (guards OK, no src diff); `mru:cycle` never focuses (`t_f_01` green in all 6 mutation runs; live N1); REQ/ADR-id comment convention respected in the new tests; test naming matches neighbours (`t_s_09_*`, `t_s_10_*_*` mirroring `t_cfg_02_*` / `t_f_03_*` / `t_f_05_*`).
- Smell baseline: no duplicated logic beyond the deliberate Fixture re-use; no Speculative Generality (two tests, no new seams); no Primitive Obsession introduced; no Shotgun Surgery (one test file). Judgement call only: `t_s_09` serialises two sessions in one case (mirrors `t_cfg_02`) — slightly longer than a single-purpose test, but that is what makes leg 2 (M5/M6) observable without a second fixture; acceptable.
- New tests are not over-specified: they assert observable domain outcomes (`fg.focused`, `last_end_reason`, `ui.ends`, `is_active`) and no unrelated state; each `TEST` builds its own `Fixture`, so `MockFocusGateway.focused` cannot accumulate across cases (accumulation **within** `t_s_09` is intentional and asserted: `size()==1` twice). No reliance on `set_policy` ordering beyond the intended mid-session refresh.
- **No hard Standards violations.**

## 4. AGENTS.md §6.2 Definition-of-Done checklist

| # | Check | Verdict | Evidence |
|---|---|---|---|
| 1 | SPEC-compliant behaviour (or SPEC updated in the same PR) | ✅ | §6 clarification + §9 rows land in the same branch; no src change |
| 2 | No `mru:cycle` focus side effects | ✅ | `T-F-01` green in all 6 mutation runs; live N1/N2 `activewindow` unchanged during cycles |
| 3 | Domain free of Hyprland types | ✅ | `domain-deps` guard OK; no src/include diff |
| 4 | Hash check intact in `PLUGIN_INIT` | ✅ | `src/plugin/mru_plugin.cpp` unchanged; `hash_ok()` + throw at `:302-303` |
| 5 | Config keys only `plugin:mru-switcher:` and only registered in init | ✅ | no src diff; both CI guard greps OK |
| 6 | Tests for touched T-IDs | ✅ | T-S-09 (2 legs) + T-S-10 (2 cases) added; green under gcc, clang `-Werror`, ASan+UBSan |
| 7 | Skills/docs updated if workflow changed | ✅ | CHANGELOG, README, SPEC §6/§9, REQ-TRACE, COMPAT, USER, nest report updated; no workflow change |
| 8 | CI green (when CI exists) | ✅ local equivalent | `ci.yml` `unit` (clang 22 Debug `-Werror`, PLUGIN=OFF) 12/12; `sanitize` 12/12; `domain-deps`/`plugin-guards` greps OK; `clang-format` clean. GitHub-side run not observed (U-3) |
| 9 | Issue #34 DoD: “SESSION.md + PROGRESS.md M4 checkbox closed” | ❌ | `PROGRESS.md:82` still `- [ ] restore_focus_on_cancel optional path`; `SESSION.md` modified **uncommitted** and stale (“Last artifact: …plans/… (planner in flight)”) → **M-1** |
| 10 | Issue #34 DoD: nest report committed, out-of-live-matrix item documented | ✅ | report committed; T-S-10 boundary documented in its “Gaps / notes” + the COMPAT boundary note |

## 5. Findings

### MEDIUM

- **M-1 (DoD / state hygiene; issue #34 DoD).** `docs/agent-state/PROGRESS.md:82` still shows the M4 restore item unchecked, and `docs/agent-state/SESSION.md` is only modified in the working tree (not committed) with stale content — it still says the planner artifact is “in flight” although the branch already carries the tests, the docs and the nest report. AGENTS §14.2 lets the checkbox flip happen at merge, but issue #34 lists it in this branch’s DoD, and a dirty state file is exactly the recovery key AGENTS §19.3 depends on. Fix before merge: commit an updated `SESSION.md` (branch / next action = reviewer → PR) and flip `PROGRESS.md:82` when the PR merges. Anchors: issue #34 DoD; AGENTS §6.2/§14.2/§19.3.

### LOW

- **L-1 (traceability; REQ-S-006).** `docs/REQ-TRACE.md:15` still reads `| REQ-S-006 | T-F-04 | …` although the new T-S-10 row in SPEC §9 (`docs/SPEC.md:469`) explicitly cites REQ-S-006, the new SPEC §6 sentence cites it, and `t_s_10_empty_snapshot_never_restores` exercises exactly the empty-snapshot/`NoWindows` path (mutation M2 confirms it is that path’s discriminator). Add T-S-10: `| REQ-S-006 | T-F-04, T-S-10 | …`.
- **L-2 (traceability over-claim; REQ-R-002).** `REQ-TRACE.md:86` now maps `REQ-R-002 → T-S-06, T-S-10`, and `CHANGELOG.md` cites REQ-R-002 for T-S-10. REQ-R-002 is the *invalid-origin* clause; T-S-10 deliberately keeps the origin **valid but out of the snapshot**, so it does not exercise REQ-R-002 (T-S-06 does; the plan §2 mapped REQ-R-002 → T-S-06 only). T-S-10’s true anchors are the new §6 sentence and REQ-S-006/REQ-S-005. Recommend dropping T-S-10 from the REQ-R-002 row, or separating the two claims in that row. Also SPEC §9’s T-S-10 row cites only REQ-S-006 while the shutdown case is REQ-S-005 — harmless, but a `REQ-S-005/006` pair would be exact.
- **L-3 (SPEC wording).** The new sentence at `docs/SPEC.md:398` is unnumbered, and its second clause (“a session ended for any other reason (… apply) never moves focus”) is literally false for the `apply` end path, which *does* move focus to the selection (REQ-S-004 / REQ-F-002) — the intended subject is “the optional restore step”. Suggested rephrase + ID, e.g. **REQ-R-003**: “The optional restore step SHALL run only for an explicit `mru:cancel`; when the session ends for any other reason (REQ-S-006 empty snapshot/`NoWindows`, plugin shutdown, apply) the controller SHALL NOT perform a restore focus.” That also gives T-S-10 a citable normative anchor (see L-2).
- **L-4 (evidence-scope wording; REQ-R-002 live leg).** N3 (origin killed mid-session, then cancel) is a valid *no-crash / no-focus-side-effect* observation but it is **not guard-discriminating**: with the validity guard removed, a focus call on a dead `WindowRef` still resolves through the weak-lock validity path (ADR-013/016) and no-ops, so the live row cannot distinguish a guarded from an unguarded implementation. The guarding evidence is T-S-06 (mutation M3 RED). The report (“which is what makes the REQ-R-002 leg meaningful”) and `docs/COMPAT.md` (“`mru:cancel` changed no focus (REQ-R-002)”) read as stronger than the observation supports; add a one-line qualifier (“no crash / no focus side effect; the guard itself is unit-discriminated by T-S-06”). The analogous T-S-10 boundary **is** documented honestly, so this is a consistency nit, not a gap.
- **L-5 (artifact hygiene).** The plan of record `docs/agent-state/plans/2026-09-19-m4-restore-on-cancel.md` (referenced by issue #34, the nest report and `SESSION.md`) is **untracked** — it exists on disk only and contains a duplicated heading (`## 5. DoD / merge gates…` at lines 117-118). Commit it with the closeout PR or drop the reference; otherwise the §3.1a correction — which this review verified is exactly what the delivered tests implement — is not preserved in git.

## 6. Items that could not be verified

- **U-1** The nested-session run itself is not reproducible in this review (nest runs are out of scope/forbidden here). The N1–N4 verdicts rest on the recorded raw artifacts, which exist, are internally consistent, and match the committed report (sha256, byte size, mtime, crash scan, pre-cancel reference shots, config diffs). Independent re-execution would be required to falsify them.
- **U-2** Repository/PR-level state: issue #34 is still `OPEN` and no PR exists yet, so the PR body (`Fixes #34`) and the merge-time checkbox flip are unverifiable here (folded into M-1).
- **U-3** The GitHub CI run was not observed. Local equivalents were run instead (clang 22 `-Werror` unit 12/12, ASan+UBSan 12/12, the four static guards, clang-format clean). The `plugin-build` CI job (Arch container against real pinned headers) was not reproduced; unaffected in practice because no `src/`/`include/` file changed and the recorded `.so` still loads live.
- **U-4** That the `.so` exercised in the nest was built at exactly `bb03942`: circumstantially supported (built 11:24:20, nest booted 11:25:05; report and COMPAT agree; on-disk hash matches), but build provenance cannot be proven post hoc. Behaviourally irrelevant — `src/` is identical across `main`, `bb03942` and `HEAD`.
- **U-5** The raw nest artifacts live under `/tmp` (COMPAT precedent from the 2026-09-18 matrix) and are not committed; the report’s inlined annex mitigates this, but `/tmp` is ephemeral.

## 7. Verdict

**APPROVE WITH NITS** — commit-ready except for the state-hygiene item the issue itself demands.

The branch is a clean, behaviour-neutral closeout: `src/` and `include/` are untouched, so every SPEC/ADR-adjacent guarantee (hash check, cycle-never-focuses, domain purity, config namespace, single FocusGateway) holds by construction and was re-confirmed by builds and guards. The two new regression tests are **genuinely discriminating** (M1–M6 above, with the M2/§3.1a trap explicitly avoided), the §6 sentence is a documentation-only clarification consistent with `cancel()` and `TRANSITION-TABLE.md`, no ADR is required, the live evidence chain is traceable to artifacts that still exist and hash-match the reviewed source (`bb03942..HEAD` = docs only), and the documented unit-only boundary for T-S-10 is honest.

**Mandatory pre-merge fixes:**
1. **M-1** — commit a fresh `docs/agent-state/SESSION.md` and flip `docs/agent-state/PROGRESS.md:82` (issue #34 DoD).
2. **L-1** — add `T-S-10` to the `REQ-S-006` row in `docs/REQ-TRACE.md:15`.

Recommended, non-blocking (fold into the same docs commit if convenient): **L-2** (REQ-R-002 → T-S-10 over-claim), **L-3** (number/reword the SPEC §6 sentence), **L-4** (qualify the N3/COMPAT evidence wording), **L-5** (commit the plan of record).

**Counts: 0 BLOCKER · 0 HIGH · 1 MEDIUM (M-1) · 5 LOW (L-1…L-5) · 5 unverifiable (U-1…U-5).** Mutation check: M1–M6 all behaved as predicted (4 required + 2 bonus; control M4 green).
