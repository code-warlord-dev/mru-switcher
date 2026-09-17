#include "border_highlight_ui.hpp"

#include <utility>

namespace mru::plugin {

BorderHighlightUI::BorderHighlightUI(BorderPropIo &io, Validator is_valid, BorderStyle style, std::string color,
                                     int size, Warn warn)
    : io_(io), is_valid_(std::move(is_valid)), style_(style), color_(std::move(color)), size_(size),
      warn_(std::move(warn)) {}

void BorderHighlightUI::on_session_start(const mru::domain::Snapshot &snapshot, std::size_t index) {
    restore_all(); // defensive: never leak a highlight from a previous session
    snapshot_ = snapshot;
    highlight(index);
}

void BorderHighlightUI::on_selection_changed(std::size_t index) {
    restore_all(); // REQ-UI-004: clear the previous highlight before applying the new one
    highlight(index);
}

void BorderHighlightUI::on_session_end(mru::domain::UIEndReason) {
    restore_all(); // REQ-UI-005: full clear on every end path
    snapshot_.reset();
}

const std::string &BorderHighlightUI::style_color() const {
    switch (style_) {
    case BorderStyle::Solid:
    default:
        return color_;
    }
}

void BorderHighlightUI::highlight(std::size_t index) {
    if (!snapshot_ || index >= snapshot_->size())
        return;
    const mru::domain::WindowRef &ref = snapshot_->at(index);
    if (!is_valid_ || !is_valid_(ref))
        return; // REQ-UI-010: invalid target -> skip highlight, session continues

    captures_.push_back(capture(ref)); // REQ-UI-004: at most one highlighted window
}

BorderHighlightUI::WindowCapture BorderHighlightUI::capture(const mru::domain::WindowRef &ref) {
    WindowCapture cap;
    cap.ref = ref;
    const std::string &highlight = style_color();
    cap.active = capture_slot(ref.address, BorderSlot::ActiveColor, highlight);
    cap.inactive = capture_slot(ref.address, BorderSlot::InactiveColor, highlight);
    // border_size is best-effort: `-1` (default) leaves the size untouched. The pin
    // does not expose border_size through getprop, so restore uses `unset` to drop
    // the SET_PROP override (R0 F3/F10; colour is the M4 requirement).
    if (size_ >= 0)
        cap.size_applied = safe_set(ref.address, BorderSlot::Size, std::to_string(size_));
    return cap;
}

BorderHighlightUI::SlotCapture BorderHighlightUI::capture_slot(std::uint64_t address, BorderSlot slot,
                                                               const std::string &highlight) {
    SlotCapture out;
    try {
        out.value = io_.get(address, slot);
        out.valid = !out.value.empty();
    } catch (...) {
        out.valid = false; // fail-soft: restore falls back to best-effort clear
    }
    out.applied = safe_set(address, slot, highlight);
    return out;
}

void BorderHighlightUI::restore_window(WindowCapture &cap) {
    const auto restore_slot = [this, &cap](BorderSlot slot, const SlotCapture &s) {
        if (!s.applied && !s.valid)
            return; // never touched this slot
        if (s.valid)
            safe_set(cap.ref.address, slot, s.value); // restore-by-value (REQ-UI-005)
        else
            safe_set(cap.ref.address, slot, "-1"); // capture failed: best-effort clear
    };
    restore_slot(BorderSlot::ActiveColor, cap.active);
    restore_slot(BorderSlot::InactiveColor, cap.inactive);
    if (cap.size_applied)
        safe_set(cap.ref.address, BorderSlot::Size, "unset"); // drop the SET_PROP size override
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
