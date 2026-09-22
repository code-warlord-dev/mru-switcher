#include "plugin_dispatch.hpp"

#include <exception>
#include <string>
#include <utility>

#include <hyprland/src/plugins/PluginAPI.hpp>

#include "config_value.hpp"
#include "dispatch_args.hpp"
#include "plugin_internal.hpp"
#include "status_format.hpp"

namespace mru::plugin {

// --- dispatchers (REQ-DISP-001/002) --------------------------------------------

SDispatchResult ok_result() {
    SDispatchResult r;
    r.success = true;
    return r;
}

SDispatchResult err_result(std::string error) {
    SDispatchResult r;
    r.success = false;
    r.error = std::move(error);
    return r;
}

SDispatchResult dispatch_cycle(std::string args) {
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

SDispatchResult dispatch_apply(std::string) {
    if (!state().controller)
        return err_result("mru-switcher: not initialized");
    const auto r = state().controller->apply();
    if (!r.ok)
        return err_result(r.error);
    return ok_result();
}

SDispatchResult dispatch_cancel(std::string) {
    if (!state().controller)
        return err_result("mru-switcher: not initialized");
    const auto r = state().controller->cancel();
    return r.ok ? ok_result() : err_result(r.error);
}

SDispatchResult dispatch_status(std::string) {
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

void register_dispatchers() {
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

} // namespace mru::plugin
