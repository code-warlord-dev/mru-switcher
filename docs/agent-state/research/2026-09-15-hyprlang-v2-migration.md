# hyprlang V1 → V2 config API migration — pin v0.56.2 (efb5099)

Date: 2026-09-15
Status: research memo for `feat/m3-config-v2` (ADR-016 __6__, Q5); feeds planning only, no code yet.
Source: pinned Hyprland headers `/usr/include/hyprland/…`, upstream hyprwm/hyprland-plugins examples, hyprland-virtual-desktops#124 migration PR.

## Why migrate (V1 is a dead path)

- `HyprlandAPI::addConfigValue` and `getConfigValue` are `[[deprecated]]` AND return `false`/`nullptr` when the config manager is **not** `CONFIG_LEGACY` — i.e. they silently no-op on the Lua config backend. V2 (`addConfigValueV2`) is the only path that works on *both* legacy-hyprlang and Lua config.
- Our only runtime config bug (#3, `std::bad_any_cast` via `dataPtr()` on STRING in hyprlang 0.6.x) lives in this exact layer; migrating removes the manual type-cast path entirely.
- M2 deliberately pinned V1 (plan Task 6 Step 3); M3 migrates per ADR-016 __6__ before scope work.

## V1 → V2 mapping of what we use today

| Concern | V1 (current: `mru_plugin.cpp:43-126`) | V2 (pinned headers) |
|---|---|---|
| Register config value | `HyprlandAPI::addConfigValue(PHANDLE, "plugin:mru-switcher:debounce_ms", Hyprlang::CConfigValue{INT(400)})` | `HyprlandAPI::addConfigValueV2(PHANDLE, Config::Values::makeConfigValue<Config::Values::Int>("plugin:mru-switcher:debounce_ms", desc, 400, {.min=0,.max=5000}))` (PluginAPI.hpp:348) |
| Read config value | `HyprlandAPI::getConfigValue(PHANDLE, key)` → `Hyprlang::CConfigValue*` → `dataPtr()` (cast) / `getDataStaticPtr()` (string) | **No `getConfigValueV2` exists.** Registration returns `bool`; you **keep** the `SP<Config::Values::IValue>` you handed to `addConfigValueV2` and read via `value->value()` / `->defaultVal()`. |
| Value object types | `Hyprlang::INT`, `Hyprlang::STRING`, `Hyprlang::CConfigValue` variant | typed classes under `Config::Values`: `Int`, `String`, `Bool`, `Float`, `Color`, `Vec2`, `CssGap`, `FontWeight`, `Gradient` (ConfigValues.hpp:12-57) |
| Options / constraints | none (clamp done manually: `clamp_debounce_ms`) | `SIntValueOptions{min,max,map,refresh}`, `SStringValueOptions{validator,refresh}`, `SBoolValueOptions{refresh}` — validator is `std::function<std::expected<void,std::string>(const Config::STRING&)>` |
| Bool semantics | stored as INT 0/1, cast at read | dedicated `Config::BOOL` (`CBoolValue`) |
| Enum strings (`default_scope`, `start_offset`, `ui`) | string + manual `parse_*` | `SStringValueOptions.validator` can reject unknown tokens at parse time, or keep current fallback+warn approach |
| Reload freshness | pointer cached at init; re-read on `config.reloaded` event | `IValue::commence()` called by `addConfigValueV2`; `CConfigValueBase::flushCaches()` on reload; keep the same SP and read `value()` (stays fresh) |
| Error visibility | getConfigValue may return nullptr | `addConfigValueV2` returns `false` and logs `"failed to register plugin value”`; `underlying()` gives `std::type_info` for the value |

## The HIGH-4 critical difference (README_CRITICAL)

- V1 threw `std::bad_any_cast` (catchable) on the STRING read — our `guarded()`/try-catch could absorb it.
- V2 **replaces exceptions with `RASSERT` → `raise(SIGABRT)`** (macros.hpp:50-58) on a type mismatch inside `CConfigValue<T>::operator*` (ConfigValue.hpp:74-76, 86-87: "on a FUCKED type"). **Not catchable.**
- Consequence for the migration PR (issue #14 req. 2): the new read path must guarantee validity by contract — keep the exact typed `SP<T>` you registered, rely on `operator*`/`value()` only on the same `Config::*` type, and audit every `*->value()` call site so no user-typed config value can reach a mismatched accessor (legacy config strings are typed by hyprlang at parse, but the plugin must not re-interpret).
- These guarantees must sit *inside* the existing HIGH-4 barriers (`guarded()` dispatchers, `guarded_listener()` on `config.reloaded`, try/catch in PLUGIN_INIT) — the barriers stay, and the migration adds a type-contract layer so nothing can SIGABRT across the C ABI.

## Value read pattern to adopt (upstream-confirmed)

```cpp
// PLUGIN_INIT:
auto debounce_ms_v = Config::Values::makeConfigValue<Config::Values::Int>(
    "plugin:mru-switcher:debounce_ms", "…", 400, {.min = 0, .max = 5000});
if (!HyprlandAPI::addConfigValueV2(PHANDLE, debounce_ms_v)) { /* abort init */ }
// store SP in plugin state; read later:
const auto ms = debounce_ms_v->value();
```

Confirmed by hyprland-virtual-desktops#124 (`SConfig` container + `config.*->value()`) and borders-plus-plus.

## RASSERT attribution — is SIGABRT triggered by user config or our bug? (STOP-POINT answer)

Authored after reading pinned sources: `legacy/ConfigManager.cpp` @efb5099 (2184 lines) + `ConfigValue.cpp` @efb5099 + installed headers (`ConfigValue.hpp`, `IntValue.cpp`).

**Verdict: on pin v0.56.2 legacy-hyprlang backend, a user's wrong-typed config line CANNOT reach `RASSERT → SIGABRT`. The `RASSERT` in the V2 type-mismatch path is reachable only from a plugin programming error — never from config content.**

Chain of proof:

1. **`registerPluginValue` maps V2 typed values to hyprlang slots by RTTI, not by user input.** Legacy `ConfigManager.cpp:2153-2184`: `CIntValue→Hyprlang::INT`, `CFloatValue→FLOAT`, `CBoolValue→INT(0/1)`, `CStringValue→STRING`, plus VEC2/custom for the rest; anything unknown returns `std::unexpected("unknown value type")` — **no RASSERT**. So the stored hyprlang type is fixed at *registration*, from our `IValue` class.
2. **The user's token does not change the slot type.** `plugin { … }` lines parse into the already-registered typed slot; hyprlang with `SConfigOptions{.throwAllErrors = true}` (line 486) turns a wrong-typed token into a *parse error*, keeping the registered default. It never stores a value of a different type under that key.
3. **`getConfigValue` returns only registered slots.** Legacy `ConfigManager.cpp:1169-1184`: for `plugin:` values it reads `m_config->getSpecialConfigValuePtr("plugin", …)`; if unregistered → `return {}` (null `dataptr`). The type reported is `VAL->getValue().type()` — the **registered** type, always one of the `Hyprlang::*` we chose at registration.
4. **`m_typeIndex` therefore always equals our registered type.** `local__configValuePopulate` (`ConfigValue.cpp` @efb5099) sets `m_typeIndex = std::type_index(*BIGP.type)` from `Config::mgr()->getConfigValue(val)`. Since `Hyprlang::INT` = `int64_t` = `Config::INTEGER` (hyprlang.hpp:30, Types.hpp:16), the typed accessor `CConfigValue<Config::INTEGER>::operator*` matches (`ConfigValue.hpp:81-89`) *when the plugin reads the same type it registered*.
5. **The only `RASSERT` in the legacy config manager is constructor-internal** (line 513, over compositor's own static `CONFIG_VALUES`) — an internal invariant, not user-config-reachable, and not in the plugin registration path.
6. **`RASSERT` fires only when plugin code mismatches its own accessor** — e.g. registering `String` but reading via `CConfigValue<Config::INTEGER>`, or calling `CConfigValue<std::string>::ptr()`. Those are *our* bugs, guardable by the type-contract layer (issue #14 req. 2), not by anything a user can type.

**Residual footguns to keep out of the migration contract (all programmer-side):**
- `CConfigValue<std::string>::ptr()` unconditionally RASSERTs (`ConfigValue.hpp:63-67`) — never take a string pointer. Read via `value()`/`operator*` (handles `const char*` → `std::string`).
- Reading a **non-registered** key through any `CConfigValue<T>`: `BIGP.dataptr` is null → `local__configValuePopulate` RASSERT. Mitigation: only read via the exact `SP<IValue>` that `addConfigValueV2` returned `true` for; never re-derive key names at read time.
- This analysis is for the **legacy-hyprlang** backend (default for hyprpm/`.conf`). The Lua backend funnels through a different implementation; M3 is legacy-only per user decision (see Open items #4). If Lua support is ever added, re-run attribution with the Lua config-manager source before relying on the same contract.

**Migration-conclusion:** the HIGH-4 barrier stays (`guarded()` / `guarded_listener()` / PLUGIN_INIT try-catch abort), but the *reason* becomes belt-and-suspenders, not the last line of defense: the primary safety is the **type-contract** (register and read the same `Config::*` type, always through the stored SP). No user-typed config value can SIGABRT the compositor on this pin.

## Open items for the migration plan

1. Keep `clamp_debounce_ms` manual, or fold into `.min/.max` options (min/max clamp or reject?). REQ-CFG-004 says clamp; decide whether `SIntValueOptions.min/max` rejects or clamps, then match SPEC.
2. Enum validator vs fallback+warn: validator changes *user-visible error* at config parse; current behavior is fallback + once-warn. Align to REQ-CFG-001.
3. `underlying()`/`load` of `Config::STRING` reads: `CStringValue::value()` returns `Config::STRING` (const char*); note `CConfigValue<std::string>` has NO `ptr()` (RASSERTs) — do not try to take a `std::string` pointer.
4. Lua config backend: key names under Lua are mangled (`plugin.mru_switcher.*`, dot-separated — virtual-desktops#124 documented a name-mangling mismatch). Decide whether M3 supports both backends or legacy-hyprlang only (SPEC currently says nothing; add a line in API.md/USER.md if legacy-only).