#include "overlay_socket_server.hpp"

#include <cerrno>
#include <cstdint>
#include <utility>

#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "overlay_protocol.hpp"

namespace mru::plugin {

namespace {

void set_nonblocking(int fd) {
    const int flags = fcntl(fd, F_GETFL, 0);
    if (flags >= 0)
        (void)fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

} // namespace

OverlaySocketServer::~OverlaySocketServer() {
    stop();
}

bool OverlaySocketServer::start(const std::string &path, LineHandler on_line) {
    if (path.empty())
        return false;
    if (started())
        return false;

    sockaddr_un addr{};
    if (path.size() >= sizeof(addr.sun_path))
        return false; // AF_UNIX path limit (108 bytes incl. NUL)

    const int fd = ::socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (fd < 0)
        return false;

    ::unlink(path.c_str()); // stale socket from a previous run (best-effort)

    addr.sun_family = AF_UNIX;
    // memcpy-style copy: sun_path is a fixed char array; size was bounds-checked.
    for (std::size_t i = 0; i < path.size(); ++i)
        addr.sun_path[i] = path[i];

    if (::bind(fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) != 0) {
        ::close(fd);
        return false;
    }

    set_nonblocking(fd); // belt-and-suspenders (SOCK_NONBLOCK already set)
    if (::listen(fd, 4) != 0) {
        ::close(fd);
        ::unlink(path.c_str());
        return false;
    }

    listen_fd_ = fd;
    path_ = path;
    on_line_ = std::move(on_line);
    return true;
}

void OverlaySocketServer::stop() {
    close_client();
    if (listen_fd_ >= 0) {
        ::close(listen_fd_);
        listen_fd_ = -1;
    }
    if (!path_.empty()) {
        ::unlink(path_.c_str());
        path_.clear();
    }
    recv_buf_.clear();
    on_line_ = nullptr;
}

void OverlaySocketServer::close_client() {
    if (client_fd_ >= 0) {
        ::close(client_fd_);
        client_fd_ = -1;
    }
    recv_buf_.clear();
}

void OverlaySocketServer::drop_client() {
    close_client();
}

int OverlaySocketServer::poll_accept() {
    if (listen_fd_ < 0)
        return -1;

    int accepted = -1;
    for (;;) {
        const int fd = ::accept4(listen_fd_, nullptr, nullptr, SOCK_NONBLOCK | SOCK_CLOEXEC);
        if (fd < 0) {
            if (errno == EINTR)
                continue;
            break; // EAGAIN/EWOULDBLOCK or hard error: nothing more pending
        }
        if (client_fd_ < 0) {
            client_fd_ = fd; // first client wins (ADR-018)
            accepted = fd;
        } else {
            ::close(fd); // single-client policy: drop extras
        }
    }
    return accepted;
}

bool OverlaySocketServer::poll_client() {
    if (client_fd_ < 0)
        return false;

    for (;;) {
        char buf[4096];
        const ssize_t n = ::recv(client_fd_, buf, sizeof(buf), 0);
        if (n > 0) {
            recv_buf_.append(buf, static_cast<std::size_t>(n));
            if (recv_buf_.size() > overlay_protocol::kMaxLineBytes) {
                // REQ-O-005: oversized/incomplete line -> drop the buffer.
                recv_buf_.clear();
                ++dropped_lines_;
            }
            continue;
        }
        if (n == 0) {
            close_client();
            return false; // peer closed
        }
        if (errno == EINTR)
            continue;
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            break;
        close_client();
        return false; // hard error (ECONNRESET, ...)
    }

    dispatch_buffer();
    return true;
}

void OverlaySocketServer::dispatch_buffer() {
    for (;;) {
        const std::size_t nl = recv_buf_.find('\n');
        if (nl == std::string::npos)
            return;
        // Copy BEFORE erasing: erase shifts the tail over the line bytes, which
        // would corrupt a view into recv_buf_.
        std::string line(recv_buf_.data(), nl);
        // Trailing CR from CRLF peers is tolerated by the protocol decoder.
        recv_buf_.erase(0, nl + 1);
        if (line.empty())
            continue;
        if (on_line_)
            on_line_(line); // parse/validate in the caller (REQ-O-005)
    }
}

bool OverlaySocketServer::send_line(std::string_view line) {
    if (client_fd_ < 0) {
        ++dropped_lines_;
        return false;
    }

    std::string frame(line);
    frame.push_back('\n');

    const ssize_t n = ::send(client_fd_, frame.data(), frame.size(), MSG_NOSIGNAL);
    if (n == static_cast<ssize_t>(frame.size()))
        return true;

    ++dropped_lines_; // REQ-O-002: best-effort; a slow peer never blocks us
    if (n >= 0 || (errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK)) {
        // Partial write corrupts the line stream, or the peer is gone -> reset.
        close_client();
    }
    return false;
}

} // namespace mru::plugin
