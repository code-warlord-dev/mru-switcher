#pragma once

#include <memory>
#include <optional>
#include <vector>

#include "config_v2.hpp"
#include "config_value.hpp"
#include "dispatch_args.hpp"
#include "hypr/hypr_focus_gateway.hpp"
#include "hypr/hypr_scheduler.hpp"
#include "hypr/hypr_window_source.hpp"
#include "hypr/hyprctl_border_prop_io.hpp"
#include "hypr/null_ui.hpp"
#include "mru/domain/history_tracker.hpp"
#include "mru/domain/session_controller.hpp"
#include "mru/domain/ui_port.hpp"
#include <hyprutils/signal/Signal.hpp>

namespace mru::plugin {

// All plugin-owned state. Function-local static in mru_plugin.cpp; explicitly
// torn down in PLUGIN_EXIT. Contract: teardown runs in *reverse* declaration
// order — listeners -> controller -> ui -> border_io -> fg -> source -> tracker
// -> registry -> scheduler, then config/config_v2 — because implicit destruction
// (move-assignment) would run in declaration order and free the scheduler before
// the tracker's cancel_pending() dereferences it (REQ-H-008). Invariants: the
// tracker holds a Validator closing over the registry and a SchedulerPort&, so it
// must be destroyed before both; the source holds a HistoryTracker&, so the
// tracker must outlive it (ADR-015); the ui proxy's factory builds a
// BorderHighlightUI holding a BorderPropIo&, so border_io must outlive ui
// (REQ-UI-009). The reverse order is enforced by an explicit
// teardown_state(); this type is therefore never copyable or movable.
struct PluginState {
    PluginState() = default;
    PluginState(const PluginState &) = delete;
    PluginState &operator=(const PluginState &) = delete;
    PluginState(PluginState &&) = delete;
    PluginState &operator=(PluginState &&) = delete;

    std::unique_ptr<HyprlandSchedulerPort> scheduler; // torn down last
    std::unique_ptr<WindowIdentityRegistry> registry;
    std::unique_ptr<mru::domain::HistoryTracker> tracker; // must outlive source
    std::unique_ptr<HyprlandWindowSource> source;
    std::unique_ptr<HyprlandFocusGateway> fg;
    std::unique_ptr<HyprctlBorderPropIo> border_io; // must outlive ui (factory builds BorderHighlightUI)
    std::unique_ptr<mru::domain::UIPort> ui;        // SessionUIBackendProxy (REQ-UI-009, ADR-017)
    std::unique_ptr<mru::domain::SessionController> controller;
    PluginConfig config;
    mru::plugin::config::Values config_v2;                         // typed V2 config slots (register_all)
    std::vector<Hyprutils::Signal::CHyprSignalListener> listeners; // torn down first
};

} // namespace mru::plugin
