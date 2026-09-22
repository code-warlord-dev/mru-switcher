#include "plugin_overlay_wiring.hpp"

#include <exception>
#include <memory>
#include <string>
#include <string_view>

#include <hyprland/src/desktop/view/Window.hpp>
#include <hyprland/src/plugins/PluginAPI.hpp>

#include "border_highlight_ui.hpp"
#include "config_value.hpp"
#include "external_overlay_ui.hpp"
#include "hypr/null_ui.hpp"
#include "mru/domain/session_controller.hpp"
#include "overlay_window_info.hpp"
#include "plugin_dispatch.hpp"
#include "plugin_internal.hpp"

namespace mru::plugin {

// REQ-O-004/REQ-O-007: peer commands map onto existing, bounds-safe controller
// paths; guarded() (HIGH-4) keeps a hostile peer from escaping into the compositor.
// The handler closes over PluginState (stable for the plugin lifetime) and is
// stored by value inside the socket server, so it must never capture locals.
HyprlandOverlaySocket::CommandHandler overlay_command_handler() {
    return [](const overlay_protocol::Command &cmd) {
        auto &st = state();
        (void)guarded([&] {
            const auto to_result = [](const mru::domain::SessionController::CommandResult &r) {
                return r.ok ? ok_result() : err_result(r.error);
            };
            switch (cmd.type) {
            case overlay_protocol::CommandType::Select:
                return to_result(st.controller->select_index(cmd.index));
            case overlay_protocol::CommandType::Apply:
                return to_result(st.controller->apply());
            case overlay_protocol::CommandType::Cancel:
                return to_result(st.controller->cancel());
            }
            return ok_result();
        });
    };
}

// Binds/refreshes the overlay socket when the effective backend is `external`.
// ensure_started is idempotent for the same path and restarts on a changed path
// (config reload); returns false when the path is empty or the socket cannot
// start (REQ-O-001) so the facade degrades to NullUI. A non-external/empty-path
// config tears the socket down so no listener outlives the backend that owns it
// (REQ-O-001/REQ-O-008); `stop()` is idempotent and never runs on the hot path.
bool try_start_overlay_socket() {
    auto &st = state();
    if (!st.overlay_socket)
        return false;
    if (effective_ui_backend(st.config) != UiBackend::External || st.config.external_socket.empty()) {
        st.overlay_socket->stop();
        return false;
    }
    return st.overlay_socket->ensure_started(st.config.external_socket, overlay_command_handler());
}

std::unique_ptr<mru::domain::UIPort> create_session_backend() {
    if (effective_ui_backend(state().config) == UiBackend::Border) {
        // ADR-026 / REQ-UI-012: the view-follow toggle is effective only when the
        // border backend is built, and per-session by construction (the proxy
        // rebuilds a fresh backend for every session, so a reload change applies
        // to the NEXT session — REQ-CFG-002). `false` picks the shared no-op
        // navigator = exact pre-ADR-026 off-screen highlight.
        WorkspaceNavigator &navigator = state().config.selection_follow_workspace
            ? static_cast<WorkspaceNavigator &>(*state().workspace_navigator)
            : null_workspace_navigator();
        return std::make_unique<BorderHighlightUI>(
            *state().border_io,
            [&](const mru::domain::WindowRef &ref) { return static_cast<bool>(state().registry->resolve(ref)); },
            state().config.border_style, state().config.border_color, state().config.border_size,
            [](std::string_view reason) {
                // REQ-UI-002 warn-once at plugin-load scope: the proxy builds a
                // FRESH backend per session, so an instance-level latch alone
                // would reset every session and spam one notification per
                // Alt+Tab session while the border API is broken. This static
                // drops repeat warns BEFORE addNotification, mirroring the
                // ui=external pattern below; the backend's own `warned_`
                // instance latch only dedupes within one backend instance.
                static bool warned = false;
                if (warned)
                    return;
                warned = true;
                HyprlandAPI::addNotification(PHANDLE, std::string("mru-switcher: ") + std::string(reason),
                                             CHyprColor{1, 0.7, 0, 1}, 5000);
            },
            navigator);
    }
    if (effective_ui_backend(state().config) == UiBackend::External) {
        if (try_start_overlay_socket()) {
            return std::make_unique<ExternalOverlayUI>(*state().overlay_socket, [&](const mru::domain::WindowRef &ref) {
                OverlayWindowInfo info;
                if (const PHLWINDOW w = state().registry->resolve(ref)) {
                    info.title = w->m_title;
                    info.window_class = w->m_class;
                }
                return info; // REQ-O-003: metadata best-effort, addr from ref
            });
        }
    }
    if (state().config.ui_external) {
        static bool warned = false;
        if (!warned) {
            warned = true;
            HyprlandAPI::addNotification(
                PHANDLE, "mru-switcher: ui=external unavailable (empty/broken external_socket), using ui=null",
                CHyprColor{1, 0.7, 0, 1}, 5000);
        }
    }
    return std::make_unique<NullUI>();
}

} // namespace mru::plugin
