#include "sidecar_config.hpp"

#include <cctype>
#include <cerrno>
#include <charconv>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

namespace mru::plugin::sidecar {
namespace {

std::string trim(std::string_view s) {
    std::size_t begin = 0;
    while (begin < s.size() && std::isspace(static_cast<unsigned char>(s[begin])))
        ++begin;
    std::size_t end = s.size();
    while (end > begin && std::isspace(static_cast<unsigned char>(s[end - 1])))
        --end;
    return std::string(s.substr(begin, end - begin));
}

// Cut an inline comment: the first `#` / `;` at the start of the value or
// preceded by whitespace ends the value (so `0xffffd9a0;` hex colours without
// a space keep working, `border ; comment` still strips).
std::string_view strip_inline_comment(std::string_view s) {
    for (std::size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '#' || s[i] == ';') {
            if (i == 0 || std::isspace(static_cast<unsigned char>(s[i - 1])))
                return s.substr(0, i);
        }
    }
    return s;
}

bool assign_raw(SidecarOverrides &o, const std::string &key, std::string value) {
    if (key == "debounce_ms")
        o.debounce_ms = std::move(value);
    else if (key == "default_scope")
        o.default_scope = std::move(value);
    else if (key == "start_offset")
        o.start_offset = std::move(value);
    else if (key == "wrap")
        o.wrap = std::move(value);
    else if (key == "ui")
        o.ui = std::move(value);
    else if (key == "border_style")
        o.border_style = std::move(value);
    else if (key == "border_color")
        o.border_color = std::move(value);
    else if (key == "border_size")
        o.border_size = std::move(value);
    else if (key == "selection_follow_workspace")
        o.selection_follow_workspace = std::move(value);
    else if (key == "pulse_period_ms")
        o.pulse_period_ms = std::move(value);
    else if (key == "dim_alpha")
        o.dim_alpha = std::move(value);
    else if (key == "lock_history_on_session")
        o.lock_history_on_session = std::move(value);
    else if (key == "restore_focus_on_cancel")
        o.restore_focus_on_cancel = std::move(value);
    else if (key == "external_socket")
        o.external_socket = std::move(value);
    else
        return false;
    return true;
}

// Strict whole-string integer: optional sign + digits only (no "12x").
// False on empty / junk / overflow; the caller warns and keeps base.
bool parse_int_strict(std::string_view s, std::int64_t &out) {
    if (s.empty())
        return false;
    const char *first = s.data();
    const char *last = s.data() + s.size();
    const auto res = std::from_chars(first, last, out);
    return res.ec == std::errc{} && res.ptr == last;
}

// Strict bool: `true`/`false`/`1`/`0` (lowercase only, matching hyprlang).
bool parse_bool_strict(std::string_view s, bool &out) {
    if (s == "true" || s == "1") {
        out = true;
        return true;
    }
    if (s == "false" || s == "0") {
        out = false;
        return true;
    }
    return false;
}

// Strict whole-string double via strtod: consumes the entire token (no trailing
// junk), rejects empty, and requires a finite result (INF/NaN are invalid).
bool parse_double_strict(std::string_view s, double &out) {
    if (s.empty())
        return false;
    std::string tmp(s);
    char *end = nullptr;
    errno = 0;
    const double d = std::strtod(tmp.c_str(), &end);
    if (end == tmp.c_str() || end[0] != '\0' || !std::isfinite(d))
        return false;
    out = d;
    return true;
}

} // namespace

std::string default_path(const char *xdg_config_home, const char *home) {
    if (xdg_config_home != nullptr && xdg_config_home[0] != '\0')
        return std::string(xdg_config_home) + "/mru-switcher/config";
    if (home != nullptr && home[0] != '\0')
        return std::string(home) + "/.config/mru-switcher/config";
    return {};
}

std::string default_path() {
    return default_path(std::getenv("XDG_CONFIG_HOME"), std::getenv("HOME"));
}

SidecarParse parse(std::string_view text) {
    SidecarParse result;
    std::string line;
    std::istringstream in{std::string(text)};
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r')
            line.pop_back(); // tolerate CRLF checkouts
        std::string_view view = line;
        std::size_t begin = 0;
        while (begin < view.size() && std::isspace(static_cast<unsigned char>(view[begin])))
            ++begin;
        if (begin == view.size())
            continue; // blank line
        if (view[begin] == '#' || view[begin] == ';')
            continue; // full-line comment
        const std::size_t eq = view.find('=', begin);
        if (eq == std::string_view::npos) {
            result.warnings.push_back("mru-switcher sidecar: ignoring malformed line (no '='): " + trim(view));
            continue;
        }
        const std::string key = trim(view.substr(0, eq));
        const std::string value = trim(strip_inline_comment(view.substr(eq + 1)));
        if (key.empty()) {
            result.warnings.push_back("mru-switcher sidecar: ignoring line with empty key");
            continue;
        }
        if (!assign_raw(result.overrides, key, value))
            result.warnings.push_back("mru-switcher sidecar: ignoring unknown key '" + key + "'");
    }
    return result;
}

// __PART2__

SidecarParse load_file(const std::string &path) {
    SidecarParse result;
    if (path.empty())
        return result; // no location available: sidecar absent, not an error
    std::ifstream in(path, std::ios::binary);
    if (!in)
        return result; // opt-in file: missing/unreadable is not an error
    std::ostringstream text;
    text << in.rdbuf();
    if (in.bad())
        return result;
    return parse(text.str());
}

SidecarParse load_default() {
    return load_file(default_path());
}

void apply_overlay(PluginConfig &cfg, const SidecarParse &parsed,
                   const std::function<void(const std::string &)> &warn) {
    const SidecarOverrides &o = parsed.overrides;
    auto invalid = [&](const char *key, const std::string &value) {
        warn(std::string("mru-switcher sidecar: ignoring invalid '") + key + "' value '" + value +
             "' (keeping config value)");
    };
    if (o.debounce_ms.has_value()) {
        std::int64_t raw = 0;
        if (parse_int_strict(*o.debounce_ms, raw))
            cfg.debounce_ms = clamp_debounce_ms(raw); // REQ-CFG-004
        else
            invalid("debounce_ms", *o.debounce_ms);
    }
    if (o.default_scope.has_value()) {
        if (parse_scope_token(*o.default_scope).has_value()) // REQ-CFG-001: unknown keeps base
            cfg.default_scope = parse_scope(*o.default_scope);
        else
            invalid("default_scope", *o.default_scope);
    }
    if (o.start_offset.has_value()) {
        if (*o.start_offset == "first" || *o.start_offset == "second")
            cfg.start_offset = parse_start_offset(*o.start_offset);
        else
            invalid("start_offset", *o.start_offset);
    }
    if (o.wrap.has_value()) {
        bool b = false;
        if (parse_bool_strict(*o.wrap, b))
            cfg.wrap = b;
        else
            invalid("wrap", *o.wrap);
    }
    if (o.ui.has_value()) {
        const ParsedUi ui = parse_ui_backend(*o.ui); // REQ-CFG-001 fallback
        if (ui.matched) {
            cfg.ui_null = ui.kind == ParsedUi::Kind::Null;
            cfg.ui_border = ui.kind == ParsedUi::Kind::Border;
            cfg.ui_external = ui.kind == ParsedUi::Kind::External;
            cfg.ui_matched = true;
        } else {
            invalid("ui", *o.ui);
        }
    }
    if (o.border_style.has_value()) {
        const ParsedBorderStyle style = parse_border_style(*o.border_style); // REQ-UI-007
        cfg.border_style = style.style;
        if (style.should_warn)
            warn("mru-switcher sidecar: unknown border_style '" + *o.border_style + "', using solid");
    }
    if (o.border_color.has_value())
        cfg.border_color = *o.border_color; // verbatim setprop value (REQ-UI-008)
    if (o.border_size.has_value()) {
        std::int64_t raw = 0;
        if (parse_int_strict(*o.border_size, raw) && raw >= -1 && raw <= 100)
            cfg.border_size = static_cast<int>(raw);
        else
            invalid("border_size", *o.border_size);
    }
    // ADR-026 / REQ-UI-012: ui=border view-follow toggle. Unknown values keep the
    // base value and warn (REQ-CFG-001), same as every other bool key here.
    if (o.selection_follow_workspace.has_value()) {
        bool b = false;
        if (parse_bool_strict(*o.selection_follow_workspace, b))
            cfg.selection_follow_workspace = b;
        else
            invalid("selection_follow_workspace", *o.selection_follow_workspace);
    }
    // ADR-028 / REQ-UI-013: sidecar parity for the pulse period (clamped like the
    // hyprlang channel, REQ-CFG-004 discipline).
    if (o.pulse_period_ms.has_value()) {
        std::int64_t raw = 0;
        if (parse_int_strict(*o.pulse_period_ms, raw))
            cfg.pulse_period_ms = clamp_pulse_period_ms(raw);
        else
            invalid("pulse_period_ms", *o.pulse_period_ms);
    }
    // ADR-028 / REQ-UI-014: sidecar parity for the dim strength (clamped; a
    // dim_alpha >= 1.0 disables dimming).
    if (o.dim_alpha.has_value()) {
        double raw = 0.0;
        if (parse_double_strict(*o.dim_alpha, raw))
            cfg.dim_alpha = clamp_dim_alpha(raw);
        else
            invalid("dim_alpha", *o.dim_alpha);
    }
    // REQ-H-010 / ADR-021: reserved key is stored for compat but never affects
    // behaviour (lock-in is mandatory); `false` additionally warns.
    if (o.lock_history_on_session.has_value()) {
        bool b = true;
        if (parse_bool_strict(*o.lock_history_on_session, b)) {
            cfg.lock_history_on_session = b;
            if (!b)
                warn("mru-switcher sidecar: lock_history_on_session is reserved and ignored "
                     "(lock-in is always on while switching)");
        } else {
            invalid("lock_history_on_session", *o.lock_history_on_session);
        }
    }
    if (o.restore_focus_on_cancel.has_value()) {
        bool b = false;
        if (parse_bool_strict(*o.restore_focus_on_cancel, b))
            cfg.restore_focus_on_cancel = b;
        else
            invalid("restore_focus_on_cancel", *o.restore_focus_on_cancel);
    }
    if (o.external_socket.has_value())
        cfg.external_socket = *o.external_socket; // empty -> external degrades to null
}
} // namespace mru::plugin::sidecar
