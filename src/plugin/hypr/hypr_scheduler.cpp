#include "hypr_scheduler.hpp"

#include <hyprland/src/managers/eventLoop/EventLoopManager.hpp>
#include <hyprland/src/managers/eventLoop/EventLoopTimer.hpp>

namespace mru::plugin {

mru::domain::JobId HyprlandSchedulerPort::schedule_after(std::uint32_t delay_ms, std::function<void()> cb) {
    const mru::domain::JobId id = next_id_++;
    auto timer = makeShared<CEventLoopTimer>(
        std::chrono::milliseconds(delay_ms),
        [this, id, cb = std::move(cb)](SP<CEventLoopTimer>, void *) {
            timers_.erase(id); // one-shot; drop bookkeeping before running cb
            cb();
        },
        nullptr);
    timers_[id] = timer;
    g_pEventLoopManager->addTimer(timer);
    return id;
}

void HyprlandSchedulerPort::cancel(mru::domain::JobId id) {
    const auto it = timers_.find(id);
    if (it == timers_.end())
        return;
    it->second->cancel(); // disarm; no callback on fire
    g_pEventLoopManager->removeTimer(it->second);
    timers_.erase(it);
}

HyprlandSchedulerPort::~HyprlandSchedulerPort() {
    for (auto &[id, t] : timers_) {
        t->cancel();
        g_pEventLoopManager->removeTimer(t);
    }
    timers_.clear();
}

} // namespace mru::plugin