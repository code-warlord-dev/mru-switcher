# MRU Window Switcher for Hyprland

Niri-style Alt+Tab: most-recently-used order, frozen snapshot while tabbing, focus applied on modifier release.

This repository currently contains **architecture, specification, and roadmap documentation**. Implementation follows the contracts in `docs/`.

---

## Agents

Primary operating contract for AI agents: **[AGENTS.md](AGENTS.md)** (orchestrator role, subagents, skills, git/GitHub).

Skills live under [.agents/skills/](.agents/skills/).

## Documentation

| File | Description |
|------|-------------|
| [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) | MRU plugin design, domain, ports, sequence diagrams |
| [docs/HYPRLAND-PLUGIN-SYSTEM.md](docs/HYPRLAND-PLUGIN-SYSTEM.md) | **Host** plugin system: load lifecycle, HyprlandAPI, Event::bus, constraints |
| [docs/SPEC.md](docs/SPEC.md) | **Normative** technical specification (requirements, dispatchers, config, tests) |
| [docs/DECISIONS.md](docs/DECISIONS.md) | Architecture Decision Records (ADRs) |
| [docs/ROADMAP.md](docs/ROADMAP.md) | Milestones M0–M6, risks, versioning |
| [docs/USER.md](docs/USER.md) | User guide: binds, scopes, config, FAQ |
| [docs/API.md](docs/API.md) | Short public API reference (dispatchers + config) |
| [docs/DIAGRAMS.md](docs/DIAGRAMS.md) | Standalone Mermaid / PlantUML diagrams |
| [docs/CONTRIBUTING.md](docs/CONTRIBUTING.md) | Contribution guidelines |
| [docs/SECURITY.md](docs/SECURITY.md) | Trust model and security notes |
| [docs/FAILURE-MODES.md](docs/FAILURE-MODES.md) | Failure matrix (state, dispatcher, UI, tests) |
| [docs/TRANSITION-TABLE.md](docs/TRANSITION-TABLE.md) | SessionController transitions |
| [docs/COMPAT.md](docs/COMPAT.md) | Hyprland pin matrix |
| [docs/GOVERNANCE.md](docs/GOVERNANCE.md) | ADR index, DoD, dependency rules |
| [docs/OBSERVABILITY.md](docs/OBSERVABILITY.md) | Logging, session_id, mru:status |
| [docs/THREAT-MODEL.md](docs/THREAT-MODEL.md) | Threats and mitigations |
| [docs/SUPPORT-AND-RELEASE.md](docs/SUPPORT-AND-RELEASE.md) | Support axes, release, rollback |

**Reading order for implementers:**  
HYPRLAND-PLUGIN-SYSTEM → ARCHITECTURE → SPEC → DECISIONS → ROADMAP → CONTRIBUTING.

**Reading order for users (once shipped):** USER → API.

---

## Design summary

- **Snapshot** on session start — list does not reorder while you tab.
- **Apply-on-release** — real focus only on `mru:apply` / modifier release.
- **History lock-in + debounce** — intermediate focuses do not pollute MRU during a session.
- **Scopes:** global, monitor, workspace, visible, app.
- **UI as strategy:** null / border highlight / external overlay.
- **Hyprland integration:** `Event::bus()`, `addDispatcherV2`, config under `plugin:mru-switcher:`, hash check in `PLUGIN_INIT`.
- **Language:** native plugin is **C++ only** (Hyprland plugin ABI). External overlay may use any language.

---

## Planned dispatcher contract

```text
mru:cycle  [next|prev] [scope?]
mru:apply
mru:cancel
mru:status
```

Example binds:

```conf
bind   = ALT, TAB,       mru:cycle, next
bind   = ALT SHIFT, TAB, mru:cycle, prev
bindrt = ALT, ALT_L,     mru:apply
bind   = ALT, Escape,    mru:cancel
```

---

## Roadmap (short)

| Milestone | Focus |
|-----------|--------|
| M0 | Docs & contracts (current) |
| M1 | Domain + unit tests |
| M2 | Hyprland MVP plugin (Null UI) |
| M3 | Scopes + full config |
| M4 | Border UI |
| M5 | External overlay protocol (optional) |
| M6 | Hardening → v1.0 |

Details: [docs/ROADMAP.md](docs/ROADMAP.md).

---

## License

To be decided with the implementation.
