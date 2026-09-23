#include "config_value.hpp"

#include <algorithm>
#include <cmath>
#include <string_view>

namespace mru::plugin {

PluginConfig default_plugin_config() {
    return {};
}

std::optional<mru::domain::Scope> parse_scope_token(std::string_view s) {
    using mru::domain::Scope;
    if (s == "global")
        return Scope::Global;
    if (s == "monitor")
        return Scope::Monitor;
    if (s == "workspace")
        return Scope::Workspace;
    if (s == "visible")
        return Scope::Visible;
    if (s == "app")
        return Scope::App;
    return std::nullopt;
}

mru::domain::Scope parse_scope(std::string_view s) {
    return parse_scope_token(s).value_or(mru::domain::Scope::Global); // REQ-CFG-001 fallback
}

std::string_view scope_name(mru::domain::Scope scope) {
    switch (scope) {
    case mru::domain::Scope::Global:
        return "global";
    case mru::domain::Scope::Monitor:
        return "monitor";
    case mru::domain::Scope::Workspace:
        return "workspace";
    case mru::domain::Scope::Visible:
        return "visible";
    case mru::domain::Scope::App:
        return "app";
    }
    return "global";
}

int clamp_debounce_ms(std::int64_t raw) {
    // MEDIUM-9: clamp on the full-width value BEFORE narrowing to int, so huge or
    // negative INT64 config values land on 0/5000 instead of truncating to a 32-bit
    // bit-pattern and slipping past the clamp (e.g. 4294967296 -> 0 as int).
    return static_cast<int>(std::clamp<std::int64_t>(raw, 0, 5000)); // REQ-CFG-004
}

ParsedUi parse_ui_backend(std::string_view s) {
    if (s == "null")
        return {ParsedUi::Kind::Null, true};
    if (s == "border")
        return {ParsedUi::Kind::Border, true};
    if (s == "external")
        return {ParsedUi::Kind::External, true};
    return {ParsedUi::Kind::Null, false}; // REQ-CFG-001 fallback to default
}

ParsedBorderStyle parse_border_style(std::string_view s) {
    if (s == "solid")
        return {BorderStyle::Solid, false};
    if (s == "pulse")
        return {BorderStyle::Pulse, false}; // ADR-028 / REQ-UI-013
    if (s == "dim")
        return {BorderStyle::Dim, false}; // ADR-028 / REQ-UI-014
    // REQ-UI-007: any unknown token behaves as `solid` and warrants one warning.
    return {BorderStyle::Solid, true};
}

// ADR-028 / REQ-UI-013: clamp on the full-width value BEFORE narrowing to int
// (same MEDIUM-9 discipline as clamp_debounce_ms), so out-of-range config values
// land on the clamp bounds instead of a truncated bit-pattern.
int clamp_pulse_period_ms(std::int64_t raw) {
    return static_cast<int>(std::clamp<std::int64_t>(raw, 200, 10000));
}

// ADR-028 / REQ-UI-014: dim strength clamp [0.0, 1.0]. Handles the INT64 special
// value NaN semantics the same way the config layer reads floats (finite check).
double clamp_dim_alpha(double raw) {
    if (!std::isfinite(raw))
        return 0.7; // default on NaN/Inf from a bad config read
    return std::clamp(raw, 0.0, 1.0);
}

UiBackend effective_ui_backend(const PluginConfig &cfg) {
    if (cfg.ui_border)
        return UiBackend::Border;
    if (cfg.ui_external)
        return UiBackend::External; // ADR-018; runtime socket failure degrades to Null
    return UiBackend::Null;
}

mru::domain::StartOffset parse_start_offset(std::string_view s) {
    if (s == "first")
        return mru::domain::StartOffset::First;
    return mru::domain::StartOffset::Second; // includes "second"; fallback (REQ-SEL-002)
}

} // namespace mru::plugin