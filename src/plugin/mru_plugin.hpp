#pragma once

#include <memory>
#include <optional>
#include <vector>

#include <hyprutils/signal/Signal.hpp>
#include "config_value.hpp"
#include "dispatch_args.hpp"
#include "hypr/hypr_focus_gateway.hpp"
#include "hypr/hypr_scheduler.hpp"
#include "hypr/hypr_window_source.hpp"
#include "hypr/null_ui.hpp"
#include "mru/domain/history_tracker.hpp"
#include "mru/domain/session_controller.hpp"

namespace mru::plugin {

// All plugin-owned state. Function-local static in mru_plugin.cpp; explicitly
// torn down in PLUGIN_EXIT. Member order = reverse destruction order: listeners
// die first, then controller/tracker, scheduler last (HistoryTracker holds a
// SchedulerPort& and must not outlive it).
struct PluginState {
    std::unique_ptr<HyprlandSchedulerPort>          scheduler;
    std::unique_ptr<WindowIdentityRegistry>         registry;
    std::unique_ptr<HyprlandWindowSource>           source;
    std::unique_ptr<HyprlandFocusGateway>           fg;
    std::unique_ptr<NullUI>                         ui;
    std::unique_ptr<mru::domain::HistoryTracker>    tracker;
    std::unique_ptr<mru::domain::SessionController> controller;
    PluginConfig                                    config;
    std::vector<Hyprutils::Signal::CHyprSignalListener> listeners;
};

} // namespace mru::plugin