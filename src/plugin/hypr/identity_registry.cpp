#include "identity_registry.hpp"

#include <hyprland/src/desktop/DesktopTypes.hpp>
#include <hyprland/src/desktop/view/Window.hpp>

namespace mru::plugin {

std::uint64_t address_of(PHLWINDOW w) {
    return reinterpret_cast<std::uint64_t>(w.get());
}

WindowRef WindowIdentityRegistry::register_window(PHLWINDOW w) {
    const std::uint64_t addr = address_of(w);
    auto               &entry = by_address_[addr];
    if (entry.ref.address == addr && !entry.closed) {
        // same live window: identity stays stable (no generation bump)
        return entry.ref;
    }
    // new generation for a new / re-opened window at the same address (ADR-013)
    entry.ref    = WindowRef{addr, entry.next_generation++};
    entry.closed = false;
    entry.window = w;
    return entry.ref;
}

std::optional<WindowRef> WindowIdentityRegistry::last_ref(PHLWINDOW w) const {
    const auto it = by_address_.find(address_of(w));
    if (it == by_address_.end())
        return std::nullopt;
    return it->second.ref;
}

std::optional<PHLWINDOW> WindowIdentityRegistry::resolve(const WindowRef &ref) const {
    const auto it = by_address_.find(ref.address);
    if (it == by_address_.end() || it->second.closed || it->second.ref.generation != ref.generation)
        return std::nullopt; // invalid identity (REQ-F-005)
    return it->second.window;
}

void WindowIdentityRegistry::on_window_close(PHLWINDOW w) {
    const auto it = by_address_.find(address_of(w));
    if (it != by_address_.end())
        it->second.closed = true;
}

bool WindowIdentityRegistry::is_known(std::uint64_t addr) const {
    return by_address_.contains(addr);
}

} // namespace mru::plugin