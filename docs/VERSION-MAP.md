# Version Map

Semver for the **MRU Switcher** project. Plugin binaries still require a rebuild matching the Hyprland header hash; version tags describe *project* contracts and milestone completion, not universal binary compatibility.

| Version | Tag | Milestone | Hyprland pin | Contract notes |
|---------|-----|-----------|--------------|----------------|
| 0.0.0 | — | M0 | — | Documentation, skills, AGENTS only |
| 0.1.0 | `v0.1.0` | M1 | — | Domain + unit tests; no `.so` required |
| 0.2.0 | `v0.2.0` | M2 | documented in release notes | First loadable plugin; Null UI; `mru:cycle|apply|cancel` |
| 0.3.0 | `v0.3.0` | M3 | v0.56.2 (`efb5099`) | All scopes + full config surface + `mru:status` |
| 0.4.0 | `v0.4.0` | M4 | v0.56.2 (`efb5099`) | Border UI (`solid`); restore normalized to `setprop` grammar; `keyword`-channel limitation documented |
| 0.5.0 | `v0.5.0` | M5 | v0.56.2 (`efb5099`) | External overlay protocol (`ui = external`, AF_UNIX socket); ADR-019 removable fd watch |
| 1.0.0 | `v1.0.0` | M6 | v0.56.2 (`efb5099`), pinned in `hyprpm.toml` `commit_pins` | Stable dispatcher names, config keys, snapshot/apply semantics; **contract freeze declared in M6-T1 (issue #50)**; plugin-side pin hash finalized to release-prep main `320c4cb` (M6-T9 prep); **tag pending the explicit human release command** |

## Rules

- **0.x** — prefer additive changes; breaking user contracts only with CHANGELOG entry.
- **1.x** — breaking dispatcher/config/session semantics requires major bump.
- Update this table when cutting a tag.
- Hyprland pin column: commit hash or release version tested in that tag’s release notes / `hyprpm.toml`.

## See also

- `docs/ROADMAP.md` — milestone exit criteria  
- `CHANGELOG.md` — user-visible changes  
- `AGENTS.md` §15–16 — versioning and release procedure  
