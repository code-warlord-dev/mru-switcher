#pragma once

#include <optional>
#include <vector>

#include "config_value.hpp"
#include "identity_registry.hpp"
#include "mru/domain/history_tracker.hpp"
#include "mru/domain/window_source.hpp"

namespace mru::plugin {

// WindowSource adapter over the pinned compositor (ADR-007). The plugin-owned
// HistoryTracker order is the primary candidate order (REQ-H-004a, ADR-015); the
// compositor enumeration is a fallback tail and a register-on-sight seed source
// (REQ-H-004b) for windows opened before plugin load.
class HyprlandWindowSource : public mru::domain::WindowSource {
  public:
    HyprlandWindowSource(WindowIdentityRegistry &registry, const PluginConfig &cfg,
                         const mru::domain::HistoryTracker &tracker);
    ~HyprlandWindowSource() override = default;

    std::vector<mru::domain::WindowRef> candidates(mru::domain::Scope scope) const override;
    bool is_valid(const mru::domain::WindowRef &ref) const override;
    std::optional<mru::domain::WindowRef> focused() const override;

  private:
    WindowIdentityRegistry &registry_;
    const PluginConfig &cfg_;
    const mru::domain::HistoryTracker &tracker_;
};

} // namespace mru::plugin