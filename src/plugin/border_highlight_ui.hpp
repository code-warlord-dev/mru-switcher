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

namespace mru::plugin {

// Per-window border property slots used by BorderHighlightUI. Slot names map to
// the pinned Hyprland `setprop` grammar (`active_border_color`,
// `inactive_border_color`, `border_size`); the mapping is adapter-private
// (REQ-UI-011, docs/COMPAT.md).
enum class BorderSlot { ActiveColor, InactiveColor, Size };

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

// M4 border highlight backend (ADR-017): solid colour on the virtually selected
// window. Never calls focus (REQ-UI-006); never throws into the controller
// (REQ-UI-001). Two safety rules from the R0 memo:
//   * REQ-UI-002 runtime probe: if the border API is unavailable at session start
//     the backend degrades to null for that session (no writes, one warning);
//   * restore-by-value: a colour is overridden only after both prior values were
//     read back, and restored from those captured values — never with a bare `-1`.
class BorderHighlightUI : public mru::domain::UIPort {
  public:
    // Same identity rules as focus: address + generation / weak-lock (REQ-UI-010).
    using Validator = std::function<bool(const mru::domain::WindowRef &)>;
    // Optional warn-once sink for runtime API failures (REQ-UI-001/002, FM-21).
    using Warn = std::function<void(std::string_view)>;

    BorderHighlightUI(BorderPropIo &io, Validator is_valid, BorderStyle style, std::string color, int size,
                      Warn warn = {});

    void on_session_start(const mru::domain::Snapshot &snapshot, std::size_t index) override;
    void on_selection_changed(std::size_t index) override;
    void on_session_end(mru::domain::UIEndReason reason) override;

  private:
    // Captured prior value for one slot plus whether our override was applied.
    struct SlotCapture {
        std::string value;    // prior effective value, read back before overriding
        bool applied = false; // our override write succeeded
    };
    struct WindowCapture {
        mru::domain::WindowRef ref{};
        SlotCapture active;
        SlotCapture inactive;
        bool size_applied = false;
    };

    void highlight(std::size_t index);
    void restore_window(WindowCapture &cap);
    void restore_all();
    void warn_once(std::string_view reason);
    // REQ-UI-002: probe the border API on the first valid target; false -> degrade.
    bool probe_available();

    // Style strategy (ADR-017 §3): M4 implements SolidStyle only; reserved/unknown
    // tokens were coerced to Solid at config parse (REQ-UI-007). Future styles slot
    // in here without branching in SessionController.
    const std::string &style_color() const;

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
    bool warned_ = false;   // per lifetime: at most one runtime warning (REQ-UI-001)
    bool degraded_ = false; // per session: REQ-UI-002 runtime probe failure

    std::optional<mru::domain::Snapshot> snapshot_; // frozen session snapshot copy (ADR-017)
    std::vector<WindowCapture> captures_;
};

} // namespace mru::plugin
