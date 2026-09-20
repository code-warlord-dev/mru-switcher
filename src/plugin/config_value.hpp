#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "mru/domain/scope.hpp"

namespace mru::plugin {

// Border highlight style tokens (REQ-UI-007). M4 implements only `solid`; reserved
// (`pulse`, `dim`) and unknown tokens are coerced to `solid` by parse_border_style().
enum class BorderStyle { Solid };

struct PluginConfig {
    int debounce_ms = 400; // clamp [0,5000] (REQ-CFG-004)
    mru::domain::Scope default_scope = mru::domain::Scope::Global;
    mru::domain::StartOffset start_offset = mru::domain::StartOffset::Second;
    bool wrap = true;
    // REQ-H-010 / ADR-021: RESERVED, value ignored. Lock-in while a session is Active
    // is mandatory (REQ-H-001). The key stays registered under plugin:mru-switcher: so
    // existing 0.x configs keep parsing; a `false` value only triggers one warning
    // notification at reload. Read here so read_config() can emit that warn-once.
    bool lock_history_on_session = true;
    bool restore_focus_on_cancel = false;
    bool ui_null = true;      // explicit ui=null
    bool ui_border = false;   // M4: ui=border -> BorderHighlightUI (REQ-UI-003)
    bool ui_external = false; // M5: ui=external -> ExternalOverlayUI (REQ-O-*, ADR-018)
    bool ui_matched = true;
    BorderStyle border_style = BorderStyle::Solid; // REQ-UI-007
    std::string border_color = "0xffffd9a0";       // REQ-UI-008: verbatim setprop value
    int border_size = -1;                          // REQ-UI-008: -1 = leave size untouched
    std::string external_socket;                   // REQ-O-001: AF_UNIX path (empty -> null fallback)
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

// REQ-UI-007: `solid` has effect in M4; reserved (`pulse`, `dim`) and unknown
// tokens behave as `solid`. The pure parser carries no side effects; `should_warn`
// signals the caller to emit one warn-once notification for that condition.
struct ParsedBorderStyle {
    BorderStyle style = BorderStyle::Solid;
    bool should_warn = false;
};
ParsedBorderStyle parse_border_style(std::string_view s);

// Backend actually constructed for the session (ADR-018): `external` may still
// degrade to Null at runtime when the socket cannot start (REQ-O-001).
enum class UiBackend { Null, Border, External };
UiBackend effective_ui_backend(const PluginConfig &cfg);

// REQ-SEL-002/REQ-S-009: `first`|`second`; unknown falls back to `second`.
mru::domain::StartOffset parse_start_offset(std::string_view s);

} // namespace mru::plugin