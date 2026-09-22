#pragma once

namespace mru::plugin {

// Event::bus wiring; listener objects are kept alive in PluginState (torn down
// first). Defined in plugin_events.cpp; subscribed after build_state() (HIGH-5),
// every listener runs behind a swallow-and-notify barrier (HIGH-4).
void subscribe_events();

} // namespace mru::plugin
