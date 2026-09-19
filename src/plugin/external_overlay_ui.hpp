#pragma once

#include <cstddef>
#include <functional>
#include <string_view>

#include "mru/domain/snapshot.hpp"
#include "mru/domain/ui_port.hpp"
#include "mru/domain/window_ref.hpp"
#include "overlay_window_info.hpp"

namespace mru::plugin {

// Output port for the external overlay (ADR-018): a single best-effort line
// channel. Implemented by the socket adapter; kept Hyprland-free so
// ExternalOverlayUI is unit-testable (ADR-007). send() MUST NOT throw (REQ-O-002)
// and returns false when the line could not be delivered (peer absent/backpressure).
class OverlayTransport {
  public:
    virtual ~OverlayTransport() = default;
    virtual bool send(std::string_view line) = 0;
};

// M5 `external` backend (ADR-018). Translates UIPort lifecycle calls into
// Appendix B lines (REQ-O-003); never focuses (REQ-UI-006) and never throws into
// the controller (REQ-UI-001/REQ-O-002). Stateless: each call encodes and sends
// immediately.
class ExternalOverlayUI : public mru::domain::UIPort {
  public:
    // Adapter-supplied metadata for a WindowRef (title/class); address is taken
    // from the ref itself. Missing metadata may return empty strings.
    using MetadataResolver = std::function<OverlayWindowInfo(const mru::domain::WindowRef &)>;

    // `transport` and `resolve` must outlive the backend.
    ExternalOverlayUI(OverlayTransport &transport, MetadataResolver resolve);

    void on_session_start(const mru::domain::Snapshot &snapshot, std::size_t index) override;
    void on_selection_changed(std::size_t index) override;
    void on_session_end(mru::domain::UIEndReason reason) override;

  private:
    OverlayTransport &transport_;
    MetadataResolver resolve_;
};

} // namespace mru::plugin
