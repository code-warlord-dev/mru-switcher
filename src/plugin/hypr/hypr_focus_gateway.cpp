#include "hypr_focus_gateway.hpp"

#include <hyprland/src/desktop/DesktopTypes.hpp>
#include <hyprland/src/desktop/state/FocusState.hpp>
#include <hyprland/src/desktop/view/Window.hpp>

namespace mru::plugin {

HyprlandFocusGateway::HyprlandFocusGateway(WindowIdentityRegistry &registry) : registry_(registry) {}

mru::domain::FocusResult HyprlandFocusGateway::focus(const mru::domain::WindowRef &ref) {
    const auto w = registry_.resolve(ref); // PHLWINDOW; empty when unresolved (L-5)
    if (!w)
        return mru::domain::FocusResult::InvalidTarget;
    Desktop::focusState()->fullWindowFocus(w, Desktop::FOCUS_REASON_DISPATCH_FOCUSWINDOW);
    return mru::domain::FocusResult::Applied;
}

} // namespace mru::plugin