#include "mru_plugin.hpp"

#include <string_view>

#include <hyprland/src/desktop/view/Window.hpp>
#include <hyprland/src/plugins/PluginAPI.hpp>
#include <hyprland/src/plugins/PluginSystem.hpp>

#include "border_highlight_ui.hpp"
#include "config_v2.hpp"
#include "mru/domain/scope.hpp"
#include "mru_version.hpp"
#include "session_ui.hpp"
#include "status_format.hpp"

HANDLE PHANDLE = nullptr;

namespace mru::plugin {
namespace {

static PluginState &state() {
    static PluginState s;
    return s;
}

// --- hash check (fail closed) -------------------------------------------------

static bool hash_ok() {
    const char *h = __hyprland_api_get_hash();
    const char *c = __hyprland_api_get_client_hash();
    if (h && c && std::string_view(h) == std::string_view(c))
        return true;
    // no notification here: the caller (PLUGIN_INIT) throws and the single catch
    // below is the fail-closed path that also surfaces the message
    return false;
}

static mru::domain::SessionPolicy policy_from_config(const PluginConfig &cfg) {
    mru::domain::SessionPolicy policy;
    policy.default_scope = cfg.default_scope;
    policy.start_offset = cfg.start_offset; // REQ-SEL-002/REQ-S-009
    policy.wrap = cfg.wrap;
    // No lock-in flag: `lock_history_on_session` is reserved and ignored, lock-in
    // while Active is mandatory (REQ-H-001/010, ADR-021).
    policy.restore_focus_on_cancel = cfg.restore_focus_on_cancel;
    return policy;
}

// --- dispatchers (REQ-DISP-001/002) --------------------------------------------

static SDispatchResult ok_result() {
    SDispatchResult r;
    r.success = true;
    return r;
}

static SDispatchResult err_result(std::string error) {
    SDispatchResult r;
    r.success = false;
    r.error = std::move(error);
    return r;
}

// HIGH-4: an exception escaping into the compositor through a C-ABI-like entry
// point is std::terminate at best, UB at worst — one malformed config line must
// not take down a user session. Every dispatcher and listener runs behind a
// barrier that converts exceptions into an error result / notification.
template <class F> static SDispatchResult guarded(F &&f) noexcept {
    try {
        return f();
    } catch (const std::exception &e) {
        return err_result(std::string("internal error: ") + e.what());
    } catch (...) {
        return err_result("internal error");
    }
}

static SDispatchResult dispatch_cycle(std::string args) {
    const CycleArgs parsed = parse_cycle_args(args);
    if (!parsed.ok)
        return err_result(parsed.error);
    if (!state().controller) // HIGH-5: never deref null after partial init
        return err_result("mru-switcher: not initialized");
    const auto r = state().controller->cycle(parsed.dir, parsed.scope); // never focuses (REQ-F-003)
    if (!r.ok)
        return err_result(r.error);
    return ok_result();
}

static SDispatchResult dispatch_apply(std::string) {
    if (!state().controller)
        return err_result("mru-switcher: not initialized");
    const auto r = state().controller->apply();
    if (!r.ok)
        return err_result(r.error);
    return ok_result();
}

static SDispatchResult dispatch_cancel(std::string) {
    if (!state().controller)
        return err_result("mru-switcher: not initialized");
    const auto r = state().controller->cancel();
    return r.ok ? ok_result() : err_result(r.error);
}

static SDispatchResult dispatch_status(std::string) {
    auto &st = state();
    if (!st.controller)
        return err_result("mru-switcher: not initialized");
    const auto &snapshot = st.controller->active_snapshot();
    const bool active = st.controller->is_active() && snapshot.has_value();

    SDispatchResult r;
    r.success = true;
    // SPEC §3.4: the status string MAY travel in the error field of a successful result.
    r.error = format_status(active, active ? st.controller->index() : 0, active ? snapshot->size() : 0,
                            scope_name(active ? snapshot->scope() : st.config.default_scope),
                            st.controller->session_id(), st.controller->last_end_reason());
    return r;
}

static void register_dispatchers() {
    // HIGH-4/5: every entry is wrapped; dispatchers are registered only after
    // build_state() so a dispatch can never observe a half-built state.
    HyprlandAPI::addDispatcherV2(PHANDLE, "mru:cycle",
                                 [](std::string a) { return guarded([&] { return dispatch_cycle(std::move(a)); }); });
    HyprlandAPI::addDispatcherV2(PHANDLE, "mru:apply",
                                 [](std::string a) { return guarded([&] { return dispatch_apply(std::move(a)); }); });
    HyprlandAPI::addDispatcherV2(PHANDLE, "mru:cancel",
                                 [](std::string a) { return guarded([&] { return dispatch_cancel(std::move(a)); }); });
    HyprlandAPI::addDispatcherV2(PHANDLE, "mru:status",
                                 [](std::string a) { return guarded([&] { return dispatch_status(std::move(a)); }); });
}

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

static bool try_start_overlay_socket();

static void subscribe_events() {
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

// --- wiring ---------------------------------------------------------------------

// REQ-O-004/REQ-O-007: peer commands map onto existing, bounds-safe controller
// paths; guarded() (HIGH-4) keeps a hostile peer from escaping into the compositor.
// The handler closes over PluginState (stable for the plugin lifetime) and is
// stored by value inside the socket server, so it must never capture locals.
static HyprlandOverlaySocket::CommandHandler overlay_command_handler() {
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
static bool try_start_overlay_socket() {
    auto &st = state();
    if (!st.overlay_socket)
        return false;
    if (effective_ui_backend(st.config) != UiBackend::External || st.config.external_socket.empty()) {
        st.overlay_socket->stop();
        return false;
    }
    return st.overlay_socket->ensure_started(st.config.external_socket, overlay_command_handler());
}

static void build_state() {
    auto &st = state();
    st.config = mru::plugin::config::read_config(st.config_v2);

    st.registry = std::make_unique<WindowIdentityRegistry>();
    st.scheduler = std::make_unique<HyprlandSchedulerPort>();
    st.tracker = std::make_unique<mru::domain::HistoryTracker>(
        *st.scheduler,
        [&st](const mru::domain::WindowRef &ref) { return static_cast<bool>(st.registry->resolve(ref)); },
        static_cast<std::uint32_t>(st.config.debounce_ms));
    st.source = std::make_unique<HyprlandWindowSource>(*st.registry, *st.tracker);
    st.fg = std::make_unique<HyprlandFocusGateway>(*st.registry);

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

    st.ui = std::make_unique<SessionUIBackendProxy>([&st]() -> std::unique_ptr<mru::domain::UIPort> {
        if (effective_ui_backend(st.config) == UiBackend::Border) {
            return std::make_unique<BorderHighlightUI>(
                *st.border_io,
                [&st](const mru::domain::WindowRef &ref) { return static_cast<bool>(st.registry->resolve(ref)); },
                st.config.border_style, st.config.border_color, st.config.border_size,
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
                });
        }
        if (effective_ui_backend(st.config) == UiBackend::External) {
            if (try_start_overlay_socket()) {
                return std::make_unique<ExternalOverlayUI>(
                    *st.overlay_socket, [&st](const mru::domain::WindowRef &ref) {
                        OverlayWindowInfo info;
                        if (const PHLWINDOW w = st.registry->resolve(ref)) {
                            info.title = w->m_title;
                            info.window_class = w->m_class;
                        }
                        return info; // REQ-O-003: metadata best-effort, addr from ref
                    });
            }
        }
        if (st.config.ui_external) {
            static bool warned = false;
            if (!warned) {
                warned = true;
                HyprlandAPI::addNotification(
                    PHANDLE, "mru-switcher: ui=external unavailable (empty/broken external_socket), using ui=null",
                    CHyprColor{1, 0.7, 0, 1}, 5000);
            }
        }
        return std::make_unique<NullUI>();
    });

    st.controller = std::make_unique<mru::domain::SessionController>(*st.source, *st.fg, *st.ui, *st.tracker,
                                                                     policy_from_config(st.config));

    // Seed the plugin-owned MRU list from the compositor history, registering every
    // window found on the way (REQ-H-004b, ADR-015).
    st.tracker->seed(st.source->candidates(mru::domain::Scope::Global));
}

static void teardown_state() {
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
    st.fg.reset();
    st.source.reset();
    st.tracker.reset(); // cancel_pending() still sees a live scheduler
    st.registry.reset();
    st.scheduler.reset(); // destroyed last
    st.config = PluginConfig{};
    st.config_v2 = mru::plugin::config::Values{};
}

} // namespace
} // namespace mru::plugin

APICALL EXPORT std::string PLUGIN_API_VERSION() {
    return HYPRLAND_API_VERSION;
}

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle) {
    PHANDLE = handle;

    try {
        if (!mru::plugin::hash_ok())
            throw std::runtime_error("mru-switcher: header hash mismatch, refusing to load");

        if (!mru::plugin::config::register_all(PHANDLE, mru::plugin::state().config_v2)) {
            // fail closed: an empty PLUGIN_DESCRIPTION_INFO does NOT unload the plugin
            // on pin 0.56.2 (loadPluginInternal has no empty-description check). The
            // only reliable fail-closed channel is an exception: loadPluginInternal
            // wraps initFunc in try/catch + setjmp and on std::exception runs
            // unloadPlugin(PLUGIN, true) and rejects the load. So PLUGIN_INIT lets a
            // std::exception propagate; the compositor's own catch is that barrier.
            throw std::runtime_error("mru-switcher: failed to register config values");
        }
        mru::plugin::build_state();          // constructs all members, seeds MRU (HIGH-5)
        mru::plugin::subscribe_events();     // HIGH-5: listeners after state exists
        mru::plugin::register_dispatchers(); // HIGH-5: dispatchers after state exists
    } catch (const std::exception &e) {
        mru::plugin::teardown_state();
        HyprlandAPI::addNotification(PHANDLE, std::string("mru-switcher: init failed: ") + e.what(),
                                     CHyprColor{1, 0, 0, 1}, 5000);
        // HIGH-4: at PLUGIN_INIT the fail-closed barrier is deliberately the
        // *compositor's* catch in loadPluginInternal (pin efb5099): it unloads the
        // plugin and rejects the load. Rethrow instead of returning {} — the latter
        // is treated as a successful load with empty metadata. Dispatchers/listeners
        // still use guarded()/guarded_listener() and never propagate (HIGH-4).
        throw; // NOLINT: intentional C-ABI escape, caught by the compositor (verified)
    } catch (...) {
        mru::plugin::teardown_state();
        HyprlandAPI::addNotification(PHANDLE, "mru-switcher: init failed (internal error)", CHyprColor{1, 0, 0, 1},
                                     5000);
        // non-std exception: the compositor's catch only handles std::exception, so
        // devolve to the empty return (cannot reliably fail closed; do not crash).
        return {};
    }

    return {"mru-switcher", "Niri-style MRU Alt+Tab (snapshot, apply-on-release, lock-in)", "mru",
            MRU_SWITCHER_VERSION};
}

APICALL EXPORT void PLUGIN_EXIT() {
    try {
        mru::plugin::teardown_state(); // listeners first, scheduler last: no use-after-unload
    } catch (...) {
        // best-effort: swallow to satisfy C-ABI boundary (HIGH-4)
    }
    PHANDLE = nullptr;
}