#pragma once

#include "mru/domain/focus_context.hpp"
#include "mru/domain/scope.hpp"
#include "mru/domain/window_meta.hpp"

namespace mru::domain {

// Whether `candidate` belongs to `scope`, anchored to `focus` captured at
// snapshot time (ADR-016 __2__, REQ-SC-002/002a/002b). Pure free function, no
// state, no compositor types. The shared validity gate (mapped && !hidden)
// enforces the uniform special-workspace rule for all five scopes before any
// scope-specific comparison runs.
bool scope_matches(Scope scope, const WindowMeta &candidate, const FocusContext &focus);

} // namespace mru::domain