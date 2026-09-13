#pragma once

#include <cstdint>
#include <functional>
#include <vector>

#include "mru/domain/scheduler_port.hpp"
#include "mru/domain/window_ref.hpp"

namespace mru::domain {

class HistoryTracker {
public:
  using Validator = std::function<bool(const WindowRef&)>;

  HistoryTracker(SchedulerPort& scheduler, Validator is_valid, std::uint32_t debounce_ms);
  ~HistoryTracker();

  HistoryTracker(const HistoryTracker&) = delete;
  HistoryTracker& operator=(const HistoryTracker&) = delete;

  void on_focus(WindowRef ref);
  void set_session_locked(bool locked);
  void set_debounce_ms(std::uint32_t debounce_ms);
  void seed(std::vector<WindowRef> initial);

  const std::vector<WindowRef>& order() const { return order_; }
  JobId pending_job() const { return pending_; }

private:
  void cancel_pending();
  void commit(WindowRef ref);

  SchedulerPort& scheduler_;
  Validator is_valid_;
  std::uint32_t debounce_ms_;
  bool locked_ = false;
  JobId pending_ = kInvalidJobId;
  std::vector<WindowRef> order_;
};

} // namespace mru::domain