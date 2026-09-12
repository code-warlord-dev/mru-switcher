# Contributing

## Before writing code

1. Read **ARCHITECTURE.md**, **SPEC.md**, and **DECISIONS.md**.
2. Prefer changes that preserve domain purity (no Hyprland types in domain/).
3. Behaviour changes need a SPEC amendment and, if design-level, a new ADR.

## Code layout (target)

```text
include/mru/     # public headers for domain + ports
src/domain/      # pure logic
src/adapters/    # Hyprland + UI backends
src/plugin/      # PLUGIN_INIT, dispatchers
tests/           # unit tests (no compositor)
docs/            # this documentation set
```

## Implementation rules

- Match SPEC requirement IDs when implementing tests (`T-S-01`, …).
- Dispatchers: parse → call SessionController → map to `SDispatchResult`.
- Never focus from `mru:cycle`.
- Hash check on load; fail closed.

## Hyprland plugin hygiene

- Compile against matching headers; document required Hyprland version/commit in README.
- Use `Event::bus()` for events.
- Config keys only under `plugin:mru-switcher:` and only registered in `PLUGIN_INIT`.
- No compositor-touching background threads.

## Pull requests

- One logical change per PR when possible.
- Update SPEC/USER if user-visible behaviour changes.
- Include test plan (unit + manual binds checklist from USER.md).

## Code style

Follow Hyprland / project clang-format if provided; otherwise consistent C++23, clear names, no silent failure on invariant breaks in debug builds.
