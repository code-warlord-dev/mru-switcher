#pragma once

#include <cstdint>
#include <string>

namespace mru::plugin {

// Overlay-facing window description (ADR-018, SPEC Appendix B). Hyprland-free:
// the adapter fills `title`/`window_class` from the live window, while the
// overlay-facing `addr` always comes from WindowRef.address. Missing metadata is
// serialized as "".
struct OverlayWindowInfo {
    std::uint64_t address = 0;
    std::string title;
    std::string window_class;
};

} // namespace mru::plugin
