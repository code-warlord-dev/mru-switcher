# Nested repro report — M6-B1: same-path changed-`.so` reload crash (issue #55)

**Date:** 2026-09-20
**Branch:** `docs/m6-b1-reload` (from `origin/main` @ `68a5666`)
**Pin:** Hyprland v0.56.2 / `efb50993780079460b0cbed1363e2166a2de1d9f` (host pid 1250 — **untouched**), aquamarine 0.15.0, Wayland nested backend.
**Build under test:** `mru-switcher.so` 0.4.0 Release, `MRU_BUILD_PLUGIN=ON`, pure domain tests off, built out-of-repo in `$SANDBOX/build*`.
**Sandbox:** `/tmp/mru-nest-b1/` (builds, nest config, logs, crash reports, `report/`).
**Verdict:** REPRODUCED — with a sharpened trigger: not "changed bytes" but "file overwritten **in place** (same inode) between unload and reload". 3/3 crash. New-inode replacement: 3/3 clean.

## Artifacts under test

| Tag | sha256 | BuildID | Diff from clean |
|-----|--------|---------|-----------------|
| A | `0509b82…a5cf` | `d162362c…` | clean `origin/main` source |
| B | `b8ef814…6ad2` | `6b1f3338…` | cosmetic: plugin description string gains "; B1 probe build" |

Both are valid ELF shared objects (`file`: ELF 64-bit LSB shared object, x86-64). A rebuild of clean source reproduced byte-identically (`0509b82…`).

## Nest setup

Fresh compositor per run, never reusing a process:
```bash
setsid env -u HYPRLAND_INSTANCE_SIGNATURE -u HYPRLAND_CONFIG \
  XDG_CACHE_HOME=/tmp/mru-nest-b1/cache \
  Hyprland -c /tmp/mru-nest-b1/hypr-nest.conf
```
Config: single `WAYLAND-1` output 1280x800, two `foot` windows, no host includes. Sanity baseline each run before the reload: `mru:cycle next` → `ok` (focus **unchanged** mid-session), `mru:apply` → `ok` (focus moved). `hyprctl` targeted by `HYPRLAND_INSTANCE_SIGNATURE` of the nest only.

## Results

All runs: fresh nest → `plugin load P` → sanity → `plugin unload P` → replace file at **same path** P (`/tmp/mru-nest-b1/mru-switcher.so`) → `plugin load P` again.

| # | Nest pid | 1st load | Replace method | 2nd load | Result |
|---|----------|----------|----------------|----------|--------|
| 1 | 432122 | A | **cp in place** (same inode) | B | **CRASH SIGSEGV** `hyprlandCrashReport432122` |
| 2 | 433417 | B | **cp in place** (same inode; **identical bytes**) | B | **CRASH SIGSEGV** `hyprlandCrashReport433417` |
| 3 | 434525 | A | rm + cp (new inode) | B | ok, nest alive |
| 4 | 435160 | B | rm + cp (new inode) | A | ok, nest alive |
| 5 | 436113 | A | rm + cp (new inode) | B | ok, nest alive |
| 6 | 436772 | A | **cp -f in place** (same inode) | B | **CRASH SIGSEGV** `hyprlandCrashReport436772` |

Specified ×3 changed-binary cycles = runs 1, 3, 5 (attempts 2, 4, 6 are the controls that isolate the trigger). Control from M5 smoke (13/13 PASS, step 13) covers load→unload→load **without touching the file**: clean.

## Evidence excerpt (identical in all three reports)

`hyprlandCrashReport436772.txt` (mirrors 432122 / 433417), key frames:
```
Hyprland received signal 11(SEGV)          Version: efb50993780079460b0cbed1363e2166a2de1d9f
Plugins: ()                                 # "may not be Hyprland's fault"
#12 | dlsym+0x8e                                   (libc)
#13 | CPluginSystem::loadPluginInternal(..)+0x344  (Hyprland)
#14..#16 | loadPluginInternal / loadPlugin / hyprctl dispatch path
```
Crash site is inside the dynamic loader's `dlsym` while `CPluginSystem::loadPluginInternal` re-resolves the plugin entry point after `dlopen` of a same-inode-rewritten file. Full reports preserved at `/tmp/mru-nest-b1/cache/hyprland/hyprlandCrashReport{432122,433417,436772}.txt`.

## Decision recommendation

**Reproduced.** Under the original *"changed binary at same path"* framing alone the crash was intermittent (2/5); it becomes **deterministic (3/3) when the file is overwritten in place (same inode) after unload** — glibc keeps the unloaded object keyed by path/(dev,ino), revisits the stale symbol table on reload, and `dlsym` walks inconsistent `l_info` state → SEGV. Replacement via **new inode** (rm+cp / atomic rename) reloads cleanly (3/3). Byte content is irrelevant (run 2 crashed with identical bytes).

Per brief step 2: no speculative mitigation attempted. Orchestrator choices:
1. **Documented limitation (recommended):** operator guidance sharpens to "never overwrite `mru-switcher.so` in place; install via rename/new path, and prefer a fresh compositor for upgrades" + one release-note line. Reopen as docs/issue `docs/m6-b1-reload`.
2. Optional follow-up `fix/` ticket if a cheap plugin-side guard is wanted (e.g., refuse load in-process after a prior load of the same path with a different file identity) — needs SPEC/ADR-aware ticket, still non-blocking for v1.0.

## Teardown proof

- All nest instances (pids 432122, 433417, 434525, 435160, 436113, 436772) exited or crashed; their `hypr/*` instance dirs removed.
- **Only** live Hyprland lock at teardown: `efb50993780079460b0cbed1363e2166a2de1d9f_1789804326_83560980` → pid **1250** (the host, untouched; host `plugin list` = `no plugins loaded` before/during/after).
- Xwayland `pid 1389` predates the session (started 2026-09-19 10:52) — host-session process, not a nest leftover.
- No build artifacts inside the repo; all binaries/logs under `/tmp/mru-nest-b1/`.