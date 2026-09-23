#pragma once

#include <concepts>
#include <optional>
#include <stdexcept>
#include <string>
#include <typeinfo>

#include "config_value.hpp"
#include <hyprland/src/config/values/ConfigValues.hpp>
#include <hyprland/src/plugins/PluginAPI.hpp>

namespace mru::plugin::config {

// Typed holders for every plugin:mru-switcher: value registered via
// addConfigValueV2. Ownership lives here (in PluginState), not in statics:
// register_all() fills the slots once in PLUGIN_INIT; read_config() reads
// only through these SPs, never via the V1 config API or name re-lookup (RP-1).
struct Values {
    SP<Config::Values::Int> debounce_ms;
    SP<Config::Values::String> default_scope;
    SP<Config::Values::String> start_offset;
    SP<Config::Values::String> ui;
    // REQ-UI-008: border highlight surface (M4). border_color is a String, not a
    // Config::Values::Color: the value is passed verbatim to the `setprop` colour
    // grammar (hex 0xAARRGGBB / rgb() / rgba()), so it must not be normalized.
    SP<Config::Values::String> border_style;
    SP<Config::Values::String> border_color;
    SP<Config::Values::Int> border_size;
    // ADR-026 / REQ-UI-012: keep the selected window visible during a border
    // session (active-workspace elevation, no window focus); read by read_config().
    SP<Config::Values::Bool> selection_follow_workspace;
    // ADR-028 / REQ-UI-013: one full pulse throb cycle in ms (clamped [200, 10000]
    // in clamp_pulse_period_ms); effective only when border_style = pulse.
    SP<Config::Values::Int> pulse_period_ms;
    // ADR-028 / REQ-UI-014: dim strength for the non-selected ring windows
    // (clamped [0.0, 1.0] in clamp_dim_alpha); effective only when border_style = dim.
    SP<Config::Values::Float> dim_alpha;
    SP<Config::Values::Bool> wrap;
    // REQ-H-010 / ADR-021: reserved key — still registered (0.x configs keep
    // parsing) and still read so read_config() can warn once when it is `false`;
    // its value never reaches SessionPolicy (lock-in is mandatory, REQ-H-001/011).
    SP<Config::Values::Bool> lock_history_on_session;
    SP<Config::Values::Bool> restore_focus_on_cancel;
    // REQ-O-001 / ADR-018: `external_socket` consumed by read_config() (M5);
    // empty path means `ui=external` degrades to null.
    SP<Config::Values::String> external_socket;
};

// Compile-time type table: the slot member type is the single source of truth
// for the Config::* type a key was registered with, so a register/read mismatch
// fails to compile instead of RASSERTing at runtime (HIGH-4 belt-and-suspenders).
template <typename V> struct value_traits;

template <> struct value_traits<Config::Values::Int> {
    using type = Config::INTEGER;
};

template <> struct value_traits<Config::Values::String> {
    using type = Config::STRING;
};

template <> struct value_traits<Config::Values::Bool> {
    using type = Config::BOOL;
};

template <> struct value_traits<Config::Values::Float> {
    using type = Config::FLOAT;
};

// Type-contract read: throws (catchable) on a null slot or a storage type that
// does not match the Config::* type we registered. Never take a raw pointer —
// CStringValue::ptr() is a hard RASSERT (RP-3); Bool slots are stored as INT
// 0/1 and only read back correctly through the typed value() accessor.
template <typename V>
    requires std::derived_from<V, Config::Values::IValue>
auto read(const SP<V> &value) {
    if (!value)
        throw std::runtime_error("mru::plugin::config::read: null config value");
    if (*value->underlying() != typeid(typename value_traits<V>::type))
        throw std::runtime_error(std::string("mru::plugin::config::read: type mismatch for '") + value->name() + "'");
    return value->value();
}

// Registers the 14 documented keys; fail closed on the first registration error
// (short-circuit): PLUGIN_INIT aborts with a notification instead of half-registering.
// Returns nullopt on success, or a human-readable reason for the host rejection
// (e.g. "name collision" when the same keys are already registered by a loaded
// instance). Plain bool hid that cause (`addConfigValueV2` returns only bool).
std::optional<std::string> register_all(HANDLE handle, Values &out);

// Port of the M2 PluginConfig derivation: manual clamp [0,5000] (REQ-CFG-004),
// enum fallback + once-warn (REQ-CFG-001), ui mapping (REQ-UI-002).
mru::plugin::PluginConfig read_config(const Values &values);

} // namespace mru::plugin::config