# Plan — M3-S3: scope adapter (`PHLWINDOW → WindowMeta`, visible special-ws rule, remaining keys, T-SC-05)

Date: 2026-09-17 · Branch: `feat/m3-scope-adapter` · Issue: #20 (controlling spec: S3-1..S3-6)
Source: planner subagent artifact; feeds the implementer brief only — **no implementation code here**.

**Precondition — nest is WORKING** (Hyprland v0.56.2 / aquamarine 0.15.0, Omarchy; Wayland nested backend): a
nested session starts, is interactive, loads the plugin. Recipe: `docs/COMPAT.md` §“Nest recipe”; evidence:
`docs/agent-state/research/2026-09-17-nest-aquamarine-diagnosis.md` (STATUS=working). §7 below is therefore
**executable live**; no “verify by pinned source” workaround is needed. DRM `seatd.sock`/logind failure in nest
logs is expected and benign (auto-fallback to Wayland). B2 (reentrant `window.active` inside `focus()`) is
**verified live, not an assumption** — applying registers the window as MRU head (research artifact above).

## 1. Purpose + issue #20 linkage

Wire the M3-S2 pure predicate into the adapter layer (ADR-016 __6__ step 2): make `HyprlandWindowSource`
derive `WindowMeta`/`FocusContext` from pinned-0.56.2 compositor windows, drive `scope_matches()` as the
candidate filter for all five scopes, register the last documented config key, and close the T-SC-05
parse/behavior split (S3-1..S3-6 of issue #20). Session/selection behavior stays M2-identical for
`global` (drop-in), so the M2 dispatcher/unit tests remain green. DoD repo gates: both CI ctest matrices green,
`plugin-guards`/`domain-deps` pass, `rg` Hyprland-free guard on domain, CHANGELOG bullet, REQ-TRACE rows
updated, PROGRESS M3-S3 checkbox.

## 2. Step 1 — Mapping capsule `PHLWINDOW → WindowMeta` (S3-1, S3-2)

**Location:** adapter only — new file-local helpers in `src/plugin/hypr/hypr_window_source.cpp` (ADR-007,
ADR-016 __2__ decision "adapter single duty"). Domain untouched.

Pinned fields (`CWindow`, `/usr/include/hyprland/src/desktop/view/Window.hpp`):

| WindowMeta field | pinned source | file:line |
|---|---|---|
| `monitor_id` | `static_cast<uint64_t>(w->monitorID())` | Window.hpp:372 |
| `workspace_id` | `static_cast<uint64_t>(w->workspaceID())` | Window.hpp:371 |
| `mapped` | `w->m_isMapped` | Window.hpp:172 |
| `app_class` | `w->m_class` (the class; **NOT** `m_initialClass` Window.hpp:168, REQ-SC-002b) | Window.hpp:166 |
| `hidden` | special-workspace-not-shown, single expression below | — |

**THE one expression (S3-2 single source).** `hidden` and `FocusContext.visible_workspaces` MUST derive
from the same query. The plan fixes that query as the **enumerated visible-workspace id set** built once
per snapshot from every enabled monitor's active + active-special workspace:

```text
visible_set = { m->activeWorkspaceID() } ∪ { m->activeSpecialWorkspaceID() != WORKSPACE_INVALID }
              for each m in State::monitorState()->monitors()

candidate.hidden = w->m_workspace                     // guard null (unmanaged)
                && w->m_workspace->m_isSpecialWorkspace
                && visible_set.count(w->workspaceID()) == 0
```

The same `visible_set` populates `FocusContext.visible_workspaces`. Window on the shown scratchpad →
`hidden=false` and its workspace IS in the set; toggled away → `hidden=true` and the workspace leaves the
set. A MEDIUM-class drift between "adapter visible" and "domain expected visible" is impossible by
construction: both sides read the one vector field (`focus.visible_workspaces`) that `candidates()` filled
from the one enumeration.

**Justification vs `isVisible()`/`isVisibleNotCovered()` (Workspace.hpp:66/67):** both are declared but their
exact semantics (special-workspace "shown on a monitor", animation/fade influence) are NOT confirmable from
installed headers alone (no `Workspace.cpp` installed). The set built from `Monitor::CMonitor::m_activeWorkspace`
/ `m_activeSpecialWorkspace` (Monitor.hpp:75/76) matches the REQ-SC-002a wording verbatim ("currently shown on
a monitor", "member of the visible workspace set"), so it is both confirmable and the single source.

**`is_candidate()` unchanged** at hypr_window_source.cpp:15-16 (`w && m_isMapped && !isHidden()`) — the
REQ-SNAP-002 subset (ADR-016 __7__, issue #18 S2-7). A special workspace that is shown does not mark the
window hidden; a hidden one also trips `isHidden()` in M2 — redundant-but-consistent with the domain gate.

**Facts NOT confirmable with file:line (double-check in review):**
- `CWindow` has NO `class()`/`initialClass()` accessor on this pin; only public `m_class` (166), `m_initialClass`
  (168), and `fetchClass()` (402). Plan uses `m_class`; confirm at review that `fetchClass()` == `m_class`.
- `Workspace::isVisible()` / `isVisibleNotCovered()` (Workspace.hpp:66/67) internals — source not installed; avoided by design.
- `Monitor::CMonitor::activeSpecialWorkspaceID()` (Monitor.hpp:278) return when no special workspace — headers
  give only `WORKSPACE_INVALID = -1L` (macros.hpp:26); assume `WORKSPACE_INVALID`, verify.
- `g_pCompositor->m_vMonitors` does NOT exist on this pin (Compositor.hpp:30-113 scanned); the monitor list is
  `State::monitorState()->monitors()` (MonitorState.hpp:20,32:43). `monitors()` vs `allMonitors()` (19: enabled-only
  vs incl. disabled) semantics not confirmable from headers — assume `monitors()` = enabled, verify.

## 3. Step 2 — `FocusContext` once per snapshot (S3-2, S3-3)

Add two file-local helpers in `hypr_window_source.cpp` (members on `HyprlandWindowSource`, matching the
`registry_`/`tracker_` style):

```text
mru::domain::FocusContext current_focus() const;   // reads focus, fills anchors + visible_workspaces in ONE pass
WindowMeta derive_meta(const PHLWINDOW&, const mru::domain::FocusContext&) const;
```

- **Focused window:** `Desktop::focusState()->window()` (FocusState.hpp:51, include
  `desktop/state/FocusState.hpp` — already included). Null/empty → `has_focus=false`; else fill `monitor_id`
  from `w->monitorID()`, `workspace_id` from `w->workspaceID()`, `app_class` from `w->m_class`.
- **Visible set (single enumeration):** loop `for (const auto& m : State::monitorState()->monitors())`
  (include `state/MonitorState.hpp`); push `m->activeWorkspaceID()` and, when `!= WORKSPACE_INVALID`,
  `m->activeSpecialWorkspaceID()`; cast `WORKSPACEID`(int64_t, SharedDefs.hpp:60) → `uint64_t`. Store into
  `FocusContext.visible_workspaces`.
- **Passed by const&:** `candidates(Scope)` calls `current_focus()` exactly once per call (snapshot build),
  then passes the same `const FocusContext&` to every `scope_matches()` and to every `derive_meta()`. Never
  recomputed per window (ADR-016 __2__).

## 4. Step 3 — `candidates(scope)` rewiring (S3-3)

Replace the M2 Global-only wall (`if (scope != Global) return {};` hypr_window_source.cpp:25-26) with a
scope filter applied to BOTH candidate paths, preserving the ADR-015 order (primary = `tracker_.order()`
REQ-H-004a; fallback = `windowTracker()->fullHistory()` newest-first + `live_refs_newest_first()` REQ-H-004b;
dedup via `merge_mru_order`):

```text
candidates(scope):
    focus = current_focus()                       // once (Step 2)
    if history: for each lockable entry w:
        if !is_candidate(w): skip
        ref = registry_.register_window(w)        // register on sight, unchanged (REQ-H-004b)
        if scope_matches(scope, derive_meta(w, focus), focus): fallback.push_back(ref)
    for ref in registry_.live_refs_newest_first():  // same filter; resolve + is_candidate first
        ...
    primary: for ref in tracker_.order():
        w = registry_.resolve(ref)
        if w && is_candidate(w) && scope_matches(scope, derive_meta(w, focus), focus): primary.push_back(ref)
    return merge_mru_order(primary, fallback)
```

- Both paths share `derive_meta()`; the domain (`scope_matches`) is the ONLY membership rule — its
  `mapped && !hidden` gate (scope_predicate.cpp:12-13) enforces REQ-SC-002a uniformly for all five scopes.
- `is_valid()` stays exactly REQ-ID-006 (`static_cast<bool>(registry_.resolve(ref))`, hypr_window_source.cpp:52-54)
  — registry weak-lock validity (ADR-016 __1__); no scope influence.
- **Drop-in guarantee for `global`:** with `scope_matches(Global, …)` the gate reduces to `mapped && !hidden`;
  `derive_meta.hidden` is false for every non-special window, so the filtered set equals M2's. M2 unit/nest tests stay green.

## 5. Step 4 — Config surface (S3-4)

SPEC §4 table (SPEC.md:306-315) lists **8** keys; `register_all` currently registers **7**. The one remaining
documented key:

- **`external_socket`** → add `SP<Config::Values::String> external_socket` to `Values` (config_v2.hpp:18-26) and
  register in `config_v2.cpp` `register_all`: `KEY_EXTERNAL_SOCKET = "plugin:mru-switcher:external_socket"`,
  default `""`, description "External UI protocol socket — reserved, no effect until M5 (ADR-016 __5__)".
  `read_config` does NOT consume it (no `PluginConfig` member; M5). Registered only in `PLUGIN_INIT` (ADR-008);
  `hyprctl getoption plugin:mru-switcher:*` then shows all 8 documented keys.
- **Remove the MEDIUM-7 block** (config_v2.cpp:66-81): the "non-global default_scope → force global + once-warn"
  is obsolete now that all five scopes are implemented. `default_scope` resolves as parsed.
- **Kept:** REQ-CFG-001 enum fallback (invalid string → default via `parse_scope`, config_value.hpp:25) and the
  `ui=border/external → null` once-warn (still M4/M5, REQ-CFG-003); debounce clamp [0,5000] (REQ-CFG-004).
- CI `plugin-guards` (ci.yml:154-163) already allows `addConfigValueV2` only in config_v2/mru_plugin and keys
  only under `plugin:mru-switcher:` — the new key satisfies both.

## 6. Step 5 — T-SC-05 split (S3-5)

- **Parse layer — 100% unit, both CI configs:** make the unknown-token error explicit and distinct from the
  grammar errors. In `dispatch_args.cpp:47-48` change the message from `"unknown argument: " + tok` to
  `"unknown scope token: " + tok` (scope-position token that is neither direction nor valid scope token;
  `"too many arguments"` at :55 stays for the extra-token case). Add tests to `tests/plugin/test_dispatch_args.cpp`
  asserting `parse_cycle_args("bogus")` and `parse_cycle_args("next bogus")` → `ok=false` and
  `error.find("unknown scope token") != npos`; and that it is distinct from `parse_cycle_args("next workspace extra")`
  (`error` differs). dispatch_args is in `mru_plugin_core` (Hyprland-free) → automatic under `unit`, `sanitize`,
  `gcc`, and `plugin-build` jobs.
- **Behavior layer — nested-smoke only:** "session not started" surfaces through the dispatcher against a live
  compositor; tagged in REQ-TRACE as `T-SC-05 parse = unit, T-SC-05 behavior = nested checklist` (item 8 below).
  Do NOT claim automatic coverage of the dispatcher path.

## 7. Step 6 — Nested-smoke checklist (runnable live)

**Run in a live nested session** — nest verified working 2026-09-17 (Hyprland v0.56.2 / aquamarine 0.15.0);
start it with `docs/COMPAT.md` §“Nest recipe” (`hyprctl instances -j` for SIG → `plugin load` into the nest).
Every step below is executable as written; no “pinned source” substitute is needed. Each step:

1. **Load + config surface.** `hyprctl plugin load` OK (hash check), then `hyprctl getoption plugin:mru-switcher:*`
   lists all 8 keys including `external_socket`. **Expected:** 8 options present, defaults match SPEC §4.
   **Proves:** REQ-CFG-004, REQ-CFG-001, ADR-008, S3-4.
2. **Scratchpad shown.** Focus inside a scratchpad window, `hyprctl dispatch togglespecialworkspace` to show it;
   run `mru:cycle global|monitor|workspace|visible|app` (each). **Expected:** scratchpad window is a candidate
   in every scope. **Proves:** REQ-SC-002a "shown" leg, S3-2 set membership.
3. **Scratchpad hidden.** Toggle special workspace away; repeat the five cycles. **Expected:** scratchpad window
   absent from all five candidate lists. **Proves:** REQ-SC-002a "hidden" leg, `hidden` flag + domain gate.
4. **2 monitors / 2+ workspaces.** Window on monitor A ws 1, monitor B ws 2, monitor A ws 3; focus monitor A ws 1.
   **Expected:** `monitor` → A-windows only; `workspace` → ws-1 windows only; `visible` → A-ws1 + B-ws2 (all shown);
   `global` → all valid. **Proves:** REQ-SC-002 rows, T-SC-01 behavior.
5. **app byte-exact.** Two same-class windows (e.g. two `kitty`) + one `Firefox`; focus a kitty.
   **Expected:** `app` lists exactly the two `kitty` windows (focused one included); a case-variant class
   (e.g. `Kitty`) does not match. **Proves:** REQ-SC-002b, T-SC-04 behavior, deliberate-case-sensitivity.
6. **Focus-less global degrade.** Clear focus (focus some surface/root); `mru:cycle monitor|workspace|app`.
   **Expected:** all behave as `global`; `visible` still filters to shown workspaces. **Proves:** REQ-SC-002
   no-focus pin, T-SC-02 behavior.
7. **T-SC-05 behavior.** `hyprctl dispatch 'mru:cycle bogus'`. **Expected:** dispatcher returns failure with an
   error naming the unknown scope token; `mru:status` shows idle (session NOT started). **Proves:** REQ-SC-003.
8. **M2 drop-in regression.** Default `mru:cycle` / `mru:apply` / `mru:cancel` on plain windows.
   **Expected:** order = plugin-owned MRU first, pre-load windows visible, apply-on-release focuses once, no
   `mru:cycle` focus side effect. **Proves:** issue #20 Goal ("M2-identical for global"), REQ-F-003, ADR-015.

## 8. Step 7 — Test plan / seam mapping (S3-6)

| Seam | Unit-testable via fakes? | CI coverage |
|---|---|---|
| `scope_matches` five-scope semantics (T-SC-01..04) | YES (already in tests/domain/test_scope_predicate.cpp) | unit, sanitize, gcc, plugin-build |
| T-SC-05 parse (`parse_cycle_args`) | YES — new tests in test_dispatch_args.cpp | all 4 jobs, 100% automatic |
| Config enums/fallback (parse_scope, parse_scope_token, clamp) | YES (existing test_config_value.cpp) | all 4 jobs |
| `register_all` + `read_config` (config_v2) | NO — needs HyprlandAPI/addNotification | plugin-build + nest (getoption) |
| `PHLWINDOW → WindowMeta` mapping (S3-1/2) | NO — the ONE M3 function without a compositor-free test (ADR-016 accepted risk) | nest checklist 2-4 |
| `candidates()` scope rewiring between registry/tracker/merge | PARTIAL — pure ordering via merge_mru_order (T-merge-*) stays; scope filter wiring needs compositor | nest checklist 2-6 |

REQ-TRACE updates (same PR): REQ-SC-002 rows (M3 + nest), REQ-SC-002a (T-SC-03 + nest 2-3), REQ-SC-002b
(T-SC-04 + nest 5), REQ-SC-003 (T-SC-05 split: parse=unit, behavior=nest 7); T-SC-05 index row split note.

## 9. Step ordering, per-file edits, verification (implementer loop)

**Order** (each step independently reviewable; TDD where the seam allows):

0. **ALREADY DONE (uncommitted), run + commit — do not redo.** Step 1 below is partly applied in the working
   tree: `src/plugin/dispatch_args.cpp` (error message → `"unknown scope token: "`) and
   `tests/plugin/test_dispatch_args.cpp` (`TEST(disp_05_unknown_scope_token)`: `bogus` / `next bogus` match
   `unknown scope token`, `next workspace extra` yields the distinct `too many arguments`). Confirm the diff
   matches §6, run the core matrix, then commit before any new work:
   ```bash
   git diff -- src/plugin/dispatch_args.cpp tests/plugin/test_dispatch_args.cpp   # expect exactly the §6 change
   cmake -S . -B build -DMRU_BUILD_TESTS=ON -DMRU_BUILD_PLUGIN=OFF -DMRU_WARNINGS_AS_ERRORS=ON \
     && cmake --build build -j && ctest --test-dir build --output-on-failure   # disp_05 must pass
   git add src/plugin/dispatch_args.cpp tests/plugin/test_dispatch_args.cpp
   git commit -m "fix(dispatch): distinguish unknown scope token from grammar errors (T-SC-05 parse)"
   ```
1. (rest of Step 1 = commit above is done) — keep §6 wording as the review reference for that commit.
2. `config_v2.hpp` (Values +external_socket) + `config_v2.cpp` (register key; delete MEDIUM-7 block) → plugin build.
3. `hypr_window_source.hpp` (helper decls if members) + `hypr_window_source.cpp` (current_focus, derive_meta,
   candidates rewiring) → plugin build.
4. Docs same PR: `docs/REQ-TRACE.md` (rows above), `CHANGELOG.md` Unreleased bullet, `PROGRESS.md` M3-S3
   checkbox (on merge), `SESSION.md`.
5. Full local matrix, self-review (plugin-spec-compliance + code-review), push, PR vs main, squash-merge.

**Acceptance gates for S3 (all must hold before push; restates the §1 DoD as a checklist):**

- [ ] **Step 0** committed (dispatch_args message + `disp_05_unknown_scope_token`).
- [ ] `unit` matrix green (MRU_BUILD_PLUGIN=OFF build + `ctest --test-dir build --output-on-failure`).
- [ ] `plugin-build` matrix green (MRU_BUILD_PLUGIN=ON; pkg-config hyprland = 0.56.2) + same `ctest`.
- [ ] `domain-deps` guard: `rg '#include[ <"]*hypr(land|utils)' src/domain include/mru/domain` → no matches.
- [ ] `plugin-guards`: `addConfigValueV2` only in config_v2/mru_plugin; keys only `plugin:mru-switcher:`; no V1 API.
- [ ] `clang-format-22 --dry-run -Werror` clean on every touched `.cpp/.hpp`.
- [ ] `sanitize` (ASan+UBSan, clang++-22, PLUGIN=OFF) green.
- [ ] **§7 live in the nest** (COMPAT.md recipe): steps 1-8 PASS, incl. 8-key `getoption` surface and the M2 drop-in regression.
- [ ] `docs/REQ-TRACE.md` rows updated (T-SC-05 parse=unit / behavior=nest split noted).
- [ ] `CHANGELOG.md` Unreleased bullet + `PROGRESS.md` M3-S3 checkbox (on merge) + `SESSION.md` refreshed.

**Per-file edit list:**
- Modify: `src/plugin/dispatch_args.cpp:47-48`, `tests/plugin/test_dispatch_args.cpp` (add T-SC-05 tests),
  `src/plugin/config_v2.hpp` (Values struct), `src/plugin/config_v2.cpp` (key + drop MEDIUM-7),
  `src/plugin/hypr/hypr_window_source.hpp`, `src/plugin/hypr/hypr_window_source.cpp`,
  `docs/REQ-TRACE.md`, `CHANGELOG.md`, `docs/agent-state/PROGRESS.md`.
- No domain `src/domain/*`, no `include/mru/domain/*` edits unless a real bug appears.

**Verification commands** (from CI .github/workflows/ci.yml + repo conventions):
- Domain + plugin-core unit (MRU_BUILD_PLUGIN=OFF): `cmake -S . -B build -DMRU_BUILD_TESTS=ON -DMRU_BUILD_PLUGIN=OFF -DMRU_WARNINGS_AS_ERRORS=ON && cmake --build build -j && ctest --test-dir build --output-on-failure`
- Plugin .so against pinned headers (MRU_BUILD_PLUGIN=ON, pkg-config hyprland = 0.56.2):
  `cmake -S . -B build -DMRU_BUILD_TESTS=ON -DMRU_BUILD_PLUGIN=ON -DMRU_WARNINGS_AS_ERRORS=ON && cmake --build build -j && ctest --test-dir build --output-on-failure`
- Domain Hyprland-free guard (ADR-007): `rg '#include[ <"]*hypr(land|utils)' src/domain include/mru/domain` → **no matches** (also enforced by ci.yml `domain-deps` 132-141).
- Format: `clang-format-22 --dry-run -Werror` on the touched `.cpp/.hpp` (ci.yml:34-38).
- Sanitize (ASan+UBSan, ci.yml:40-67): `cmake -S . -B build-san -DCMAKE_CXX_COMPILER=clang++-22 -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer" -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined" -DMRU_BUILD_TESTS=ON -DMRU_BUILD_PLUGIN=OFF && cmake --build build-san -j && ctest --test-dir build-san --output-on-failure`
- Plugin-guards locally: run the `rg` steps of ci.yml `plugin-guards` (addConfigValueV2 only in config_v2/mru_plugin; keys only `plugin:mru-switcher:`; no V1 API) over the diff.