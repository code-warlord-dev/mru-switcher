#include "plugin_lifecycle.hpp"

#include <cstdint>
#include <memory>
#include <string_view>

#include <hyprland/src/plugins/PluginAPI.hpp>
#include <hyprland/src/plugins/PluginSystem.hpp>

#include "border_highlight_ui.hpp"
#include "config_v2.hpp"
#include "hypr/hypr_focus_gateway.hpp"
#include "hypr/hypr_scheduler.hpp"
#include "hypr/hypr_window_source.hpp"
#include "hypr/hyprctl_border_prop_io.hpp"
#include "hypr/hyprland_overlay_socket.hpp"
#include "hypr/null_ui.hpp"
#include "mru/domain/history_tracker.hpp"
#include "mru/domain/session_controller.hpp"
#include "plugin_internal.hpp"
#include "plugin_overlay_wiring.hpp"
#include "session_ui.hpp"
#include "sidecar_config.hpp"

namespace mru::plugin {

namespace {

// Warn-once sink for the sidecar overlay at load (HIGH-4: notifications only,
// never throws into the compositor).
void sidecar_warn_load(const std::string &msg) {
    static bool warned = false;
    if (warned)
        return;
    warned = true;
    HyprlandAPI::addNotification(PHANDLE, msg, CHyprColor{1, 0.7, 0, 1}, 5000);
}

void apply_sidecar_overlay(PluginConfig &cfg) {
    const auto parsed = sidecar::load_default();
    sidecar::apply_overlay(cfg, parsed, sidecar_warn_load);
    if (!parsed.warnings.empty())
        sidecar_warn_load(parsed.warnings.front());
}

} // namespace

PluginState &state() {
    static PluginState s;
    return s;
}

// --- hash check (fail closed) -------------------------------------------------

bool hash_ok() {
    const char *h = __hyprland_api_get_hash();
    const char *c = __hyprland_api_get_client_hash();
    if (h && c && std::string_view(h) == std::string_view(c))
        return true;
    // no notification here: the caller (PLUGIN_INIT) throws and the single catch
    // below is the fail-closed path that also surfaces the message
    return false;
}

mru::domain::SessionPolicy policy_from_config(const PluginConfig &cfg) {
    mru::domain::SessionPolicy policy;
    policy.default_scope = cfg.default_scope;
    policy.start_offset = cfg.start_offset; // REQ-SEL-002/REQ-S-009
    policy.wrap = cfg.wrap;
    // No lock-in flag: `lock_history_on_session` is reserved and ignored, lock-in
    // while Active is mandatory (REQ-H-001/010, ADR-021).
    policy.restore_focus_on_cancel = cfg.restore_focus_on_cancel;
    return policy;
}

void build_state() {
    auto &st = state();
    st.config = mru::plugin::config::read_config(st.config_v2);
    // ADR-024: Lua hosts cannot set plugin keys via hl.config — the sidecar
    // file overlays the same SPEC §4 keys (absent on hyprlang hosts: no-op).
    apply_sidecar_overlay(st.config);

    st.registry = std::make_unique<WindowIdentityRegistry>();
    st.scheduler = std::make_unique<HyprlandSchedulerPort>();
    st.tracker = std::make_unique<mru::domain::HistoryTracker>(
        *st.scheduler,
        [&st](const mru::domain::WindowRef &ref) { return static_cast<bool>(st.registry->resolve(ref)); },
        static_cast<std::uint32_t>(st.config.debounce_ms));
    st.source = std::make_unique<HyprlandWindowSource>(*st.registry, *st.tracker);
    st.fg = std::make_unique<HyprlandFocusGateway>(*st.registry);
    // ADR-026: persistent session-agnostic navigator. BorderHighlightUI (built per
    // session by the proxy factory) borrows a reference while a session lives;
    // begin/end keep its capture state per session.
    st.workspace_navigator = std::make_unique<HyprlandWorkspaceNavigator>(*st.registry);

    // Backend factory (ADR-004 / ADR-017 / ADR-018, ARCHITECTURE §11). REQ-UI-009:
    // the controller holds a UIPort&, so a stable SessionUIBackendProxy is installed
    // once and rebuilds the concrete backend from the CURRENT config on each
    // session start. A `hyprctl reload` therefore takes effect on the NEXT session,
    // never mid-session. `external` binds the socket at load (so a peer can connect
    // ahead of the first session) and degrades to NullUI + warn-once when the path
    // is empty or the socket cannot start (REQ-O-001); `null`/unknown use the no-op
    // backend.
    st.border_io = std::make_unique<HyprctlBorderPropIo>();
    st.overlay_socket = std::make_unique<HyprlandOverlaySocket>();
    // Best-effort early bind (see try_start_overlay_socket). On this pin the
    // registered config values are not yet populated during PLUGIN_INIT, so the
    // reliable bind happens on the first `config.reloaded` that follows the load;
    // the factory call below is the final safety net.
    (void)try_start_overlay_socket();

    st.ui = std::make_unique<SessionUIBackendProxy>([]() { return create_session_backend(); });

    st.controller = std::make_unique<mru::domain::SessionController>(*st.source, *st.fg, *st.ui, *st.tracker,
                                                                     policy_from_config(st.config));

    // Seed the plugin-owned MRU list from the compositor history, registering every
    // window found on the way (REQ-H-004b, ADR-015).
    st.tracker->seed(st.source->candidates(mru::domain::Scope::Global));
}

void teardown_state() {
    auto &st = state();

    // Explicit reverse-order teardown (mru_plugin.hpp PluginState contract):
    // listeners first, scheduler last — implicit destruction would run in
    // declaration order and free the scheduler before the tracker's
    // cancel_pending() dereferences it (REQ-H-008).
    st.listeners.clear(); // stop all callbacks first
    if (st.controller)
        st.controller->plugin_shutdown(); // L-11: end an active session (UI on_session_end +
                                          // state reset) while controller/ui/tracker are alive
    st.controller.reset();
    st.ui.reset();
    st.overlay_socket.reset(); // after ui: ExternalOverlayUI holds its transport& (ADR-018)
    st.border_io.reset();
    st.workspace_navigator.reset(); // holds registry&; dies before the registry below
    st.fg.reset();
    st.source.reset();
    st.tracker.reset(); // cancel_pending() still sees a live scheduler
    st.registry.reset();
    st.scheduler.reset(); // destroyed last
    st.config = PluginConfig{};
    st.config_v2 = mru::plugin::config::Values{};
}

} // namespace mru::plugin
