#pragma once

#include <vector>

#include "mru/domain/window_ref.hpp"

namespace mru::plugin {

// Merge the plugin-owned MRU order (REQ-H-004a) with an adapter enumeration of
// in-scope windows (REQ-H-004b fallback/seed). Primary entries keep their order;
// fallback entries are appended in their own order. Identity = address+generation
// (REQ-ID-003), first occurrence wins. Hyprland-free by design (ADR-007/ADR-015).
std::vector<mru::domain::WindowRef> merge_mru_order(const std::vector<mru::domain::WindowRef> &primary,
                                                    const std::vector<mru::domain::WindowRef> &fallback);

} // namespace mru::plugin