#pragma once

#include <optional>
#include <vector>

#include "config_value.hpp"
#include "identity_registry.hpp"
#include "mru/domain/window_source.hpp"

namespace mru::plugin {

// WindowSource adapter over the pinned compositor (ADR-007). MRU order comes
// from Desktop::History windowTracker fullHistory, oldest→newest, reversed.
class HyprlandWindowSource : public mru::domain::WindowSource {
  public:
    HyprlandWindowSource(WindowIdentityRegistry &registry, const PluginConfig &cfg);
    ~HyprlandWindowSource() override = default;

    std::vector<mru::domain::WindowRef> candidates(mru::domain::Scope scope) const override;
    bool                                is_valid(const mru::domain::WindowRef &ref) const override;
    std::optional<mru::domain::WindowRef> focused() const override;

  private:
    WindowIdentityRegistry &registry_;
    const PluginConfig     &cfg_;
};

} // namespace mru::plugin