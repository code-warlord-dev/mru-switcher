# Threat model (plugin)

**Status:** Draft for 1.0 readiness  
**Related:** SECURITY.md, SPEC Appendix B (external UI)

## Assets

- Compositor process integrity  
- User input focus (which window receives keys)  
- Window titles/classes in logs/overlay  
- Local session credentials indirectly via focused apps  

## Trust boundaries

| Boundary | Trust |
|----------|--------|
| Plugin `.so` | Fully trusted — equivalent to compositor code |
| Hyprland core | Trusted |
| External overlay process (M5) | **Untrusted peer** |
| hyprctl callers (same user) | Same user trust |
| Network | Plugin must not expose network listeners by default |

## Threats and mitigations

| Threat | Mitigation |
|--------|------------|
| Malicious plugin binary | Only load self-built/signed artifacts; hash check does not replace trust |
| Stale address focuses wrong app | WindowRef generation (ADR-013) |
| Overlay sends crafted messages | Size limits, version field, schema validation, ignore unknown; no shell exec |
| Path traversal via `external_socket` | Absolute path allowlist under runtime dir; reject `..` |
| Symlink swap on socket path | Stale-socket cleanup removes only `lstat`-confirmed sockets (a swapped path fails the bind → REQ-O-001 null degrade); plugin **enforces mode `0600`** on the bound socket file (`fchmod`); parent-directory protection is operator responsibility (`0700` runtime dir) |
| Log sensitive titles | Default log without titles; opt-in |
| DoS via log spam | Rate limit |
| DoS via overlay flood | Backpressure: drop/coalesce; never block compositor loop |
| Supply chain | SBOM at 1.0; pin Hyprland; reproducible recipe |

## External protocol minimum controls (M5)

- Max message size (`kMaxLineBytes` = 64 KiB; oversized/incomplete line → dropped, REQ-O-005)
- Protocol version negotiation; reject unknown version
- Timeouts on write; non-blocking I/O
- **Socket file mode `0600` — enforced by the plugin** (`fchmod` on the bound listener fd after `bind()`; failure is non-fatal and logged)
- **Parent-directory protection — operator responsibility**: run the socket under a `0700`-permission directory (e.g. `$XDG_RUNTIME_DIR`)
- **Fuzz parser in CI** — `T-FUZZ-01`: `LLVMFuzzerTestOneInput` harness over `overlay_protocol::parse_command` with deterministic generated/mutated inputs; runs as a ctest under every CI job incl. the ASan/UBSan `sanitize` job  

## Out of scope

Sandboxing in-process plugins (impossible without compositor redesign).
