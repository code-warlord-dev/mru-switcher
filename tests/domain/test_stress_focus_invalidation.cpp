// M6-T8 (issue #54) — focus-invalidation stress harness (T-H-07).
//
// Domain-only regression battery for SPEC §2.8 apply-after-invalidation
// (REQ-F-006 at-most-one-successful-focus, REQ-F-007 exactly one UI end,
// REQ-F-008 structured FocusResult with a single bounded retry):
//
//   1. apply: InvalidTarget then Applied  -> Applied, exactly one success.
//   2. apply: InvalidTarget twice         -> InvalidSelection, exactly 2 attempts.
//   3. apply: Failed                      -> FocusFailed, exactly 1 attempt.
//   4. storm: 500 applies × (InvalidTarget, Applied) -> exactly 2 gateway calls
//      per apply (1000 total), linear, no recursion, no exceptions.
//   5. scope=monitor with every window carried away -> NoWindows, no focus,
//      history unlocked again (debounce commit accepted).
//
// Self-contained harness (no Hyprland types, no gtest): FakeClock + mock
// ports + the real HistoryTracker / SessionController.
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

#include "mru/domain/fake_clock.hpp"
#include "mru/domain/history_tracker.hpp"
#include "mru/domain/scheduler_port.hpp"
#include "mru/domain/session_controller.hpp"
#include "mru/domain/window_ref.hpp"

#include "test_fakes.hpp"
#include "test_framework.hpp"

namespace {

using mru::domain::Direction;
using mru::domain::FakeClock;
using mru::domain::FocusResult;
using mru::domain::HistoryTracker;
using mru::domain::kInvalidJobId;
using mru::domain::Scope;
using mru::domain::SessionController;
using mru::domain::SessionEndReason;
using mru::domain::SessionPolicy;
using mru::domain::UIEndReason;
using mru::domain::WindowRef;
using mru::test::MockFocusGateway;
using mru::test::MockUIPort;
using mru::test::MockWindowSource;

WindowRef ref(std::uint64_t address, std::uint64_t generation = 1) { return WindowRef{address, generation}; }

// Domain test harness: FakeClock + mock ports + real HistoryTracker (same
// shape as tests/domain/test_session_controller.cpp).
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
        for (auto &[entry, v] : source.validity) {
            if (entry == r) {
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
};

// Open a session and return the currently selected identity (start_offset
// defaults to Second, so cycle lands on the last candidate of a pair).
WindowRef open_session(Fixture &f, const std::vector<WindowRef> &windows) {
    f.candidates(windows);
    CHECK(f.sc.cycle(Direction::Next).ok);
    CHECK(f.sc.is_active());
    return f.sc.active_snapshot()->at(f.sc.index());
}

// --- T-H-07 (scenario 1): apply-after-invalidation repaired by the bounded
// retry. Exactly one *successful* focus (REQ-F-006), exactly one UI end
// (REQ-F-007), internal reason Applied (REQ-F-008/009).
TEST(t_h_07_01_invalid_then_applied_single_success) {
    Fixture f;
    const WindowRef selected = open_session(f, {ref(10), ref(20)});
    f.fg.script = {FocusResult::InvalidTarget, FocusResult::Applied};

    const SessionController::CommandResult r = f.sc.apply();
    CHECK(r.ok);
    CHECK(f.fg.focused.size() == 2); // exactly one retry, no more
    CHECK(f.fg.focused[0] == selected);
    CHECK(f.fg.focused[1] == selected);
    CHECK(f.ui.ends.size() == 1);
    CHECK(f.ui.ends[0] == UIEndReason::Applied);
    CHECK(f.sc.last_end_reason() == SessionEndReason::Applied);
    CHECK(!f.sc.is_active());
}

// --- T-H-07 (scenario 2): repeated InvalidTarget is bounded — exactly two
// attempts, then the session ends InvalidSelection with no successful focus
// (REQ-F-008, §2.8 symmetry) and the UI sees one Cancelled end (REQ-F-007).
TEST(t_h_07_02_invalid_twice_ends_invalid_selection) {
    Fixture f;
    (void)open_session(f, {ref(10), ref(20)});
    f.fg.result = FocusResult::InvalidTarget; // policy: always invalid

    const SessionController::CommandResult r = f.sc.apply();
    CHECK(!r.ok);
    CHECK(r.error == "selection invalid");
    CHECK(f.fg.focused.size() == 2); // bounded: first attempt + single retry
    CHECK(f.ui.ends.size() == 1);
    CHECK(f.ui.ends[0] == UIEndReason::Cancelled);
    CHECK(f.sc.last_end_reason() == SessionEndReason::InvalidSelection);
    CHECK(!f.sc.is_active());
}

// --- T-H-07 (scenario 3): FocusResult::Failed is definitive — no retry, the
// session ends FocusFailed with exactly one attempt (REQ-F-008, FM-10).
TEST(t_h_07_03_focus_failed_single_attempt) {
    Fixture f;
    (void)open_session(f, {ref(10), ref(20)});
    f.fg.result = FocusResult::Failed;

    const SessionController::CommandResult r = f.sc.apply();
    CHECK(!r.ok);
    CHECK(r.error == "focus failed");
    CHECK(f.fg.focused.size() == 1); // no retry after Failed
    CHECK(f.ui.ends.size() == 1);
    CHECK(f.ui.ends[0] == UIEndReason::Cancelled);
    CHECK(f.sc.last_end_reason() == SessionEndReason::FocusFailed);
    CHECK(!f.sc.is_active());
}

// --- T-H-07 (scenario 4): storm — 500 applies where every first attempt
// returns InvalidTarget and the retry lands. Each apply must consume exactly
// two FocusGateway calls (cumulative total 1000), end with exactly one UI end
// and one successful focus. Any unbounded retry would push the cumulative
// count above 2·(i+1) and fail here; an escaped exception would abort the run.
TEST(t_h_07_04_storm_invalid_then_applied_1000_calls) {
    Fixture f;
    constexpr int kStorm = 500;

    for (int i = 0; i < kStorm; ++i) {
        const std::vector<WindowRef> windows = {ref(static_cast<std::uint64_t>(1000 + i)),
                                                ref(static_cast<std::uint64_t>(2000 + i))};
        const WindowRef selected = open_session(f, windows);
        f.fg.script = {FocusResult::InvalidTarget, FocusResult::Applied};

        const SessionController::CommandResult r = f.sc.apply();
        CHECK(r.ok);
        CHECK(f.fg.focused.size() == 2 * (static_cast<std::size_t>(i) + 1)); // linear: 2 per apply
        CHECK(f.fg.focused.back() == selected);                              // retry hit the target
        CHECK(f.ui.ends.size() == static_cast<std::size_t>(i) + 1);          // one end per apply
        CHECK(f.ui.ends.back() == UIEndReason::Applied);
        CHECK(!f.sc.is_active());
    }

    CHECK(f.fg.focused.size() == 1000); // storm total: 500 × 2, never more
    CHECK(f.ui.ends.size() == 500);     // exactly one UI end per apply
    CHECK(f.sc.session_id() == 500); // one new session per storm iteration
    CHECK(!f.sc.is_active());
}

// --- T-H-07 (scenario 5): monitor disconnect storm — a monitor-scope session
// whose every snapshot entry is carried away. §2.8 step 2 prunes the snapshot
// to empty on the first resolve, so the gateway is never reached (the
// InvalidTarget×2 policy below would only matter for a buggy controller) and
// the session ends NoWindows with zero focus calls (REQ-F-006/007). After the
// storm the history lock is released again: a debounce commit is accepted
// (REQ-H-001 unlock path).
TEST(t_h_07_05_monitor_scope_all_carried_away_no_focus_unlocked_history) {
    SessionPolicy p;
    p.default_scope = Scope::Monitor;
    Fixture f(p);
    (void)open_session(f, {ref(10), ref(20)});
    CHECK(f.sc.active_snapshot()->scope() == Scope::Monitor);

    f.set_valid(ref(10), false); // monitor gone: every entry carried away
    f.set_valid(ref(20), false);
    f.fg.result = FocusResult::InvalidTarget; // even the gateway would refuse

    const SessionController::CommandResult r = f.sc.apply();
    CHECK(!r.ok);
    CHECK(r.error == "no windows");
    CHECK(f.fg.focused.empty()); // no focus at all (REQ-F-006)
    CHECK(f.ui.ends.size() == 1);
    CHECK(f.ui.ends[0] == UIEndReason::Cancelled);
    CHECK(f.sc.last_end_reason() == SessionEndReason::NoWindows);
    CHECK(!f.sc.is_active());

    // History unlocked (REQ-H-001): a fresh valid window's focus event is
    // debounced and committed once the timer fires.
    const WindowRef newcomer = ref(30); // another monitor survived
    f.set_valid(newcomer, true);
    f.sc.on_focus(newcomer);
    CHECK(f.tracker.pending_job() != kInvalidJobId);
    f.clock.advance(50);
    CHECK(f.tracker.order().size() == 1);
    CHECK(f.tracker.order().front() == newcomer);
}

} // namespace

int main() {
    return mru::test::run_all();
}