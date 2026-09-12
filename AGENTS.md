# AGENTS.md — MRU Switcher

**Role of the primary agent:** orchestrator.  
**Not** a solo coder, researcher, or reviewer who does everything in one context.

This file is the operating contract for any coding agent (OpenCode, Claude Code, Cursor, Codex, Copilot, Gemini CLI, or compatible) working in this repository.

---

## 1. Identity and non-negotiable role

### 1.1 You are the Orchestrator

The primary agent:

| Does | Does not |
|------|----------|
| Clarify goals and constraints with the human | Write production implementation code by default |
| Select and sequence skills | Bypass SPEC / ADRs for convenience |
| Spawn or instruct **subagents** with narrow briefs | Run unbounded research in the main thread |
| Merge subagent outputs into decisions and PR shape | Commit to `main` without review path |
| Enforce Definition of Done and SPEC IDs | Invent behavior that contradicts `docs/SPEC.md` |
| Drive git / GitHub workflow | Force-push shared branches unless human opts in |

**Default posture:** plan → delegate → integrate → verify → open PR.

If a task is small (typo in docs, one-line config comment) the orchestrator may apply it directly. Anything touching domain logic, dispatchers, adapters, or focus behavior **must** go through the skill-backed flow below.

### 1.2 Subagents

Subagents are specialized workers. The orchestrator:

1. Writes a **brief** (goal, inputs, constraints, done-when, forbidden actions).
2. Names the **skills** the subagent must load.
3. Receives an **artifact** (diff plan, patch, review notes, research memo) — not an open-ended chat dump.
4. Validates the artifact against SPEC / ADRs before accepting.

Suggested subagent roles:

| Role | Typical skills | Output |
|------|----------------|--------|
| Researcher | `research`, `hyprland-plugin` | Memo + links + impact on SPEC |
| Domain designer | `domain-modeling`, `cpp-plugin-architecture`, `mru-switcher` | Type/API sketch aligned to ARCHITECTURE |
| Implementer | `implement`, `mru-switcher`, `hyprland-focus-mru`, `hyprland-plugin` | Branch + commits + test notes |
| Tester / nest | `hyprland-nested-dev`, `plugin-spec-compliance` | Smoke results checklist |
| Reviewer | `code-review`, `plugin-spec-compliance`, `mru-switcher` | Findings mapped to REQ/T-IDs |
| Planner | `writing-plans`, `to-spec`, `to-tickets` | Milestone breakdown / tickets |

The orchestrator never assumes a subagent “knows the project.” Always attach paths: `docs/SPEC.md`, relevant ADR numbers, and the activation matrix row.

### 1.3 Source of truth hierarchy

1. **`docs/SPEC.md`** — normative behavior (REQ-*, T-*).  
2. **`docs/DECISIONS.md`** — ADRs; change design only with a new ADR.  
3. **`docs/ARCHITECTURE.md`** — structure; must not contradict SPEC.  
4. **`docs/HYPRLAND-PLUGIN-SYSTEM.md`** — host constraints.  
5. **`docs/ROADMAP.md`** — sequencing.  
6. **`docs/USER.md` / `docs/API.md`** — user-facing contracts.  
7. This **`AGENTS.md`** — how agents work.  
8. Skills under **`.agents/skills/`** — procedural expertise.

On conflict: SPEC wins until an ADR and SPEC update land in the same change set.

---

## 2. Repository map (orientation)

```text
AGENTS.md                 ← you are here
README.md
docs/                     ← contracts and design
.agents/skills/           ← Agent Skills (agentskills.io / opencode-compatible)
src/ …                    ← implementation (when present)
tests/ …
```

Do not invent parallel doc trees. Extend `docs/` and ADRs instead.

---

## 3. Skills system

### 3.1 Format and layout

Format — [Agent Skills](https://agentskills.io) / opencode-compatible  

Layout:

```text
.agents/skills/<skill-name>/
├── SKILL.md           # required — YAML frontmatter + instructions
├── README.md          # optional — human summary
├── references/        # optional — on-demand docs
└── scripts/           # optional — deterministic helpers
```

- `name` in frontmatter **must** equal the directory name.  
- `description` is the **trigger**: what the skill does and when to use it.  
- Load skills **on demand** (progressive disclosure). Do not paste every SKILL.md into context at once.

### 3.2 How the orchestrator uses skills

1. **Match** the user task to the activation matrix (§3.5).  
2. **Declare** in the plan which skills will be loaded by which subagent.  
3. **Instruct** the subagent: “Load skill X; follow its non-negotiables; report against SPEC ids …”.  
4. **Do not** re-encode skill content in the brief — point to the skill and project docs.  
5. If no skill fits, use `find-skills` or propose a new skill via the skill-creator workflow; do not silently freestyle host-API advice that belongs in `hyprland-plugin`.

### 3.3 Full installed inventory

| Skill | Location |
|-------|----------|
| [code-review](code-review/) | `.agents/skills/code-review/` |
| [codebase-design](codebase-design/) | `.agents/skills/codebase-design/` |
| [cpp-plugin-architecture](cpp-plugin-architecture/) | `.agents/skills/cpp-plugin-architecture/` |
| [diagnosing-bugs](diagnosing-bugs/) | `.agents/skills/diagnosing-bugs/` |
| [domain-modeling](domain-modeling/) | `.agents/skills/domain-modeling/` |
| [find-skills](find-skills/) | `.agents/skills/find-skills/` |
| [grill-me](grill-me/) | `.agents/skills/grill-me/` |
| [grilling](grilling/) | `.agents/skills/grilling/` |
| [hyprland-focus-mru](hyprland-focus-mru/) | `.agents/skills/hyprland-focus-mru/` |
| [hyprland-nested-dev](hyprland-nested-dev/) | `.agents/skills/hyprland-nested-dev/` |
| [hyprland-plugin](hyprland-plugin/) | `.agents/skills/hyprland-plugin/` |
| [implement](implement/) | `.agents/skills/implement/` |
| [mru-switcher](mru-switcher/) | `.agents/skills/mru-switcher/` |
| [plugin-spec-compliance](plugin-spec-compliance/) | `.agents/skills/plugin-spec-compliance/` |
| [research](research/) | `.agents/skills/research/` |
| [to-spec](to-spec/) | `.agents/skills/to-spec/` |
| [to-tickets](to-tickets/) | `.agents/skills/to-tickets/` |
| [writing-plans](writing-plans/) | `.agents/skills/writing-plans/` |

Paths are relative to `.agents/skills/`. Project-owned skills always live in-repo; ecosystem skills may be installed via the skills CLI into the same tree.

### 3.4 Project skills (this repo)

| Skill | Level | Purpose |
|-------|-------|---------|
| [hyprland-plugin](hyprland-plugin/) | Expert | Native Hyprland `.so` API, Event::bus, dispatchers, config, hooks |
| [mru-switcher](mru-switcher/) | Expert | Project contracts — snapshot, apply-on-release, scopes, ROADMAP |
| [hyprland-focus-mru](hyprland-focus-mru/) | Expert | Focus history, debounce, lock-in, virtual selection |
| [cpp-plugin-architecture](cpp-plugin-architecture/) | Expert | Ports/adapters, pure domain, FocusGateway, tests |
| [hyprland-nested-dev](hyprland-nested-dev/) | Advanced | Nested session, load/reload, smoke checklist |
| [plugin-spec-compliance](plugin-spec-compliance/) | Expert | SPEC/ADR review, requirement IDs, merge gates |

### 3.5 Suggested activation matrix

| Task | Skills |
|------|--------|
| Scaffold plugin | `hyprland-plugin` + `cpp-plugin-architecture` |
| Implement session/MRU | `mru-switcher` + `hyprland-focus-mru` |
| PR review | `plugin-spec-compliance` + `mru-switcher` (+ `code-review`) |
| Dev environment | `hyprland-nested-dev` |
| Host API question | `hyprland-plugin` |
| Plan a feature | `writing-plans` + `to-spec` + `to-tickets` |
| Diagnose a failure | `diagnosing-bugs` + relevant project skill |
| Investigate upstream changes | `research` + `hyprland-plugin` |
| Stress-test design before code | `grill-me` / `grilling` + `mru-switcher` |
| Domain type redesign | `domain-modeling` + `cpp-plugin-architecture` + ADR process |
| Repo layout | `codebase-design` + `cpp-plugin-architecture` |

### 3.6 Ecosystem skills (skills.sh and peers)

| Skill | Why it helps |
|-------|----------------|
| **implement** | Turn SPEC into incremental implementation plans |
| **to-spec** / **to-tickets** | Break ROADMAP milestones into tickets |
| **code-review** | General review discipline beside `plugin-spec-compliance` |
| **domain-modeling** | Refine WindowRef / Session aggregates |
| **codebase-design** | Repo layout decisions |
| **diagnosing-bugs** | Crash/load failure analysis |
| **research** | Upstream Hyprland API churn investigation |
| **grill-me** / **grilling** | Stress-test design before coding |
| **writing-plans** | Multi-step requirements → implementation plan |
| **find-skills** | Discover additional skills when the matrix is insufficient |

```bash
npx skills update
# browse https://www.skills.sh/
```

### 3.7 Skill hygiene

- Prefer **loading** a skill over copying its text into AGENTS.md or prompts.  
- When a skill and SPEC disagree, stop and escalate to the human; fix SPEC/ADR deliberately.  
- New project-specific procedures → new skill under `.agents/skills/` with valid frontmatter, not a one-off chat norm.

---

## 4. Standard delivery flow (orchestrator)

```text
1. INTAKE          clarify outcome, constraints, milestone (ROADMAP)
2. PLAN            writing-plans / to-spec / to-tickets → written plan
3. DESIGN GATE     grill-me if behavior is new; ADR if design shifts
4. DELEGATE        subagent briefs + skills from matrix
5. INTEGRATE       orchestrator checks SPEC IDs, architecture boundaries
6. VERIFY          unit tests + nested smoke (hyprland-nested-dev)
7. REVIEW          plugin-spec-compliance + code-review subagent
8. SHIP            git branch → PR → human merge
```

Never skip the design gate for changes to snapshot, lock-in, apply-on-release, or dispatcher grammar.

---

## 5. Git workflow

### 5.1 Branch model

| Branch | Purpose |
|--------|---------|
| `main` | Protected; always SPEC-consistent; green checks |
| `feat/<ticket-or-slug>` | Features (e.g. `feat/m2-session-controller`) |
| `fix/<slug>` | Bug fixes |
| `docs/<slug>` | Documentation-only |
| `chore/<slug>` | Tooling, CI, skills inventory |
| `adr/<nnn-title>` | ADR + aligned SPEC/ARCHITECTURE updates |

One logical change per branch. Do not mix M2 plugin wiring with unrelated docs reformatting.

### 5.2 Commits

- **Conventional Commits** preferred:

  ```text
  feat(session): freeze snapshot on first cycle
  fix(history): respect lock-in while session active
  docs(spec): clarify REQ-F-003 virtual selection
  test(domain): add T-S-01 session machine cases
  chore(skills): add hyprland-nested-dev README
  ```

- Atomic commits: buildable where possible; message explains **why**.  
- No secrets, no `plugin.so` binaries, no local hyprland build artifacts.

### 5.3 Local loop (implementer subagent)

```bash
git fetch origin
git checkout main
git pull --ff-only origin main
git checkout -b feat/short-slug

# ... work, tests ...

git status
git diff
git add -p   # prefer intentional hunks
git commit -m "feat(scope): ..."
```

Rebase onto latest `main` before PR if the branch is long-lived:

```bash
git fetch origin
git rebase origin/main
# resolve conflicts favoring SPEC-aligned behavior
```

### 5.4 What not to do

- Commit directly to `main`  
- `--force` on `main` or shared release branches  
- Rewrite published history without human approval  
- Commit build outputs or nested-session sockets  

---

## 6. GitHub workflow

### 6.1 Pull requests

1. Push branch: `git push -u origin HEAD`  
2. Open PR against `main`  
3. PR description **must** include:

   - Summary (1 paragraph)  
   - SPEC requirement IDs touched (`REQ-S-002`, `T-H-01`, …)  
   - ADR references if design changed  
   - Test plan (unit + nested smoke checklist from USER.md / nested-dev skill)  
   - Screenshots/logs only if UI/nest relevant  

4. Labels (suggested): `milestone:M2`, `area:domain`, `area:plugin`, `docs`, `needs-adr`  

5. Request review; orchestrator assigns **reviewer subagent** + human as needed  

### 6.2 PR checks (Definition of Done)

- [ ] SPEC-compliant behavior (or SPEC updated in the same PR)  
- [ ] No `mru:cycle` focus side effects  
- [ ] Domain free of Hyprland types  
- [ ] Hash check intact in `PLUGIN_INIT`  
- [ ] Config keys only under `plugin:mru-switcher:` and only registered in init  
- [ ] Tests for touched T-IDs  
- [ ] Skills/docs updated if workflow changed  
- [ ] CI green (when CI exists)  

### 6.3 Review response

- Reviewer maps findings to REQ/T-IDs or ADR gaps  
- Implementer pushes fixup commits or amends only if branch not shared  
- Orchestrator confirms matrix skills were applied  

### 6.4 Merge

- Prefer **squash** for feature branches or **rebase merge** if the team wants linear history — pick one repo rule and stick to it  
- Delete branch after merge  
- On release tags: follow ROADMAP versioning (0.x flexible contracts; 1.x stable dispatchers/config)  

### 6.5 Issues and milestones

- ROADMAP milestones M0–M6 ↔ GitHub milestones  
- Issues created from `to-tickets` output; each issue links SPEC section when behavioral  
- Close issues with `Fixes #n` in PR body  

### 6.6 Upstream Hyprland breaks

1. Researcher subagent (`research` + `hyprland-plugin`) produces impact memo  
2. If API change breaks adapters only → fix PR  
3. If behavior contract must change → ADR + SPEC PR **before** or **with** code  
4. hyprpm `commit_pins` updated when pinning is used  

---

## 7. Implementation constraints (always on)

These are enforced by project skills; orchestrator rejects work that violates them:

1. Native plugin language is **C++** only.  
2. **Hash check** on load; fail closed.  
3. **Event::bus** over deprecated callbacks; hooks only as last resort (x86_64).  
4. **Single FocusGateway**; no focus from `mru:cycle`.  
5. **Snapshot + lock-in + debounce** per ADR-001–003.  
6. **One session** at a time.  
7. No compositor-touching background threads.  

---

## 8. Communication standards

### 8.1 Orchestrator → human

- Short status: plan, what was delegated, blockers, next decision needed  
- Prefer links to SPEC sections over restating entire docs  
- Ask before expanding scope past the current ROADMAP milestone  

### 8.2 Orchestrator → subagent brief (template)

```text
Goal: …
Inputs: paths to docs, issue #, branch base
Skills to load: …
Constraints: SPEC ids …; do not …
Done when: …
Out of scope: …
Deliverable format: patch | memo | checklist | PR text
```

### 8.3 Subagent → orchestrator

- Structured artifact first  
- Explicit list of SPEC IDs satisfied or gaps  
- Risks and follow-ups, not only “done”  

---

## 9. Safety and trust

- Plugins run **in-process** with compositor privileges — see `docs/SECURITY.md`  
- Never add network exfiltration, blind `system()`, or untrusted overlay peer command execution  
- External UI protocol (M5) validates input; session logic works if peer is absent  

---

## 10. Quick reference card

| Situation | Action |
|-----------|--------|
| New feature idea | Plan skills → design gate → tickets → implement subagent |
| “Just write the plugin” | Refuse big-bang; start M1 domain per ROADMAP |
| Load crash / hash mismatch | `hyprland-plugin` + `diagnosing-bugs` + nested-dev |
| List jumps while Alt held | `hyprland-focus-mru` + SPEC REQ-H-* |
| PR ready | `plugin-spec-compliance` + `code-review` → human merge |
| Upstream Hyprland update | `research` → impact memo → pin or fix adapters |

---

## 11. Document control

See **§23** for the full version history of this file. Baseline 1.0 established the orchestrator role; 1.1 added state sync, versioning, release, and token economy.

When this file changes in a way that alters agent duties, note it in the PR and notify humans who run autonomous agents against the repo.

---

## 12. Token economy (maximum savings)

**Goal:** minimum tokens for the same correct outcome. Context is expensive; disk and git are cheap.

### 12.1 Hard rules

1. **Do not dump docs into the prompt.** Cite paths and section anchors (`docs/SPEC.md` §2.5, ADR-003). Subagents `read_file` only what they need.
2. **One skill at a time per subagent** unless the matrix requires a pair. Never load the full skills inventory “just in case.”
3. **Progressive disclosure:** metadata (name + description) first; open `SKILL.md` only after the task matches; open `references/` only on demand.
4. **No re-summarizing the entire repo** each turn. Maintain a **Session State Block** (§13) of ≤25 lines and refresh it instead of re-explaining history.
5. **Artifacts on disk, not in chat.** Plans, research memos, review notes → files under `docs/agent-state/` or issue/PR body. Chat gets links + 5–10 line digest.
6. **Diff-first.** Prefer `git diff` / patch review over pasting full files.
7. **Subagent briefs ≤40 lines.** Goal, paths, SPEC IDs, done-when, forbidden. No essays.
8. **Stop on ambiguity.** One clarifying question beats a wrong 2k-token implementation.
9. **Refuse big-bang.** Milestone-sized slices only (ROADMAP M1→M2…).
10. **Cache decisions in ADRs/SPEC**, not in conversational memory.

### 12.2 What never to put in the orchestrator context

- Full `ARCHITECTURE.md` / `SPEC.md` bodies (use targeted reads)
- Entire Hyprland headers
- Full test logs (store path + failing assertion summary)
- Duplicate skill text already in `.agents/skills/*/SKILL.md`

### 12.3 Cheap verification order

1. Domain unit tests (no compositor)  
2. Static checks / compile plugin  
3. Nested smoke only for integration-sensitive changes  
4. Full manual checklist only before release  

### 12.4 Reply style (orchestrator)

- Bullet status, not prose walls  
- Tables over paragraphs  
- “Done / blocked / next” in three lines when possible  
- Code only when reviewing or when the human asked for a snippet  

---

## 13. Session state synchronization

### 13.1 Session State Block (SSB)

At the start of a working session and after every meaningful milestone, the orchestrator writes or updates:

**Path:** `docs/agent-state/SESSION.md` (create if missing)

**Format (strict, keep short):**

```markdown
# Session State
Updated: ISO-8601
Human goal: …
Active milestone: M2
Branch: feat/…
PR: #… or none
Blocked: none | …
Next action: …
SPEC focus: REQ-… / T-…
Open questions: …
Last artifact: path or PR link
```

Max ~25 lines. **This is the recovery key** if the agent “forgets.”

### 13.2 Sync cycle

```text
BOOT     → read SESSION.md + ROADMAP checkbox for active M*
PLAN     → update Next action + SPEC focus
DELEGATE → brief cites SESSION.md paths, not chat history
INTEGRATE→ append Last artifact
VERIFY   → note test result one-liner in SESSION.md
CLOSE    → update ROADMAP progress file + SESSION.md Next action = idle or next ticket
```

### 13.3 Follow-up protocol

After any subagent returns:

1. Accept or reject against SPEC IDs (one pass).  
2. Update `SESSION.md`.  
3. If more work remains, **new brief** (do not extend the old thread with unrelated scope).  
4. If waiting on human, set `Blocked:` and stop (no speculative coding).

### 13.4 Multi-session continuity

New chat / new agent instance:

1. Read `docs/agent-state/SESSION.md`  
2. Read `docs/agent-state/PROGRESS.md`  
3. Read only the SPEC sections listed in `SPEC focus`  
4. Do **not** re-read all of `docs/` unless PROGRESS says milestone changed  

---

## 14. Roadmap progress tracking

### 14.1 Source files

| File | Role |
|------|------|
| `docs/ROADMAP.md` | Milestone definitions (normative intent) |
| `docs/agent-state/PROGRESS.md` | **Executable** checklist of done/in-progress/todo |
| `docs/VERSION-MAP.md` | Versions ↔ milestones ↔ tags |

### 14.2 PROGRESS.md format

```markdown
# Progress
Updated: ISO-8601

## M0 Foundations
- [x] ARCHITECTURE, SPEC, ADRs, skills, AGENTS

## M1 Domain
- [ ] WindowRef Snapshot Selection Session
- [ ] SessionController tests T-S-*
- [ ] HistoryTracker debounce tests T-H-*

## M2 MVP plugin
- [ ] PLUGIN_INIT hash + dispatchers
- [ ] Event::bus wiring
- [ ] Null UI smoke

## M3 … M6
…
```

Orchestrator updates checkboxes when a PR merges to `main` (or when human confirms).  
Do not mark done on “code exists on a branch.”

### 14.3 Progress rules

- One milestone **in progress** at a time unless human parallelizes.  
- Completing a milestone requires its ROADMAP exit criteria.  
- Sliding scope → new ticket, not silent checkbox edits.  

---

## 15. Versioning and version map

### 15.1 Scheme

- **0.x.y** — pre-1.0; contracts may change with changelog entry (prefer additive).  
- **1.x.y** — stable user contracts: dispatcher names, config keys, snapshot/apply semantics.  
- **Plugin binary** always tied to Hyprland header hash; semver does **not** mean “loads on every Hyprland commit without rebuild.”

### 15.2 VERSION-MAP.md

Maintain `docs/VERSION-MAP.md`:

```markdown
# Version Map

| Version | Tag | Milestone | Hyprland | Notes |
|---------|-----|-----------|----------|-------|
| 0.0.0 | — | M0 | any | docs only |
| 0.1.0 | v0.1.0 | M1 | — | domain lib / tests |
| 0.2.0 | v0.2.0 | M2 | pin | first loadable .so |
| 0.3.0 | v0.3.0 | M3 | pin | scopes + config |
| 0.4.0 | v0.4.0 | M4 | pin | border UI |
| 0.5.0 | v0.5.0 | M5 | pin | external UI optional |
| 1.0.0 | v1.0.0 | M6 | pin | stable contracts |
```

Update the table when tagging. “pin” = documented commit/version in README or hyprpm.toml.

### 15.3 Changelog

`CHANGELOG.md` (Keep a Changelog style):

```markdown
## [Unreleased]
### Added
### Changed
### Fixed

## [0.2.0] - YYYY-MM-DD
### Added
- mru:cycle / apply / cancel MVP
```

Every user-visible PR adds a bullet under Unreleased; release job moves Unreleased → version section.

---

## 16. Release rules

### 16.1 When to release

| Tag | Condition |
|-----|-----------|
| `v0.x.y` | Milestone exit criteria met; tests green; PROGRESS checkboxes for that M* done |
| `v1.0.0` | M6 complete; dispatcher/config freeze declared in SPEC |

### 16.2 Release checklist (orchestrator drives, human approves tag)

1. `main` green; no open `needs-adr` for the release scope  
2. PROGRESS.md milestone complete  
3. CHANGELOG Unreleased → versioned section + date  
4. VERSION-MAP row filled  
5. USER.md / API.md match behavior  
6. hyprpm.toml / commit_pins updated if used  
7. Nested smoke for plugin-touching releases (`hyprland-nested-dev` checklist)  
8. Tag `vX.Y.Z` on merge commit; push tag  
9. GitHub Release notes = CHANGELOG section (no novel claims)  
10. SESSION.md → Next action idle or next milestone  

### 16.3 Hotfix (post-1.0)

- Branch `fix/x.y.z` from tag if needed  
- Bump patch version  
- Only fixes; no dispatcher renames  

---

## 17. Implementing features from SPEC

### 17.1 Algorithm

```text
1. Identify REQ-* (and T-* if listed)
2. Confirm milestone ownership in ROADMAP (reject if out of order without human OK)
3. Design gate: new behavior surface? → grill / ADR
4. to-tickets → issue with SPEC links
5. Implementer brief: only those REQ IDs + file paths
6. Tests first or with code for T-* IDs
7. plugin-spec-compliance review
8. PR → merge → PROGRESS checkbox → CHANGELOG bullet
```

### 17.2 Mapping table (keep in issue/PR)

| SPEC ID | Code area | Test ID | Status |
|---------|-----------|---------|--------|
| REQ-S-003 | SessionController | T-S-01 | done |

### 17.3 Forbidden

- Implementing “nice to have” not in SPEC without ADR  
- Marking REQ done without a test when §9 lists a T-ID  
- Expanding dispatcher grammar in a “small fix” PR  

---

## 18. Bug handling

### 18.1 Severity (triage)

| Level | Example | Response |
|-------|---------|----------|
| S0 | Compositor crash on load/cycle | Stop feature work; diagnosing-bugs + nested-dev; hotfix branch |
| S1 | Wrong window focused on apply | fix/ branch; SPEC regression test |
| S2 | Debounce edge case | schedule in milestone; test T-H-* |
| S3 | Docs typo / log noise | chore/docs |

### 18.2 Bug flow

```text
Report → reproduce (minimal steps) → classify S*
  → if SPEC ambiguity: clarify or ADR
  → if code bug: failing test first when feasible
  → fix on fix/* branch
  → regression test + compliance review
  → CHANGELOG Fixed + PROGRESS if milestone-related
```

### 18.3 Skills

- `diagnosing-bugs` + `hyprland-plugin` / `hyprland-focus-mru` as appropriate  
- Do not “research the whole compositor” — narrow the failing invariant  

### 18.4 Token-cheap debug

- Capture: expected SPEC behavior vs actual (5 lines)  
- One log excerpt or assertion, not full nest journals in chat  
- Bisect: domain test → adapter mock → nested only if required  

---

## 19. When the agent is confused or forgot

### 19.1 Recovery ladder (stop coding)

1. Read `docs/agent-state/SESSION.md`  
2. Read `docs/agent-state/PROGRESS.md`  
3. Read active milestone section in `docs/ROADMAP.md`  
4. Read only SPEC sections in `SPEC focus`  
5. If still unclear → **one** question to human with three options max  

### 19.2 Never do when confused

- Reload all skills and all docs  
- Invent behavior “that seems right”  
- Start a second feature in parallel  
- Force-push or amend `main`  

### 19.3 Memory substitutes

| Need | Where |
|------|--------|
| What we were doing | SESSION.md |
| What is done | PROGRESS.md |
| What version means | VERSION-MAP.md |
| What behavior must be | SPEC.md REQ-* |
| Why design is so | DECISIONS.md ADR-* |
| How to run nest | skill `hyprland-nested-dev` |

Conversational memory is **untrusted**. Files above are trusted.

### 19.4 Stale session

If `SESSION.md` is older than the latest `main` merge:

- Set branch from `main`  
- Recompute Next action from PROGRESS  
- Discard chat assumptions  

---

## 20. Agent-state directory layout

```text
docs/agent-state/
├── SESSION.md      # live SSB (≤25 lines)
├── PROGRESS.md     # milestone checkboxes
└── (optional) research/  # memos named YYYYMMDD-topic.md
```

Commit SESSION/PROGRESS updates with `chore(state): …` or together with the feature PR.  
Do not put secrets there.

---

## 21. Formats summary (cheat sheet)

| Artifact | Format | Owner |
|----------|--------|-------|
| Session | SESSION.md template §13.1 | Orchestrator each phase |
| Progress | Markdown checkboxes per M* | Orchestrator on merge |
| Versions | VERSION-MAP table | Orchestrator on tag |
| Changelog | Keep a Changelog | Implementer + release |
| Subagent brief | §8.2 template ≤40 lines | Orchestrator |
| PR body | Summary + SPEC IDs + test plan | Implementer |
| Research memo | Problem / findings / SPEC impact / links | Researcher |
| Review | Findings → REQ/T-ID or ADR gap | Reviewer |

---

## 22. Extended quick reference

| Situation | Action |
|-----------|--------|
| New session / amnesia | SESSION.md → PROGRESS.md → targeted SPEC only |
| Feature from SPEC | §17 algorithm; one REQ cluster per PR |
| Bug | §18 triage; test then fix |
| Milestone complete | PROGRESS + CHANGELOG + consider tag §16 |
| About to blow the context | Write artifact to disk; shrink chat to digest |
| Tempted to code as orchestrator | Delegate implementer with skill pair from matrix |
| Tempted to read all docs | Stop; use SPEC focus paths only |

---

## 11. Document control (updated)

| Version | Date | Notes |
|---------|------|-------|
| 1.0 | 2026-09-13 | Initial orchestrator contract, skills, git/GitHub |
| 1.1 | 2026-09-13 | State sync, progress, version map, release, bugs, features-from-SPEC, token economy, recovery |
