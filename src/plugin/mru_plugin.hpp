#pragma once

#include <memory>
#include <optional>
#include <vector>

#include "config_value.hpp"
#include "dispatch_args.hpp"
#include "hypr/hypr_focus_gateway.hpp"
#include "hypr/hypr_scheduler.hpp"
#include "hypr/hypr_window_source.hpp"
#include "hypr/null_ui.hpp"
#include "mru/domain/history_tracker.hpp"
#include "mru/domain/session_controller.hpp"
#include <hyprutils/signal/Signal.hpp>

namespace mru::plugin {

// All plugin-owned state. Function-local static in mru_plugin.cpp; explicitly
// torn down in PLUGIN_EXIT. Destruction runs in *reverse* declaration order:
// listeners -> controller -> ui -> fg -> source -> tracker -> registry ->
// scheduler. Invariants: the tracker holds a Validator closing over the registry
// and a SchedulerPort&, so it must be destroyed before both; the source holds a
// HistoryTracker&, so the tracker must outlive it (ADR-015).
struct PluginState {
    std::unique_ptr<HyprlandSchedulerPort> scheduler; // destroyed last
    std::unique_ptr<WindowIdentityRegistry> registry;
    std::unique_ptr<mru::domain::HistoryTracker> tracker; // must outlive source
    std::unique_ptr<HyprlandWindowSource> source;
    std::unique_ptr<HyprlandFocusGateway> fg;
    std::unique_ptr<NullUI> ui;
    std::unique_ptr<mru::domain::SessionController> controller;
    PluginConfig config;
    std::vector<Hyprutils::Signal::CHyprSignalListener> listeners; // destroyed first
};

} // namespace mru::plugin