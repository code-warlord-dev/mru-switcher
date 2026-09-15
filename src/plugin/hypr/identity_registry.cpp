#include "identity_registry.hpp"

#include <algorithm>
#include <utility>

#include <hyprland/src/desktop/DesktopTypes.hpp>
#include <hyprland/src/desktop/view/Window.hpp>

namespace mru::plugin {

std::uint64_t address_of(PHLWINDOW w) {
    return reinterpret_cast<std::uint64_t>(w.get());
}

WindowRef WindowIdentityRegistry::register_window(PHLWINDOW w) {
    const std::uint64_t addr = address_of(w);
    auto &entry = by_address_[addr];
    if (entry.ref.address == addr && !entry.closed) {
        // same live window: identity stays stable (no generation bump)
        return entry.ref;
    }
    // new generation for a new / re-opened window at the same address (ADR-013)
    entry.ref = WindowRef{addr, entry.next_generation++};
    entry.seq = ++seq_counter_; // registration order for fallback enumeration (ADR-015)
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

std::vector<WindowRef> WindowIdentityRegistry::live_refs_newest_first() const {
    std::vector<std::pair<std::uint64_t, WindowRef>> tmp;
    tmp.reserve(by_address_.size());
    for (const auto &[addr, entry] : by_address_) {
        if (!entry.closed)
            tmp.emplace_back(entry.seq, entry.ref);
    }
    std::sort(tmp.begin(), tmp.end(), [](const auto &a, const auto &b) { return a.first > b.first; });

    std::vector<WindowRef> out;
    out.reserve(tmp.size());
    for (const auto &[seq, ref] : tmp)
        out.push_back(ref);
    return out;
}

} // namespace mru::plugin