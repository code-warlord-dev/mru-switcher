#pragma once

#include <cstddef>

#include "mru/domain/snapshot.hpp"

namespace mru::domain {

// Coarse UI-facing end reason (REQ-F-009); the richer internal SessionEndReason
// stays in the domain. UI failures must not abort apply/cancel (REQ-UI-001).
enum class UIEndReason { Applied, Cancelled };

class UIPort {
  public:
    virtual ~UIPort() = default;

    virtual void on_session_start(const Snapshot &snapshot, std::size_t index) = 0;
    virtual void on_selection_changed(std::size_t index) = 0;
    virtual void on_session_end(UIEndReason reason) = 0;
};

} // namespace mru::domain
