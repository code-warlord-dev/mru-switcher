#include "mru/domain/history_tracker.hpp"

#include <algorithm>

namespace mru::domain {

HistoryTracker::HistoryTracker(SchedulerPort &scheduler, Validator is_valid, std::uint32_t debounce_ms)
    : scheduler_(scheduler), is_valid_(std::move(is_valid)), debounce_ms_(std::clamp(debounce_ms, 0u, 5000u)) {}

HistoryTracker::~HistoryTracker() {
    cancel_pending();
}

void HistoryTracker::on_focus(WindowRef ref) {
    if (locked_) // lock-in: session Active, no MRU updates (REQ-H-001)
        return;

    cancel_pending(); // replace previous debounce job (REQ-H-006, REQ-SCH-003)
    pending_ref_ = ref;
    pending_ = scheduler_.schedule_after(debounce_ms_, [this, ref] { commit(ref); });
}

void HistoryTracker::set_session_locked(bool locked) {
    if (locked && !locked_)
        cancel_pending(); // no pending job may outlive lock-in (REQ-H-006/008)
    locked_ = locked;
}

void HistoryTracker::flush_pending() {
    if (pending_ == kInvalidJobId || !pending_ref_.has_value())
        return; // nothing pending -> no-op (REQ-H-011)

    const WindowRef ref = *pending_ref_;
    cancel_pending(); // the scheduled job must not fire later (REQ-H-006/008)
    commit(ref);      // same path as a timer fire: REQ-H-009 guard applies
}

void HistoryTracker::set_debounce_ms(std::uint32_t debounce_ms) {
    debounce_ms_ = std::clamp(debounce_ms, 0u, 5000u);
}

void HistoryTracker::seed(std::vector<WindowRef> initial) {
    order_.clear();
    for (const WindowRef &ref : initial) {
        if (!is_valid_(ref))
            continue; // REQ-H-004c: destroyed identities removed
        if (std::find(order_.begin(), order_.end(), ref) != order_.end())
            continue; // REQ-H-004c: at most one entry per identity
        order_.push_back(ref);
    }
}

void HistoryTracker::cancel_pending() {
    pending_ref_.reset();
    if (pending_ == kInvalidJobId)
        return;
    scheduler_.cancel(pending_);
    pending_ = kInvalidJobId;
}

void HistoryTracker::commit(WindowRef ref) {
    pending_ = kInvalidJobId;
    pending_ref_.reset();

    if (!is_valid_(ref))
        return; // REQ-H-009: invalid before fire, never commit

    order_.erase(std::remove(order_.begin(), order_.end(), ref), order_.end());
    order_.insert(order_.begin(), std::move(ref)); // MRU-first
}

} // namespace mru::domain