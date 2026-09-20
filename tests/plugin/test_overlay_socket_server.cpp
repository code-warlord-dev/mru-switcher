#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

#include "overlay_protocol.hpp"
#include "overlay_socket_server.hpp"

#include "test_framework.hpp"

namespace {

using mru::plugin::OverlaySocketServer;

std::string test_path() {
    return "/tmp/mru-overlay-test-" + std::to_string(::getpid()) + ".sock";
}

int connect_client(const std::string &path) {
    const int fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0)
        return -1;
    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    for (std::size_t i = 0; i < path.size() && i < sizeof(addr.sun_path) - 1; ++i)
        addr.sun_path[i] = path[i];
    if (::connect(fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) != 0) {
        ::close(fd);
        return -1;
    }
    return fd;
}

std::string recv_line(int fd) {
    std::string out;
    for (;;) {
        char c = 0;
        const ssize_t n = ::recv(fd, &c, 1, 0);
        if (n <= 0)
            break;
        if (c == '\n')
            break;
        out.push_back(c);
    }
    return out;
}

// Push `data` to the server while draining it, so a >socket-buffer payload does
// not deadlock on a blocking client fd.
void pump_send(OverlaySocketServer &server, int client_fd, std::string_view data) {
    std::size_t sent = 0;
    while (sent < data.size()) {
        const ssize_t n = ::send(client_fd, data.data() + sent, data.size() - sent, MSG_NOSIGNAL);
        if (n <= 0)
            break;
        sent += static_cast<std::size_t>(n);
        (void)server.poll_client();
    }
}

struct Harness {
    OverlaySocketServer server;
    std::vector<std::string> lines;
    std::string path = test_path();

    Harness() {
        ::unlink(path.c_str());
        server.start(path, [this](std::string_view line) { lines.emplace_back(line); });
    }
    ~Harness() { server.stop(); }
};

// --- T-O-07 (a): start binds + listens; empty path fails; stop unlinks (REQ-O-001/008).
TEST(t_o_07_start_stop_lifecycle) {
    OverlaySocketServer server;
    CHECK(!server.start("", [](std::string_view) {})); // REQ-O-001: empty path
    CHECK(!server.started());

    const std::string path = test_path();
    ::unlink(path.c_str());
    CHECK(server.start(path, [](std::string_view) {}));
    CHECK(server.started());
    CHECK(server.listener_fd() >= 0);
    CHECK(server.path() == path);

    // THREAT-MODEL: the bound socket file is forced to mode 0600 (fchmod) so an
    // unrelated local user cannot connect. Parent-directory 0700 stays operator.
    struct stat st{};
    CHECK(::fstat(server.listener_fd(), &st) == 0);
    CHECK((st.st_mode & 0777) == 0600);

    server.stop();
    CHECK(!server.started());
    CHECK(server.listener_fd() < 0);
    CHECK(::access(path.c_str(), F_OK) != 0); // socket file removed
}

// --- T-O-07 (b): the first client wins; extra clients are dropped (ADR-018).
TEST(t_o_07_first_client_wins_and_framing) {
    Harness h;
    const int c1 = connect_client(h.path);
    CHECK(c1 >= 0);
    const int accepted = h.server.poll_accept();
    CHECK(accepted == h.server.client_fd());
    CHECK(h.server.has_client());

    const int c2 = connect_client(h.path); // second connection
    CHECK(c2 >= 0);
    h.server.poll_accept();                  // accepted then closed (single-client)
    CHECK(h.server.client_fd() == accepted); // still the first client

    // Two pipelined lines in one write are dispatched independently.
    const std::string payload = "{\"v\":1,\"type\":\"apply\"}\n{\"v\":1,\"type\":\"cancel\"}\n";
    pump_send(h.server, c1, payload);
    CHECK(h.lines.size() == 2);
    EQ(h.lines[0], std::string("{\"v\":1,\"type\":\"apply\"}"));
    EQ(h.lines[1], std::string("{\"v\":1,\"type\":\"cancel\"}"));

    ::close(c1);
    ::close(c2);
}

// --- T-O-07 (c): best-effort send delivers a newline-framed line (REQ-O-002/003).
TEST(t_o_07_send_line_frames_with_newline) {
    Harness h;
    const int c1 = connect_client(h.path);
    CHECK(c1 >= 0);
    CHECK(h.server.poll_accept() >= 0);

    CHECK(h.server.send_line("{\"v\":1,\"type\":\"selection\",\"index\":1}"));
    EQ(recv_line(c1), std::string("{\"v\":1,\"type\":\"selection\",\"index\":1}"));
    ::close(c1);
}

// --- T-O-07 (d): peer disconnect surfaces as poll_client()==false (REQ-O-006/008).
TEST(t_o_07_peer_close_detected) {
    Harness h;
    const int c1 = connect_client(h.path);
    CHECK(c1 >= 0);
    CHECK(h.server.poll_accept() >= 0);

    ::close(c1); // peer gone -> EOF on the next read
    CHECK(!h.server.poll_client());
    CHECK(!h.server.has_client());
    CHECK(h.server.client_fd() < 0);
}

// --- T-O-07 (e): an oversized unterminated line is dropped, later valid lines
// still dispatch (REQ-O-005); send with no client is best-effort false (REQ-O-002).
TEST(t_o_07_oversized_drop_and_no_client_send) {
    Harness h;
    CHECK(!h.server.send_line("{\"v\":1,\"type\":\"apply\"}")); // no client yet
    CHECK(h.server.dropped_lines() == 1);

    const int c1 = connect_client(h.path);
    CHECK(c1 >= 0);
    CHECK(h.server.poll_accept() >= 0);

    // REQ-O-005: every oversized drop is reported through the (optional) log sink.
    int log_calls = 0;
    h.server.set_log_sink([&log_calls](std::string_view) { ++log_calls; });

    const std::string huge(mru::plugin::overlay_protocol::kMaxLineBytes + 16, 'x');
    pump_send(h.server, c1, huge); // never newline-terminated
    CHECK(h.lines.empty());
    CHECK(h.server.dropped_lines() >= 2);
    CHECK(log_calls > 0);

    pump_send(h.server, c1, "{\"v\":1,\"type\":\"select\",\"index\":0}\n");
    CHECK(h.lines.size() == 1);
    EQ(h.lines[0], std::string("{\"v\":1,\"type\":\"select\",\"index\":0}"));
    ::close(c1);
}

} // namespace

int main() {
    return mru::test::run_all();
}
