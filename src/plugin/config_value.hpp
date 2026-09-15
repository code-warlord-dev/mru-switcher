#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

#include "mru/domain/scope.hpp"

namespace mru::plugin {

struct PluginConfig {
    int debounce_ms = 400; // clamp [0,5000] (REQ-CFG-004)
    mru::domain::Scope default_scope = mru::domain::Scope::Global;
    mru::domain::StartOffset start_offset = mru::domain::StartOffset::Second;
    bool wrap = true;
    bool lock_history_on_session = true;
    bool restore_focus_on_cancel = false;
    bool ui_null = true;    // REQ-UI-003: solely null in M2
    bool ui_border = false; // parsed, falls back to null (REQ-UI-002)
    bool ui_external = false;
    bool ui_matched = true;
};

PluginConfig default_plugin_config();
mru::domain::Scope parse_scope(std::string_view s);
// Strict scope token for dispatcher arguments: std::nullopt when unknown
// (REQ-DISP-003). Config parsing keeps the fallback path via parse_scope().
std::optional<mru::domain::Scope> parse_scope_token(std::string_view s);
int clamp_debounce_ms(std::int64_t raw); // clamps [0,5000] before truncation (MEDIUM-9)

// Inverse of parse_scope_token for diagnostics (mru:status, SPEC §3.4).
std::string_view scope_name(mru::domain::Scope scope);

struct ParsedUi {
    enum class Kind { Null, Border, External };
    Kind kind = Kind::Null;
    bool matched = false;
};
ParsedUi parse_ui_backend(std::string_view s);

// REQ-SEL-002/REQ-S-009: `first`|`second`; unknown falls back to `second`.
mru::domain::StartOffset parse_start_offset(std::string_view s);

} // namespace mru::plugin