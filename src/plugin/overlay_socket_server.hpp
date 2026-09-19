#pragma once

#include <cstddef>
#include <functional>
#include <string>
#include <string_view>

namespace mru::plugin {

// AF_UNIX SOCK_STREAM server for the external overlay (ADR-018): single client
// (first accepted wins), newline-framed lines, all fds non-blocking. Hyprland-free
// and testable over a real loopback socket (T-O-07). Owns every fd; the adapter
// only watches listener_fd()/client_fd() for readability and calls poll_*.
class OverlaySocketServer {
  public:
    // Called for each complete line (newline stripped), on the compositor thread.
    using LineHandler = std::function<void(std::string_view line)>;
    // Optional diagnostics sink; invoked with a short human-readable message when
    // peer input is dropped (REQ-O-005). Hyprland-free; the adapter wires it to Log.
    using LogSink = std::function<void(std::string_view message)>;

    OverlaySocketServer() = default;
    ~OverlaySocketServer();
    OverlaySocketServer(const OverlaySocketServer &) = delete;
    OverlaySocketServer &operator=(const OverlaySocketServer &) = delete;

    // Installs the diagnostic sink (REQ-O-005). Cleared by stop().
    void set_log_sink(LogSink sink);

    // Bind + listen; false on empty/too-long path or any syscall failure
    // (REQ-O-001). Must be called again only after stop().
    bool start(const std::string &path, LineHandler on_line);
    // Closes listener + client and unlinks the socket path; idempotent (REQ-O-008).
    void stop();

    int listener_fd() const { return listen_fd_; }
    int client_fd() const { return client_fd_; }
    bool has_client() const { return client_fd_ >= 0; }
    bool started() const { return listen_fd_ >= 0; }
    const std::string &path() const { return path_; }
    std::size_t dropped_lines() const { return dropped_lines_; } // diagnostics (REQ-O-002)

    // Listener readable: accepts the first pending client (extra ones are closed).
    // Returns the accepted client fd, or -1 when no new client.
    int poll_accept();
    // Client readable: reads available bytes and dispatches complete lines.
    // Returns false when the peer disconnected/errored (client already closed).
    bool poll_client();
    // Closes the current client without touching the listener; idempotent. Used
    // by the adapter on a watched-fd HUP/ERROR (REQ-O-008).
    void drop_client();
    // Best-effort, non-blocking send of `line` + '\n'. false = dropped (REQ-O-002).
    bool send_line(std::string_view line);

  private:
    void close_client();
    void dispatch_buffer();

    int listen_fd_ = -1;
    int client_fd_ = -1;
    std::string path_;
    LineHandler on_line_;
    LogSink log_sink_;
    std::string recv_buf_;
    std::size_t dropped_lines_ = 0;
};

} // namespace mru::plugin
