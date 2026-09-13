#pragma once

#include <optional>
#include <vector>

#include "mru/domain/scope.hpp"
#include "mru/domain/window_ref.hpp"

namespace mru::domain {

// Compositor-facing port: supplies MRU-ordered candidates for a scope,
// validates identities, and reports the currently focused window.
// Implemented by adapters (M2); domain stays free of Hyprland types (ADR-007).
class WindowSource {
  public:
    virtual ~WindowSource() = default;

    // Candidates for the scope, most-recently-used first (REQ-SNAP-001).
    virtual std::vector<WindowRef> candidates(Scope scope) const = 0;

    // Whether address AND generation still resolve to a live window (REQ-F-005).
    virtual bool is_valid(const WindowRef &ref) const = 0;

    // Current focused window at snapshot time, if known (REQ-S-007).
    virtual std::optional<WindowRef> focused() const = 0;
};

} // namespace mru::domain
