#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "mru/domain/session_controller.hpp"

namespace mru::plugin {

// Human-readable `mru:status` payload (SPEC §3.4). Frozen since the M6-T1
// contract freeze (SPEC §0): `active= index= size= scope= session= last_end=`
// in order; new keys may only be appended (additive, tolerate unknown).
std::string format_status(bool active, std::size_t index, std::size_t size, std::string_view scope,
                          std::uint64_t session_id, std::optional<mru::domain::SessionEndReason> last_end_reason);

} // namespace mru::plugin
