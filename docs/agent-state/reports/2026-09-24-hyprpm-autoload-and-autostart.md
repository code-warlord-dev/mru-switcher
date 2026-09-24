# hyprpm autoload + autostart on the live Lua/Omarchy host (2026-09-24)

**Scope:** why the plugin is missing after a reboot, and what the correct fix is.
Host: Hyprland **0.56.2** (`efb5099`), Lua config (Omarchy), plugin installed via hyprpm
(`/var/cache/hyprpm/code_warlord/mru-switcher/mru-switcher.so`, v0.5.0).

## Symptom

`hyprctl plugin list` → `no plugins loaded` in a fresh session (user report), although
`hyprpm enable`'s intent was set in an earlier session. Alt+Tab then raises Lua errors
(`bindings.lua` calls `hl.plugin.mru.*` directly).

## Findings (all verified on the host, read-only except where noted)

1. **Nothing persists between sessions.** `hyprpm enable` only records `enabled` in its state;
   the load is performed by `hyprpm reload`. Nothing runs it at login.
2. **`hyprpm enable` needs root, and hyprpm refuses to run as root.** `hyprpm/src/helpers/Sys.cpp`
   declares `SUPERUSER_BINARIES = {sudo, doas, run0}`; `hyprpm/src/core/DataState.cpp:30,37`
   writes the state to `$XDG_RUNTIME_DIR/hyprpm/.temp-state` and then
   `NSys::root::install(tmp, target, "644")` → `sudo install -m644 -o 0 -g 0 …`.
   Observed: `✖ Failed to write plugin state` (state store was root-owned), journal shows
   `pam_unix(sudo:auth): conversation failed` (non-interactive sudo), and
   `pkexec hyprpm enable …` → `[ERR] Don't run hyprpm as a superuser.`
   The staged temp file already contained `enabled = true` — only the root install step failed.
3. **`hyprpm reload` is root-free.** `hyprpm/src/main.cpp` `reload` branch calls only
   `ensurePluginsLoadState()`: it reads state, queries `j/plugins list` over the Hyprland socket,
   unloads disabled plugins and loads enabled ones (`PluginManager.cpp:949+`). No state write, no
   `cacheSudo()`. This is the only command that must run on every login.
4. **The Lua API on this build has no `hl.exec_once`.** `/usr/share/hypr/stubs/hl.meta.lua`
   exposes `hl.exec_cmd`, `hl.exec_raw` and `hl.on(event, cb)` with event `hyprland.start`;
   Omarchy's own `/usr/share/omarchy/default/hypr/autostart.lua` and `helpers.lua:112`
   (`o.exec_on_start`) use `hl.on("hyprland.start", …)`. A copy-pasted `hl.exec_once(...)` would be
   a nil call at config load — documented so nobody repeats it.

## Actions taken

- `~/.config/hypr/autostart.lua` (user config): appended
  `hl.on("hyprland.start", function() hl.exec_cmd("hyprpm reload -n") end)`
  — backup `autostart.lua.bak.20260924-090758`, diff reviewed, `hyprctl reload` clean
  (`hyprctl configerrors` empty).
- `pkexec chown -R code_warlord:code_warlord /var/cache/hyprpm` (human-approved) so the cache
  directories are user-writable for future `hyprpm update`/builds. Note: state files are
  re-installed as `root:root 0644` by hyprpm on the next state write — that is by design.
- Docs: README "Part 4 — load the plugin on every login (hyprpm)" + reboot troubleshooting;
  USER.md Quick-start section, "If the plugin is gone after a reboot" recovery and FAQ entry;
  COMPAT rows for both host facts.

## Remaining (human, one command)

`hyprpm enable mru-switcher` must be run **from a real terminal** (hyprpm runs its own sudo and
prompts for the password). After that, `hyprpm reload -n` loads the plugin, and the autostart line
keeps doing it at every login. Verification: `hyprpm list` → `enabled: true`,
`hyprctl plugin list` → `mru-switcher`.

## Notes / non-issues

- An `LD_PRELOAD` file-syscall shim used for diagnosis crashed **its own** `hyprpm` process
  (recursive `fopen` interception) — the compositor was unaffected; no plugin or session state was
  lost. The plugin was unloaded/reloaded once on purpose to prove the load path
  (`hyprctl plugin unload … ` → `hyprpm reload -n`) and is loaded in the live session.
- `hyprpm update` is **not** required here: the pinned build already matches the running headers
  (`efb5099`, version 0.5.0). The post-1.0 rebuild will pick up the new author attribution
  (`Yuriy Tretyakov (code-warlord-dev)`) once the pin is refreshed at the `v1.0.0` tag.
