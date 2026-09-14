#include "hypr_window_source.hpp"

#include <hyprland/src/desktop/DesktopTypes.hpp>
#include <hyprland/src/desktop/history/WindowHistoryTracker.hpp>
#include <hyprland/src/desktop/state/FocusState.hpp>
#include <hyprland/src/desktop/view/Window.hpp>

namespace mru::plugin {

HyprlandWindowSource::HyprlandWindowSource(WindowIdentityRegistry &registry, const PluginConfig &cfg)
    : registry_(registry), cfg_(cfg) {}

std::vector<mru::domain::WindowRef> HyprlandWindowSource::candidates(mru::domain::Scope scope) const {
    if (scope != mru::domain::Scope::Global)
        return {}; // M2: global only; other scopes wired in M3
    std::vector<mru::domain::WindowRef> out;
    const auto                         &history = Desktop::History::windowTracker()->fullHistory();
    out.reserve(history.size());
    for (auto it = history.rbegin(); it != history.rend(); ++it) {
        const auto w = it->lock();
        if (!w || !registry_.is_known(address_of(w)))
            continue;
        if (const auto ref = registry_.last_ref(w))
            out.push_back(*ref);
    }
    return out;
}

bool HyprlandWindowSource::is_valid(const mru::domain::WindowRef &ref) const {
    return registry_.resolve(ref).has_value();
}

std::optional<mru::domain::WindowRef> HyprlandWindowSource::focused() const {
    const auto w = Desktop::focusState()->window();
    if (!w)
        return std::nullopt;
    if (!registry_.is_known(address_of(w)))
        return std::nullopt;
    return registry_.last_ref(w);
}

} // namespace mru::plugin