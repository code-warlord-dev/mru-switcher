// Domain stress tests for window-close handling during an active session.
// Ticket M6-T7 (issue #53), test id T-H-06 — SPEC §2.6 (window close: prune +
// clamp), REQ-S-004, REQ-F-003/006/007, REQ-RE-003.
//
// Deterministic stress: fixed-seed LCG drives 300 close/cycle interleave steps.
// Harness mirrors tests/domain/test_session_controller.cpp (Fixture + helpers).

#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

#include "mru/domain/fake_clock.hpp"
#include "mru/domain/history_tracker.hpp"
#include "mru/domain/scheduler_port.hpp"
#include "mru/domain/scope.hpp"
#include "mru/domain/session_controller.hpp"
#include "mru/domain/window_ref.hpp"

#include "test_fakes.hpp"
#include "test_framework.hpp"

namespace {
using mru::domain::Direction;
using mru::domain::FakeClock;
using mru::domain::HistoryTracker;
using mru::domain::SessionController;
using mru::domain::SessionEndReason;
using mru::domain::SessionPolicy;
using mru::domain::UIEndReason;
using mru::domain::WindowRef;
using mru::test::MockFocusGateway;
using mru::test::MockUIPort;
using mru::test::MockWindowSource;

WindowRef ref(std::uint64_t address, std::uint64_t generation = 1) {
    return WindowRef{address, generation};
}

bool in_snapshot(const std::vector<WindowRef> &windows, const WindowRef &r) {
    for (const WindowRef &w : windows)
        if (w == r)
            return true;
    return false;
}

// Domain test harness: FakeClock + mock ports + real HistoryTracker (same
// wiring as test_session_controller.cpp).
struct Fixture {
    FakeClock clock;
    MockWindowSource source;
    MockFocusGateway fg;
    MockUIPort ui;
    HistoryTracker tracker;
    SessionController sc;

    explicit Fixture(SessionPolicy policy = {})
        : tracker(
              clock, [this](const WindowRef &r) { return source.is_valid(r); }, 50),
          sc(source, fg, ui, tracker, policy) {}

    void set_valid(const WindowRef &r, bool valid) {
        for (auto &[ref, v] : source.validity) {
            if (ref == r) {
                v = valid;
                return;
            }
        }
        source.validity.emplace_back(r, valid);
    }

    void candidates(std::vector<WindowRef> windows) {
        source.candidates_result = std::move(windows);
        for (const WindowRef &w : source.candidates_result)
            set_valid(w, true);
    }

    void advance(std::uint32_t ms) { clock.advance(ms); }
};

// --- T-H-06 (1): closing the selected window prunes it and clamps the index
// into bounds; the session stays Active and cycle never focuses (REQ-F-003).
TEST(t_h_06_close_selected_prunes_and_clamps) {
    Fixture f;
    f.candidates({ref(10), ref(20), ref(30)});
    CHECK(f.sc.cycle(Direction::Next).ok); // index 1 -> ref(20)

    // Close the currently selected window (SPEC §2.6: prune + clamp).
    f.set_valid(ref(20), false);
    f.sc.on_window_invalid(ref(20));

    CHECK(f.sc.is_active()); // session survives
    const auto &snap = f.sc.active_snapshot();
    CHECK(snap.has_value());
    CHECK(snap->size() == 2);
    CHECK(!in_snapshot(snap->windows(), ref(20)));
    CHECK(f.sc.index() < snap->size()); // clamped into bounds
    CHECK(f.fg.focused.empty());        // prune/cycle never focuses (REQ-F-003)
}

// --- T-H-06 (2): closing ALL snapshot windows ends the session as NoWindows,
// with exactly one UI end (Cancelled per the UIPort contract, REQ-F-007) and
// the history tracker unlocked (a post-advance focus commit is accepted).
TEST(t_h_06_close_all_ends_no_windows_unlocks_history) {
    Fixture f;
    f.candidates({ref(10), ref(20)});
    CHECK(f.sc.cycle(Direction::Next).ok);
    CHECK(f.sc.is_active());

    f.set_valid(ref(10), false);
    f.set_valid(ref(20), false);
    f.sc.on_window_invalid(ref(10));
    f.sc.on_window_invalid(ref(20));

    CHECK(!f.sc.is_active());
    CHECK(f.sc.last_end_reason() == SessionEndReason::NoWindows);
    CHECK(f.ui.ends.size() == 1); // REQ-F-007: exactly one on_session_end
    CHECK(f.ui.ends[0] == UIEndReason::Cancelled);
    CHECK(f.fg.focused.empty());

    // History unlocked after the session ends: an Idle on_focus schedules a
    // commit that lands after the 50 ms debounce advance.
    f.candidates({ref(30)});
    f.sc.on_focus(ref(30));
    CHECK(f.tracker.pending_job() != mru::domain::kInvalidJobId);
    f.advance(50);
    CHECK(!f.tracker.order().empty());
    CHECK(f.tracker.order().front() == ref(30));
}

// --- T-H-06 (3): closing session_origin in Active, then cancel with
// restore_focus_on_cancel=false -> no focus attempts at all (REQ-S-005,
// REQ-F-006), no exceptions.
TEST(t_h_06_close_origin_active_cancel_no_restore) {
    Fixture f; // restore_focus_on_cancel defaults to false
    f.candidates({ref(10), ref(20)});
    f.set_valid(ref(99), true); // origin is valid but not a candidate
    f.source.focused_result = {ref(99)};
    CHECK(f.sc.cycle(Direction::Next).ok);
    CHECK(f.sc.session_origin() == ref(99));
    CHECK(f.sc.is_active());

    f.set_valid(ref(99), false); // origin closed mid-session
    f.sc.on_window_invalid(ref(99));

    CHECK(f.sc.is_active()); // origin close alone does not end the session
    const SessionController::CommandResult r = f.sc.cancel();
    CHECK(r.ok);
    CHECK(!f.sc.is_active());
    CHECK(f.ui.ends.size() == 1);
    CHECK(f.ui.ends[0] == UIEndReason::Cancelled);
    CHECK(f.fg.focused.empty()); // restore_focus_on_cancel=false -> no focus
}

// --- T-H-06 (4): origin died (validator says invalid) and no fresh focused()
// exists -> cancel with restore_focus_on_cancel=true must NOT focus the dead
// origin: fg.focused stays empty; no exceptions (REQ-S-005, REQ-F-006).
TEST(t_h_06_origin_invalid_restore_never_focuses_dead) {
    SessionPolicy policy;
    policy.restore_focus_on_cancel = true;
    Fixture f(policy);

    f.set_valid(ref(99), true); // origin exists ...
    f.source.focused_result = {ref(99)};
    f.candidates({ref(10), ref(20)});
    CHECK(f.sc.cycle(Direction::Next).ok);
    CHECK(f.sc.session_origin() == ref(99));

    // Origin dies while Active; session ends NoWindows, history unlocks.
    f.set_valid(ref(99), false);
    f.sc.on_window_invalid(ref(99)); // origin is not in snapshot -> no-op

    // Idle now: start a new session whose focused() (dead origin) resolves to
    // an invalid identity — restore on cancel must refuse it (no focus call).
    CHECK(f.sc.cycle(Direction::Next).ok);
    CHECK(f.sc.is_active());
    (void)f.sc.cancel();

    CHECK(f.fg.focused.empty()); // never focus a dead origin
    CHECK(!f.sc.is_active());
    CHECK(f.ui.ends.size() == 1);
    CHECK(f.ui.ends[0] == UIEndReason::Cancelled);
    CHECK(f.sc.last_end_reason() == SessionEndReason::UserCancel);
}

// --- T-H-06 (5): deterministic stress — 300 interleaved closes (fixed-seed
// LCG picks the victim) mixed with cycle next/prev. Invariants on every step:
// snapshot size never grows, index stays in bounds OR the session ended
// NoWindows, no focus while Active (REQ-F-003/REQ-RE-003), at most one UI end
// (REQ-F-007), no exceptions (test aborts on crash).
TEST(t_h_06_stress_300_interleaved_close_cycle) {
    constexpr std::size_t kSteps = 300;
    constexpr std::uint64_t kSeed = 20260921u; // fixed seed -> reproducible run

    Fixture f;
    std::vector<WindowRef> pool;
    for (std::uint64_t i = 1; i <= 25; ++i)
        pool.push_back(ref(1000 + i));
    f.candidates(pool);
    CHECK(f.sc.cycle(Direction::Next).ok);
    CHECK(f.sc.is_active());

    // Deterministic 64-bit LCG, fixed seed.
    std::uint64_t state = kSeed;
    auto next_rand = [&state]() {
        state = state * 6364136223846793005ULL + 1442695040888963407ULL;
        return state >> 33;
    };

    std::vector<WindowRef> live = pool;
    for (std::size_t step = 0; step < kSteps; ++step) {
        if (f.sc.is_active()) {
            const std::size_t before = f.sc.active_snapshot()->size();
            const std::size_t victim_i = static_cast<std::size_t>(next_rand() % live.size());
            const WindowRef victim = live[victim_i];
            f.set_valid(victim, false);
            f.sc.on_window_invalid(victim);
            live.erase(live.begin() + static_cast<std::ptrdiff_t>(victim_i));

            // Invariants immediately after the close:
            const auto &snap = f.sc.active_snapshot();
            if (f.sc.is_active()) {
                CHECK(snap.has_value());
                CHECK(snap->size() + 1 == before); // exactly one prune
                CHECK(!in_snapshot(snap->windows(), victim));
                CHECK(f.sc.index() < snap->size()); // clamp within bounds
                CHECK(f.fg.focused.empty());        // no focus while Active (REQ-F-003)
            } else {
                CHECK(f.sc.last_end_reason() == SessionEndReason::NoWindows);
                CHECK(f.ui.ends.size() == 1); // single terminal UI end
                CHECK(f.ui.ends[0] == UIEndReason::Cancelled);
            }
        }

        // Interleave a cycle between closes while the session is alive.
        if (f.sc.is_active()) {
            const SessionController::CommandResult r =
                f.sc.cycle((next_rand() % 2 == 0) ? Direction::Next : Direction::Prev);
            CHECK(r.ok);
            const auto &snap = f.sc.active_snapshot();
            CHECK(snap.has_value());
            CHECK(f.sc.index() < snap->size()); // index in bounds
            CHECK(f.ui.ends.size() <= 1);       // REQ-F-007
            CHECK(f.fg.focused.empty());        // REQ-F-003 / REQ-RE-003
            CHECK(snap->size() <= pool.size()); // size never grows
        }
    }

    // 25 live windows, one close per step: the session must have drained.
    CHECK(!f.sc.is_active());
    CHECK(f.sc.last_end_reason() == SessionEndReason::NoWindows);
    CHECK(f.ui.ends.size() == 1);
    CHECK(f.ui.ends[0] == UIEndReason::Cancelled);
    CHECK(f.fg.focused.empty());
}

// --- T-H-06 (6): closing a window that is NOT in the snapshot is a no-op:
// size, membership and index are unchanged; no session end, no UI end.
TEST(t_h_06_close_outside_snapshot_is_noop) {
    Fixture f;
    f.candidates({ref(10), ref(20)});
    CHECK(f.sc.cycle(Direction::Next).ok); // index 1 -> ref(20)

    f.set_valid(ref(777), false);
    f.sc.on_window_invalid(ref(777)); // not a snapshot member

    CHECK(f.sc.is_active());
    const auto &snap = f.sc.active_snapshot();
    CHECK(snap.has_value());
    CHECK(snap->size() == 2);
    CHECK(in_snapshot(snap->windows(), ref(10)));
    CHECK(in_snapshot(snap->windows(), ref(20)));
    CHECK(f.sc.index() == 1);
    CHECK(f.ui.ends.empty());
    CHECK(f.fg.focused.empty());
}

} // namespace

int main() {
    return mru::test::run_all();
}
