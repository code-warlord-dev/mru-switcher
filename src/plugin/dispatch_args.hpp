#pragma once

#include <optional>
#include <string>
#include <string_view>

#include "mru/domain/scope.hpp"

namespace mru::plugin {

struct CycleArgs {
    mru::domain::Direction dir = mru::domain::Direction::Next;
    std::optional<mru::domain::Scope> scope;
    bool ok = false;
    std::string error;
};

// Grammar: mru:cycle [next|prev] [global|monitor|workspace|visible|app]
// Direction omitted -> next (REQ-DISP-001). Scope without direction is invalid.
CycleArgs parse_cycle_args(std::string_view args);

} // namespace mru::plugin