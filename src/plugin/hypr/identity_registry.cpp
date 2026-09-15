#include "identity_registry.hpp"

#include <algorithm>
#include <utility>

#include <hyprland/src/desktop/DesktopTypes.hpp>
#include <hyprland/src/desktop/view/Window.hpp>

namespace mru::plugin {

std::uint64_t address_of(const PHLWINDOW &w) {
    return reinterpret_cast<std::uint64_t>(w.get());
}

WindowRef WindowIdentityRegistry::register_window(const PHLWINDOW &w) {
    const std::uint64_t addr = address_of(w);
    auto &entry = by_address_[addr];
    // Same address AND the same live object: identity stays stable (no generation
    // bump). The weak lock() check protects against ABA — when the CWindow at this
    // address died, lock() is null even if the close event has not arrived, so a
    // re-used address is treated as a fresh identity (ADR-013, HIGH-3).
    if (entry.ref.address == addr && !entry.closed && entry.window.lock() == w)
        return entry.ref;
    // new generation for a new / re-opened window at the same address (ADR-013)
    entry.ref = WindowRef{addr, entry.next_generation++};
    entry.seq = ++seq_counter_; // registration order for fallback enumeration (ADR-015)
    entry.closed = false;
    entry.window = w;
    return entry.ref;
}

std::optional<WindowRef> WindowIdentityRegistry::last_ref(const PHLWINDOW &w) const {
    const auto it = by_address_.find(address_of(w));
    if (it == by_address_.end())
        return std::nullopt;
    return it->second.ref;
}

std::optional<WindowRef> WindowIdentityRegistry::live_ref(const PHLWINDOW &w) const {
    const auto it = by_address_.find(address_of(w));
    if (it == by_address_.end() || it->second.closed)
        return std::nullopt; // unseen or already closed (L-7)
    return it->second.ref;
}

PHLWINDOW WindowIdentityRegistry::resolve(const WindowRef &ref) const {
    const auto it = by_address_.find(ref.address);
    if (it == by_address_.end() || it->second.closed || it->second.ref.generation != ref.generation)
        return {}; // invalid identity (REQ-F-005)
    // HIGH-3: validity is ultimately decided by the weak ref. lock() is null as
    // soon as the CWindow dies, so this is robust even when no close event was
    // observed (ADR-016, §HIGH-3).
    return it->second.window.lock();
}

void WindowIdentityRegistry::on_window_close(const PHLWINDOW &w) {
    const auto it = by_address_.find(address_of(w));
    if (it != by_address_.end())
        it->second.closed = true;
    prune_closed();
}

void WindowIdentityRegistry::prune_closed() {
    // HIGH-3: drop closed entries so by_address_ does not grow without bound.
    // Generation bookkeeping is no longer needed once the entry is gone: a later
    // window re-using the address simply starts a fresh generation via
    // register_window (ADR-013). prune removes only entries that are both closed
    // AND whose weak ref is dead — a closed-but-still-alive finalizer window stays
    // resolvable by lock() until it is fully destroyed.
    for (auto it = by_address_.begin(); it != by_address_.end();) {
        if (it->second.closed && !it->second.window.lock())
            it = by_address_.erase(it);
        else
            ++it;
    }
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