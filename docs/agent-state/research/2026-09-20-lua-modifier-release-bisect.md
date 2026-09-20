# Lua keybind path — modifier-release binds never fire (bisect, 2026-09-20)

**Status:** Closed (ADR-022; docs + packaging change, no plugin behavior change)

## Problem

On the user's live host (Omarchy, Hyprland Lua config via `~/.config/hypr/hyprland.lua`,
Hyprland commit `efb50993780079460b0cbed1363e2166a2de1d9f`, tag `v0.56.2`, compose pid 1245),
the documented apply-on-release recipe never committed:

- `hl.bind("ALT_L", … { release = true })` → apply dispatcher never fired.
- `hl.bind("ALT + ALT_L", … { release = true })` → same.

Cycling worked (`mru:status` showed `active=true`), so the dispatchers themselves were fine.

## Hypothesis → bisect matrix

Bindings injected at runtime via `hyprctl repl` (active configuration, keybind-manager reload
observable in `hyprctl binds -j`). Each row: does the bound handler fire?

| Binding (release) | Key kind | modmask | Fires? |
|-------------------|----------|---------|--------|
| `hl.bind("F9", …, {release=true})` | ordinary | 0 | **Yes** |
| `hl.bind("ALT + TAB", …, {release=true})` | ordinary | 8 | **Yes** |
| `hl.bind("ALT + ALT_R", …, {release=true})` | modifier | 8 | **No** |
| `hl.bind("ALT + ALT_L", …, {release=true})` | modifier | 8 | **No** |

Also confirmed: binding the bare key token `hl.bind("ALT_L")` yields modmask **0** in
`hyprctl binds -j` and never matches a real Alt press (effective modmask 8), so that form is
wrong regardless of the release issue.

## Conclusion

Rule (on this pin, Lua keybind path): **release binds whose key is a modifier are dropped;**
ordinary keys fire release binds even under the same modmask. This is a host limitation in the
Lua `ConfigManager`/keybind path. The hyprlang `bindrt = ALT, ALT_L` form on the same pin was
not exercised live but remains the documented hyprlang route (ADR-022 keeps it).

## Fix shipped (ADR-022)

- Lua recipe (`examples/mru-switcher-bindings.lua`) unbinds Omarchy defaults and commits apply
  on **Tab release** (`"ALT + TAB"`, modmask 8 rebuilt by release order) — same Niri-style UX.
- `scripts/setup-bindings.sh` installs the right fragment per backend with a live conflict check.
- README/USER/API/COMPAT make the keybinding step mandatory and document the caveat.

## Traceability

- ADR-022 (docs/DECISIONS.md) — full evidence + decision.
- COMPAT.md "Known host limitations" table.
- CHANGELOG Unreleased > "Keybinding delivery is a first-class install step".