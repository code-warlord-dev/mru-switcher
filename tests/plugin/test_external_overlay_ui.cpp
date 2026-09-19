#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "external_overlay_ui.hpp"
#include "mru/domain/scope.hpp"
#include "mru/domain/snapshot.hpp"
#include "mru/domain/ui_port.hpp"
#include "mru/domain/window_ref.hpp"

#include "test_framework.hpp"

namespace {

using mru::domain::Scope;
using mru::domain::Snapshot;
using mru::domain::UIEndReason;
using mru::domain::WindowRef;
using mru::plugin::ExternalOverlayUI;
using mru::plugin::OverlayTransport;
using mru::plugin::OverlayWindowInfo;

// Collects every outbound line; `accept` models a present/absent peer (REQ-O-002).
struct FakeTransport : OverlayTransport {
    std::vector<std::string> lines;
    bool accept = true;

    bool send(std::string_view line) override {
        lines.emplace_back(line);
        return accept;
    }
};

WindowRef ref(std::uint64_t address) {
    return WindowRef{address, 1};
}

OverlayWindowInfo resolve(const WindowRef &r) {
    if (r.address == 0x10)
        return OverlayWindowInfo{0, "term", "foot"};
    if (r.address == 0x20)
        return OverlayWindowInfo{0, "editor", "neovide"};
    return OverlayWindowInfo{}; // unknown -> empty metadata
}

// --- T-O-06: ExternalOverlayUI emits the Appendix B sequence, and a failing
// transport (peer absent) never aborts the session (REQ-O-002/003, REQ-UI-001).
TEST(t_o_06_absent_peer_is_safe_and_sequence_is_correct) {
    FakeTransport transport;
    transport.accept = false; // peer absent: every send is a best-effort drop
    ExternalOverlayUI ui(transport, resolve);

    const Snapshot snapshot({ref(0x10), ref(0x20), ref(0x30)}, Scope::Global);
    ui.on_session_start(snapshot, 1);
    ui.on_selection_changed(2);
    ui.on_session_end(UIEndReason::Cancelled);

    CHECK(transport.lines.size() == 3);
    EQ(transport.lines[0], std::string("{\"v\":1,\"type\":\"session_start\",\"windows\":["
                                       "{\"addr\":\"0x10\",\"title\":\"term\",\"class\":\"foot\"},"
                                       "{\"addr\":\"0x20\",\"title\":\"editor\",\"class\":\"neovide\"},"
                                       "{\"addr\":\"0x30\",\"title\":\"\",\"class\":\"\"}],\"index\":1}"));
    EQ(transport.lines[1], std::string("{\"v\":1,\"type\":\"selection\",\"index\":2}"));
    EQ(transport.lines[2], std::string("{\"v\":1,\"type\":\"session_end\",\"reason\":\"cancelled\"}"));
}

// --- T-O-06 (b): a null resolver degrades to empty metadata; addr always comes
// from the ref (ADR-013). Applied end reason maps through the encoder.
TEST(t_o_06_null_resolver_and_applied_end) {
    FakeTransport transport;
    ExternalOverlayUI ui(transport, nullptr);

    const Snapshot snapshot({ref(0xabc)}, Scope::Monitor);
    ui.on_session_start(snapshot, 0);
    ui.on_session_end(UIEndReason::Applied);

    EQ(transport.lines[0], std::string("{\"v\":1,\"type\":\"session_start\",\"windows\":["
                                       "{\"addr\":\"0xabc\",\"title\":\"\",\"class\":\"\"}],\"index\":0}"));
    EQ(transport.lines[1], std::string("{\"v\":1,\"type\":\"session_end\",\"reason\":\"applied\"}"));
}

} // namespace

int main() {
    return mru::test::run_all();
}
