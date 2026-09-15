#pragma once

#include <cstdint>
#include <optional>
#include <unordered_map>
#include <vector>

#include <hyprutils/memory/SharedPtr.hpp>

#include "mru/domain/window_ref.hpp"

#include <hyprland/src/desktop/DesktopTypes.hpp>

namespace mru::plugin {

using mru::domain::WindowRef;

// Raw pointer value of the underlying CWindow (stable id on this pin, ADR-013).
std::uint64_t address_of(PHLWINDOW w);

// Address + generation identity bookkeeping for windows seen by the plugin
// (REQ-F-005). A live window keeps a stable WindowRef; a closed window invalidates
// it; a window re-opened at the same address gets a fresh generation (ADR-013).
class WindowIdentityRegistry {
  public:
    // Registers (or re-returns the identity of) a live window.
    WindowRef register_window(PHLWINDOW w);
    // Last known identity for a window, if the plugin has seen it.
    std::optional<WindowRef> last_ref(PHLWINDOW w) const;
    // Resolves a WindowRef to the live window; nullopt when closed or stale generation.
    std::optional<PHLWINDOW> resolve(const WindowRef &ref) const;
    // Marks the window closed: future resolve() fails for its current generation.
    void on_window_close(PHLWINDOW w);
    // HIGH-3: drop entries that are closed AND whose weak ref is dead, bounding
    // by_address_ growth. Call after close/destroy batches.
    void prune_closed();
    bool is_known(std::uint64_t addr) const;
    // Live identities, most recently registered first: fallback enumeration order
    // for windows the plugin has not seen focus events for yet (ADR-015).
    std::vector<WindowRef> live_refs_newest_first() const;

  private:
    struct Entry {
        WindowRef ref{};
        std::uint64_t next_generation = 1;
        std::uint64_t seq = 0; // monotonic registration order (ADR-015)
        bool closed = false;
        PHLWINDOWREF window; // weak: validity by lock(), not lifetime (HIGH-3, ADR-016)
    };
    std::unordered_map<std::uint64_t, Entry> by_address_;
    std::uint64_t seq_counter_ = 0;
};

} // namespace mru::plugin