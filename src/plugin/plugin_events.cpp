#include "plugin_events.hpp"

#include <cstdint>
#include <exception>
#include <functional>
#include <string>
#include <string_view>

#include <hyprland/src/desktop/view/Window.hpp>
#include <hyprland/src/plugins/PluginAPI.hpp>
#include <hyprland/src/plugins/PluginSystem.hpp>

#include "config_v2.hpp"
#include "plugin_internal.hpp"
#include "plugin_lifecycle.hpp"
#include "plugin_overlay_wiring.hpp"

namespace mru::plugin {
namespace {

// --- Event::bus wiring (listeners kept alive in PluginState) -------------------

// HIGH-4: a listener that throws would resume into the compositor's event
// dispatch. Swallow, log a tear-off notification (never propagate).
static void guarded_listener(std::string_view tag, const std::function<void(void)> &fn) {
    try {
        fn();
    } catch (const std::exception &e) {
        HyprlandAPI::addNotification(PHANDLE, std::string("mru-switcher: ") + std::string(tag) + ": " + e.what(),
                                     CHyprColor{1, 0, 0, 1}, 5000);
    } catch (...) {
        HyprlandAPI::addNotification(PHANDLE, std::string("mru-switcher: ") + std::string(tag) + ": internal error",
                                     CHyprColor{1, 0, 0, 1}, 5000);
    }
}

} // namespace

void subscribe_events() {
    auto &st = state();

    st.listeners.push_back(Event::bus()->m_events.window.open.listen([](PHLWINDOW w) {
        guarded_listener("window.open", [&] {
            if (w)
                state().registry->register_window(w);
        });
    }));

    st.listeners.push_back(Event::bus()->m_events.window.active.listen([](PHLWINDOW w, Desktop::eFocusReason) {
        guarded_listener("window.active", [&] {
            if (!w)
                return;
            auto &st = state();
            const auto ref = st.registry->register_window(w);
            st.controller->on_focus(ref); // controller ignores while Active (lock-in)
        });
    }));

    st.listeners.push_back(Event::bus()->m_events.window.close.listen([](PHLWINDOW w) {
        guarded_listener("window.close", [&] {
            if (!w)
                return;
            auto &st = state();
            st.registry->on_window_close(w);
            if (const auto ref = st.registry->last_ref(w))
                st.controller->on_window_invalid(*ref);
        });
    }));

    st.listeners.push_back(Event::bus()->m_events.window.destroy.listen([](PHLWINDOWREF w) {
        guarded_listener("window.destroy", [&] {
            const auto locked = w.lock();
            if (!locked)
                return;
            auto &st = state();
            st.registry->on_window_close(locked);
            if (const auto ref = st.registry->last_ref(locked))
                st.controller->on_window_invalid(*ref);
        });
    }));

    st.listeners.push_back(Event::bus()->m_events.config.reloaded.listen([]() {
        guarded_listener("config.reloaded", [&] {
            // REQ-CFG-002: refresh cached values. The active session is untouched
            // (REQ-S-009); new values apply to the next session and to later debounce windows.
            auto &st = state();
            st.config = mru::plugin::config::read_config(st.config_v2);
            st.tracker->set_debounce_ms(static_cast<std::uint32_t>(st.config.debounce_ms));
            st.controller->set_policy(policy_from_config(st.config));
            // Early bind for `ui=external`: on this pin config values are only
            // populated after the reload that follows plugin load, so this is the
            // first reliable moment to open the socket for a peer that wants to
            // connect before the first session. It also tears the socket down when
            // the reloaded config no longer selects `external`.
            (void)try_start_overlay_socket();
        });
    }));
}

} // namespace mru::plugin
