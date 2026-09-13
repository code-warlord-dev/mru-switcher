#pragma once

#include <cstdint>
#include <functional>

namespace mru::domain {

using JobId = std::uint64_t;
constexpr JobId kInvalidJobId = 0;

class SchedulerPort {
public:
  virtual ~SchedulerPort() = default;
  virtual JobId schedule_after(std::uint32_t delay_ms, std::function<void()> cb) = 0;
  virtual void cancel(JobId id) = 0;
};

} // namespace mru::domain
