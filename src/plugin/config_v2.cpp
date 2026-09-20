#include "config_v2.hpp"

#include <string>
#include <utility>

#include <hyprland/src/plugins/PluginAPI.hpp>

extern HANDLE PHANDLE;

namespace mru::plugin::config {
namespace {

constexpr const char *KEY_DEBOUNCE_MS = "plugin:mru-switcher:debounce_ms";
constexpr const char *KEY_DEFAULT_SCOPE = "plugin:mru-switcher:default_scope";
constexpr const char *KEY_START_OFFSET = "plugin:mru-switcher:start_offset";
constexpr const char *KEY_WRAP = "plugin:mru-switcher:wrap";
constexpr const char *KEY_UI = "plugin:mru-switcher:ui";
constexpr const char *KEY_BORDER_STYLE = "plugin:mru-switcher:border_style";
constexpr const char *KEY_BORDER_COLOR = "plugin:mru-switcher:border_color";
constexpr const char *KEY_BORDER_SIZE = "plugin:mru-switcher:border_size";
constexpr const char *KEY_LOCK_HISTORY_ON_SESSION = "plugin:mru-switcher:lock_history_on_session";
constexpr const char *KEY_RESTORE_FOCUS_ON_CANCEL = "plugin:mru-switcher:restore_focus_on_cancel";
constexpr const char *KEY_EXTERNAL_SOCKET = "plugin:mru-switcher:external_socket";

} // namespace

bool register_all(HANDLE handle, Values &out) {
    // REQ-CFG-002: defaults under plugin:mru-switcher:, registered only in PLUGIN_INIT.
    // 3-arg makeConfigValue — no .min/.max (manual clamp_debounce_ms stays, REQ-CFG-004).
    out.debounce_ms = Config::Values::makeConfigValue<Config::Values::Int>(
        KEY_DEBOUNCE_MS, "Debounce before MRU commits the focus change (ms); 0 = immediate", 400);
    if (!HyprlandAPI::addConfigValueV2(handle, out.debounce_ms))
        return false;

    out.default_scope = Config::Values::makeConfigValue<Config::Values::String>(
        KEY_DEFAULT_SCOPE, "Default scope for mru:cycle: global | monitor | workspace | visible | app", "global");
    if (!HyprlandAPI::addConfigValueV2(handle, out.default_scope))
        return false;

    out.start_offset = Config::Values::makeConfigValue<Config::Values::String>(
        KEY_START_OFFSET, "Selection start offset: first | second", "second");
    if (!HyprlandAPI::addConfigValueV2(handle, out.start_offset))
        return false;

    out.wrap = Config::Values::makeConfigValue<Config::Values::Bool>(KEY_WRAP, "Wrap the selection at list ends", true);
    if (!HyprlandAPI::addConfigValueV2(handle, out.wrap))
        return false;

    out.ui =
        Config::Values::makeConfigValue<Config::Values::String>(KEY_UI, "UI backend: null | border | external", "null");
    if (!HyprlandAPI::addConfigValueV2(handle, out.ui))
        return false;

    out.border_style = Config::Values::makeConfigValue<Config::Values::String>(
        KEY_BORDER_STYLE, "Border highlight style: solid (pulse/dim reserved, treated as solid) (REQ-UI-007)", "solid");
    if (!HyprlandAPI::addConfigValueV2(handle, out.border_style))
        return false;

    // Verbatim pass-through to `setprop <color>`: a String keeps accepted formats
    // (hex 0xAARRGGBB / rgb() / rgba()) intact; Color would normalize them away.
    out.border_color = Config::Values::makeConfigValue<Config::Values::String>(
        KEY_BORDER_COLOR, "Border highlight colour (hex 0xAARRGGBB or rgb()/rgba()) (REQ-UI-008)", "0xffffd9a0");
    if (!HyprlandAPI::addConfigValueV2(handle, out.border_color))
        return false;

    out.border_size = Config::Values::makeConfigValue<Config::Values::Int>(
        KEY_BORDER_SIZE, "Border size override; -1 = do not change the window border size (REQ-UI-008)", -1);
    if (!HyprlandAPI::addConfigValueV2(handle, out.border_size))
        return false;

    out.lock_history_on_session = Config::Values::makeConfigValue<Config::Values::Bool>(
        KEY_LOCK_HISTORY_ON_SESSION,
        "Reserved and ignored: MRU history is always locked while a session is active (REQ-H-010)", true);
    if (!HyprlandAPI::addConfigValueV2(handle, out.lock_history_on_session))
        return false;

    out.restore_focus_on_cancel = Config::Values::makeConfigValue<Config::Values::Bool>(
        KEY_RESTORE_FOCUS_ON_CANCEL, "On cancel, refocus the window that was focused at session start", false);
    if (!HyprlandAPI::addConfigValueV2(handle, out.restore_focus_on_cancel))
        return false;

    // ADR-018: AF_UNIX path bound by the plugin when `ui=external`. Registered in
    // PLUGIN_INIT; read_config() consumes it for the M5 external overlay.
    out.external_socket = Config::Values::makeConfigValue<Config::Values::String>(
        KEY_EXTERNAL_SOCKET, "External UI protocol AF_UNIX socket path (ui=external; ADR-018)", "");
    if (!HyprlandAPI::addConfigValueV2(handle, out.external_socket))
        return false;

    return true;
}

mru::plugin::PluginConfig read_config(const Values &values) {
    mru::plugin::PluginConfig cfg;
    cfg.debounce_ms = clamp_debounce_ms(read(values.debounce_ms));
    cfg.default_scope = parse_scope(read(values.default_scope));
    cfg.start_offset = parse_start_offset(read(values.start_offset));
    cfg.wrap = read(values.wrap);
    cfg.lock_history_on_session = read(values.lock_history_on_session);
    // REQ-H-010 / ADR-021: the key is reserved and its value is ignored — lock-in is
    // mandatory (REQ-H-001). A non-default value gets exactly one notification per
    // plugin lifetime: read_config() runs again on every `config.reloaded`, and an
    // old config copied from 0.x must not spam the user (same warn-once pattern as
    // the border_style fallback below).
    if (!cfg.lock_history_on_session) {
        static bool warned = false;
        if (!warned) {
            warned = true;
            HyprlandAPI::addNotification(
                PHANDLE,
                "mru-switcher: lock_history_on_session is reserved and ignored - MRU lock-in is always on "
                "while switching (see docs/USER.md)",
                CHyprColor{1, 0.7, 0, 1}, 5000);
        }
    }
    cfg.restore_focus_on_cancel = read(values.restore_focus_on_cancel);

    const ParsedUi ui = parse_ui_backend(read(values.ui));
    cfg.ui_null = ui.kind == ParsedUi::Kind::Null;
    cfg.ui_border = ui.kind == ParsedUi::Kind::Border; // M4: BorderHighlightUI (REQ-UI-003)
    cfg.ui_external = ui.kind == ParsedUi::Kind::External;
    cfg.ui_matched = ui.matched;

    // REQ-UI-007: `solid` is the only effective style in M4; reserved/unknown tokens
    // are coerced to solid by parse_border_style() and surface one warning.
    const ParsedBorderStyle border_style = parse_border_style(read(values.border_style));
    cfg.border_style = border_style.style;
    if (border_style.should_warn) {
        static bool warned = false;
        if (!warned) {
            warned = true;
            HyprlandAPI::addNotification(PHANDLE, "mru-switcher: unknown border_style, using solid (REQ-UI-007)",
                                         CHyprColor{1, 0.7, 0, 1}, 5000);
        }
    }

    // border_color is forwarded verbatim to `setprop`; border_size -1 = untouched.
    cfg.border_color = read(values.border_color);
    cfg.border_size = static_cast<int>(read(values.border_size));

    // REQ-O-001 / ADR-018: empty path means `ui=external` degrades to null.
    cfg.external_socket = read(values.external_socket);
    return cfg;
}

} // namespace mru::plugin::config