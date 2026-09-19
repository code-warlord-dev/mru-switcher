#include "hyprland_overlay_socket.hpp"

#include <exception>
#include <utility>

#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/debug/log/Logger.hpp>
#include <wayland-server.h>

namespace mru::plugin {

HyprlandOverlaySocket::~HyprlandOverlaySocket() {
    stop();
}

bool HyprlandOverlaySocket::ensure_started(const std::string &path, CommandHandler on_command) {
    if (path.empty())
        return false; // REQ-O-001: no path -> facade degrades to NullUI

    if (server_.started() && server_.path() == path) {
        on_command_ = std::move(on_command);
        return true;
    }

    stop();
    server_.set_log_sink([](std::string_view message) {
        Log::logger->log(Log::DEBUG, message); // REQ-O-005: dropped peer input is logged at debug
    });
    if (!server_.start(path, [this](std::string_view line) {
            const std::optional<overlay_protocol::Command> cmd = overlay_protocol::parse_command(line);
            if (!cmd) {
                // REQ-O-005: unknown/malformed line ignored and logged at debug.
                Log::logger->log(Log::DEBUG,
                                 "mru-switcher: overlay peer line ignored (unknown/malformed, REQ-O-005): {}", line);
                return;
            }
            if (on_command_)
                on_command_(*cmd);
        }))
        return false;

    on_command_ = std::move(on_command);
    watch_listener();
    if (!listen_source_) {
        server_.stop(); // could not watch: do not leave a half-open socket
        return false;
    }
    return true;
}

void HyprlandOverlaySocket::stop() {
    unwatch_client();
    unwatch_listener();
    server_.stop();
    on_command_ = nullptr;
}

bool HyprlandOverlaySocket::send(std::string_view line) {
    return server_.send_line(line);
}

void HyprlandOverlaySocket::watch_listener() {
    if (listen_source_ || !server_.started() || !g_pCompositor)
        return;
    listen_source_ = wl_event_loop_add_fd(g_pCompositor->m_wlEventLoop, server_.listener_fd(), WL_EVENT_READABLE,
                                          listener_callback, this);
}

void HyprlandOverlaySocket::unwatch_listener() {
    if (!listen_source_)
        return;
    wl_event_source_remove(listen_source_);
    listen_source_ = nullptr;
}

void HyprlandOverlaySocket::watch_client(int fd) {
    unwatch_client();
    if (!g_pCompositor) {
        server_.drop_client();
        return;
    }
    client_source_ = wl_event_loop_add_fd(g_pCompositor->m_wlEventLoop, fd, WL_EVENT_READABLE, client_callback, this);
    if (!client_source_)
        server_.drop_client(); // cannot watch -> do not keep an unreadable peer
}

void HyprlandOverlaySocket::unwatch_client() {
    if (!client_source_)
        return;
    wl_event_source_remove(client_source_);
    client_source_ = nullptr;
}

void HyprlandOverlaySocket::on_listener_readable() {
    const int fd = server_.poll_accept();
    if (fd >= 0)
        watch_client(fd);
}

void HyprlandOverlaySocket::on_client_readable() {
    if (!server_.poll_client()) // false = peer gone, core closed the fd
        unwatch_client();       // remove the dead watch (REQ-O-008)
}

int HyprlandOverlaySocket::listener_callback(int, std::uint32_t mask, void *data) {
    auto *self = static_cast<HyprlandOverlaySocket *>(data);
    try {
        if (mask & (WL_EVENT_HANGUP | WL_EVENT_ERROR)) {
            self->unwatch_listener();
            self->server_.stop();
            return 0;
        }
        self->on_listener_readable();
    } catch (const std::exception &e) {
        // ADR-019: callbacks must never throw into the compositor.
        Log::logger->log(Log::ERR, "mru-switcher: overlay listener callback threw: {}", e.what());
    } catch (...) {
        Log::logger->log(Log::ERR, "mru-switcher: overlay listener callback threw (unknown)");
    }
    return 0;
}

int HyprlandOverlaySocket::client_callback(int, std::uint32_t mask, void *data) {
    auto *self = static_cast<HyprlandOverlaySocket *>(data);
    try {
        if (mask & (WL_EVENT_HANGUP | WL_EVENT_ERROR)) {
            self->unwatch_client();
            self->server_.drop_client();
            return 0;
        }
        self->on_client_readable();
    } catch (const std::exception &e) {
        // ADR-019: callbacks must never throw into the compositor.
        Log::logger->log(Log::ERR, "mru-switcher: overlay client callback threw: {}", e.what());
    } catch (...) {
        Log::logger->log(Log::ERR, "mru-switcher: overlay client callback threw (unknown)");
    }
    return 0;
}

} // namespace mru::plugin
