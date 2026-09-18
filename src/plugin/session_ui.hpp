#pragma once

#include <cstddef>
#include <functional>
#include <memory>

#include "mru/domain/ui_port.hpp"

namespace mru::plugin {

// REQ-UI-009: SessionController holds a UIPort& for the plugin lifetime, so the
// concrete backend cannot be swapped in place on `hyprctl reload`. This stable
// proxy rebuilds the backend from the CURRENT config on each on_session_start and
// freezes it for the session: a reload changes only the NEXT session, never the
// one in flight. The core owns no concrete backend (a factory keeps this file
// Hyprland-free). Fail-soft: a missing/throwing factory or backend must not break
// apply/cancel (REQ-UI-001).
class SessionUIBackendProxy : public mru::domain::UIPort {
  public:
    using Factory = std::function<std::unique_ptr<mru::domain::UIPort>()>;

    explicit SessionUIBackendProxy(Factory make_backend);

    void on_session_start(const mru::domain::Snapshot &snapshot, std::size_t index) override;
    void on_selection_changed(std::size_t index) override;
    void on_session_end(mru::domain::UIEndReason reason) override;

  private:
    Factory make_backend_;
    std::unique_ptr<mru::domain::UIPort> active_; // frozen for the current session
};

} // namespace mru::plugin
