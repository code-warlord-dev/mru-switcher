#include "border_highlight_ui.hpp"

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace mru::plugin {

// Live pin 0.56.2 evidence (M4-S3 nest smoke, evidence 09-border-size-probe.txt in
// docs/agent-state/reports/2026-09-18-m4-s3-nest-smoke.md):
//   * getprop answers "<hex> <N>deg" (e.g. "ff44cc88 0deg") for colour slots and a
//     bare integer for border_size, while setprop accepts ONLY 0x…/rgb()/rgba()
//     forms WITHOUT the angle suffix — a suffix-echoed value written back verbatim
//     parses to an empty gradient, i.e. an invisible border;
//   * border_size IS exposed through getprop (the earlier "no getprop for
//     border_size" assumption in R0 F3 was wrong), so the prior size can be
//     restored by value.
// normalize_capture turns every BorderPropIo::get() reply into a value setprop
// accepts (REQ-UI-004 exact restore, REQ-UI-005 no stuck/invisible borders):
//   1. trimmed; empty stays empty (capture-failure path unchanged);
//   2. a trailing whitespace-separated token ending in "deg" is dropped;
//   3. colour values not already 0x-prefixed and not rgb(/rgba( form get "0x"
//      prepended (6- and 8-hex-digit forms; already-0x values keep any nonzero
//      hex-digit count, e.g. short seeded forms);
//   4. Size slot: pure digit string, empty otherwise;
//   5. anything still not matching a 0x-prefixed hex value / rgb(...) / rgba(...)
//      returns empty -> existing read_slot/highlight fail-soft skip applies.
std::string normalize_capture(std::string raw, BorderSlot slot) {
    constexpr std::string_view kWhitespace = " \t\r\n";
    const auto first = raw.find_first_not_of(kWhitespace);
    if (first == std::string::npos)
        return {};
    const auto last = raw.find_last_not_of(kWhitespace);
    raw = raw.substr(first, last - first + 1);

    if (slot == BorderSlot::Size) {
        if (raw.find_first_not_of("0123456789") != std::string::npos)
            return {}; // not a pure integer -> treat as unreadable
        return raw;
    }

    // Colour slot: drop a trailing whitespace-separated token ending in "deg"
    // (getprop echoes the gradient angle; setprop must not receive it).
    const std::size_t space = raw.find_last_of(kWhitespace);
    if (space != std::string::npos) {
        const std::string_view tail = std::string_view{raw}.substr(space + 1);
        if (tail.size() >= 3 && tail.substr(tail.size() - 3) == "deg")
            raw = std::string{raw.substr(0, space)};
        else
            return {}; // extra tokens other than the angle are not restorable
    }

    // setprop grammar: 0xRRGGBB | 0xAARRGGBB | rgb(...) | rgba(...), 0x REQUIRED.
    const bool rgb_form = raw.starts_with("rgb(") || raw.starts_with("rgba(");
    if (!raw.starts_with("0x") && !rgb_form) {
        if (raw.size() != 6 && raw.size() != 8)
            return {}; // not a recognised hex length
        raw.insert(0, "0x");
    }
    if (rgb_form)
        return raw;

    // Already-0x-prefixed: accept any nonzero hex-digit count (existing seeded
    // short forms like "0xa1" must round-trip unchanged; getprop echoes the
    // configured width). A bare value must be 6 or 8 digits before prepending.
    const std::string_view hex_digits = std::string_view{raw}.substr(2);
    if (hex_digits.empty())
        return {};
    if (hex_digits.find_first_not_of("0123456789abcdefABCDEF") != std::string::npos)
        return {}; // non-hex digits are not restorable
    return raw;
}

BorderHighlightUI::BorderHighlightUI(BorderPropIo &io, Validator is_valid, BorderStyle style, std::string color,
                                     int size, Warn warn)
    : io_(io), is_valid_(std::move(is_valid)), style_(style), color_(std::move(color)), size_(size),
      warn_(std::move(warn)) {}

void BorderHighlightUI::on_session_start(const mru::domain::Snapshot &snapshot, std::size_t index) {
    restore_all(); // defensive: never leak a highlight from a previous session
    snapshot_ = snapshot;
    degraded_ = false; // REQ-UI-002: the degrade decision is per session
    if (!probe_available()) {
        degraded_ = true;
        warn_once("border API unavailable; highlight disabled for this session");
        return;
    }
    highlight(index);
}

void BorderHighlightUI::on_selection_changed(std::size_t index) {
    restore_all(); // REQ-UI-004: clear the previous highlight before applying the new one
    highlight(index);
}

void BorderHighlightUI::on_session_end(mru::domain::UIEndReason) {
    restore_all(); // REQ-UI-005: full clear on every end path
    snapshot_.reset();
    degraded_ = false;
}

const std::string &BorderHighlightUI::style_color() const {
    switch (style_) {
    case BorderStyle::Solid:
    default:
        return color_;
    }
}

bool BorderHighlightUI::probe_available() {
    if (!snapshot_)
        return true; // nothing to probe; no writes will happen anyway
    for (std::size_t i = 0; i < snapshot_->size(); ++i) {
        const mru::domain::WindowRef &ref = snapshot_->at(i);
        if (!is_valid_ || !is_valid_(ref))
            continue;
        try {
            return !io_.get(ref.address, BorderSlot::ActiveColor).empty();
        } catch (...) {
            return false;
        }
    }
    return true; // no valid target to probe
}

void BorderHighlightUI::highlight(std::size_t index) {
    if (degraded_ || !snapshot_ || index >= snapshot_->size())
        return;
    const mru::domain::WindowRef &ref = snapshot_->at(index);
    if (!is_valid_ || !is_valid_(ref))
        return; // REQ-UI-010: invalid target -> skip highlight, session continues

    // Restore safety (R0 F10): only override once BOTH prior colour values were
    // read back, otherwise a failed restore could leave an invisible border.
    WindowCapture cap;
    cap.ref = ref;
    if (!read_slot(ref.address, BorderSlot::ActiveColor, cap.active) ||
        !read_slot(ref.address, BorderSlot::InactiveColor, cap.inactive)) {
        warn_once("border property read failed; skipping highlight");
        return;
    }

    const std::string &highlight = style_color();
    apply_slot(ref.address, BorderSlot::ActiveColor, highlight, cap.active.applied);
    apply_slot(ref.address, BorderSlot::InactiveColor, highlight, cap.inactive.applied);
    // border_size IS readable via getprop on the pin (live evidence: nest smoke,
    // evidence 09-border-size-probe.txt in
    // docs/agent-state/reports/2026-09-18-m4-s3-nest-smoke.md — the earlier R0 F3
    // claim that it was not exposed was wrong). Capture the prior integer BEFORE
    // the override write (a post-write read would see our own value) and restore
    // by value; `unset` is only the fallback when that read fails.
    if (size_ >= 0) {
        SlotCapture prior_size;
        const bool size_read = read_slot(ref.address, BorderSlot::Size, prior_size);
        apply_slot(ref.address, BorderSlot::Size, std::to_string(size_), cap.size_applied);
        if (cap.size_applied && size_read) {
            cap.size_captured = true;
            cap.size_value = prior_size.value;
        }
    }
    captures_.push_back(std::move(cap));
}

bool BorderHighlightUI::read_slot(std::uint64_t address, BorderSlot slot, SlotCapture &out) {
    try {
        out.value = normalize_capture(io_.get(address, slot), slot);
    } catch (...) {
        return false;
    }
    return !out.value.empty();
}

void BorderHighlightUI::apply_slot(std::uint64_t address, BorderSlot slot, const std::string &value, bool &applied) {
    applied = safe_set(address, slot, value);
}

void BorderHighlightUI::restore_window(WindowCapture &cap) {
    // REQ-UI-005: restore only the slots we actually overrode, from captured values
    // (already normalized to the setprop grammar at capture time).
    if (cap.active.applied)
        safe_set(cap.ref.address, BorderSlot::ActiveColor, cap.active.value);
    if (cap.inactive.applied)
        safe_set(cap.ref.address, BorderSlot::InactiveColor, cap.inactive.value);
    if (!cap.size_applied)
        return;
    if (cap.size_captured)
        safe_set(cap.ref.address, BorderSlot::Size, cap.size_value); // exact prior size
    else
        safe_set(cap.ref.address, BorderSlot::Size, "unset"); // read failed: drop the override
}

void BorderHighlightUI::restore_all() {
    for (WindowCapture &cap : captures_)
        restore_window(cap);
    captures_.clear();
}

bool BorderHighlightUI::safe_set(std::uint64_t address, BorderSlot slot, const std::string &value) {
    try {
        if (io_.set(address, slot, value))
            return true;
        warn_once("border property write failed");
    } catch (...) {
        warn_once("border property write threw");
    }
    return false;
}

void BorderHighlightUI::warn_once(std::string_view reason) {
    if (warned_)
        return;
    warned_ = true;
    if (!warn_)
        return;
    try {
        warn_(reason);
    } catch (...) {
        // never let the warn sink break the session (REQ-UI-001)
    }
}

} // namespace mru::plugin
