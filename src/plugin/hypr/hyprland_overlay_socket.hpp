#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <string_view>

#include "external_overlay_ui.hpp"
#include "overlay_protocol.hpp"
#include "overlay_socket_server.hpp"

struct wl_event_source;

namespace mru::plugin {

// Thin Hyprland adapter for the external overlay (ADR-018). Owns the Wayland fd
// watches for the Hyprland-free OverlaySocketServer: `wl_event_loop_add_fd` is
// the same main-loop mechanism as `CEventLoopManager::doOnReadable` but returns a
// removable event source, which is required to drop a disconnected peer without
// leaking the fd watch (R0 memo F1/F2).
class HyprlandOverlaySocket : public OverlayTransport {
  public:
    using CommandHandler = std::function<void(const overlay_protocol::Command &)>;

    HyprlandOverlaySocket() = default;
    ~HyprlandOverlaySocket() override;

    // Starts the server for `path` and watches the listener; idempotent for the
    // same path. Returns false when the socket cannot start (REQ-O-001) so the
    // facade can degrade to NullUI. A changed path restarts (config reload).
    bool ensure_started(const std::string &path, CommandHandler on_command);
    // Removes the fd watches and closes/unlinks the socket; idempotent (REQ-O-008).
    void stop();

    bool send(std::string_view line) override; // OverlayTransport, best-effort (REQ-O-002)
    bool started() const { return server_.started(); }

  private:
    void watch_listener();
    void unwatch_listener();
    void watch_client(int fd);
    void unwatch_client();
    void on_listener_readable();
    void on_client_readable();

    static int listener_callback(int fd, std::uint32_t mask, void *data);
    static int client_callback(int fd, std::uint32_t mask, void *data);

    OverlaySocketServer server_;
    CommandHandler on_command_;
    wl_event_source *listen_source_ = nullptr;
    wl_event_source *client_source_ = nullptr;
};

} // namespace mru::plugin
