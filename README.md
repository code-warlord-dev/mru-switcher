# MRU Window Switcher for Hyprland

Niri-style Alt+Tab: most-recently-used order, frozen snapshot while tabbing, focus applied on modifier release.

This repository is an **M0/M1 scaffold**: normative docs, agent operating system, CMake/CI for domain tests. Plugin `.so` lands in **M2** (see [docs/ROADMAP.md](docs/ROADMAP.md)).

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
- **UI:** default **`null`** until M4; `border` / `external` fall back to null if unavailable.
- **Identity:** `WindowRef { address, generation }`.
- **Language:** native plugin **C++ only**; overlay may be any language.

---

## Build (domain tests)

```bash
cmake -S . -B build -DMRU_BUILD_TESTS=ON -DMRU_BUILD_PLUGIN=OFF
cmake --build build -j
ctest --test-dir build --output-on-failure
```

`MRU_BUILD_PLUGIN=ON` **fails configure** until M2 wires the `.so` target (see [CMakeLists.txt](CMakeLists.txt), [hyprpm.toml](hyprpm.toml)).

---

## Dispatchers (planned)

```text
mru:cycle  [next|prev] [scope?]   # omitted direction = next
mru:apply
mru:cancel
mru:status
```

Example binds: see [docs/USER.md](docs/USER.md).

---

## Roadmap (short)

| Milestone | Focus |
|-----------|--------|
| M0 | Docs, skills, AGENTS, consistency |
| M1 | Domain + unit tests |
| M2 | Loadable plugin (Null UI), COMPAT pin |
| M3–M6 | Scopes, border UI, overlay, v1.0 |

Details: [docs/ROADMAP.md](docs/ROADMAP.md), progress: [docs/agent-state/PROGRESS.md](docs/agent-state/PROGRESS.md).

---

## License

[MIT](LICENSE)
