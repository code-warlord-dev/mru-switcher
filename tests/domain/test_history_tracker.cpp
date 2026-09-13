#include "mru/domain/fake_clock.hpp"
#include "mru/domain/history_tracker.hpp"
#include "mru/domain/window_ref.hpp"

#include "test_framework.hpp"

namespace {
using mru::domain::FakeClock;
using mru::domain::HistoryTracker;
using mru::domain::JobId;
using mru::domain::kInvalidJobId;
using mru::domain::WindowRef;
} // namespace

// T-H-02: rapid focuses commit only the last after the quiet period.
TEST(h02_quiet_period_commits_last) {
    FakeClock clock;
    HistoryTracker tracker(clock, [](const WindowRef &) { return true; }, 200);
    const WindowRef a{0x100, 1};
    const WindowRef b{0x200, 1};
    const WindowRef c{0x300, 1};

    tracker.on_focus(a); // due at t=200
    clock.advance(100);  // t=100, quiet
    tracker.on_focus(b); // replaces A, due at t=300
    clock.advance(50);   // t=150, quiet
    tracker.on_focus(c); // replaces B, due at t=350
    clock.advance(150);  // t=300, still quiet

    CHECK(tracker.order().empty());

    clock.advance(50); // t=350 -> only C fires and commits
    const auto &order = tracker.order();
    CHECK(order.size() == 1);
    CHECK(order[0] == c);
    CHECK(tracker.pending_job() == kInvalidJobId);
}

// T-H-03: a focus before the debounce fires cancels the previous pending job.
TEST(h03_reuse_replaces_pending_job) {
    FakeClock clock;
    HistoryTracker tracker(clock, [](const WindowRef &) { return true; }, 200);
    const WindowRef a{0x100, 1};
    const WindowRef b{0x200, 1};

    tracker.on_focus(a);
    const JobId first = tracker.pending_job();
    CHECK(first != kInvalidJobId);
    CHECK(clock.pending_count() == 1);

    clock.advance(50); // A still pending until t=200
    tracker.on_focus(b);
    const JobId second = tracker.pending_job();
    CHECK(second != kInvalidJobId);
    CHECK(second != first);
    CHECK(clock.pending_count() == 1); // exactly one pending job (REQ-SCH-003)

    clock.advance(100); // t=150, quiet (B due at t=250)
    CHECK(tracker.order().empty());

    clock.advance(100); // t=250 -> B commits, A never does
    const auto &order = tracker.order();
    CHECK(order.size() == 1);
    CHECK(order[0] == b);
}

// T-H-04: pending job is cancelled on tracker destruction and never runs.
TEST(h04_pending_cancelled_on_destroy) {
    FakeClock clock;
    int validity_calls = 0;
    {
        HistoryTracker tracker(
            clock,
            [&](const WindowRef &) {
                ++validity_calls;
                return true;
            },
            100);
        tracker.on_focus(WindowRef{0x100, 1});
        CHECK(clock.pending_count() == 1);
    } // destructor cancels the pending job

    CHECK(clock.pending_count() == 0);
    clock.advance(200);
    CHECK(validity_calls == 0); // commit never ran after teardown
}

// T-H-05: a window invalidated before fire is never committed (REQ-H-009).
TEST(h05_invalid_before_fire_not_committed) {
    FakeClock clock;
    bool valid = true;
    HistoryTracker tracker(clock, [&](const WindowRef &) { return valid; }, 100);
    const WindowRef a{0x100, 1};

    tracker.on_focus(a);
    valid = false; // window destroyed before the debounce fires
    clock.advance(100);

    CHECK(tracker.order().empty());
    CHECK(tracker.pending_job() == kInvalidJobId);
}

// T-H-01: lock-in ignores focus events and never schedules a job.
TEST(h01_lock_in_keeps_order_and_no_job) {
    FakeClock clock;
    HistoryTracker tracker(clock, [](const WindowRef &) { return true; }, 100);
    const WindowRef a{0x100, 1};
    const WindowRef b{0x200, 1};

    tracker.on_focus(a);
    clock.advance(100); // commit A

    tracker.set_session_locked(true);
    CHECK(tracker.pending_job() == kInvalidJobId);

    tracker.on_focus(b); // must be ignored while locked
    CHECK(tracker.pending_job() == kInvalidJobId);
    CHECK(clock.pending_count() == 0);

    clock.advance(500);
    const auto &order = tracker.order();
    CHECK(order.size() == 1);
    CHECK(order[0] == a); // MRU order unchanged
}

// T-H-lockin: locking also cancels an already-pending job.
TEST(h01_lock_cancels_pending) {
    FakeClock clock;
    HistoryTracker tracker(clock, [](const WindowRef &) { return true; }, 100);

    tracker.on_focus(WindowRef{0x100, 1});
    CHECK(tracker.pending_job() != kInvalidJobId);

    tracker.set_session_locked(true);
    CHECK(tracker.pending_job() == kInvalidJobId);
    CHECK(clock.pending_count() == 0);
    clock.advance(200);
    CHECK(tracker.order().empty());
}

// T-H-seed: seed dedups identities and drops invalid ones (REQ-H-004b/c).
TEST(h_seed_dedups_and_removes_invalid) {
    FakeClock clock;
    const WindowRef dead{0x300, 1};
    HistoryTracker tracker(clock, [&](const WindowRef &w) { return w != dead; }, 100);

    tracker.seed({WindowRef{0x100, 1}, WindowRef{0x200, 1}, WindowRef{0x100, 1}, dead});

    const auto &order = tracker.order();
    CHECK(order.size() == 2);
    CHECK((order[0] == WindowRef{0x100, 1})); // MRU-first kept on first occurrence
    CHECK((order[1] == WindowRef{0x200, 1}));
}

// T-H-seed: empty seed starts empty and populates from focus events.
TEST(h_seed_empty_then_populate) {
    FakeClock clock;
    HistoryTracker tracker(clock, [](const WindowRef &) { return true; }, 100);

    tracker.seed({});
    CHECK(tracker.order().empty());

    tracker.on_focus(WindowRef{0x400, 2});
    clock.advance(100);
    const auto &order = tracker.order();
    CHECK(order.size() == 1);
    CHECK((order[0] == WindowRef{0x400, 2}));
}

// T-H-004c bonus: committing a known identity moves it to the front.
TEST(h_commit_moves_existing_to_front) {
    FakeClock clock;
    HistoryTracker tracker(clock, [](const WindowRef &) { return true; }, 100);
    const WindowRef a{0x100, 1};
    const WindowRef b{0x200, 1};

    tracker.seed({a, b});
    tracker.on_focus(b);
    clock.advance(100);

    const auto &order = tracker.order();
    CHECK(order.size() == 2);
    CHECK(order[0] == b);
    CHECK(order[1] == a);
}

// set_debounce_ms clamps and applies to subsequent scheduling.
TEST(h_debounce_clamp_applies) {
    FakeClock clock;
    HistoryTracker tracker(clock, [](const WindowRef &) { return true; }, 100);

    tracker.set_debounce_ms(9000); // clamped to 5000
    tracker.on_focus(WindowRef{0x100, 1});
    clock.advance(4999);
    CHECK(tracker.order().empty());
    clock.advance(1);
    CHECK(tracker.order().size() == 1);
}

int main() {
    return mru::test::run_all();
}