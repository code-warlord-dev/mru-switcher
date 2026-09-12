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
| Symlink swap on socket path | Document; prefer `SOCK_CLOEXEC` + mode `0600` on user runtime dir |
| Log sensitive titles | Default log without titles; opt-in |
| DoS via log spam | Rate limit |
| DoS via overlay flood | Backpressure: drop/coalesce; never block compositor loop |
| Supply chain | SBOM at 1.0; pin Hyprland; reproducible recipe |

## External protocol minimum controls (M5)

- Max message size (e.g. 64 KiB)  
- Protocol version negotiation; reject unknown major  
- Timeouts on write; non-blocking I/O  
- Fuzz parser in CI before 1.0  
- Socket permissions `0600`, directory `0700`  

## Out of scope

Sandboxing in-process plugins (impossible without compositor redesign).
