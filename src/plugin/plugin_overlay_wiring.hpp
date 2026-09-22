#pragma once

#include <memory>

#include "hypr/hyprland_overlay_socket.hpp"
#include "mru/domain/ui_port.hpp"

namespace mru::plugin {

// Overlay socket + session-backend factory (REQ-O-001/004/007/008, REQ-UI-002/009).
// Defined in plugin_overlay_wiring.cpp.
HyprlandOverlaySocket::CommandHandler overlay_command_handler();
bool try_start_overlay_socket();
std::unique_ptr<mru::domain::UIPort> create_session_backend();

} // namespace mru::plugin
