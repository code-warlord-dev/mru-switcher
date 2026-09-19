#include <cstdint>
#include <string>
#include <vector>

#include "mru/domain/ui_port.hpp"
#include "overlay_protocol.hpp"

#include "test_framework.hpp"

namespace {

using mru::domain::UIEndReason;
using mru::plugin::OverlayWindowInfo;
using mru::plugin::overlay_protocol::Command;
using mru::plugin::overlay_protocol::CommandType;

OverlayWindowInfo info(std::uint64_t addr, std::string title, std::string cls) {
    return OverlayWindowInfo{addr, std::move(title), std::move(cls)};
}

// --- T-O-03: session_start serializes per Appendix B incl. JSON escaping and
// the `0x`-prefixed lowercase address (REQ-O-003).
TEST(t_o_03_encode_session_start) {
    const std::string line = mru::plugin::overlay_protocol::encode_session_start(
        {info(0x1a2b, "term", "foot"), info(0x0, "a\"b\\c\nd", "")}, 1);

    EQ(line, std::string("{\"v\":1,\"type\":\"session_start\",\"windows\":["
                         "{\"addr\":\"0x1a2b\",\"title\":\"term\",\"class\":\"foot\"},"
                         "{\"addr\":\"0x0\",\"title\":\"a\\\"b\\\\c\\nd\",\"class\":\"\"}],"
                         "\"index\":1}"));
    CHECK(line.find('\n') == std::string::npos); // newline escaped, framing safe
}

// --- T-O-03 (b): selection + session_end reason mapping (REQ-O-003, REQ-F-009).
TEST(t_o_03_encode_selection_and_end) {
    EQ(mru::plugin::overlay_protocol::encode_selection(2),
       std::string("{\"v\":1,\"type\":\"selection\",\"index\":2}"));
    EQ(mru::plugin::overlay_protocol::encode_session_end(UIEndReason::Applied),
       std::string("{\"v\":1,\"type\":\"session_end\",\"reason\":\"applied\"}"));
    EQ(mru::plugin::overlay_protocol::encode_session_end(UIEndReason::Cancelled),
       std::string("{\"v\":1,\"type\":\"session_end\",\"reason\":\"cancelled\"}"));
}

// --- T-O-04: peer control messages decode (REQ-O-004).
TEST(t_o_04_parse_peer_commands) {
    const auto select = mru::plugin::overlay_protocol::parse_command("{\"v\":1,\"type\":\"select\",\"index\":3}");
    CHECK(select.has_value());
    CHECK(select->type == CommandType::Select);
    EQ(select->index, std::size_t{3});

    const auto apply = mru::plugin::overlay_protocol::parse_command("{\"v\":1,\"type\":\"apply\"}");
    CHECK(apply.has_value());
    CHECK(apply->type == CommandType::Apply);

    const auto cancel = mru::plugin::overlay_protocol::parse_command("  {\"v\": 1, \"type\": \"cancel\"}\r\n");
    CHECK(cancel.has_value());
    CHECK(cancel->type == CommandType::Cancel);
}

// --- T-O-05: unknown version/type, malformed JSON, oversized input, negative
// index -> std::nullopt; the session is never affected (REQ-O-005).
TEST(t_o_05_reject_invalid_peer_input) {
    CHECK(!mru::plugin::overlay_protocol::parse_command("{\"v\":2,\"type\":\"apply\"}").has_value());
    CHECK(!mru::plugin::overlay_protocol::parse_command("{\"v\":1,\"type\":\"focus\"}").has_value());
    CHECK(!mru::plugin::overlay_protocol::parse_command("{\"v\":1,\"type\":\"select\"}").has_value());
    CHECK(!mru::plugin::overlay_protocol::parse_command("{\"v\":1,\"type\":\"select\",\"index\":-1}").has_value());
    CHECK(!mru::plugin::overlay_protocol::parse_command("not json").has_value());
    CHECK(!mru::plugin::overlay_protocol::parse_command("{").has_value());
    CHECK(!mru::plugin::overlay_protocol::parse_command("{\"type\":\"apply\"}").has_value()); // no version
    CHECK(!mru::plugin::overlay_protocol::parse_command("").has_value());

    std::string huge = "{\"v\":1,\"type\":\"apply\",\"pad\":\"";
    huge.append(mru::plugin::overlay_protocol::kMaxLineBytes, 'x');
    huge += "\"}";
    CHECK(!mru::plugin::overlay_protocol::parse_command(huge).has_value());
}

} // namespace

int main() {
    return mru::test::run_all();
}
