#pragma once

#include "mru/domain/window_ref.hpp"

namespace mru::domain {

// Structured focus outcome (REQ-F-008). Adapters map this to a concrete
// compositor result; the domain only branches on these three values.
enum class FocusResult { Applied, InvalidTarget, Failed };

// The single path through which compositor focus changes (ADR-006).
class FocusGateway {
  public:
    virtual ~FocusGateway() = default;

    // Applies focus to the resolved live window. Called at most once per apply.
    virtual FocusResult focus(const WindowRef &ref) = 0;
};

} // namespace mru::domain
