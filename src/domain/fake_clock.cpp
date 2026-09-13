#include "mru/domain/fake_clock.hpp"

#include <algorithm>

namespace mru::domain {

JobId FakeClock::schedule_after(std::uint32_t delay_ms, std::function<void()> cb) {
  const JobId id = next_job_++;
  jobs_.push_back({id, now_ + delay_ms, std::move(cb)});
  return id;
}

void FakeClock::cancel(JobId id) {
  jobs_.erase(std::remove_if(jobs_.begin(), jobs_.end(),
                             [id](const Job& j) { return j.id == id; }),
              jobs_.end());
}

void FakeClock::advance(std::uint32_t ms) {
  now_ += ms;

  // Snapshot the due job ids, then run them in time order (insertion order on
  // ties). Jobs scheduled by callbacks during this sweep are not re-run.
  std::vector<JobId> due;
  due.reserve(jobs_.size());
  for (const auto& job : jobs_) {
    if (job.run_at <= now_)
      due.push_back(job.id);
  }

  for (JobId id : due) {
    const auto it = std::find_if(jobs_.begin(), jobs_.end(),
                                 [id](const Job& j) { return j.id == id; });
    if (it == jobs_.end())
      continue; // cancelled while pending
    Job job = std::move(*it);
    jobs_.erase(it);
    job.cb();
  }
}

} // namespace mru::domain