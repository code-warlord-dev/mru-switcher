#pragma once

#include <cstddef>
#include <optional>
#include <vector>

#include "mru/domain/scope.hpp"
#include "mru/domain/window_ref.hpp"

namespace mru::domain {

class WindowSource;

// Immutable ordered list of window identities captured at session start
// (ADR-001, REQ-SNAP-001). Only pruning of invalid identities is allowed
// during a session (REQ-SNAP-003); pruning produces a new Snapshot.
class Snapshot {
  public:
    Snapshot(std::vector<WindowRef> windows, Scope scope);

    const std::vector<WindowRef> &windows() const;
    std::size_t size() const;
    bool empty() const;
    const WindowRef &at(std::size_t i) const; // pre: i < size()
    Scope scope() const;

  private:
    std::vector<WindowRef> windows_;
    Scope scope_;
};

// Stable-order prune of invalid identities. Returns std::nullopt when nothing
// survives (the caller maps that to an empty/cancelled session, REQ-SNAP-004).
std::optional<Snapshot> pruned(const Snapshot &snapshot, const WindowSource &source);

} // namespace mru::domain
