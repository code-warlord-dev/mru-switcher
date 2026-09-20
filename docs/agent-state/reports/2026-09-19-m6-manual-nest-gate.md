# Manual nest gate — pre-tag smoke before v1.0.0 (M6-T3, issue #49)

Date: 2026-09-19. Status: ready-to-execute checklist.
Why manual: a CI runner cannot stage an honest nested Wayland/compositor
deterministically (see `.agents/skills/hyprland-nested-dev/SKILL.md` §CI note).
CI (`release-guard` job) enforces the static invariants; this gate enforces the
live ones. Run it once on the release candidate commit, then file the filled
report next to the M6 smokes as `docs/agent-state/reports/<date>-m6-pre-tag-smoke.md`.

Evidence it builds on: `2026-09-19-m6-rapid-tab-smoke.md` (T-H-08),
`2026-09-19-m6-window-close-smoke.md` (T-H-06),
`2026-09-19-m6-monitor-disconnect-smoke.md` (T-H-07).

## 0. Prerequisites

- Host Hyprland = pin (`docs/COMPAT.md`, currently 0.56.2 / `efb50993`).
- Terminal available in the nest (`foot` worked on the pin; check `which foot kitty xterm`).
- Fresh sandbox: `export SANDBOX=/tmp/nest-gate && mkdir -p $SANDBOX/cache`.
- NEVER touch the host compositor config; never `plugin load` on the host.

## 1. Build the candidate .so

```bash
cmake -S <repo> -B $SANDBOX/build -DMRU_BUILD_PLUGIN=ON -DMRU_BUILD_TESTS=OFF -DCMAKE_CXX_COMPILER=g++
cmake --build $SANDBOX/build -j
ls -la $SANDBOX/build/mru-switcher.so
```

Expect: configure rc=0, build rc=0, `.so` present.

## 2. Start the nest (WITHOUT env -u WAYLAND_DISPLAY)

The nested backend is a client of host Wayland — unsetting `WAYLAND_DISPLAY`
(T6 failure mode) leaves the nest with no backend. Launch so the nest inherits
it, with its own cache and config:

```bash
cat > $SANDBOX/hypr-nest.conf <<'EOF'
monitor = ,preferred,auto,1
exec-once = foot
exec-once = foot
exec-once = foot
exec-once = foot
EOF
XDG_CACHE_HOME=$SANDBOX/cache Hyprland -c $SANDBOX/hypr-nest.conf > $SANDBOX/nest.log 2>&1 &
sleep 3
hyprctl instances   # expect: host + nest
```

## 3. Smoke checklist (from hyprland-nested-dev SKILL + M6 reports)

Target the nest in every command (`HYPRLAND_INSTANCE_SIGNATURE=<nest-sig>`
or `hyprctl -i <nest>`):

- [ ] `hyprctl plugin load $SANDBOX/build/mru-switcher.so` → `ok`; plugin in `hyprctl plugin list`.
- [ ] `hyprctl clients -j` shows 4 windows; `hyprctl dispatch mru:cycle next` → `ok`.
- [ ] Recommended binds in nest config (if testing binds); release-modifier triggers apply (`bindrt`).
- [ ] `mru:cycle` never moves focus (compare `hyprctl activewindow -j` before/after) — REQ-F-003.
- [ ] Escape / `mru:cancel` cancels; `mru:status` → `ok` (note: on 0.56.2 `hyprctl dispatch`
      prints only `ok` without the payload body — compare via `activewindow -j`).
- [ ] **Rapid-Tab 200 (T-H-08):** loop 200× `dispatch mru:cycle next`, then `mru:apply`;
      expect rc=0, no `FAIL@i`, focus moved exactly once, nest alive (~1s is normal).
- [ ] **Close-storm (T-H-06):** mid-session kill 1 window → apply lands on MRU neighbour;
      kill all-but-one → focus on survivor; kill all → graceful end, no crash.
- [ ] **Monitor-disconnect (T-H-07):** `hyprctl output create headless` for a 2nd monitor
      (`monitor=` lines in config do NOT raise it on this pin); `mru:cycle next monitor`,
      then `output remove HEADLESS-1` mid-session → `mru:apply` ends cleanly, no crash.
- [ ] Unload leaves nothing stuck: `mru:cancel`, `plugin unload`, nest `plugin list` empty.

## 4. Teardown hygiene

```bash
hyprctl -i <nest> dispatch mru:cancel
hyprctl -i <nest> plugin unload $SANDBOX/build/mru-switcher.so
kill <nest-pid>; sleep 2; pgrep -a Hyprland   # only the host pid must remain
hyprctl plugin list                            # host: no plugins loaded
```

## 5. Verdict

- **GATE PASS** → tag `v1.0.0` may proceed (per `docs/SUPPORT-AND-RELEASE.md` checklist).
- Any crash / stuck focus / host pollution → **GATE FAIL**, file S0/S1 per
  `SUPPORT-AND-RELEASE.md` §Bug severity; do not tag.
