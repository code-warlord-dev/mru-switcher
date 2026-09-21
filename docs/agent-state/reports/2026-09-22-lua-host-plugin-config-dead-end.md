# Report — Lua config host: plugin configuration channel is dead (visual feedback stuck on `ui = null`)

- **Date:** 2026-09-22
- **Host:** live user desktop (not nest). Hyprland **0.56.2**, pin `efb5099…` (same pin as all M6 nest gates). Config backend: **Lua** (Omarchy, `~/.config/hypr/*.lua` via `hl.*` API).
- **Plugin instance:** `mru-switcher` **v0.4.0**, loaded from **hyprpm** cache (see §4), repo state hash `57f3164` = current `main` (ADR-023 included). Bindings on the host are the B2 poll recipe from ADR-023.
- **Related:** PR #87 (ADR-023), research memo `docs/agent-state/research/2026-09-21-modifier-hold-poll-api.md`, predecessor repro `docs/agent-state/reports/2026-09-20-m6-b1-reload-repro.md`.

---

## 0. TL;DR

The user reported: cycling works (window stack shifts, applies on Alt release — B2 poll behaves as designed), but **there is no visual feedback** on Tab. Root cause chain:

1. The plugin's built-in default is `ui = null` (no visuals) — correct per ADR-011; the user simply never set `ui = border`.
2. The user **cannot** set it: on this Lua-config build of Hyprland, **every configuration channel for `plugin:mru-switcher:*` keys is dead**. The Lua config layer rejects plugin keys with `unknown config key 'plugin.mru-switcher.*'` — both from `hl.config()` calls and from the real config-file parse on reload.
3. This is a **host gap, not a plugin bug**: our keys are registered correctly (all 10 keys show up in `hyprctl getoption` with SPEC §4 defaults once the plugin is loaded). The host's Lua frontend has no working path into the hyprlang *special category* `plugin` that plugins register into.

Impact surface: **the entire `plugin:mru-switcher:*` config surface** (all 10 keys) is unreachable on Lua hosts — not just `ui`.

## 1. Symptom (user-visible)

- `ALT+Tab` cycling between windows (including across workspaces) works; list frozen while Alt held; commit happens on Alt release (B2 poll recipe in `~/.config/hypr/bindings.lua`).
- No visual highlight of the selected window at any point.
- `hyprctl getoption plugin:mru-switcher:ui` → `str: null`, `set: false` → plugin runs Null UI. Border UI (ADR-011, M4) is implemented and nest-verified — it is simply never enabled because no channel can set the key.

## 2. Candidate channels tested — all dead or unproven

| # | Channel | Result on this build | Evidence |
|---|---------|----------------------|----------|
| 1 | `hyprctl keyword plugin:… ui border` | **Blocked by the build**: `keyword can't work with non-legacy parsers. Use eval.` | §9.1 |
| 2 | `hl.config({ plugin = { ["mru-switcher"] = { ui = … } } })` (nested form) | **Rejected by validator**: `unknown config key 'plugin.mru-switcher.ui'` — as `hyprctl eval` shim **and** from the real config file on `hyprctl reload` | §9.2, §9.3 |
| 3 | Flat form `hl.config({ ["plugin:mru-switcher:ui"] = … })` | Accepted shape-wise (pcall ok) but the **async validator still rejects** it (shim prints `unknown config key` after `ok`), value not applied | §9.2 |
| 4 | Config **file** + `hyprctl reload` (the real parser — canonical path) | Same `unknown config key` errors in `hyprctl configerrors`; `getoption` stays at default `set: false` | §9.3 |
| 5 | `hl.get_config("plugin:mru-switcher:ui")` | `nil` (read-only channel anyway; core keys work: `general:gaps_in` → 8) | §9.4 |
| 6 | `hl.plugin.load(path, config?)` with a config table | Signature exists and accepts a table (arity errors prove it), **but Lua-path load itself fails silently** (`ok=true`, 0 plugins) with and without the table; the *same* `.so` loads fine via `hyprctl plugin load`. Config-table efficacy therefore **unproven** | §9.5 |
| 7 | Omarchy `o.*` helpers | Full dump: window/bind/launch helpers only — no plugin-config facility | §9.6 |
| 8 | Duplicate-load workaround (load already-loaded path with config) | **Silently no-ops** (plugin count unchanged, config unchanged) | §9.5 |

Ruled out early: plugin-side registration bug (all 10 keys visible via `hyprctl getoption` with correct SPEC defaults once loaded); stale plugin build (hyprpm state hash = `57f3164` = merged ADR-023 main).

## 3. Why this happens (host mechanics, from pinned-tree excerpts in `/tmp/mru-bindrt-test`)

- On this pin, plugin config lives in a hyprlang **special category**: `ConfigManager.cpp` does `m_config->addSpecialCategory("plugin", {nullptr, true})` and registers `plugin` as a config handler; plugin keys are stored as special-category fields named `"<name>:<key>"` (e.g. `mru-switcher:ui`) — that is why `hyprctl getoption plugin:mru-switcher:ui` works.
- The **Lua frontend's `hl.config()` validator walks a different key model** and does not resolve into special categories → any `plugin.*` table is "unknown" to it. Core categories (`general`, `decoration`) validate and apply fine — exactly what the error taxonomy shows.
- There is **no `hl.keyword`** on this build (`type(hl.keyword) == nil`; full `hl.*` dump in §9.4), so the runtime escape hatch known from upstream docs does not exist here either.

## 4. How the plugin actually gets onto this host (hyprpm, nonstandard cache)

- Loaded `.so` (discovered via `/proc/$(pgrep -x Hyprland)/maps`): **`/var/cache/hyprpm/code_warlord/mru-switcher/mru-switcher.so`** — root-owned, built 2026-09-22 01:28, 550 640 B.
- State file `/var/cache/hyprpm/code_warlord/mru-switcher/state.toml`:

  ```toml
  [mru-switcher]
  enabled = true
  failed = false
  filename = 'mru-switcher.so'

  [repository]
  author = 'code-warlord-dev'
  hash = '57f31648285d92ca17914cab99ae50e4104db7be'
  name = 'mru-switcher'
  rev = ''
  url = 'https://github.com/code-warlord-dev/mru-switcher'
  ```

- Consequence: the plugin **survives reboot** (enabled=true ⇒ loaded at compositor start). An in-session fear that it lived only as a manual load was wrong and is corrected here.
- Layout notes: state lives under **`/var/cache/hyprpm/<user>/`** (XDG-inconsistent — `~/.cache/hyprpm` does not exist; `AGENTS.md` §9.1 already references `/var/cache/hyprpm` for cache inspection). `hyprpm list` reports **live compositor state**, not disk state (output invariant under `HOME` and `XDG_RUNTIME_DIR` overrides; no on-disk manifest found under `$HOME`).
- Runtime ops verified here: `hyprctl plugin unload <full-path>` and `hyprctl plugin load <full-path>` both work. `hyprctl plugin unload` by name / handle / short path does **not** ("plugin not loaded").
- **Root-owned cache ⇒ any future `hyprpm update`/rebuild needs privilege escalation (`pkexec` per `AGENTS.md` §9.1). Never `chmod`/`chown` these paths without explicit human approval.**

## 5. Impact surface

Everything downstream of config on Lua hosts runs on **compiled defaults**:

| Key | Default in effect | User impact |
|-----|-------------------|-------------|
| `ui` | `null` | **No visual feedback** (the reported bug) |
| `border_color` / `border_size` / `border_style` | border defaults | unreachable while `ui=null` |
| `debounce_ms` | `400` | acceptable |
| `default_scope` | `global` | acceptable |
| `start_offset` | `second` | classic Alt+Tab — acceptable |
| `wrap` | `true` | acceptable |
| `lock_history_on_session` | reserved / warn-once | n/a (ADR-021 made lock-in unconditional) |
| `restore_focus_on_cancel` | `false` | cancel returns to pre-cycle focus via snapshot semantics — acceptable |
| `external_socket` | empty ⇒ `ui=external` behaves as `null` | M5 external overlay **unavailable** on Lua hosts |

The deliverability story (SPEC §11, ADR-022/023 bindings narrative) is fine on Lua hosts, but the **configuration story is hyprlang-only today**. `docs/USER.md` currently presents `examples/mru-switcher.conf` without flagging that Lua-backend users cannot apply any of it.

## 6. Related host quirks (same session, same shim family)

- `hyprctl dispatch mru:status` fails: the hyprctl→Lua shim forwards the argument unquoted (`mru:status` → Lua syntax error). Our binds call `hl.plugin.mru.*` directly, so only the CLI status leg is affected.
- `hyprctl eval` prints only `ok` and swallows return values; ground truth requires side-effect probes (write-to-file from Lua) or `getoption`.
- `hl.config` validation errors surface **asynchronously, outside `pcall`** (the validator runs after the call returns) — same class as the swallowed return value.

## 7. Live-host state after the investigation (recovery notes)

- Test lines added to `~/.config/hypr/looknfeel.lua` during §9.3 were **fully reverted** (file back to pre-test content; test `.bak` removed); `hyprctl configerrors` → empty.
- Plugin was unloaded/reloaded during `hl.plugin.load` probes; restored via `hyprctl plugin load /var/cache/hyprpm/code_warlord/mru-switcher/mru-switcher.so` — `plugin list`: v0.4.0 present, `configerrors` empty, `ui` at default.
- `~/.config/hypr/bindings.lua` still carries the working B2 poll recipe (backup: `bindings.lua.bak.20260922-013054`).
- No visual feedback is possible on this host until a config channel exists; the bindings behavior itself is correct.

## 8. Options / next steps

1. **Upstream research** (skills: `research` + `hyprland-plugin`): how is the Lua backend *supposed* to expose plugin special categories on/after 0.56.2 (`plugin {}` support on newer builds? a planned `hl.*` API?). Then file an upstream issue with this report as evidence. *Not started.*
2. **Docs honesty pass** (touches the DIST narrative → `plugin-spec-compliance` review; possibly `needs-adr`): COMPAT.md row "Lua config backend (0.56.2 efb5099): `plugin:mru-switcher:*` keys cannot be set — `hl.config`/config-file reject `plugin.*`, `hyprctl keyword` disabled; hyprlang hosts only"; USER.md caveat near Quick start / §3.
3. **Plugin-side mitigation** — none identified that respects our own constraints (keys registered only in `PLUGIN_INIT`; runtime config mutation host-blocked; `ui=external` config-gated too). A `mru:*` "set ui" dispatcher would contradict the frozen-grammar discipline → design gate + ADR required.
4. **Nest-verify on a newer Hyprland** whether Lua plugin blocks work there; if yes, document "Lua hosts: requires ≥ X.Y".
5. Still open from ADR-023 (unaffected): empirical `bindrt` nest check on this pin (hyprlang leg), then M6-T9 tag — human-gated.

## 9. Evidence appendix

**§9.1 — keyword disabled**
```
$ hyprctl keyword plugin:mru-switcher:ui border
keyword can't work with non-legacy parsers. Use eval.
```

**§9.2 — hl.config rejects plugin keys (via `hyprctl eval` shim)**
```
$ hyprctl eval 'hl.config({ plugin = { ["mru-switcher"] = { ui = "border" } } })'
error: …: unknown config key 'plugin.mru-switcher.ui'
$ hyprctl eval 'hl.config({ ["plugin:mru-switcher:ui"] = "border" })'   # pcall ok=true, then:
error: …: unknown config key 'plugin.mru-switcher:ui'
# control: hl.config({ general = { gaps_in = 0 } }) → OK, applied (getoption set: true)
```

**§9.3 — real config file + reload (decisive experiment)**
```
# appended to ~/.config/hypr/looknfeel.lua:
hl.config({ plugin = { ["mru-switcher"] = { ui = "border", border_color = "0xffffd9a0" } } })
$ hyprctl reload && hyprctl configerrors
/home/code_warlord/.config/hypr/looknfeel.lua:57: unknown config key 'plugin.mru-switcher.border_color'
/home/code_warlord/.config/hypr/looknfeel.lua:57: unknown config key 'plugin.mru-switcher.ui'
$ hyprctl getoption plugin:mru-switcher:ui     # → str: null, set: false
# lines reverted afterwards; configerrors clean again
```

**§9.4 — API surface ground truth** (dumped via `dofile` probe writing to /tmp; the shim swallows return values):
- `hl.keyword` = **nil**; present as functions: `hl.config`, `hl.get_config`, `hl.plugin.load`, `hl.is_key_down`, `hl.timer`, `hl.bind`, `hl.unbind`, `hl.on`, `hl.dispatch`, `hl.env`, `hl.get_loaded_plugins`, `hl.plugin.mru.{cycle,apply,cancel,status}` (full dump preserved in session notes).
- `hl.get_config("general:gaps_in")` → `8` (core read path works); `hl.get_config("plugin:mru-switcher:ui")` → `nil`.
- `hl.plugin` namespace dump: `mru = table`, `load = function`; `hl.plugin.mru`: `cycle`, `apply`, `cancel`, `status`.

**§9.5 — hl.plugin.load probes** (pcall, results captured to /tmp file):
```
load() no args / load(nil) / non-string first arg → arity errors (signature: load(path: string, config?: table))
load("/var/cache/hyprpm/code_warlord/mru-switcher/mru-switcher.so")   → ok=true, plugins=0 (silent fail)
load("…same…", { ui = "border" })                                     → ok=true, plugins=0 (silent fail)
load on already-loaded path (earlier probe, eval shim)                → ok, no-op (count unchanged)
hyprctl plugin load "…same path…" (immediately after)                 → loads fine, v0.4.0
```

**§9.6 — Omarchy `o.*` dump:** `window, bind, bind_toggle, launch, launch_on_start, launch_sole, launch_webapp, launch_webapp_sole, notify, shell_quote, shell_succeeds, cmd_present, cmd_missing, preinstalled_bindings_enabled, exec_on_start` — no config/plugin facility.

**§9.7 — load-path discovery:**
```
$ grep mru /proc/$(pgrep -x Hyprland)/maps | awk '{print $NF}' | sort -u
→ /var/cache/hyprpm/code_warlord/mru-switcher/mru-switcher.so
$ hyprpm list → Repository mru-switcher (by code-warlord-dev): enabled: true
  (invariant under HOME=… and XDG_RUNTIME_DIR=… overrides ⇒ live state, not disk)
$ cat /var/cache/hyprpm/code_warlord/mru-switcher/state.toml   # see §4
$ md5sum /var/cache/hyprpm/code_warlord/mru-switcher/mru-switcher.so
6d6e3c21ce894db0740cf71decff0d03
```

## 10. See also

- `docs/SPEC.md` §4 (config surface + defaults), §11 (deliverability note)
- `docs/DECISIONS.md` — ADR-011 (UI default null), ADR-022 / ADR-023 (bindings model)
- `docs/COMPAT.md` — nested-tested rows (hyprlang host); the Lua-backend row is the §8.2 candidate
- `docs/agent-state/SESSION.md`, `docs/agent-state/PROGRESS.md` — link back to this report
- `docs/agent-state/research/2026-09-20-lua-modifier-release-bisect.md`, `docs/agent-state/research/2026-09-21-modifier-hold-poll-api.md` — Lua-host API behavior on the binds path (which **does** work)

