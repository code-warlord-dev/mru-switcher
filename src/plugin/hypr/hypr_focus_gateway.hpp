#pragma once

#include "identity_registry.hpp"
#include "mru/domain/focus_gateway.hpp"

namespace mru::plugin {

// The single path through which the plugin focuses windows (ADR-006).
// Never called from mru:cycle; used by SessionController on apply only.
class HyprlandFocusGateway : public mru::domain::FocusGateway {
  public:
    explicit HyprlandFocusGateway(WindowIdentityRegistry &registry);

    mru::domain::FocusResult focus(const mru::domain::WindowRef &ref) override;

  private:
    WindowIdentityRegistry &registry_;
};

} // namespace mru::plugin