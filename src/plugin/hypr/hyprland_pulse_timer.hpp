#pragma once

#include <functional>

#include "border_highlight_ui.hpp"

struct wl_event_source;

namespace mru::plugin {

// Hyprland adapter for PulseTimerPort (ADR-028/REQ-UI-013): a `wl_event_loop`
// timer on the compositor main thread (`g_pCompositor->m_wlEventLoop`), re-armed
// after every tick with `wl_event_source_timer_update`. Same main-loop mechanism
// as the fd watches in HyprlandOverlaySocket but time-driven. The callback never
// throws out: on_tick is wrapped in try/catch so a domain bug can't kill the
// event loop. cancel() inside on_tick (e.g. a session ending mid-tick) safely
// disarms: no re-arm happens once source_ is gone.
class HyprlandPulseTimer final : public PulseTimerPort {
  public:
    HyprlandPulseTimer() = default;
    ~HyprlandPulseTimer() override;

    bool schedule(int period_ms, std::function<void()> on_tick) override;
    void cancel() override;

  private:
    static int timer_callback(void *data);
    void arm(int period_ms);

    wl_event_source *timer_source_ = nullptr;
    int armed_period_ms_ = 0;
    std::function<void()> on_tick_;
};

} // namespace mru::plugin