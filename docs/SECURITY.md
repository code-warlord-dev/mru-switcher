# Security considerations

**Status:** Release pass for 1.0 (M6-T4)  
**Related:** [THREAT-MODEL.md](THREAT-MODEL.md) (assets, trust boundaries, threat table), [OBSERVABILITY.md](OBSERVABILITY.md) (security-relevant logging), [SUPPORT-AND-RELEASE.md](SUPPORT-AND-RELEASE.md) (release checklist, artifact checksums), SPEC §12 Appendix B (external protocol), [HYPRLAND-PLUGIN-SYSTEM.md](HYPRLAND-PLUGIN-SYSTEM.md) §1/§7 (in-process model, permissions)

This document states the **release trust model** for `mru-switcher`. It complements the
asset- and attacker-focused [THREAT-MODEL.md](THREAT-MODEL.md): claims here are cross-checked
against it, not duplicated. Everything below describes what the shipped plugin does — this pass is
documentation-only and implies no behavior change.

## Trust model

A Hyprland **plugin runs inside the compositor process** (`dlopen` into Hyprland, not a separate
process — [HYPRLAND-PLUGIN-SYSTEM.md](HYPRLAND-PLUGIN-SYSTEM.md) §1). It therefore carries the
**full privileges of the compositor**: input state, window surfaces and metadata of all clients,
and every API Hyprland itself can call. There is no privilege boundary between the plugin and the
compositor. Loading a plugin is equivalent to trusting that code as compositor code
([THREAT-MODEL.md](THREAT-MODEL.md) trust boundaries: “Fully trusted — equivalent to compositor
code”).

**Consequence — load only trusted code.** Do not load plugins from untrusted sources. Prefer
building from source you control, pinned to the Hyprland revision you run (hyprpm `commit_pins`,
[SUPPORT-AND-RELEASE.md](SUPPORT-AND-RELEASE.md)).

### Distribution trust (1.0)

- **Supported channels for 1.0:** build from source (pinned, documented recipe) and — when binary
  artifacts are published — **hash-verified artifacts** whose checksums ship with the release
  ([SUPPORT-AND-RELEASE.md](SUPPORT-AND-RELEASE.md) release checklist: “Checksums for artifacts”).
- **Do not download and load prebuilt `mru-switcher.so` (or any plugin `.so`) from untrusted
  remotes, third-party mirrors, or pasted links.** A hostile `.so` in-process owns the session; no
  runtime mechanism in this project can contain it (see Non-goals).
- The **header hash check** protects against wrong-ABI *accidents*, not malicious content: a
  malicious rebuild carries the correct hash by construction
  ([THREAT-MODEL.md](THREAT-MODEL.md): “hash check does not replace trust”).

## Attack surface

Each row: exposure → threat vector → measure. The asset side (what an attacker gains) is
[THREAT-MODEL.md](THREAT-MODEL.md) §Assets; per-boundary trust is its §Trust boundaries.

| # | Surface | Exposure | Threat vector | Measure |
|---|---------|----------|---------------|---------|
| 1 | **Plugin `.so` in-process** (`src/plugin/*`) | Same process and privileges as Hyprland; code runs on the compositor thread | Malicious or tampered binary at load time (supply chain, or `plugin =` pointing at a writable path an attacker can replace) | Load from trusted sources only (trust model above); pin Hyprland; fail-closed init checks (row 2); compositor-thread-only execution — no compositor-touching background threads ([HYPRLAND-PLUGIN-SYSTEM.md](HYPRLAND-PLUGIN-SYSTEM.md) §6) |
| 2 | **Header hash check** (`PLUGIN_INIT` → `hash_ok()`, `mru_plugin.cpp`) | First gate of every load | Plugin compiled against different Hyprland sources than the running binary → wrong-ABI calls into compositor internals, arbitrary corruption or crash | `__hyprland_api_get_hash()` vs `__hyprland_api_get_client_hash()` compared in `PLUGIN_INIT`; on mismatch init **throws and Hyprland refuses the load** — fail closed, no partial registration (SPEC REQ-ERR-003); surfaced per [OBSERVABILITY.md](OBSERVABILITY.md) (`warn`, then abort). Limits: catches mismatch *accidents*, not a malicious build — see Distribution trust |
| 3 | **AF_UNIX overlay socket** (`ui = external`, opt-in; `overlay_socket_server.*`) | Local IPC: the plugin binds at `plugin:mru-switcher:external_socket`; first accepted client is the overlay, extra connections are closed | Malicious local process connects and floods/sends crafted lines; stale-socket or symlink swap on the path | Peer is an **untrusted local process** ([THREAT-MODEL.md](THREAT-MODEL.md)). Controls: 64 KiB line cap (`kMaxLineBytes`, REQ-O-005); version+schema validation, malformed input **ignored**, never applied (REQ-O-005, T-O-05); `select` index bounds-checked against the snapshot (REQ-O-004, T-O-02) — a peer can move the virtual selection at most, it cannot focus (T-F-01: `mru:cycle`/peer path never calls FocusGateway; focus only on `apply`, which the peer can trigger only as the same `mru:apply` the user could dispatch anyway); `SOCK_CLOEXEC` + non-blocking fds on listener and client; stale-socket cleanup removes only a path that `lstat` confirms is a socket, so a misconfigured path to a regular file is never destroyed; bind failure degrades to `ui = null` (REQ-O-001) — no partially-initialized surface; recommendation: keep the path under `$XDG_RUNTIME_DIR` (directory `0700`, socket `0600` by runtime-dir policy) so an unrelated local user cannot reach the listener |
| 4 | **Config values** (hyprlang, namespace `plugin:mru-switcher:`) | Parsed from the user's own `hyprland.conf` / `hyprctl keyword`; typed values registered via `addConfigValueV2` **only inside `PLUGIN_INIT`** (host enforces both the init window and the namespace) | Malicious config file — already arbitrary-code-equivalent for the user account (`exec-once` etc.), so the plugin does not elevate config trust; unsafe string values, e.g. `external_socket` outside the intended runtime dir; unknown enum values | Config is user-owned input; the plugin trusts it as far as the user's own privileges reach and never elevates it. Keys registered **only** under `plugin:mru-switcher:` and only in init; `external_socket` is length-checked (AF_UNIX `sun_path` 108-byte limit incl. NUL) and used only to bind/unlink the socket — no other file operations; invalid enum values fall back to documented defaults (unknown `ui` → `null`, unknown `border_style` → `solid`; T-UI-01/07) instead of aborting |
| 5 | **External overlay protocol framing** (SPEC §12 Appendix B; `overlay_protocol.cpp`) | Newline-framed UTF-8 JSON, plugin→peer and peer→plugin, one flat object per line | Oversized or unterminated peer line (memory/CPU pressure on the compositor thread); control bytes in `title`/`class` breaking framing or the peer's rendering; escape-sequence abuse in the peer's JSON | Plugin→peer: every string JSON-escaped, control bytes < 0x20 emitted as `\u00XX` — no raw newlines inside strings, framing unambiguous; peer→plugin: bounded buffers (4 KiB `recv` chunks into the 64 KiB-capped buffer); one-line rejection contract — unknown version/`type`, unsupported escapes, negative index, oversized or unterminated input → **whole line dropped** (`nullopt`, REQ-O-005), no partial application; logged at debug only; backpressure = drop, compositor loop never blocks ([THREAT-MODEL.md](THREAT-MODEL.md) “DoS via overlay flood”) |

Surfaces intentionally **not** table rows: compositor events (`Event::bus` — typed signals from the
trusted host, no attacker-controlled bytes; lifetimes per
[HYPRLAND-PLUGIN-SYSTEM.md](HYPRLAND-PLUGIN-SYSTEM.md) §5.3) and dispatcher args (user's own
keybinds/hyprctl — same trust level as config; defensively parsed per REQ-DISP-*). Their
attacker-relevant reach is covered by rows 3–5.

## Recommendations

1. Ship and load only self-built (or checksum-verified) binaries matched to the Hyprland pin.
2. Keep the external UI **optional**; the default UI backend is `null`. Enabling `ui = external`
   opts into the local-socket surface (row 3).
3. Keep `external_socket` inside the user's runtime directory
   (`$XDG_RUNTIME_DIR`, typically `/run/user/<uid>`), not in shared or world-writable paths.
4. Never execute peer-provided strings as commands. The plugin itself does not and must not.
5. On header mismatch, refuse to load — enforced fail-closed in `PLUGIN_INIT`.

## What security logging exists

Plugin-side security-relevant events are logged per [OBSERVABILITY.md](OBSERVABILITY.md):
hash mismatch (`warn`, then abort), UI/transport degradation (`warn`), dropped oversized peer lines
and rejected peer commands (debug, plus the `dropped_lines_` counter per REQ-O-002/005). Window
titles/classes are not logged by default and are opt-in only. That document owns the log-level
contract; this document does not duplicate it.

## Reporting issues

If you believe you found a security-relevant issue (compositor crash, focus redirected to an
attacker-controlled window, socket or config abuse), please **do not post full exploit detail
publicly**:

1. Preferred: open a **private security advisory** via GitHub “Report a vulnerability”
   (Security → Advisories) on `code-warlord-dev/mru-switcher` — private by default.
2. If private reporting is unavailable, file a **minimal** public issue (affected version,
   Hyprland pin, repro steps) marked `security`, without exploit payload; the maintainer will take
   the thread private.
3. Severity is triaged on the project scale in
   [SUPPORT-AND-RELEASE.md](SUPPORT-AND-RELEASE.md) (S0 compositor crash → immediate hotfix
   branch; S1 wrong focus → next patch).

Fixes ship per that document's release checklist. Coordinated-disclosure timelines beyond the
above are out of scope for this document.

## Non-goals

This project does **not** provide:

- **Sandboxing of the plugin.** An in-process plugin cannot be meaningfully sandboxed without
  compositor-level support for compartmentalized extensions (Hyprland has none);
  [THREAT-MODEL.md](THREAT-MODEL.md) lists this as out of scope as well.
- **A capability model or privilege separation** between plugin subsystems or between the plugin
  and the compositor. The load decision (rows 1–2) is the only trust boundary.
- **Defense against a peer process with equal or greater local privileges.** The socket controls
  (row 3) bound an *unprivileged same-user* peer; they are not a defense against root or a
  compromised user account.
- **Network hardening.** The plugin must not open network listeners; the only IPC surface is the
  opt-in AF_UNIX socket. Any network-facing feature would require a new ADR + threat-model
  update, not a patch here.

## Document history

| Date | Note |
|------|------|
| 2026-09-13 | Initial security notes (M0 docs baseline) |
| 2026-09-20 | Release pass (M6-T4): 5-row surface table with vectors/measures, distribution trust, reporting section, explicit non-goals, cross-links to THREAT-MODEL/OBSERVABILITY/SUPPORT-AND-RELEASE |
