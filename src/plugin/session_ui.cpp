#include "session_ui.hpp"

#include <utility>

namespace mru::plugin {

SessionUIBackendProxy::SessionUIBackendProxy(Factory make_backend) : make_backend_(std::move(make_backend)) {}

void SessionUIBackendProxy::on_session_start(const mru::domain::Snapshot &snapshot, std::size_t index) {
    // REQ-UI-009: rebuild from the config snapshot at session start. Any backend
    // left over from an unterminated session is dropped first.
    active_.reset();
    try {
        if (make_backend_)
            active_ = make_backend_();
    } catch (...) {
        active_ = nullptr; // fail-soft: a broken factory must not abort the session
    }
    if (active_)
        active_->on_session_start(snapshot, index);
}

void SessionUIBackendProxy::on_selection_changed(std::size_t index) {
    if (active_)
        active_->on_selection_changed(index);
}

void SessionUIBackendProxy::on_session_end(mru::domain::UIEndReason reason) {
    if (!active_)
        return;
    active_->on_session_end(reason);
    active_.reset(); // the frozen backend dies with its session
}

} // namespace mru::plugin
