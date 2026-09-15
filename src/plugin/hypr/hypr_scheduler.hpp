#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <unordered_map>

#include <hyprutils/memory/SharedPtr.hpp>

#include "mru/domain/scheduler_port.hpp"

class CEventLoopTimer;

namespace mru::plugin {

// Debounce scheduler for HistoryTracker only (REQ-S-011: never for apply).
// Timers run on the compositor event loop thread (safe); cancel() disarms and
// removes the timer so unload never fires into freed memory.
class HyprlandSchedulerPort : public mru::domain::SchedulerPort {
  public:
    ~HyprlandSchedulerPort() override;

    mru::domain::JobId schedule_after(std::uint32_t delay_ms, std::function<void()> cb) override;
    void cancel(mru::domain::JobId id) override;

  private:
    std::unordered_map<mru::domain::JobId, Hyprutils::Memory::CSharedPointer<CEventLoopTimer>> timers_;
    mru::domain::JobId next_id_ = 1;
};

} // namespace mru::plugin