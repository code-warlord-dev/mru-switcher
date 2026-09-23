#include "hyprland_pulse_timer.hpp"

#include <exception>

#include <wayland-server-core.h>

#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/debug/log/Logger.hpp>

namespace mru::plugin {

HyprlandPulseTimer::~HyprlandPulseTimer() {
    cancel();
}

int HyprlandPulseTimer::timer_callback(void *data) {
    auto *self = static_cast<HyprlandPulseTimer *>(data);
    // Drive the user callback while the source is still armed; cancel() from
    // inside on_tick (session ended) removes the source, so do NOT re-arm then.
    if (self->on_tick_) {
        try {
            self->on_tick_();
        } catch (const std::exception &e) {
            Log::logger->log(Log::ERR, "mru-switcher: pulse timer tick threw: {}", e.what());
        } catch (...) {
            Log::logger->log(Log::ERR, "mru-switcher: pulse timer tick threw (unknown)");
        }
    }
    if (self->timer_source_)
        wl_event_source_timer_update(self->timer_source_, self->armed_period_ms_);
    return 0;
}

bool HyprlandPulseTimer::schedule(int period_ms, std::function<void()> on_tick) {
    cancel();
    if (!g_pCompositor || !g_pCompositor->m_wlEventLoop || period_ms <= 0)
        return false;
    on_tick_ = std::move(on_tick);
    arm(period_ms);
    if (!timer_source_) {
        on_tick_ = nullptr;
        return false;
    }
    return true;
}

void HyprlandPulseTimer::cancel() {
    if (timer_source_) {
        wl_event_source_remove(timer_source_);
        timer_source_ = nullptr;
    }
    on_tick_ = nullptr;
}

// Periodic arm: wl_event_loop timers are one-shot, so the re-arm is driven from
// timer_callback (above) at `armed_period_ms_`. schedule() with a new period
// cancels first and arms a fresh source.
void HyprlandPulseTimer::arm(int period_ms) {
    armed_period_ms_ = period_ms;
    timer_source_ = wl_event_loop_add_timer(g_pCompositor->m_wlEventLoop, timer_callback, this);
    if (timer_source_)
        wl_event_source_timer_update(timer_source_, period_ms);
}

} // namespace mru::plugin