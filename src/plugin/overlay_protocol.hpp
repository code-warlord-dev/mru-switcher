#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "mru/domain/ui_port.hpp"
#include "overlay_window_info.hpp"

namespace mru::plugin::overlay_protocol {

// Frozen M5 protocol version (SPEC §12 Appendix B, ADR-018).
inline constexpr int kVersion = 1;

// REQ-O-005: lines above this bound are ignored (defends the compositor thread
// against an adversarial peer stream).
inline constexpr std::size_t kMaxLineBytes = 64 * 1024;

enum class CommandType { Select, Apply, Cancel };

struct Command {
    CommandType type = CommandType::Apply;
    std::size_t index = 0; // meaningful for Select only
};

// Plugin -> peer encoders (Appendix B): one JSON object, no trailing newline.
// Strings are JSON-escaped; metadata is never interpreted by the domain.
std::string encode_session_start(const std::vector<OverlayWindowInfo> &windows, std::size_t index);
std::string encode_selection(std::size_t index);
std::string encode_session_end(mru::domain::UIEndReason reason);

// Peer -> plugin decoder (REQ-O-005). Returns std::nullopt for unknown version,
// unknown/missing type, malformed or oversized input, or a negative index; the
// caller MUST ignore those without changing session state.
std::optional<Command> parse_command(std::string_view line);

} // namespace mru::plugin::overlay_protocol
