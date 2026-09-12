# Agent Skills — MRU Switcher / Hyprland

Format — [Agent Skills](https://agentskills.io) / opencode-compatible  
Layout — `.agents/skills/<name>/SKILL.md` (+ optional `README.md`, `references/`)

## Project skills (this repo)

| Skill | Level | Purpose |
|-------|-------|---------|
| [hyprland-plugin](hyprland-plugin/) | Expert | Native Hyprland `.so` API, Event::bus, dispatchers, config, hooks |
| [mru-switcher](mru-switcher/) | Expert | Project contracts — snapshot, apply-on-release, scopes, ROADMAP |
| [hyprland-focus-mru](hyprland-focus-mru/) | Expert | Focus history, debounce, lock-in, virtual selection |
| [cpp-plugin-architecture](cpp-plugin-architecture/) | Expert | Ports/adapters, pure domain, FocusGateway, tests |
| [hyprland-nested-dev](hyprland-nested-dev/) | Advanced | Nested session, load/reload, smoke checklist |
| [plugin-spec-compliance](plugin-spec-compliance/) | Expert | SPEC/ADR review, requirement IDs, merge gates |

## Suggested activation matrix

| Task | Skills |
|------|--------|
| Scaffold plugin | `hyprland-plugin` + `cpp-plugin-architecture` |
| Implement session/MRU | `mru-switcher` + `hyprland-focus-mru` |
| PR review | `plugin-spec-compliance` + `mru-switcher` |
| Dev environment | `hyprland-nested-dev` |
| Host API question | `hyprland-plugin` |

## Complementary skills from the ecosystem (skills.sh)

Install separately if your agent supports the skills CLI. These are **not** vendored here; they pair well with this project:

| Skill (ecosystem) | Why it helps |
|-------------------|--------------|
| **implement** (mattpocock/skills) | Turn SPEC into incremental implementation plans |
| **to-spec** / **to-tickets** | Break ROADMAP milestones into tickets |
| **code-review** | General review discipline alongside `plugin-spec-compliance` |
| **domain-modeling** | Refine WindowRef / Session aggregates |
| **codebase-design** | Repo layout decisions |
| **diagnosing-bugs** | Crash/load failure analysis |
| **research** | Upstream Hyprland API churn investigation |
| **grill-me** / **grilling** | Stress-test design before coding |

Example discovery:

```bash
npx skills update
# browse https://www.skills.sh/ for implement, code-review, domain-modeling, etc.
```

## Conventions

- `name` in frontmatter **equals** directory name
- `description` includes **what** and **when** (triggers); avoid colon-space in YAML descriptions
- Keep `SKILL.md` focused; put long refs under `references/` when needed
- Level tags in `metadata.level` — `expert` | `advanced`

## Reading order with project docs

1. `docs/HYPRLAND-PLUGIN-SYSTEM.md` + skill `hyprland-plugin`
2. `docs/ARCHITECTURE.md` + `cpp-plugin-architecture`
3. `docs/SPEC.md` + `mru-switcher` + `plugin-spec-compliance`
4. `docs/ROADMAP.md` + ecosystem `implement` / `to-tickets`
