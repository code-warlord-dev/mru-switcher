# Agent Skills — MRU Switcher / Hyprland

Format — [Agent Skills](https://agentskills.io) / opencode-compatible  
Layout — `.agents/skills/<name>/SKILL.md` (+ optional `README.md`, `references/`)

Canonical orchestrator rules: **[../../AGENTS.md](../../AGENTS.md)**.  
This file is the **skills inventory**; keep it in sync with directories under `.agents/skills/`.

---

## Full installed inventory (in-tree)

All of the following are present in this repository (project-owned **and** ecosystem skills vendored for offline agents).

| Skill | Path | Brief description | When / what to use it for |
|-------|------|------------------|---------------------------|
| [code-review](code-review/) | `.agents/skills/code-review/` | Reviews changes against repository standards and the originating specification. | Use for branch, PR, or work-in-progress reviews; report bugs, regressions, and missing tests. |
| [codebase-design](codebase-design/) | `.agents/skills/codebase-design/` | Helps design deep modules, clear interfaces, and maintainable code boundaries. | Use when improving module interfaces, deciding ownership, or finding opportunities to simplify a design. |
| [cpp-plugin-architecture](cpp-plugin-architecture/) | `.agents/skills/cpp-plugin-architecture/` | Defines ports-and-adapters architecture for testable C++ compositor plugins. | Use when separating domain logic from Hyprland types, designing gateways, or structuring plugin adapters. |
| [diagnosing-bugs](diagnosing-bugs/) | `.agents/skills/diagnosing-bugs/` | Provides a focused diagnosis loop for bugs, failures, and performance regressions. | Use when behavior is broken, crashing, failing tests, or becoming slow. |
| [domain-modeling](domain-modeling/) | `.agents/skills/domain-modeling/` | Sharpens domain vocabulary, aggregates, invariants, and architectural decisions. | Use when defining domain types, clarifying terminology, or writing CONTEXT/ADR material. |
| [find-skills](find-skills/) | `.agents/skills/find-skills/` | Helps discover and install an existing skill for a task. | Use when the required workflow or expertise is not covered by the current skill set. |
| [grill-me](grill-me/) | `.agents/skills/grill-me/` | Stress-tests a plan or decision through structured questions. | Use before committing to a design when hidden assumptions or trade-offs need exposure. |
| [grilling](grilling/) | `.agents/skills/grilling/` | Performs rigorous challenge of a proposed plan, idea, or decision. | Use when explicitly asking for adversarial review or a thorough design stress test. |
| [hyprland-focus-mru](hyprland-focus-mru/) | `.agents/skills/hyprland-focus-mru/` | Covers Hyprland focus history, MRU ordering, lock-in, debounce, and virtual selection. | Use when implementing or debugging recent-window order, Alt-Tab behavior, or focus pollution. |
| [hyprland-lua](hyprland-lua/) | `.agents/skills/hyprland-lua/` | Expert knowledge of Hyprland's Lua configuration API (hyprland.lua, hl.config, hl.bind, hl.on, hl.dsp, window/workspace rules). | Use for any Hyprland Lua config task, editing files under `~/.config/hypr/`, or `hyprctl dispatch` commands on Hyprland 0.55+. |
| [hyprland-lua-config](hyprland-lua-config/) | `.agents/skills/hyprland-lua-config/` | Expert Hyprland Lua configuration, keybinds, hyprctl, hyprpm plugin management, and Omarchy integration. | Use when writing or debugging Lua configs, installing plugins via hyprpm, adapting for Omarchy, or handling modifier-release binds and Lua API quirks. |
| [hyprland-nested-dev](hyprland-nested-dev/) | `.agents/skills/hyprland-nested-dev/` | Defines a safe nested-Hyprland workflow for plugin development and smoke testing. | Use when loading, reloading, debugging, or validating the plugin in a nested compositor. |
| [hyprland-plugin](hyprland-plugin/) | `.agents/skills/hyprland-plugin/` | Documents native Hyprland plugin APIs, events, dispatchers, config, hooks, and ABI checks. | Use for plugin initialization, host integration, dispatcher registration, config wiring, or API compatibility. |
| [implement](implement/) | `.agents/skills/implement/` | Turns a specification into an incremental implementation plan. | Use when starting implementation work from requirements and needing scoped, testable steps. |
| [mru-switcher](mru-switcher/) | `.agents/skills/mru-switcher/` | Encodes this project's MRU switcher contracts and session behavior. | Use for snapshot sessions, apply-on-release, lock-in, scopes, UI ports, and dispatcher changes. |
| [omarchy-plugin-security](omarchy-plugin-security/) | `.agents/skills/omarchy-plugin-security/` | Security review for Omarchy Quattro plugins before marketplace submission, based on maintainer review history. | Use when writing, auditing, or hardening an Omarchy plugin, or preparing a `[Plugin]`/`[Verify]` submission. |
| [plugin-spec-compliance](plugin-spec-compliance/) | `.agents/skills/plugin-spec-compliance/` | Checks implementation and tests against SPEC requirements and ADR decisions. | Use during implementation or review to map changes to REQ/T IDs and detect contract violations. |
| [research](research/) | `.agents/skills/research/` | Gathers findings from high-trust sources and records an evidence-based memo. | Use for upstream API changes, documentation questions, compatibility research, or external investigations. |
| [to-spec](to-spec/) | `.agents/skills/to-spec/` | Converts a feature idea or request into explicit specification requirements. | Use before implementation when behavior needs normative REQ and test definitions. |
| [to-tickets](to-tickets/) | `.agents/skills/to-tickets/` | Breaks a specification or roadmap item into actionable implementation tickets. | Use after requirements are clear and work must be sequenced into milestone-sized tasks. |
| [writing-plans](writing-plans/) | `.agents/skills/writing-plans/` | Produces detailed plans for multi-step engineering tasks. | Use before touching code when a feature has several dependent implementation and validation steps. |

---

## Project skills (domain-specific)

| Skill | Level | Purpose |
|-------|-------|---------|
| [hyprland-plugin](hyprland-plugin/) | Expert | Native Hyprland `.so` API, Event::bus, dispatchers, config, hooks |
| [mru-switcher](mru-switcher/) | Expert | Project contracts — snapshot, apply-on-release, scopes, ROADMAP |
| [hyprland-focus-mru](hyprland-focus-mru/) | Expert | Focus history, debounce, lock-in, virtual selection |
| [cpp-plugin-architecture](cpp-plugin-architecture/) | Expert | Ports/adapters, pure domain, FocusGateway, tests |
| [hyprland-nested-dev](hyprland-nested-dev/) | Advanced | Nested session, load/reload, smoke checklist |
| [plugin-spec-compliance](plugin-spec-compliance/) | Expert | SPEC/ADR review, requirement IDs, merge gates |
| [hyprland-lua-config](hyprland-lua-config/) | Expert | Lua/Omarchy config, hyprpm, hyprctl, sidecar config host work |
| [hyprland-lua](hyprland-lua/) | Expert | Hyprland 0.55+ Lua API — hl.bind/hl.on/hl.dsp, rules, hyprctl dispatch |
| [omarchy-plugin-security](omarchy-plugin-security/) | Expert | Omarchy plugin security review and marketplace hardening |

---

## Ecosystem skills (vendored from skills.sh / peers)

| Skill | Why it helps |
|-------|----------------|
| **implement** | Turn SPEC into incremental implementation plans |
| **to-spec** / **to-tickets** | Break ROADMAP milestones into tickets |
| **writing-plans** | Multi-step requirements → plan |
| **code-review** | Review discipline with `plugin-spec-compliance` |
| **domain-modeling** | Refine WindowRef / Session aggregates |
| **codebase-design** | Repo layout decisions |
| **diagnosing-bugs** | Crash/load failure analysis |
| **research** | Upstream Hyprland API churn |
| **grill-me** / **grilling** | Stress-test design before coding |
| **hyprland-lua** | Hyprland 0.55+ Lua API reference for host config work |
| **hyprland-lua-config** | Lua config, hyprpm, hyprctl, Omarchy integration procedures |
| **omarchy-plugin-security** | Security hardening for Omarchy plugin submissions |
| **find-skills** | Discover additional skills |

Upstream updates: [skills.sh](https://www.skills.sh/) / `npx skills update` — re-vendor deliberately; do not assume network at runtime.

---

## Suggested activation matrix

| Task | Skills |
|------|--------|
| Scaffold plugin | `hyprland-plugin` + `cpp-plugin-architecture` |
| Implement session/MRU | `mru-switcher` + `hyprland-focus-mru` |
| PR review | `plugin-spec-compliance` + `mru-switcher` + `code-review` |
| Dev environment | `hyprland-nested-dev` |
| Host API question | `hyprland-plugin` |
| Plan a feature | `writing-plans` + `to-spec` + `to-tickets` |
| Diagnose a failure | `diagnosing-bugs` + relevant project skill |
| Investigate upstream | `research` + `hyprland-plugin` |
| Stress-test design | `grill-me` / `grilling` + `mru-switcher` |
| Domain type redesign | `domain-modeling` + `cpp-plugin-architecture` |
| Repo layout | `codebase-design` + `cpp-plugin-architecture` |
| Lua/Omarchy config or binding task | `hyprland-lua-config` (+ `hyprland-lua`) |
| Omarchy plugin security review | `omarchy-plugin-security` + `code-review` |

Must match [AGENTS.md](../../AGENTS.md) §3.5.

---

## Conventions

- `name` in frontmatter **equals** directory name
- `description` includes **what** and **when** (triggers)
- Prefer loading a skill over pasting its body into prompts (token economy — AGENTS §12)
- Level tags in `metadata.level` when present — `expert` | `advanced`

## Reading order with project docs

1. `docs/HYPRLAND-PLUGIN-SYSTEM.md` + `hyprland-plugin`
2. `docs/ARCHITECTURE.md` + `cpp-plugin-architecture`
3. `docs/SPEC.md` + `mru-switcher` + `plugin-spec-compliance`
4. `docs/ROADMAP.md` + `implement` / `to-tickets` / `writing-plans`
