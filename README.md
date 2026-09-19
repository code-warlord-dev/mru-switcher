# MRU Window Switcher for Hyprland

**Alt+Tab that remembers how you actually work** — not where a window sits on a grid.

Built for **[Omarchy](https://omarchy.org)** (and anyone on **[Hyprland](https://hyprland.org)** who wants the same feel): take the best interaction ideas from **[Niri](https://github.com/YaLTeR/niri)** and make them feel native under Hyprland, without replacing the compositor or forcing a different layout philosophy.

> **Goal:** give Hyprland (and Omarchy) a Niri-class global Alt+Tab — MRU order, frozen list while tabbing, focus only on modifier release — as a proper plugin with explicit contracts, not a fragile bind script.

---

## Why this exists

Hyprland is excellent at tiling, workspaces, and motion. What many people still miss after using Niri is a **predictable global window switcher**:

| What you want | What you often get instead |
|---------------|----------------------------|
| Jump to *the window you used a moment ago* | Workspace-local or stack-order cycling |
| Hold Alt, tap Tab, list stays still | List **jumps** as focus events reorder it |
| Focus commits when you **release** Alt | Every Tab already moves real focus (and pollutes history) |

Niri’s model is simple and hard to unlearn: **most-recently-used order**, a **frozen list while you tab**, **real focus only on release**. This plugin brings that contract into Hyprland as a first-class plugin — designed for Omarchy’s “batteries included, still under your control” desktop, not as a one-off dotfiles hack.

We are **not** trying to turn Hyprland into Niri. Scrolling columns, Niri’s layout engine, and its whole shell stay where they belong. We borrow **one sharp UX idea** and implement it with Hyprland’s plugin API, explicit invariants, and a testable domain core.

---

## What you get (in one screen)

```text
Alt held  →  Tab / Shift+Tab move a *virtual* selection through an MRU snapshot
Alt up    →  focus lands on the selected window once
```

Under the hood (so it keeps working after a week of real use, not only in a demo):

- **Snapshot** — the candidate list is fixed for the whole Alt-hold session  
- **Lock-in** — intermediate focuses do not rewrite MRU while you switch  
- **Debounce** — brief focus blips do not instantly reshuffle history  
- **Stable identity** — `address + generation`, so a recycled window id cannot steal focus  
- **Scopes** — global / monitor / workspace / visible / app when you need a narrower ring  

Dispatch surface (implemented in M2, SPEC §3.3): `mru:cycle`, `mru:apply`, `mru:cancel`, `mru:status`.

---

## Project status

M0 (design gate), **M1** (pure domain core + tests), **M2** (loadable `.so`, Null UI, four dispatchers), **M3** (all five scopes + full 8-key config surface, verified live in a nested session) and **M4** (border highlight UI — `ui = border` opt-in, `solid` highlight via public window props on the pinned Hyprland, per-session frozen backend, exact restore verified live in a nested smoke) are **done on `main`** ([v0.4.0]). Optional next: the M5 external overlay. The loadable plugin is pinned to Hyprland **v0.56.2** ([docs/COMPAT.md](docs/COMPAT.md); note: runtime `hyprctl keyword` changes to plugin keys are not picked up — see [docs/USER.md](docs/USER.md)); CI builds the `.so` and runs the domain + plugin-core tests. See [docs/ROADMAP.md](docs/ROADMAP.md), [docs/agent-state/PROGRESS.md](docs/agent-state/PROGRESS.md) and [docs/VERSION-MAP.md](docs/VERSION-MAP.md).

| If you are… | Start here |
|-------------|------------|
| Curious user / Omarchy explorer | This README → [docs/USER.md](docs/USER.md) (binds & config) |
| Implementing the plugin | [docs/SPEC.md](docs/SPEC.md) → [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) |
| Reviewing design decisions | [docs/DECISIONS.md](docs/DECISIONS.md) |
| Wiring agents / automation | [AGENTS.md](AGENTS.md) |

---

## Agents

| File | Role |
|------|------|
| [AGENTS.md](AGENTS.md) | Orchestrator contract: subagents, git/GitHub, token economy, state sync |
| [.agents/skills/README.md](.agents/skills/README.md) | Full skills inventory, activation matrix, conventions |

Skills live under [`.agents/skills/`](.agents/skills/). Project + ecosystem skills are **vendored in-tree** (see skills README).

---

## Documentation

### Core contracts

| File | Description |
|------|-------------|
| [docs/SPEC.md](docs/SPEC.md) | **Normative** behaviour, dispatchers, config, tests |
| [docs/DECISIONS.md](docs/DECISIONS.md) | ADRs |
| [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) | Plugin design, domain, ports, diagrams |
| [docs/HYPRLAND-PLUGIN-SYSTEM.md](docs/HYPRLAND-PLUGIN-SYSTEM.md) | Host PluginAPI, Event::bus, constraints |
| [docs/FAILURE-MODES.md](docs/FAILURE-MODES.md) | Failure matrix (state / dispatcher / UI / tests) |
| [docs/TRANSITION-TABLE.md](docs/TRANSITION-TABLE.md) | SessionController transitions |
| [docs/REQ-TRACE.md](docs/REQ-TRACE.md) | REQ → test → implementation traceability |

### Product / user

| File | Description |
|------|-------------|
| [docs/USER.md](docs/USER.md) | Binds, scopes, config, FAQ |
| [docs/API.md](docs/API.md) | Public dispatchers + config keys |
| [docs/DIAGRAMS.md](docs/DIAGRAMS.md) | Mermaid / PlantUML |

### Planning and ops

| File | Description |
|------|-------------|
| [docs/ROADMAP.md](docs/ROADMAP.md) | Milestones M0–M6 |
| [docs/VERSION-MAP.md](docs/VERSION-MAP.md) | Semver ↔ milestones ↔ tags |
| [docs/COMPAT.md](docs/COMPAT.md) | Hyprland pin matrix |
| [docs/GOVERNANCE.md](docs/GOVERNANCE.md) | ADR index, DoD, dependency rules |
| [docs/OBSERVABILITY.md](docs/OBSERVABILITY.md) | Logging, session_id, status |
| [docs/THREAT-MODEL.md](docs/THREAT-MODEL.md) | Threats and mitigations |
| [docs/SUPPORT-AND-RELEASE.md](docs/SUPPORT-AND-RELEASE.md) | Support axes, release, rollback |
| [docs/SECURITY.md](docs/SECURITY.md) | Trust model (in-process plugin) |
| [docs/CONTRIBUTING.md](docs/CONTRIBUTING.md) | Contribution guidelines |
| [CHANGELOG.md](CHANGELOG.md) | Keep a Changelog |

### Design notes and agent state

| File | Description |
|------|-------------|
| [docs/design-notes/scheduler-and-debounce.md](docs/design-notes/scheduler-and-debounce.md) | SchedulerPort |
| [docs/design-notes/window-identity.md](docs/design-notes/window-identity.md) | WindowRef generation |
| [docs/design-notes/apply-after-invalidation.md](docs/design-notes/apply-after-invalidation.md) | Prune / clamp / apply |
| [docs/agent-state/SESSION.md](docs/agent-state/SESSION.md) | Live session state block |
| [docs/agent-state/PROGRESS.md](docs/agent-state/PROGRESS.md) | Milestone checkboxes |

**Reading order (implementers):**  
[HYPRLAND-PLUGIN-SYSTEM](docs/HYPRLAND-PLUGIN-SYSTEM.md) → [ARCHITECTURE](docs/ARCHITECTURE.md) → [SPEC](docs/SPEC.md) → [FAILURE-MODES](docs/FAILURE-MODES.md) → [DECISIONS](docs/DECISIONS.md) → [REQ-TRACE](docs/REQ-TRACE.md) → [ROADMAP](docs/ROADMAP.md) → [AGENTS](AGENTS.md).

**Reading order (users, after ship):** [USER](docs/USER.md) → [API](docs/API.md).

---

## Design summary

- **Snapshot** on session start — list does not reorder while tabbing.
- **Apply-on-release** — focus only on `mru:apply` / modifier release.
- **History lock-in + debounce** — via SchedulerPort; no MRU updates during Active session.
- **Scopes:** global, monitor, workspace, visible, app (`class` only).
- **UI:** default **`null`** in M4; `border` → `BorderHighlightUI` (public window-prop mechanism, restore-by-value, `solid` style first — ADR-017; pinned symbols in `docs/COMPAT.md`); `external` falls back to null until M5.
- **Identity:** `WindowRef { address, generation }`.
- **Language:** native plugin **C++ only**; overlay may be any language.

---

## Build (domain tests)

```bash
cmake -S . -B build -DMRU_BUILD_TESTS=ON -DMRU_BUILD_PLUGIN=OFF
cmake --build build -j
ctest --test-dir build --output-on-failure
```

Building the plugin (a real `.so`) requires the pinned Hyprland headers (v0.56.2, see [docs/COMPAT.md](docs/COMPAT.md)). With them, `MRU_BUILD_PLUGIN=ON` configures and builds `build/mru-switcher.so` — the CI `plugin-build` job does exactly this. Without those headers, leave the option off (the default) and run the tests above.

---

## Dispatchers (implemented)

```text
mru:cycle  [next|prev] [scope?]   # omitted direction = next
mru:apply
mru:cancel
mru:status
```

Example binds: see [docs/USER.md](docs/USER.md).

---

## Roadmap (short)

| Milestone | Focus | Status |
|-----------|--------|--------|
| M0 | Docs, skills, AGENTS, consistency | ✅ done |
| M1 | Domain + unit tests | ✅ done |
| M2 | Loadable plugin (Null UI), COMPAT pin | ✅ done |
| M3 | Scopes + full config surface | ✅ done |
| M4 | Border UI + style interface | **in progress** |
| M5–M6 | Overlay, v1.0 | planned |

Details: [docs/ROADMAP.md](docs/ROADMAP.md), progress: [docs/agent-state/PROGRESS.md](docs/agent-state/PROGRESS.md).

---

## License

[MIT](LICENSE)
