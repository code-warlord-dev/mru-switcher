#pragma once

#include <cstdint>
#include <string>

namespace mru::domain {

// Opaque snapshot-time description of a window; the adapter's only duty is
// translating a compositor window into this struct (ADR-016 __2__). The domain
// compares ids/classes but never interprets compositor types (ADR-007).
//
// `hidden` mirrors the compositor special-workspace hidden test: true exactly
// when the window sits on a special workspace that is NOT currently shown on a
// monitor. The uniform special-workspace rule (ADR-016 __3__, REQ-SC-002a) is
// therefore applied once, in the shared validity gate of scope_matches
// (`mapped && !hidden`), and holds for all five scopes alike: a hidden
// scratchpad is never a candidate, a shown one is. No separate
// "special-workspace shown" flag exists because `hidden` captures exactly that
// distinction without inventing a second source of truth.
//
// `fading` is deliberately absent: ADR-016 __2__ fixes WindowMeta to these
// five fields, and the adapter keeps the REQ-SNAP-002 candidate gate
// (`m_isMapped && !isHidden()`, issue #18 S2-7).
struct WindowMeta {
    std::uint64_t monitor_id{};
    std::uint64_t workspace_id{};
    bool mapped = false;
    bool hidden = false;
    std::string app_class; // class(), NOT initialClass() (REQ-SC-002b, S2-6)
};

} // namespace mru::domain
