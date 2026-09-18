#include "border_highlight_ui.hpp"

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include <utility>

namespace mru::plugin {

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
    // border_size is best-effort int prop; -1 leaves it untouched. The pin does not
    // expose border_size through getprop, so restore uses `unset` (R0 F3/F10).
    if (size_ >= 0)
        apply_slot(ref.address, BorderSlot::Size, std::to_string(size_), cap.size_applied);
    captures_.push_back(std::move(cap));
}

bool BorderHighlightUI::read_slot(std::uint64_t address, BorderSlot slot, SlotCapture &out) {
    try {
        out.value = io_.get(address, slot);
    } catch (...) {
        return false;
    }
    return !out.value.empty();
}

void BorderHighlightUI::apply_slot(std::uint64_t address, BorderSlot slot, const std::string &value, bool &applied) {
    applied = safe_set(address, slot, value);
}

void BorderHighlightUI::restore_window(WindowCapture &cap) {
    // REQ-UI-005: restore only the slots we actually overrode, from captured values.
    if (cap.active.applied)
        safe_set(cap.ref.address, BorderSlot::ActiveColor, cap.active.value);
    if (cap.inactive.applied)
        safe_set(cap.ref.address, BorderSlot::InactiveColor, cap.inactive.value);
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
