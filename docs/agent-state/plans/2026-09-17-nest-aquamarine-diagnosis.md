# Nest Diagnosis Plan — Hyprland v0.56.2 / aquamarine 0.56.2 (Omarchy)

> **For agentic workers:** This is a **diagnosis** plan, not a feature plan. It produces an artifact
> (`docs/agent-state/research/2026-09-17-nest-aquamarine-diagnosis.md`) and a nest `STATUS`
> (working / degraded / broken). No production code changes. Steps use `- [ ]` checkboxes.
> Skills to load (per task, progressive disclosure): `hyprland-nested-dev`, `diagnosing-bugs`,
> `hyprland-plugin` (host-API questions only), `research` (upstream churn only).

**Goal:** Determine why a nested Hyprland session is unavailable on this machine, and restore a
usable nest so M2/M3 nested-smoke items (COMPAT.md, M3-S3 §7 checklist) can run live instead of
"verified by pinned source".

**Architecture of the diagnosis:** bisect from "does *any* nested Hyprland start" → "does *this*
config start" → "does the *plugin* load in a nest" → "do our *checks* pass". Each step has a
falsifiable observation and an explicit pass/fail. Failures are **recorded, not worked around**;
each failure maps to an upstream or environment hypothesis with evidence.

**Tech Stack / env (measured 2026-09-17):** Omarchy (Arch-like), `hyprland 0.56.2-2` (commit
`efb50993780079460b0cbed1363e2166a2de1d9f`, tag `v0.56.2`), `aquamarine 0.15.0-2`
(`libaquamarine.so.14`, pkg `aquamarine 0.15.0`). Host session is Wayland
(`XDG_SESSION_TYPE=wayland`, `WAYLAND_DISPLAY=wayland-1`); `/usr/share/wayland-sessions`
offers `hyprland.desktop`. Headers present at `/usr/include/hyprland/{src,protocols}`.
**Spec:** `docs/COMPAT.md` (matrix + verification checklist), `docs/HYPRLAND-PLUGIN-SYSTEM.md`,
`.agents/skills/hyprland-nested-dev/SKILL.md`, M2 recipe in
`docs/agent-state/plans/2026-09-15-m2-closeout.md` §Task 8 (lines 1135-1200).

**Terminology note (title):** "aquamarine 0.56.2" in the brief is a slip — the Hyprland *package*
is `0.56.2`; **aquamarine is `0.15.0`** (the display/backend library, `pkgconf aquamarine` →
`0.15.0`). The plan fixes this in the artifact to avoid a wrong pin being recorded in COMPAT.md.

## Global Constraints

- Diagnose on the **nested** session only; **never** run candidate Hyprland builds or `plugin load`
  against the **host** session (`WAYLAND_DISPLAY=wayland-1`). Host is the live desktop.
- No `sudo`/`pacman` from the agent; system-level mutations are **human-gated tasks** (marked `HUMAN`).
- No production source edits (`src/`, `CMakeLists.txt`). Diagnosis may add files only under
  `/tmp/mru-nest-diag/` and `docs/agent-state/`.
- Every observation must be captured as a **verbatim log excerpt + exact command**, stored on disk
  (token economy, AGENTS.md §12): nest logs in `/tmp/mru-nest-diag/`, digest in the artifact.
- One artifact: `docs/agent-state/research/2026-09-17-nest-aquamarine-diagnosis.md`.
- Do not claim a fix; the exit deliverable is a **root-cause (or ranked hypotheses) + STATUS + a
  working nest recipe** (or a documented blocker with the minimal upstream repro).
- Record any nest-vs-host inconsistency (renderer chosen, EGL device, explicit sync) even if the
  nest starts — silent degradation is worse than a loud failure here.

---

## File Map

| Path | Role |
|------|------|
| `/tmp/mru-nest-diag/hypr-nest.conf` | Isolated nest config (own cache dir, no host includes) |
| `/tmp/mru-nest-diag/env` | Exported `SIG` / `NEST_WL` / `NEST_CACHE` (source-able scratch) |
| `/tmp/mru-nest-diag/nest-*.log` | Verbatim nest stdout/stderr per attempt |
| `/tmp/mru-nest-diag/report/` | `hyprctl version/chains/monitors -j` dumps |
| `docs/agent-state/research/2026-09-17-nest-aquamarine-diagnosis.md` | **The artifact** (created in Task 6) |
| `docs/COMPAT.md` | Updated **only** if nest works or blocker is recorded (Task 7) |
| `docs/agent-state/SESSION.md` | Unblock line update (Task 7) |

---

## Task 1: Baseline — pin the exact failure and environment facts

**Files:**
- Create: `/tmp/mru-nest-diag/report/00-env.txt`
- Modify: (none in repo)

**Interfaces:**
- Produces: `00-env.txt` with the measured versions/paths every later task cites; the **failure
  signature** string (whatever the session says now).

- [ ] **Step 1: Record tool + library facts**

```bash
mkdir -p /tmp/mru-nest-diag/report
{
  echo "== date =="; date -Is
  echo "== hyprland =="; Hyprland --version
  echo "== pkgs =="; pacman -Q hyprland aquamarine hyprutils hyprlang hyprgraphics hyprcursor 2>&1
  echo "== pkgconf =="; pkgconf --modversion hyprland aquamarine 2>&1
  echo "== libs =="; ls -la /usr/lib/libaquamarine.so* /usr/lib/libhyprutils.so* 2>&1
  echo "== headers =="; ls /usr/include/hyprland/; ls /usr/include/hyprland/src | head -40
  echo "== host session =="; echo "SESSION=$XDG_SESSION_TYPE WL=$WAYLAND_DISPLAY RUNTIME=$XDG_RUNTIME_DIR"
} | tee /tmp/mru-nest-diag/report/00-env.txt
```

- [ ] **Step 2: Record the *current* nest failure verbatim**

Reproduce whatever is currently broken, capture stderr, save it. Use the isolation recipe (do NOT
reuse a stale signature):

```bash
mkdir -p /tmp/mru-nest-diag/cache
set -o pipefail
env -u HYPRLAND_INSTANCE_SIGNATURE -u HYPRLAND_CONFIG \
  XDG_CACHE_HOME=/tmp/mru-nest-diag/cache \
  Hyprland -c /tmp/mru-nest-diag/hypr-nest.conf \
  > /tmp/mru-nest-diag/nest-baseline.log 2>&1 &
echo "pid=$!" | tee -a /tmp/mru-nest-diag/report/00-env.txt
sleep 5
echo "== running? ==" >> /tmp/mru-nest-diag/report/00-env.txt
pgrep -a Hyprland >> /tmp/mru-nest-diag/report/00-env.txt 2>&1
echo "== tail log ==" >> /tmp/mru-nest-diag/report/00-env.txt
tail -40 /tmp/mru-nest-diag/nest-baseline.log >> /tmp/mru-nest-diag/report/00-env.txt
```

(If `hypr-nest.conf` does not exist yet, this fails on a missing config — Task 2 creates it.
Purpose here is to capture the *first* failure string.)

- [ ] **Step 3: Extract the one-line failure signature**

```bash
grep -iE 'error|fail|fatal|abort|segv|egl|glew|gpu|drm|aquamarine|renderer|signal|assert' \
  /tmp/mru-nest-diag/nest-baseline.log | tail -30
```

**Done when:** `00-env.txt` exists and contains a single quoted **failure signature** (or
`NEST STARTS (no failure) — proceed to Task 3`).

---

## Task 2: Isolation config — rule out host config contamination

**Files:**
- Create: `/tmp/mru-nest-diag/hypr-nest.conf`
- Create: `/tmp/mru-nest-diag/env`

**Interfaces:**
- Produces: a minimal nest config referencing **no** host files, and a source-able env file
  exporting `SIG`/`NEST_WL` consumed by Tasks 3-6.

Rationale: Omarchy ships Lua-driven config (`~/.config/hypr/*.lua`). A nest that inherits host
config or `HYPRLAND_CONFIG` can fail on host-only directives — a false "aquamarine broken". This
task forces a hermetic nest.

- [ ] **Step 1: Minimal nest config (no host includes)**

```bash
cat > /tmp/mru-nest-diag/hypr-nest.conf <<'CONF'
monitor = ,1280x800@60,auto,1
misc { disable_hyprland_logo = true
       disable_splash_rendering = true
       vfr = false
       render_ahead_of_time = false }
debug { disable_logs = false }
bind = ALT, Q, killactive
bind = ALT, TAB, mru:cycle, next
bind = ALT SHIFT, TAB, mru:cycle, prev
bindrt = ALT, ALT_L, mru:apply
bind = ALT, Escape, mru:cancel
CONF
cat /tmp/mru-nest-diag/hypr-nest.conf
```

- [ ] **Step 2: Env helper (isolation + discovery)**

```bash
cat > /tmp/mru-nest-diag/env <<'ENV'
export NEST_CACHE=/tmp/mru-nest-diag/cache
export HYPRNESS=/tmp/mru-nest-diag
# SIG = newest instance signature created by OUR nest
export SIG="$(ls -t /run/user/$(id -u)/hypr/ 2>/dev/null | head -1)"
# NEST_WL = newest non-lock wayland socket (wayland-2+ expected)
export NEST_WL="$(ls -t /run/user/$(id -u)/wayland-* 2>/dev/null | grep -v '\.lock' | head -1)"
ENV
```

- [ ] **Step 3: Dry-check the config parses without a full session**

```bash
env -u HYPRLAND_INSTANCE_SIGNATURE -u HYPRLAND_CONFIG XDG_CACHE_HOME=/tmp/mru-nest-diag/cache \
  Hyprland --verify-config -c /tmp/mru-nest-diag/hypr-nest.conf 2>&1 | tee /tmp/mru-nest-diag/report/02-verify.txt || true
```

**Done when:** `hypr-nest.conf` exists, contains no `source=`/host paths, and `02-verify.txt` is
captured (parse OK or a named config error).

---


| Path | Role |
|------|------|
| `/tmp/mru-nest-diag/hypr-nest.conf` | Isolated nest config (own cache dir, no host includes) |
| `/tmp/mru-nest-diag/env` | Exported `SIG` / `NEST_WL` / `NEST_CACHE` (source-able scratch) |
| `/tmp/mru-nest-diag/nest-*.log` | Verbatim nest stdout/stderr per attempt |
| `/tmp/mru-nest-diag/report/` | `hyprctl version/chains/monitors -j` dumps |
| `docs/agent-state/research/2026-09-17-nest-aquamarine-diagnosis.md` | **The artifact** (created in Task 6) |
| `docs/COMPAT.md` | Updated **only** if nest works or blocker is recorded (Task 7) |
| `docs/agent-state/SESSION.md` | Unblock line update (Task 7) |

## Task 3: Bisect the start stack — isolate renderer vs compositor

**Files:**
- Create: `/tmp/mru-nest-diag/nest-{hdmi,headless,soft}.log`
- Modify: `00-env.txt` (append H1..H4 verdicts)

**Interfaces:**
- Consumes: `hypr-nest.conf`, `env` (Task 2).
- Produces: which *backend* (if any) yields a live nest; the discriminating log line.

Each substep toggles exactly **one** variable, waits, checks liveness via
`hyprctl -i "$SIG" version`, then records. Kill the attempt before the next.

- [ ] **Step 1: H1 — normal nested Wayland backend**

```bash
env -u HYPRLAND_INSTANCE_SIGNATURE -u HYPRLAND_CONFIG XDG_CACHE_HOME=/tmp/mru-nest-diag/cache \
  Hyprland -c /tmp/mru-nest-diag/hypr-nest.conf > /tmp/mru-nest-diag/nest-hdmi.log 2>&1 &
sleep 6; source /tmp/mru-nest-diag/env
hyprctl -i "$SIG" version 2>&1 | tee /tmp/mru-nest-diag/report/03-hdmi.txt
pkill -f 'Hyprland -c /tmp/mru-nest-diag/hypr-nest.conf'; sleep 1
```

- [ ] **Step 2: H2 — headless backend (`AQ_HEADLESS=1` / `--headless`)**

Confirm the flag first: `Hyprland --help 2>&1 | grep -i headless`. If absent, drop H2 (note it).

```bash
env -u HYPRLAND_INSTANCE_SIGNATURE -u HYPRLAND_CONFIG AQ_HEADLESS=1 XDG_CACHE_HOME=/tmp/mru-nest-diag/cache \
  Hyprland --headless -c /tmp/mru-nest-diag/hypr-nest.conf > /tmp/mru-nest-diag/nest-headless.log 2>&1 &
sleep 6; source /tmp/mru-nest-diag/env
hyprctl -i "$SIG" version 2>&1 | tee /tmp/mru-nest-diag/report/03-headless.txt
pkill -f 'Hyprland --headless'; sleep 1
```

- [ ] **Step 3: H3 — software/llvmpipe EGL probe (GPU-specific fault?)**

```bash
LIBGL_ALWAYS_SOFTWARE=1 GALLIUM_DRIVER=llvmpipe \
env -u HYPRLAND_INSTANCE_SIGNATURE -u HYPRLAND_CONFIG XDG_CACHE_HOME=/tmp/mru-nest-diag/cache \
  Hyprland -c /tmp/mru-nest-diag/hypr-nest.conf > /tmp/mru-nest-diag/nest-soft.log 2>&1 &
sleep 6; source /tmp/mru-nest-diag/env
hyprctl -i "$SIG" version 2>&1 | tee /tmp/mru-nest-diag/report/03-soft.txt
pkill -f 'Hyprland -c /tmp/mru-nest-diag/hypr-nest.conf'; sleep 1
```

- [ ] **Step 4: Extract the discriminating lines from all three logs**

```bash
for f in /tmp/mru-nest-diag/nest-hdmi.log /tmp/mru-nest-diag/nest-headless.log /tmp/mru-nest-diag/nest-soft.log; do
  echo "### $f"; grep -iE 'aquamarine|backend|renderer|egl|glew|drm|gpu|headless|socket|instance|fatal|abort|assert|signal' "$f" | head -25
done
```

- [ ] **Step 5: Classify**

Append to `00-env.txt` one verdict per hypothesis:

| Hypothesis | Verb | Verdict |
|-----------|------|---------|
| H1 | nested Wayland backend starts | starts / fails: `<line>` |
| H2 | headless backend starts | starts / fails: `<line>` |
| H3 | software EGL path starts | starts / fails: `<line>` |
| H4 | fault is config-parse, not backend | yes/no (Task 2 verify.txt) |

**Done when:** each of H1..H4 has a verdict and at least one log line quoted; **if any backend
starts**, go to Task 4 with that backend; otherwise Task 5 is mandatory.

---
## Task 4: Plugin + checks in a live nest (only if Task 3 produced a nest)

**Files:**
- Create: `/tmp/mru-nest-diag/report/04-dispatchers.txt`
- Consumes: `build/mru-switcher.so` (existing build) — do **not** rebuild here.

**Interfaces:** produces raw outputs that fill COMPAT.md's "nest tested" row in Task 7.

- [ ] **Step 1: Start the winning backend, open pre-load windows**

```bash
source /tmp/mru-nest-diag/env
for a in footA footB footC; do
  env WAYLAND_DISPLAY="$(basename $NEST_WL)" XDG_RUNTIME_DIR=/run/user/$(id -u) \
      setsid foot -a "$a" >/dev/null 2>&1 & sleep 1
done
hyprctl -i "$SIG" clients -j | grep -o '"class": "[^"]*"'
```

- [ ] **Step 2: Load plugin + dispatcher matrix (verbatim → `04-dispatchers.txt`)**

Follow M2 recipe (`2026-09-15-m2-closeout.md` §Task 8 Steps 1-2): `plugin load` →
`getoption plugin:mru-switcher:*` → `mru:apply/cancel/status/cycle` (`next|prev|none|monitor|bogus`)
→ `apply` → `status`. Capture every stdout:

```bash
hyprctl -i "$SIG" plugin load /home/code_warlord/Work/DEV/mru-switcher/build/mru-switcher.so
hyprctl -i "$SIG" plugin list
{ hyprctl -i "$SIG" dispatch mru:apply; hyprctl -i "$SIG" dispatch 'mru:status'
  hyprctl -i "$SIG" dispatch 'mru:cycle next'; hyprctl -i "$SIG" dispatch 'mru:status'
  hyprctl -i "$SIG" dispatch 'mru:cycle monitor'; hyprctl -i "$SIG" dispatch 'mru:cycle bogus'
  hyprctl -i "$SIG" dispatch mru:apply; hyprctl -i "$SIG" dispatch 'mru:status'; } \
  | tee /tmp/mru-nest-diag/report/04-dispatchers.txt
```

- [ ] **Step 3: Invariants (M3-S3 §7 + M2 drop-in)**

- [ ] `activewindow -j` unchanged across two `mru:cycle` (REQ-F-003 / T-F-01)
- [ ] `mru:apply` focuses exactly the selection (T-S-02); `cancel` unchanged (REQ-S-005)
- [ ] M3-S3 §7 items 2-6 (scratchpad shown/hidden, 2-monitor scopes, app byte-exact, focus-less)
- [ ] unload: `plugin unload` → no crash, no pending timer (REQ-H-008)

**Done when:** `04-dispatchers.txt` has the full matrix with actual outputs + pass/fail per invariant.

---

## Task 5: Upstream repro memo (only if Task 3: all backends fail)

**Files:**
- Create: `/tmp/mru-nest-diag/report/05-upstream.md`

**Interfaces:** produces a minimal, shareable repro for a Hyprland/aquamarine issue.

- [ ] **Step 1: Freeze the repro to the smallest command + config**

Record: exact `Hyprland --version`, `pacman -Q hyprland aquamarine`, the **minimal** conf (≤3 lines),
and the verbatim failure tail (≤20 lines). No project code involved — this is a host/env defect.

- [ ] **Step 2: Cross-check upstream (research skill, narrow)**

Search aquamarine/Hyprland issues for the failure signature (headless, nested-on-Wayland, EGL device
selection). Cite only matching reports with links + last-known-working version. Do **not** expand
into general Hyprland research.

- [ ] **Step 3: Rank hypotheses with evidence**

Write H1..Hn each with: symptom line, discriminating test already run (Task 3), upstream match
(yes/no + link), next discriminating command. Mark **actionable by us** vs **await upstream/human**.

**Done when:** `05-upstream.md` has a frozen repro + ranked hypotheses + a "human action needed?" line.

---
## Task 6: Artifact — write the diagnosis report

**Files:**
- Create: `docs/agent-state/research/2026-09-17-nest-aquamarine-diagnosis.md`

**Interfaces:** consumes `/tmp/mru-nest-diag/report/*`; produces the single decision doc.

- [ ] **Step 1: Write the artifact (≤ ~120 lines, structure below)**

```markdown
# Nest diagnosis — Hyprland v0.56.2 / aquamarine 0.15.0 (Omarchy)
Date / Host / Env (from 00-env.txt, incl. the aquamarine 0.15.0 correction)
## Failure signature                (verbatim line + command)
## What was ruled out               (host config, backend X, renderer Y) — 1 line each + evidence
## Root cause / ranked hypotheses   (H1..Hn, evidence, upstream links)
## STATUS                           (working | degraded | broken + one-line why)
## Working recipe                   (exact commands that produce a live nest)
## Impact on COMPAT.md / M3-S3 §7   (which smoke items can now run live)
## Human action needed              (none | sudo/pkg pin | upstream issue URL)
```

- [ ] **Step 2: Token-cheap evidence links**

Reference log paths (`/tmp/mru-nest-diag/...`) with line numbers; paste **at most** the 20-line
failure tail. No full nest logs inside the doc.

- [ ] **Step 3: Self-review against COMPAT.md policy**

Confirm the artifact distinguishes (a) **fail-closed hash** (plugin-side, expected) from (b) **nest
startup** (env-side). Do not conflate — M2 already verified (a) by pinned source.

**Done when:** file exists, has a STATUS line and a working recipe (or a frozen repro + human-action
line).

---

## Task 7: Close-out — state files + optional COMPAT row

**Files:**
- Modify: `docs/agent-state/SESSION.md` (Blocked / Next action)
- Modify: `docs/COMPAT.md` (only if STATUS = working)
- Modify: `docs/agent-state/PROGRESS.md` (only if a nested-smoke checkbox can be ticked)

- [ ] **Step 1: If nest works → fill COMPAT row + run M3-S3 §7**

Add to `docs/COMPAT.md`: same pin row, `nest tested 2026-09-17`, note = backend used + aquamarine
`0.15.0`. Then execute `docs/agent-state/plans/2026-09-17-m3-s3-scope-adapter.md` §7 items 2-8 and
record results where that plan says (REQ-TRACE `T-SC-05 behavior`).

- [ ] **Step 2: If nest broken → record blocker, do NOT fake a pass**

Set SESSION.md `Blocked: nest broken — <signature> (see research/2026-09-17-nest-aquamarine-diagnosis.md)`.
Keep the COMPAT row unchanged (still "verified by pinned source"). Link the upstream issue if Task 5
found a match.

- [ ] **Step 3: Commit state only**

```bash
git add docs/agent-state/research/2026-09-17-nest-aquamarine-diagnosis.md docs/agent-state/SESSION.md
# + docs/COMPAT.md / PROGRESS.md only if Step 1 applied
git commit -m "docs(state): nest aquamarine diagnosis — <STATUS> (no src changes)"
```

**Done when:** SESSION.md reflects reality (blocked or unblocked), and exactly the files justified by
STATUS are committed. Branch: `docs/nest-aquamarine-diagnosis` (docs-only, no `src/`).

---

## Self-Review

**Coverage vs brief:** diagnosis of nest env on this host → Tasks 1-3 (env + bisect), Task 4 (plugin
in live nest, if possible), Task 5 (upstream repro when not), Task 6 (artifact), Task 7 (state +
optional COMPAT). ✅

**Failure / placeholder scan:** every step has a concrete command or a verbatim-output requirement;
no "TBD". The Task 4 vs Task 5 branch is an explicit "only if" gate, not a placeholder.

**Name consistency:** `hypr-nest.conf`, `env` (`SIG`,`NEST_WL`,`NEST_CACHE`),
`/tmp/mru-nest-diag/report/*`, and the artifact filename are used identically across tasks. Backend
flags (`AQ_HEADLESS=1` / `--headless`) are declared once (Task 3) and consumed in Task 4.

**Risks flagged to the reviewer:** (1) `AQ_HEADLESS`/`--headless` spelling must be confirmed via
`Hyprland --help` before Task 3 — if absent, drop H2 and note it; (2) Omarchy's Lua config may set
`source=`/`env=` in a system conf the nest still inherits via `HYPRLAND_CONFIG` — Task 2 unsets it;
(3) never touch the host session (`wayland-1`); (4) the brief's "aquamarine 0.56.2" is corrected to
`0.15.0` — review before recording a pin.
---