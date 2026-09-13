#pragma once

#include <cstdint>
#include <functional>
#include <vector>

#include "mru/domain/scheduler_port.hpp"

namespace mru::domain {

class FakeClock final : public SchedulerPort {
  public:
    JobId schedule_after(std::uint32_t delay_ms, std::function<void()> cb) override;
    void cancel(JobId id) override;

    void advance(std::uint32_t ms);
    std::uint32_t now() const { return now_; }
    std::size_t pending_count() const { return jobs_.size(); }

  private:
    struct Job {
        JobId id;
        std::uint32_t run_at;
        std::function<void()> cb;
    };

    std::uint32_t now_ = 0;
    JobId next_job_ = kInvalidJobId + 1;
    std::vector<Job> jobs_;
};

} // namespace mru::domain