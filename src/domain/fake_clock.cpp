#include "mru/domain/fake_clock.hpp"

#include <algorithm>
#include <utility>

namespace mru::domain {

JobId FakeClock::schedule_after(std::uint32_t delay_ms, std::function<void()> cb) {
    const JobId id = next_job_++;
    jobs_.push_back({id, now_ + delay_ms, std::move(cb)});
    return id;
}

void FakeClock::cancel(JobId id) {
    jobs_.erase(std::remove_if(jobs_.begin(), jobs_.end(), [id](const Job &j) { return j.id == id; }), jobs_.end());
}

void FakeClock::advance(std::uint32_t ms) {
    now_ += ms;

    // Snapshot the due jobs, then run them in time order — (run_at, id), so a
    // test can depend on the sweep order and ties resolve by schedule order
    // rather than vector insertion order. Jobs scheduled by callbacks during
    // this sweep are not re-run.
    std::vector<std::pair<std::uint32_t, JobId>> due;
    due.reserve(jobs_.size());
    for (const auto &job : jobs_) {
        if (job.run_at <= now_)
            due.emplace_back(job.run_at, job.id);
    }
    std::sort(due.begin(), due.end());

    for (const auto &entry : due) {
        const JobId id = entry.second;
        const auto it = std::find_if(jobs_.begin(), jobs_.end(), [id](const Job &j) { return j.id == id; });
        if (it == jobs_.end())
            continue; // cancelled while pending
        Job job = std::move(*it);
        jobs_.erase(it);
        job.cb();
    }
}

} // namespace mru::domain