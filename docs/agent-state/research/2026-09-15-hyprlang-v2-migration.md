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

## Open items for the migration plan

1. Keep `clamp_debounce_ms` manual, or fold into `.min/.max` options (min/max clamp or reject?). REQ-CFG-004 says clamp; decide whether `SIntValueOptions.min/max` rejects or clamps, then match SPEC.
2. Enum validator vs fallback+warn: validator changes *user-visible error* at config parse; current behavior is fallback + once-warn. Align to REQ-CFG-001.
3. `underlying()`/`load` of `Config::STRING` reads: `CStringValue::value()` returns `Config::STRING` (const char*); note `CConfigValue<std::string>` has NO `ptr()` (RASSERTs) — do not try to take a `std::string` pointer.
4. Lua config backend: key names under Lua are mangled (`plugin.mru_switcher.*`, dot-separated — virtual-desktops#124 documented a name-mangling mismatch). Decide whether M3 supports both backends or legacy-hyprlang only (SPEC currently says nothing; add a line in API.md/USER.md if legacy-only).