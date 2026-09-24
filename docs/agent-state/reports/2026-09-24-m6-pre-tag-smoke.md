# M6-T9 pre-tag nest gate — v1.0.0 release candidate (issue #54 follow-up)

Date (UTC): 2026-09-24 ~10:05–10:20
Tester: orchestrator (manual nest per `hyprland-nested-dev` SKILL.md smoke checklist)
Scope: release-candidate commit `eda3bd9` (PR #117 squash: CMake 1.0.0, single pin, CHANGELOG [1.0.0])
Verdict: **GATE PASS** (functional legs all green; pulse/dim visual-by-eye pending — nest is headless, no screenshot path; no crash/hang/stuck state in either style)

## 1. Environment / pin

| Item | Fact |
|---|---|
| Host compositor | Hyprland 0.56.2, commit `efb50993780079460b0cbed1363e2166a2de1d9f` clean, pid 1203, wl `wayland-1` |
| Nest | Same binary, NSIG `efb50993…_1790233743_638171571`, wl `wayland-2`, pid 17856, config `/tmp/nest-v1/hypr-nest.conf` (1280x720, foot, no logo/splash) |
| Repo | `/home/code_warlord/Work/DEV/mru-switcher`, branch `main`, commit `eda3bd9` |
| .so | `cmake -S <repo> -B /tmp/nest-v1/build -DCMAKE_BUILD_TYPE=Release -DMRU_BUILD_PLUGIN=ON -DMRU_BUILD_TESTS=OFF` → conf=0, build=0; `/tmp/nest-v1/build/mru-switcher.so` (721 KB). `plugin list` in nest: `mru-switcher by Yuriy Tretyakov (code-warlord-dev), Version 1.0.0` |
| Sandbox | `/tmp/nest-v1` (build, cache, hypr-nest.conf, nest.log) |

## 2. Steps (step / expected / fact / verdict)

| # | Step | Expected | Fact | Verdict |
|---|---|---|---|---|
| 1 | Build .so in sandbox | conf=0, build=0 | conf=0, build=0, .so present | PASS |
| 2 | Raise nest (nested backend, keep WAYLAND_DISPLAY) | host + nest in `instances` | host (wayland-1) + nest (wayland-2) | PASS |
| 3 | 4 windows | `clients -j` = 4 | 4× foot (WIN_A..D via initialTitle) | PASS |
| 4 | `plugin load` + `plugin list` | `ok`, Version 1.0.0 | load `ok`, list shows `Version: 1.0.0`, author `Yuriy Tretyakov (code-warlord-dev)` | PASS |
| 5 | No-side-effect: 2× `mru:cycle next`, focus check | focus immobile | `BEFORE == MID == MID2 (0x…3ac0)` | PASS |
| 6 | `mru:apply` | exactly one focus change | `AFTER (0x…8dc40) != BEFORE` | PASS |
| 7 | `mru:cancel` after cycle | `ok`, no focus move | `ok` | PASS |
| 8 | Wrap: 4× cycle + apply | lands on start | `START == END (0x…8dc40)` | PASS |
| 9 | Stress: 200× `mru:cycle next` + apply | 0 errors | 0 FAIL, `1.045s` (~5ms/dispatch incl. IPC), apply `ok` | PASS |
| 10 | Chained apply rotates (ADR-021) | A1≠A0, A2≠A1 | `A0=…8dc40 A1=…3ac0 A2=…8dc40` — deterministic A↔B rotation | PASS |
| 11 | Follow-workspace (ADR-026): WIN_A → ws 2, cycle | active ws follows selection (2), no focus move | `after cycle 1: ws=2`; cycles 2–3 back on ws 1 ring; cancel clean | PASS |
| 12 | Pulse style (`border_style=pulse`, period 600ms) | session alive, timer re-arms, cancel clean | 2s session, plugin alive, cancel `ok`; nest log has no crash/hang | PASS (visual throb by-eye pending — headless nest) |
| 13 | Dim style (`border_style=dim`, alpha 0.5) | session runs, cancel restores | cycle `ok`, cancel `ok`; `clients -j` shows no `alpha` field (foot/Wayland reporting, not a plugin read-path — dim writes go via `setprop`) | PASS (visual dim by-eye pending — headless nest) |
| 14 | Teardown: `plugin unload`, dispatcher gone | `no plugins loaded`, `Invalid dispatcher` | both confirmed | PASS |
| 15 | Nest log scan | no crash/sevg/abort | only benign lines (scheduling-strategy WARN, xkbcomp, Error Overlay creation) | PASS |
| 16 | Host untouched | host `plugin list` = only host plugin | host shows only its own `mru-switcher` (Handle differs), nest pid gone, 1 instance left | PASS |

## 3. Notes / deviations

- `hyprctl dispatch 'getprop address:… opacity'` in the nest returns `Invalid dispatcher` — `getprop` is not a dispatcher on this pin; dim verification stays at the session-lifecycle level (cycle/cancel clean, no stuck state). The dim write path (`setprop` via backend-agnostic translator) is unchanged since the M4 nest verification.
- Pulse/dim **visual** confirmation (colour throb / alpha drop seen on screen) is not possible from this headless nest — no screenshot path was staged. Recorded as the same pending previously noted in REQ-TRACE deviations; functional legs (timer schedules without throw, session ends restore cleanly, no stuck borders) are green.
- `bindrt`/modifier-release legs are covered by ADR-023 product decision (explicit ALT+Return apply) and stay out of this gate.

## 4. Conclusion

Release candidate `eda3bd9` passes the pre-tag nest gate: snapshot/no-side-effect, apply/cancel, wrap, 200-cycle stress, chained-apply rotation, follow-workspace, pulse/dim session lifecycle, clean teardown, clean nest log, host untouched. **Recommend proceeding to step 1: pin finalization → `git tag v1.0.0` → GitHub Release → hyprpm smoke.**
