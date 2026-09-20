# Support, release, and compatibility guarantees

## Version axes (do not conflate)

| Axis | Meaning |
|------|---------|
| **Project semver** | `docs/VERSION-MAP.md` — features and user contracts |
| **Hyprland header pin** | `docs/COMPAT.md` — compile-time ABI |
| **Runtime hash** | Must match at load or plugin aborts |
| **Dispatcher/config contract** | Frozen at 1.x per SPEC |

## Supported versions policy (draft)

- **0.x:** best-effort; breaking changes allowed with CHANGELOG.  
- **1.x:** dispatcher names, config keys, snapshot/apply/lock-in semantics stable.  
- Only **pinned** Hyprland revisions in COMPAT matrix are “supported.”  
- Older plugins on newer Hyprland: unsupported unless matrix says so — rebuild expected.

## Deprecation policy

1. CHANGELOG + USER note.  
2. At least one minor of warn-on-use if feasible.  
3. Remove on next **major**.

## Release checklist (human + orchestrator)

See also AGENTS.md §16.

- [ ] PROGRESS milestone exit criteria  
- [ ] CHANGELOG section  
- [ ] VERSION-MAP row  
- [ ] COMPAT matrix row with commit  
- [ ] Unit tests green  
- [ ] Nested smoke (plugin releases) — manual nest gate per `docs/agent-state/reports/2026-09-19-m6-manual-nest-gate.md` (required pre-tag for v1.0.0)  
- [ ] REQ-TRACE updated for new REQs  
- [ ] Tag annotated `vX.Y.Z`  
- [ ] Release artifacts attached: the built `.so` for the pinned Hyprland commit + `sha256sums.txt`
      (and a note naming the exact Hyprland commit the artifact was built against)  
- [ ] hyprpm.toml pins updated  

## Rollback

1. `hyprctl plugin unload` path.  
2. Remove `plugin =` / hyprpm disable.  
3. Revert to previous tag build matching that era’s Hyprland pin.  
4. Config keys unknown to older plugin are ignored by Hyprland if unregistered — document removed keys in migration notes.

## Bug severity (project)

| Sev | Meaning | Response target (community, best-effort) |
|-----|---------|------------------------------------------|
| S0 | Compositor crash/corrupt | Immediate hotfix branch |
| S1 | Wrong focus / data loss of session intent | Next patch |
| S2 | Incorrect edge behavior with workaround | Current minor |
| S3 | Docs/UX polish | backlog |

No contractual SLA unless a commercial support agreement says otherwise.

## Migration guide template

```markdown
## Upgrading 0.x → 0.y
- Rebuild against Hyprland pin …
- Config changes: …
- Behavioral changes: …
```
