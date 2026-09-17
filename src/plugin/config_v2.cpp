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
constexpr const char *KEY_LOCK_HISTORY_ON_SESSION = "plugin:mru-switcher:lock_history_on_session";
constexpr const char *KEY_RESTORE_FOCUS_ON_CANCEL = "plugin:mru-switcher:restore_focus_on_cancel";

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

    out.lock_history_on_session = Config::Values::makeConfigValue<Config::Values::Bool>(
        KEY_LOCK_HISTORY_ON_SESSION, "Freeze MRU order while a session is active", true);
    if (!HyprlandAPI::addConfigValueV2(handle, out.lock_history_on_session))
        return false;

    out.restore_focus_on_cancel = Config::Values::makeConfigValue<Config::Values::Bool>(
        KEY_RESTORE_FOCUS_ON_CANCEL, "On cancel, refocus the window that was focused at session start", false);
    if (!HyprlandAPI::addConfigValueV2(handle, out.restore_focus_on_cancel))
        return false;

    return true;
}

mru::plugin::PluginConfig read_config(const Values &values) {
    mru::plugin::PluginConfig cfg;
    cfg.debounce_ms = clamp_debounce_ms(read(values.debounce_ms));
    cfg.default_scope = parse_scope(read(values.default_scope));
    // MEDIUM-7: only global is implemented until M3. A documented-but-unimplemented
    // scope must not silently produce "no windows" on every Alt+Tab — fall back to
    // global and warn once (mirror of the ui=border/external handling below).
    if (cfg.default_scope != mru::domain::Scope::Global) {
        static bool warned = false;
        if (!warned) {
            warned = true;
            HyprlandAPI::addNotification(PHANDLE,
                                         std::string("mru-switcher: scope '") +
                                             std::string(scope_name(cfg.default_scope)) +
                                             "' not implemented until M3, falling back to 'global'",
                                         CHyprColor{1, 0.7, 0, 1}, 5000);
        }
        cfg.default_scope = mru::domain::Scope::Global;
    }
    cfg.start_offset = parse_start_offset(read(values.start_offset));
    cfg.wrap = read(values.wrap);
    cfg.lock_history_on_session = read(values.lock_history_on_session);
    cfg.restore_focus_on_cancel = read(values.restore_focus_on_cancel);

    const ParsedUi ui = parse_ui_backend(read(values.ui));
    cfg.ui_null =
        ui.kind != ParsedUi::Kind::Border && ui.kind != ParsedUi::Kind::External; // M2 fallback (REQ-UI-002/003)
    cfg.ui_border = ui.kind == ParsedUi::Kind::Border;
    cfg.ui_external = ui.kind == ParsedUi::Kind::External;
    cfg.ui_matched = ui.matched;
    // REQ-UI-002: border/external are parsed but not implemented yet; fall back
    // to null and warn the user once (not on every reload).
    if (cfg.ui_border || cfg.ui_external) {
        static bool warned = false;
        if (!warned) {
            warned = true;
            HyprlandAPI::addNotification(
                PHANDLE, "mru-switcher: ui=border/external not implemented in M2, falling back to ui=null",
                CHyprColor{1, 0.7, 0, 1}, 5000);
        }
    }
    return cfg;
}

} // namespace mru::plugin::config