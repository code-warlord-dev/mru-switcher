---
name: plugin-spec-compliance
description: Enforces MRU Switcher SPEC and ADR compliance during implementation and code review. Use when reviewing PRs, checking requirement IDs, updating SPEC or DECISIONS, mapping tests to T-S and T-H cases, or deciding whether a behavior change needs an ADR.
metadata:
  level: expert
  version: "1.0"
  domain: mru-switcher
---

# Plugin SPEC and ADR Compliance

## Overview

Behavioral source of truth is **`docs/SPEC.md`**. Design decisions live in **`docs/DECISIONS.md`**. Architecture is descriptive; when conflict arises, SPEC wins until an ADR updates both.

## When to use

- PR review for mru-switcher
- Adding features or changing cycle/apply/cancel semantics
- Writing or renaming tests
- Editing SPEC / ADR / ROADMAP

## Review procedure

1. Identify user-visible behavior change.
2. Locate SPEC requirement IDs (`REQ-S-*`, `REQ-H-*`, `REQ-F-*`, …).
3. If behavior contradicts SPEC — either fix code or amend SPEC + changelog.
4. If design approach changes (e.g. live list instead of snapshot) — new ADR, update ARCHITECTURE.
5. Map tests to SPEC §9 IDs (`T-S-01`, …).

## Frozen contracts (v0.x careful / v1.x stable)

- Dispatcher names — `mru:cycle`, `mru:apply`, `mru:cancel`, `mru:status`
- Config key names under `plugin:mru-switcher:`
- Snapshot + virtual selection + lock-in (ADR-001–003)

Additive optional args are preferred over renames.

## Required checks before merge

- [ ] Hash check still present
- [ ] No focus in `mru:cycle` path
- [ ] Lock-in respected when configured
- [ ] Domain free of Hyprland types
- [ ] Config keys only registered in init
- [ ] Tests cover touched REQ/T-IDs
- [ ] USER.md updated if binds/config UX change

## Changing SPEC

1. Edit SPEC with clear REQ id updates
2. Note ROADMAP impact if milestone exit criteria change
3. Mention in PR description
4. Do not silently diverge ARCHITECTURE from SPEC

## Related skills

- `mru-switcher` — implement behavior
- `hyprland-plugin` — host API legality
- `cpp-plugin-architecture` — structure
