#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "config_value.hpp"
#include "mru/domain/snapshot.hpp"
#include "mru/domain/ui_port.hpp"
#include "mru/domain/window_ref.hpp"
#include "workspace_navigator.hpp"

namespace mru::plugin {

// Per-window border property slots used by BorderHighlightUI. Slot names map to
// the pinned Hyprland `setprop` grammar (`active_border_color`,
// `inactive_border_color`, `border_size`, `opacity`, `opacity_inactive`); the
// mapping is adapter-private (REQ-UI-011, docs/COMPAT.md). Alpha slots carry the
// per-window alpha override used by border_style = dim (ADR-028/REQ-UI-014):
// BOTH channels are overridden because the render uses the active channel for
// the focused window and the inactive channel for every other window (pin
// evidence: src/desktop/view/Window.cpp applyAlpha on alpha()/alphaInactive()).
enum class BorderSlot { ActiveColor, InactiveColor, Size, Alpha, AlphaInactive };

// Pure colour helper: a darker variant of the border colour for the pulse throb
// (ADR-028/REQ-UI-013). Accepts the verbatim setprop grammar — hex 0xAARRGGBB /
// 0xRRGGBB and rgb(...)/rgba(...) — and scales the RGB channels by 0.5 while
// keeping the alpha channel unchanged. Unparseable input is returned unchanged
// (constant-colour fallback). Hyprland-free; declared here for unit tests.
std::string darker_color(std::string_view color);

// Small output port for border-property reads/writes. Implemented in the
// Hyprland adapter (HyprctlBorderPropIo) but kept here as a Hyprland-free
// interface so BorderHighlightUI is unit-testable (ADR-007, cpp-plugin-architecture).
// Failure is signalled by an empty get() / false set(); the caller stays fail-soft
// and never throws into SessionController (REQ-UI-001).
class BorderPropIo {
  public:
    virtual ~BorderPropIo() = default;

    // Effective value of the slot, or empty when unavailable/failed (REQ-UI-011).
    virtual std::string get(std::uint64_t address, BorderSlot slot) = 0;
    // Applies the slot override; false on soft failure. Must not throw.
    virtual bool set(std::uint64_t address, BorderSlot slot, const std::string &value) = 0;
};

// ADR-028 / REQ-UI-013: pulse-period ticker port (Hyprland-free). The adapter
// (HyprlandPulseTimer) arms a `wl_event_loop` timer on the compositor main
// thread; schedule() returns false when no timer is available and the caller
// falls back to a constant colour (fail-soft, REQ-UI-001). Never re-entrant:
// on_tick is only ever invoked from the event loop, never from schedule().
class PulseTimerPort {
  public:
    virtual ~PulseTimerPort() = default;
    // (Re-)arms a one-shot tick `period_ms` away; on_tick fires on the event
    // loop. Returns false when the host cannot provide a timer (caller must not
    // throw). Cancels any previous scheduled tick.
    virtual bool schedule(int period_ms, std::function<void()> on_tick) = 0;
    // Removes a scheduled tick; a no-op when none is armed. Safe to call from
    // the tick itself.
    virtual void cancel() = 0;
};

// Null object: used by tests and by hyprland-free call sites; always declines
// scheduling (caller falls back to the constant colour path).
class NullPulseTimerPort final : public PulseTimerPort {
  public:
    bool schedule(int, std::function<void()>) override { return false; }
    void cancel() override {}
};

inline PulseTimerPort &null_pulse_timer_port() {
    static NullPulseTimerPort port;
    return port;
}

// ADR-028: effective pulse/dim tunables (clamped in the pure config layer).
struct BorderStyleParams {
    int pulse_period_ms = 1000; // full throb cycle in ms (REQ-UI-013)
    double dim_alpha = 0.7;     // dim strength for non-selected ring windows (REQ-UI-014)
};

// M4 border highlight backend (ADR-017): solid colour on the virtually selected
// window. Never calls focus (REQ-UI-006); never throws into the controller
// (REQ-UI-001). Two safety rules from the R0 memo:
//   * REQ-UI-002 runtime probe: if the border API is unavailable at session start
//     the backend degrades to null for that session (no writes, one warning);
//   * restore-by-value: colour AND size slots are restored from prior values read
//     back through getprop and normalized to the setprop grammar (live pin
//     evidence: docs/agent-state/reports/2026-09-18-m4-s3-nest-smoke.md) — never
//     with a bare `-1`; `unset` is only the size fallback when the prior read
//     failed (R0 F3/F10).
//
// ADR-026 / REQ-UI-012 (view follows selection): while a session is active the
// backend also drives a WorkspaceNavigator so the highlighted window is never
// off-screen — begin() at session start, ensure_visible() inside highlight()
// (so the session-start probe and every selection change cover it), end(reason)
// at session end. Elevation is skipped with the highlight when the runtime probe
// degrades the backend. The navigator is never the source of a window focus
// (REQ-F-003 / REQ-UI-006) and is driven fail-soft (REQ-UI-001); the default
// argument supplies the shared no-op used by ui=border with
// `selection_follow_workspace = false` (exact pre-ADR-026 behaviour).
class BorderHighlightUI : public mru::domain::UIPort {
  public:
    // Same identity rules as focus: address + generation / weak-lock (REQ-UI-010).
    using Validator = std::function<bool(const mru::domain::WindowRef &)>;
    // Optional warn-once sink for runtime API failures (REQ-UI-001/002, FM-21).
    using Warn = std::function<void(std::string_view)>;

    BorderHighlightUI(BorderPropIo &io, Validator is_valid, BorderStyle style, std::string color, int size,
                      Warn warn = {}, WorkspaceNavigator &navigator = null_workspace_navigator(),
                      BorderStyleParams params = {}, PulseTimerPort &pulse_timer = null_pulse_timer_port());

    void on_session_start(const mru::domain::Snapshot &snapshot, std::size_t index) override;
    void on_selection_changed(std::size_t index) override;
    void on_session_end(mru::domain::UIEndReason reason) override;

  private:
    // Captured prior value for one slot plus whether our override was applied.
    struct SlotCapture {
        std::string value;    // prior effective value, read back before overriding
                              // and normalized to the setprop grammar
        bool applied = false; // our override write succeeded
    };
    struct WindowCapture {
        mru::domain::WindowRef ref{};
        SlotCapture active;
        SlotCapture inactive;
        // ADR-028 / REQ-UI-014 (border_style = dim): prior per-window alpha
        // (opacity and opacity_inactive) for the non-selected ring windows,
        // captured before the dim override and restored on selection change /
        // session end. Only both-successfully-read windows get dimmed.
        SlotCapture alpha;
        SlotCapture alpha_inactive;
        bool size_applied = false;  // our size override write succeeded
        bool size_captured = false; // prior size read back via border_size getprop
        std::string size_value;     // prior effective size (pure integer) when captured
        // True for the highlighted (selected) window, false for the dim ring
        // targets (REQ-UI-014). restore_window() restores both flavours.
        bool selected = false;
    };

    void highlight(std::size_t index);
    // ADR-028/REQ-UI-014: dim every valid non-selected ring window to dim_alpha_.
    // Skips (fail-soft) any window whose alpha channels aren't both readable back
    // (restore-by-value safety, REQ-UI-004). dim_alpha_ >= 1.0 disables dimming.
    void dim_ring(std::size_t selected_index);
    void restore_window(WindowCapture &cap);
    void restore_all();
    void warn_once(std::string_view reason);
    // REQ-UI-002: probe the border API on the first valid target; false -> degrade.
    bool probe_available();

    // Style strategy (ADR-017 §3, ADR-028): Solid = constant colour, Pulse =
    // colour throb between the base colour and darker_color(color_) on a
    // half-cycle timer, Dim = constant highlight colour + dim of the ring.
    const std::string &style_color() const;
    // ADR-028/REQ-UI-013: (re)arm the pulse timer; on failure degrades to the
    // constant colour (fail-soft, REQ-UI-001).
    void start_pulse();
    void cancel_pulse();
    void pulse_tick();

    // Reads a prior effective value into `out`; false = failed/empty (no override).
    bool read_slot(std::uint64_t address, BorderSlot slot, SlotCapture &out);
    // Applies an override, recording success in `applied`; fail-soft.
    void apply_slot(std::uint64_t address, BorderSlot slot, const std::string &value, bool &applied);
    bool safe_set(std::uint64_t address, BorderSlot slot, const std::string &value);

    BorderPropIo &io_;
    Validator is_valid_;
    BorderStyle style_ = BorderStyle::Solid;
    std::string color_;
    int size_ = -1;
    Warn warn_;
    // ADR-026: view-follow port. Lifecycle is tied to the session: begin() on
    // on_session_start, ensure_visible() on every highlight, end(reason) on
    // on_session_end.
    WorkspaceNavigator &navigator_;
    bool warned_ = false;   // per backend INSTANCE: at most one runtime warning (REQ-UI-001);
                            // production backends are per-session, cross-session dedupe lives
                            // in the plugin's warn sink
    bool degraded_ = false; // per session: REQ-UI-002 runtime probe failure

    std::optional<mru::domain::Snapshot> snapshot_; // frozen session snapshot copy (ADR-017)
    std::vector<WindowCapture> captures_;
    // ADR-028 / REQ-UI-013: half-cycle pulse timer state. The timer only makes
    // sense while a session is active and a highlight is applied; it is cancelled
    // in restore_all() (session end / selection change). pulse_dark_ toggles the
    // base colour to its darker variant on every tick.
    PulseTimerPort &pulse_timer_;
    BorderStyleParams params_;
    int half_cycle_ms_ = 500; // pulse_period_ms / 2 (min 100ms via [200,10000] clamp)
    bool pulse_dark_ = false;
};

} // namespace mru::plugin
