# Observability and diagnostics

**Status:** Target for M2+ (levels designed in M1 domain as no-op hooks)  
**Related:** SPEC `mru:status`, SECURITY.md, AGENTS token economy

## Log levels

| Level | Use |
|-------|-----|
| `error` | Focus failure, unload races prevented, backend init failure |
| `warn` | UI fallback, hash mismatch (then abort), invalid config enum fallback |
| `info` | Session start/end, plugin load/unload |
| `debug` | Cycle index, prune counts, debounce schedule/cancel, scope resolution |
| `trace` | Per-event focus reason (debug builds only) |

Config (future key, optional): `plugin:mru-switcher:log_level = warn`

## Session correlation

Each Active session gets a monotonic `session_id` (uint64). All debug lines for that session include `session_id=N`.

## Structured fields (debug)

```text
event=session_start session_id=3 scope=global size=5 index=1
event=prune session_id=3 removed=2 size=3 index=1
event=apply session_id=3 result=applied address=0x… gen=2
event=debounce_schedule ref=… delay_ms=400
event=debounce_cancel reason=replace|unload|invalid
event=focus_fail reason=stale_generation|missing
```

Prefer single-line key=value for nest log grepping. No PII beyond window class/title if explicitly enabled.

## `mru:status`

Informative v0.x string. Verbose form (optional arg `verbose`):

```text
active=true session_id=3 index=1 size=4 scope=global ui=null pending_debounce=false pruned_total=2
```

Do not parse strictly until 1.0.

## Rate limiting

Repeated identical `warn`/`error` (same key) at most once per N seconds (default 5) to avoid log storms under FFM.

## Debug builds

Compile definition `MRU_DEBUG=1`: assertions on domain invariants (index bounds, single session, no focus in cycle path in tests).
