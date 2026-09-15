#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "mru/domain/session_controller.hpp"

namespace mru::plugin {

// Human-readable `mru:status` payload (SPEC §3.4). The format is informative for
// v0.x and may gain fields; clients must not parse it strictly before 1.0.
std::string format_status(bool active, std::size_t index, std::size_t size, std::string_view scope,
                          std::uint64_t session_id, std::optional<mru::domain::SessionEndReason> last_end_reason);

} // namespace mru::plugin
