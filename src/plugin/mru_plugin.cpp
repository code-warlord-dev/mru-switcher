#include "mru_plugin.hpp"

#include <string_view>

#include <hyprlang.hpp>

#include <hyprland/src/plugins/PluginAPI.hpp>
#include <hyprland/src/plugins/PluginSystem.hpp>

#include "mru/domain/scope.hpp"
#include "status_format.hpp"

// The pinned Hyprland v0.56.2 marks getConfigValue/addConfigValue deprecated in
// favor of the V2 config API; M2 intentionally uses the documented legacy path
// (plan Task 6 Step 3) and migrates in M3.
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

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
    HyprlandAPI::addNotification(PHANDLE, "mru-switcher: header hash mismatch, refusing to load",
                                 CHyprColor{1, 0, 0, 1}, 5000);
    return false;
}

// --- config (continuation of anonymous-namespace block) ------------------------

static std::int64_t cfg_int(const char *key, std::int64_t fallback) {
    const auto *v = HyprlandAPI::getConfigValue(PHANDLE, key);
    if (!v || !v->dataPtr())
        return fallback;
    return *static_cast<Hyprlang::INT *>(v->dataPtr());
}

static std::string cfg_str(const char *key, std::string_view fallback) {
    const auto *v = HyprlandAPI::getConfigValue(PHANDLE, key);
    if (!v)
        return std::string{fallback};
    // Hyprlang 0.6.x: dataPtr() on STRING values throws std::bad_any_cast.
    // Per hyprlang public.hpp, use getDataStaticPtr(): *retval is const char*.
    void *const *p = v->getDataStaticPtr();
    if (!p || !*p)
        return std::string{fallback};
    return std::string{static_cast<const char *>(*p)};
}

static PluginConfig read_config() {
    PluginConfig cfg;
    cfg.debounce_ms = clamp_debounce_ms(static_cast<int>(cfg_int("plugin:mru-switcher:debounce_ms", 400)));
    cfg.default_scope = parse_scope(cfg_str("plugin:mru-switcher:default_scope", "global"));
    cfg.start_offset = parse_start_offset(cfg_str("plugin:mru-switcher:start_offset", "second"));
    cfg.wrap = cfg_int("plugin:mru-switcher:wrap", 1) != 0;
    cfg.lock_history_on_session = cfg_int("plugin:mru-switcher:lock_history_on_session", 1) != 0;
    cfg.restore_focus_on_cancel = cfg_int("plugin:mru-switcher:restore_focus_on_cancel", 0) != 0;

    const ParsedUi ui = parse_ui_backend(cfg_str("plugin:mru-switcher:ui", "null"));
    cfg.ui_null =
        ui.kind != ParsedUi::Kind::Border && ui.kind != ParsedUi::Kind::External; // M2 fallback (REQ-UI-002/003)
    cfg.ui_border = ui.kind == ParsedUi::Kind::Border;
    cfg.ui_external = ui.kind == ParsedUi::Kind::External;
    cfg.ui_matched = ui.matched;
    return cfg;
}

static mru::domain::SessionPolicy policy_from_config(const PluginConfig &cfg) {
    mru::domain::SessionPolicy policy;
    policy.default_scope = cfg.default_scope;
    policy.start_offset = cfg.start_offset; // REQ-SEL-002/REQ-S-009
    policy.wrap = cfg.wrap;
    policy.lock_history_on_session = cfg.lock_history_on_session;
    policy.restore_focus_on_cancel = cfg.restore_focus_on_cancel;
    return policy;
}

static void register_config_keys() {
    // REQ-CFG-002: defaults under plugin:mru-switcher:, registered only in PLUGIN_INIT.
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:mru-switcher:debounce_ms",
                                Hyprlang::CConfigValue{static_cast<Hyprlang::INT>(400)});
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:mru-switcher:default_scope",
                                Hyprlang::CConfigValue{static_cast<Hyprlang::STRING>("global")});
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:mru-switcher:start_offset",
                                Hyprlang::CConfigValue{static_cast<Hyprlang::STRING>("second")});
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:mru-switcher:wrap",
                                Hyprlang::CConfigValue{static_cast<Hyprlang::INT>(1)});
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:mru-switcher:ui",
                                Hyprlang::CConfigValue{static_cast<Hyprlang::STRING>("null")});
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:mru-switcher:lock_history_on_session",
                                Hyprlang::CConfigValue{static_cast<Hyprlang::INT>(1)});
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:mru-switcher:restore_focus_on_cancel",
                                Hyprlang::CConfigValue{static_cast<Hyprlang::INT>(0)});
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

static SDispatchResult dispatch_cycle(std::string args) {
    const CycleArgs parsed = parse_cycle_args(args);
    if (!parsed.ok)
        return err_result(parsed.error);
    const auto r = state().controller->cycle(parsed.dir, parsed.scope); // never focuses (REQ-F-003)
    if (!r.ok)
        return err_result(r.error);
    return ok_result();
}

static SDispatchResult dispatch_apply(std::string) {
    const auto r = state().controller->apply();
    if (!r.ok)
        return err_result(r.error);
    return ok_result();
}

static SDispatchResult dispatch_cancel(std::string) {
    const auto r = state().controller->cancel();
    return r.ok ? ok_result() : err_result(r.error);
}

static SDispatchResult dispatch_status(std::string) {
    auto &st = state();
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
    HyprlandAPI::addDispatcherV2(PHANDLE, "mru:cycle", dispatch_cycle);
    HyprlandAPI::addDispatcherV2(PHANDLE, "mru:apply", dispatch_apply);
    HyprlandAPI::addDispatcherV2(PHANDLE, "mru:cancel", dispatch_cancel);
    HyprlandAPI::addDispatcherV2(PHANDLE, "mru:status", dispatch_status);
}

// --- Event::bus wiring (listeners kept alive in PluginState) -------------------

static void subscribe_events() {
    auto &st = state();

    st.listeners.push_back(Event::bus()->m_events.window.open.listen([](PHLWINDOW w) {
        if (w)
            state().registry->register_window(w);
    }));

    st.listeners.push_back(Event::bus()->m_events.window.active.listen([](PHLWINDOW w, Desktop::eFocusReason) {
        if (!w)
            return;
        auto &st = state();
        const auto ref = st.registry->register_window(w);
        st.controller->on_focus(ref); // controller ignores while Active (lock-in)
    }));

    st.listeners.push_back(Event::bus()->m_events.window.close.listen([](PHLWINDOW w) {
        if (!w)
            return;
        auto &st = state();
        st.registry->on_window_close(w);
        if (const auto ref = st.registry->last_ref(w))
            st.controller->on_window_invalid(*ref);
    }));

    st.listeners.push_back(Event::bus()->m_events.window.destroy.listen([](PHLWINDOWREF w) {
        const auto locked = w.lock();
        if (!locked)
            return;
        auto &st = state();
        st.registry->on_window_close(locked);
        if (const auto ref = st.registry->last_ref(locked))
            st.controller->on_window_invalid(*ref);
    }));

    st.listeners.push_back(Event::bus()->m_events.config.reloaded.listen([]() {
        // REQ-CFG-002: refresh cached values. The active session is untouched
        // (REQ-S-009); new values apply to the next session and to later debounce windows.
        auto &st = state();
        st.config = read_config();
        st.tracker->set_debounce_ms(static_cast<std::uint32_t>(st.config.debounce_ms));
        st.controller->set_policy(policy_from_config(st.config));
    }));
}

// --- wiring ---------------------------------------------------------------------

static void build_state() {
    auto &st = state();
    st.config = read_config();

    // REQ-UI-002: border/external are parsed but not implemented in M2; fall
    // back to null and warn the user once (not on every reload).
    if (st.config.ui_border || st.config.ui_external) {
        static bool warned = false;
        if (!warned) {
            warned = true;
            HyprlandAPI::addNotification(
                PHANDLE, "mru-switcher: ui=border/external not implemented in M2, falling back to ui=null",
                CHyprColor{1, 0.7, 0, 1}, 5000);
        }
    }

    st.registry = std::make_unique<WindowIdentityRegistry>();
    st.scheduler = std::make_unique<HyprlandSchedulerPort>();
    st.tracker = std::make_unique<mru::domain::HistoryTracker>(
        *st.scheduler, [&st](const mru::domain::WindowRef &ref) { return st.registry->resolve(ref).has_value(); },
        static_cast<std::uint32_t>(st.config.debounce_ms));
    st.source = std::make_unique<HyprlandWindowSource>(*st.registry, st.config, *st.tracker);
    st.fg = std::make_unique<HyprlandFocusGateway>(*st.registry);
    st.ui = std::make_unique<NullUI>();
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
    st.controller.reset();
    st.ui.reset();
    st.fg.reset();
    st.source.reset();
    st.tracker.reset(); // cancel_pending() still sees a live scheduler
    st.registry.reset();
    st.scheduler.reset(); // destroyed last
    st.config = PluginConfig{};
}

} // namespace
} // namespace mru::plugin

APICALL EXPORT std::string PLUGIN_API_VERSION() {
    return HYPRLAND_API_VERSION;
}

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle) {
    PHANDLE = handle;

    if (!mru::plugin::hash_ok())
        return {}; // empty description aborts init (fail closed)

    mru::plugin::register_config_keys();
    mru::plugin::register_dispatchers();
    mru::plugin::build_state();
    mru::plugin::subscribe_events();

    return {"mru-switcher", "Niri-style MRU Alt+Tab (snapshot, apply-on-release, lock-in)", "mru", "0.2.0"};
}

APICALL EXPORT void PLUGIN_EXIT() {
    mru::plugin::teardown_state(); // explicit reverse-order teardown: listeners first, scheduler last: no
                                   // use-after-unload
    PHANDLE = nullptr;
}