#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <vector>

#include "mru/domain/scheduler_port.hpp"
#include "mru/domain/window_ref.hpp"

namespace mru::domain {

class HistoryTracker {
  public:
    using Validator = std::function<bool(const WindowRef &)>;

    HistoryTracker(SchedulerPort &scheduler, Validator is_valid, std::uint32_t debounce_ms);
    ~HistoryTracker();

    HistoryTracker(const HistoryTracker &) = delete;
    HistoryTracker &operator=(const HistoryTracker &) = delete;

    void on_focus(WindowRef ref);
    void set_session_locked(bool locked);
    void set_debounce_ms(std::uint32_t debounce_ms);
    void seed(std::vector<WindowRef> initial);

    // Commit a pending debounced promotion NOW instead of letting the timer fire:
    // the scheduled job is cancelled and its window is committed immediately
    // through the same validity guard as a normal fire (REQ-H-009). No-op when no
    // job is pending. SessionController calls it at session start, before the
    // candidate list is built and before lock-in engages (REQ-H-011, ADR-021).
    void flush_pending();

    const std::vector<WindowRef> &order() const { return order_; }
    JobId pending_job() const { return pending_; }

  private:
    void cancel_pending();
    void commit(WindowRef ref);

    SchedulerPort &scheduler_;
    Validator is_valid_;
    std::uint32_t debounce_ms_;
    bool locked_ = false;
    JobId pending_ = kInvalidJobId;
    // Window behind `pending_`, so flush_pending() can commit it without waiting
    // for the timer. Invariant: (pending_ == kInvalidJobId) <=> !pending_ref_.
    std::optional<WindowRef> pending_ref_;
    std::vector<WindowRef> order_;
};

} // namespace mru::domain