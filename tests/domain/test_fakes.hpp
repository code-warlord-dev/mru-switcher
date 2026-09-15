#pragma once
#include <functional>

#include <cstddef>
#include <optional>
#include <utility>
#include <vector>

#include "mru/domain/focus_gateway.hpp"
#include "mru/domain/snapshot.hpp"
#include "mru/domain/ui_port.hpp"
#include "mru/domain/window_source.hpp"

namespace mru::test {

using mru::domain::FocusGateway;
using mru::domain::FocusResult;
using mru::domain::Scope;
using mru::domain::Snapshot;
using mru::domain::UIEndReason;
using mru::domain::UIPort;
using mru::domain::WindowRef;
using mru::domain::WindowSource;

struct MockWindowSource : WindowSource {
    std::vector<WindowRef> candidates_result;         // MRU-first; empty unless set
    std::vector<std::pair<WindowRef, bool>> validity; // is_valid lookup
    std::vector<WindowRef> focused_result;            // stack; last = current focus
    mutable std::size_t candidates_calls = 0;

    // Helper: any candidate that is valid in this mock.
    WindowRef lt_valid_ref() const {
        for (const WindowRef &r : candidates_result) {
            if (is_valid(r))
                return r;
        }
        return WindowRef{};
    }

    std::vector<WindowRef> candidates(Scope) const override {
        ++candidates_calls;
        return candidates_result;
    }

    bool is_valid(const WindowRef &r) const override {
        for (const auto &[ref, valid] : validity) {
            if (ref == r)
                return valid;
        }
        return false; // unknown identity is invalid by default
    }

    std::optional<WindowRef> focused() const override {
        if (focused_result.empty())
            return std::nullopt;
        return focused_result.back();
    }
};

struct MockFocusGateway : FocusGateway {
    std::vector<WindowRef> focused; // calls in order (REQ-F-006: at most one success)
    FocusResult result = FocusResult::Applied;
    std::vector<FocusResult> script; // if non-empty, consumed in order; else `result`

    FocusResult focus(const WindowRef &r) override {
        focused.push_back(r);
        if (script.empty())
            return result;
        const FocusResult next = script.front();
        script.erase(script.begin());
        return next;
    }
};

// ReentrantFocusGateway: emulates the compositor emitting a synchronous
// window.active *inside* FocusGateway::focus() (as Hyprland does within
// fullWindowFocus). The callback re-enters SessionController::on_focus, which
// must be swallowed by lock-in while a session is Active (REQ-RE-003) and must
// not corrupt state during apply / plugin_shutdown (audit BLOCKER-2 check).
struct ReentrantFocusGateway : FocusGateway {
    std::function<void(const WindowRef &)> on_reentrant_focus; // caller sets up reentry
    std::vector<WindowRef> focused;
    FocusResult result = FocusResult::Applied;

    FocusResult focus(const WindowRef &r) override {
        focused.push_back(r);
        if (on_reentrant_focus && !reentered) {
            reentered = true;
            on_reentrant_focus(r); // synchronous window.active inside focus()
        }
        return result;
    }

    bool reentered = false;
};

struct MockUIPort : UIPort {
    std::vector<std::pair<Snapshot, std::size_t>> starts; // (snapshot copy, index)
    std::vector<std::size_t> changes;
    std::vector<UIEndReason> ends;

    void on_session_start(const Snapshot &snapshot, std::size_t index) override {
        starts.emplace_back(snapshot, index);
    }
    void on_selection_changed(std::size_t index) override { changes.push_back(index); }
    void on_session_end(UIEndReason reason) override { ends.push_back(reason); }
};

} // namespace mru::test