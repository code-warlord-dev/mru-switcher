#include <cstddef>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "mru/domain/snapshot.hpp"
#include "mru/domain/ui_port.hpp"
#include "mru/domain/window_ref.hpp"
#include "session_ui.hpp"

#include "test_framework.hpp"

namespace {

using mru::domain::Scope;
using mru::domain::Snapshot;
using mru::domain::UIEndReason;
using mru::domain::UIPort;
using mru::domain::WindowRef;
using mru::plugin::SessionUIBackendProxy;

WindowRef ref(std::uint64_t address, std::uint64_t generation = 1) {
    return WindowRef{address, generation};
}

// Records lifecycle calls, tagged with a backend "generation", into a shared log
// so the log outlives the frozen backend destroyed at session end.
struct RecordingUI : UIPort {
    std::vector<std::string> *log;
    std::string tag;

    RecordingUI(std::vector<std::string> *l, std::string t) : log(l), tag(std::move(t)) {}

    void on_session_start(const Snapshot &, std::size_t index) override {
        log->push_back(tag + ":start:" + std::to_string(index));
    }
    void on_selection_changed(std::size_t index) override { log->push_back(tag + ":sel:" + std::to_string(index)); }
    void on_session_end(UIEndReason) override { log->push_back(tag + ":end"); }
};

// REQ-UI-009: the proxy freezes a backend per session; a config change (modelled
// by the factory's `generation`) applies to the next session, not mid-session.
TEST(t_ui_009_backend_swap_next_session) {
    std::vector<std::string> log;
    int generation = 0;
    SessionUIBackendProxy proxy([&log, &generation]() -> std::unique_ptr<UIPort> {
        return std::make_unique<RecordingUI>(&log, "B" + std::to_string(generation));
    });
    const Snapshot snap({ref(0xA)}, Scope::Global);

    proxy.on_session_start(snap, 0);
    EQ(log, std::vector<std::string>{"B0:start:0"});

    generation = 1; // "hyprctl reload" changes the config mid-session
    proxy.on_selection_changed(1);
    EQ(log.size(), static_cast<std::size_t>(2));
    EQ(log[1], std::string("B0:sel:1")); // still the frozen B0 backend
    for (const std::string &entry : log)
        CHECK(entry.rfind("B1:", 0) != 0);

    proxy.on_session_end(UIEndReason::Applied);
    EQ(log.back(), std::string("B0:end"));

    proxy.on_session_start(snap, 0); // the next session builds the new backend
    EQ(log.back(), std::string("B1:start:0"));
}

// REQ-UI-009: no backend lives outside a session; stray calls are no-ops.
TEST(t_ui_009_no_session_is_safe) {
    std::vector<std::string> log;
    SessionUIBackendProxy proxy(
        [&log]() -> std::unique_ptr<UIPort> { return std::make_unique<RecordingUI>(&log, "B"); });

    proxy.on_selection_changed(0);
    proxy.on_session_end(UIEndReason::Cancelled);
    CHECK(log.empty());
}

// REQ-UI-001: a throwing factory does not abort the session.
TEST(t_ui_009_factory_throw_is_swallowed) {
    SessionUIBackendProxy proxy([]() -> std::unique_ptr<UIPort> { throw std::runtime_error("factory failed"); });
    const Snapshot snap({ref(0xA)}, Scope::Global);

    proxy.on_session_start(snap, 0);
    proxy.on_selection_changed(0);
    proxy.on_session_end(UIEndReason::Applied);
    CHECK(true); // reaching here proves nothing escaped
}

} // namespace

int main() {
    return mru::test::run_all();
}
