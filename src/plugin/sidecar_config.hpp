#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "config_value.hpp"

namespace mru::plugin::sidecar {

// Second config delivery path (ADR-024): on Lua-config hosts the hyprlang
// `plugin:mru-switcher:*` channel is dead (validator rejects plugin keys), so
// the same 11 keys can be delivered through a plain sidecar file instead.
//
// This TU is plugin core: no Hyprland includes (CI plugin-guards enforce it).
// Parsing never throws on malformed content — every problem is reported as a
// warning string; callers surface those through their own warn-once path.

// Default sidecar location: `$XDG_CONFIG_HOME/mru-switcher/config` when
// XDG_CONFIG_HOME is set and non-empty, otherwise
// `$HOME/.config/mru-switcher/config`. Empty string when neither is available
// (caller then treats the sidecar as absent).
std::string default_path();
std::string default_path(const char *xdg_config_home, const char *home);

// Raw per-key overrides: a disengaged optional means "not present in the file"
// and leaves the hyprlang value untouched. Values stay raw strings here so
// validation (and its warnings) happens once, in apply_overlay().
struct SidecarOverrides {
    std::optional<std::string> debounce_ms;
    std::optional<std::string> default_scope;
    std::optional<std::string> start_offset;
    std::optional<std::string> wrap;
    std::optional<std::string> ui;
    std::optional<std::string> border_style;
    std::optional<std::string> border_color;
    std::optional<std::string> border_size;
    std::optional<std::string> selection_follow_workspace; // ADR-026 / REQ-UI-012
    std::optional<std::string> lock_history_on_session; // reserved, REQ-H-010 / ADR-021
    std::optional<std::string> restore_focus_on_cancel;
    std::optional<std::string> external_socket;
};

struct SidecarParse {
    SidecarOverrides overrides;
    std::vector<std::string> warnings; // unknown keys, malformed lines
};

// Format: one `key = value` per line. Surrounding whitespace is trimmed;
// full-line `#` / `;` comments and blank lines are skipped; an inline `#` /
// `;` comment preceded by whitespace ends the value. Unknown keys are ignored
// (one warning each). No quote handling: values are taken verbatim after trim.
SidecarParse parse(std::string_view text);

// Missing/unreadable file -> empty overrides, no warnings (sidecar is
// opt-in; absence is not an error). Never throws on malformed content.
SidecarParse load_file(const std::string &path);
SidecarParse load_default();

// Overlays the parsed sidecar onto a hyprlang-derived config: only keys that
// are explicitly present AND valid replace the base value. Invalid values
// keep the base value and produce one warn() call each (callers pass a
// warn-once sink). `lock_history_on_session` is accepted and stored but stays
// reserved and ignored downstream (REQ-H-010 / ADR-021); a `false` value
// additionally warns through the same sink.
void apply_overlay(PluginConfig &cfg, const SidecarParse &parsed, const std::function<void(const std::string &)> &warn);

} // namespace mru::plugin::sidecar
