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

Payload format is **frozen since the M6-T1 contract freeze** (SPEC §0, §3.4):

```text
active=true index=2 size=5 scope=global session=3 last_end=none
```

Keys in fixed order: `active= index= size= scope= session= last_end=`
(`last_end=` is `Applied|UserCancel|NoWindows|InvalidSelection|FocusFailed|PluginShutdown|none`).
Additional keys may be appended in minor releases — tolerate unknown keys; the frozen set above is stable.
There is no `verbose` argument (`mru:status` takes no args) and no `pending_debounce`/`pruned_total`/`session_id=` keys.

## Rate limiting

Repeated identical `warn`/`error` (same key) at most once per N seconds (default 5) to avoid log storms under FFM.

## Diagnosing stuck state

### Stuck border highlight
Symptoms: window keeps the highlight colour/size after session end or plugin unload.

1. `hyprctl getoption plugin:mru-switcher:ui` — confirm current backend.
2. End any active session: `hyprctl dispatch mru:cancel` (or apply).
3. If still stuck after graceful unload: `hyprctl plugin unload <path>` then reload, or restart Hyprland.
4. Abrupt kill (FM-22) may leave overrides until compositor restart — this is a documented gap; restore-by-value only covers graceful paths.

### Stuck / unexpected session
1. `hyprctl dispatch mru:status` — read `active=`, `session=`, `index=`, `last_end=`.
2. If `active=1` and you are not holding the switcher: `hyprctl dispatch mru:cancel`.
3. Check Hyprland log for plugin warn/error lines (hash mismatch, UI degrade, socket bind failure).

## Debug builds

Compile definition `MRU_DEBUG=1`: assertions on domain invariants (index bounds, single session, no focus in cycle path in tests).
