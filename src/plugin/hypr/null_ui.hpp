#pragma once

#include "mru/domain/ui_port.hpp"

namespace mru::plugin {

// No-op UI backend (REQ-UI-001/003). M2 ships only this; border/external are
// M4/M5. Must never throw or otherwise break apply/cancel.
class NullUI : public mru::domain::UIPort {
  public:
    void on_session_start(const mru::domain::Snapshot &, std::size_t) override {}
    void on_selection_changed(std::size_t) override {}
    void on_session_end(mru::domain::UIEndReason) override {}
};

} // namespace mru::plugin