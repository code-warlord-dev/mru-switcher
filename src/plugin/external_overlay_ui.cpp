#include "external_overlay_ui.hpp"

#include <utility>
#include <vector>

#include "overlay_protocol.hpp"

namespace mru::plugin {

ExternalOverlayUI::ExternalOverlayUI(OverlayTransport &transport, MetadataResolver resolve)
    : transport_(transport), resolve_(std::move(resolve)) {}

void ExternalOverlayUI::on_session_start(const mru::domain::Snapshot &snapshot, std::size_t index) {
    std::vector<OverlayWindowInfo> windows;
    windows.reserve(snapshot.size());
    for (const mru::domain::WindowRef &ref : snapshot.windows()) {
        OverlayWindowInfo info = resolve_ ? resolve_(ref) : OverlayWindowInfo{};
        info.address = ref.address; // addr is authoritative from the identity (ADR-013)
        windows.push_back(std::move(info));
    }
    transport_.send(overlay_protocol::encode_session_start(windows, index)); // REQ-O-003
}

void ExternalOverlayUI::on_selection_changed(std::size_t index) {
    transport_.send(overlay_protocol::encode_selection(index)); // REQ-O-003
}

void ExternalOverlayUI::on_session_end(mru::domain::UIEndReason reason) {
    transport_.send(overlay_protocol::encode_session_end(reason)); // REQ-O-003
}

} // namespace mru::plugin
