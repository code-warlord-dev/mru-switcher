# Correction: Alt-release model for Lua/Omarchy (2026-09-21)

**Triggered by:** 2026-09-21-omarchy-alt-release-and-apply-model.md (Юрий Анатольевич)
**Root cause:** ADR-022 (merged PR #85) shipped `examples/mru-switcher-bindings.lua` with
Tab-release apply as the primary Lua recipe. On Hyprland v0.56.2 / efb5099 the Lua keybind
path drops release binds keyed on modifiers; ADR-022 used Tab (ordinary key) as a workaround.
apply-on-Tab-release breaks MRU browse semantics (every Tab fires a real focus move; stack
reshuffles). It must NOT be the product model.
**Source of truth:** docs/agent-state/research/2026-09-21-modifier-hold-poll-api.md
**Status:** DONE (2026-09-21) — all Core + acceptance items complete; the empirical
bindrt nest check stays Deferred (human-gated) and is carried as an ADR-023 follow-up
before the v1.0.0 tag.

## Decision (conforms to ADR-022 correction table, §5 of the trigger file)

| Environment | Apply trigger | Status | Action |
|-------------|---------------|--------|--------|
| Hyprlang + bindrt on Alt_L/Alt_R | Release Alt | Open question (empirical nest re-verify needed) | Keep hyprlang recipe in examples/mru-switcher-bindings.conf; mark COMPAT row as "re-verify before 1.0", not "keeps working" |
| Lua + release bind on Alt_L/Alt_R | Release Alt | Broken on pin efb5099 (KeybindManager.cpp:652,737-746) | Do NOT document as working; remove from examples |
| Lua + Tab release → apply | Release Tab | Degraded (breaks MRU browse) | Remove as primary recipe; keep only as historical note if at all |
| Lua + ALT+Return → apply | Explicit key | Honest fallback — ship as supported (B1) | Default in examples/mru-switcher-bindings.lua |
| Lua + hl.is_key_down poll on Alt | Logical release Alt | Proper Lua fix — implement/document (B2) | Ship as examples/mru-switcher-bindings-poll.lua reference |

## Files to change

### Core (this branch)
- [x] examples/mru-switcher-bindings.lua — B1 (explicit ALT+Return apply) + B2 reference note; keep hl.unbind + cycle-on-press
- [x] examples/mru-switcher-bindings-poll.lua — NEW; B2 reference impl (hl.is_key_down + hl.timer)
- [x] docs/DECISIONS.md — ADR-023 (supersedes ADR-022 product framing) + ADR-022 consequences note
- [x] docs/COMPAT.md — mitigation col: "use Tab release" → "explicit apply key and/or is_key_down poll"; hyprlang bindrt row: "keeps working" → "re-verify before 1.0"
- [x] README.md — "Why release-on-Tab for Lua?" section removed; two-column table hyprlang vs Lua; Omarchy unbind mandated
- [x] docs/USER.md — Quick-start Lua recipe: Tab-release apply → explicit ALT+Return; add poll alternative note
- [x] docs/API.md — Lua bridge example: Tab-release → explicit apply; add poll note
- [x] CHANGELOG.md — Unreleased: correct ADR-022 entry (Tab-release → correction to explicit-apply + poll)
- [x] scripts/setup-bindings.sh — help text: Tab-release mentions → explicit-apply/poll; default Lua profile = B1

### Deferred (human gate / nest)
- [ ] Empirical nest check: does hyprlang `bindrt = ALT, ALT_L` fire on Alt release on efb5099?
  - Research memo strongly suggests NO (same KeybindManager::handleKeybinds path; current-mods match fails).
  - If confirmed broken: COMPAT + SPEC §11 need update; hyprlang users also need workaround (explicit apply key or native plugin Alt-event detection via Event::bus input.keyboard.key).
  - If confirmed working: keep as-is, mark COMPAT "verified".
  - **Not in this branch** — requires live nest (human-gated).

### Acceptance criteria (this branch)
- [x] examples/mru-switcher-bindings.lua: Tab-release apply NOT the primary recipe; B1 present and documented
- [x] examples/mru-switcher-bindings-poll.lua: self-contained, commented, copy-pasteable B2
- [x] ADR-023 in DECISIONS.md: status Accepted; supersedes ADR-022 product framing; references the research memo
- [x] COMPAT mitigation text corrected; hyprlang bindrt row honestly flagged
- [x] README/USER/API/CHANGELOG all consistent with the two-column model
- [x] scripts/setup-bindings.sh help text consistent; no "Tab release" as the recommended path
- [x] commit message: feat(bindings-model): correct Lua apply-on-release to explicit-apply + poll (ADR-023)
- [x] ctest still green (no domain/plugin behavior change — docs + examples only)
