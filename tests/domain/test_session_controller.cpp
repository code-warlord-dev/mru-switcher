#include <cstdint>
#include <map>
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
using mru::domain::Scope;
using mru::domain::SessionController;
using mru::domain::SessionPolicy;
using mru::domain::StartOffset;
using mru::domain::UIEndReason;
using mru::domain::WindowRef;
using mru::domain::WindowSource;

WindowRef ref(std::uint64_t address, std::uint64_t generation = 1) {
    return WindowRef{address, generation};
}

// Domain test harness: FakeClock + mock ports + real HistoryTracker.
struct Fixture {
    FakeClock clock;
    mru::test::MockWindowSource source;
    mru::test::MockFocusGateway fg;
    mru::test::MockUIPort ui;
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
};

// --- T-S-01: first cycle freezes the snapshot; second cycle does not rebuild.
TEST(t_s_01_second_cycle_does_not_rebuild) {
    Fixture f;
    f.candidates({ref(1), ref(2), ref(3)});

    const SessionController::CommandResult first = f.sc.cycle(Direction::Next);
    CHECK(first.ok);
    CHECK(f.source.candidates_calls == 1);
    CHECK(f.ui.starts.size() == 1);
    CHECK(f.sc.is_active());

    const auto frozen = f.sc.active_snapshot()->windows();
    const SessionController::CommandResult second = f.sc.cycle(Direction::Next);
    CHECK(second.ok);
    CHECK(f.source.candidates_calls == 1); // no rebuild
    CHECK(f.sc.active_snapshot()->windows() == frozen);

    // session_id is monotonic across sessions (REQ-S-008)
    (void)f.sc.apply();
    f.candidates({ref(9), ref(8)});
    (void)f.sc.cycle(Direction::Next);
    CHECK(f.sc.session_id() == 2);
}

// --- T-S-02: apply focuses the selected window and ends the session.
TEST(t_s_02_apply_focuses_selected_and_ends) {
    Fixture f;
    f.candidates({ref(10), ref(20)}); // start_offset=Second -> index 1
    (void)f.sc.cycle(Direction::Next);
    const WindowRef selected = f.sc.active_snapshot()->at(f.sc.index());

    const SessionController::CommandResult r = f.sc.apply();
    CHECK(r.ok);
    CHECK(f.fg.focused.size() == 1);
    CHECK(f.fg.focused[0] == selected);
    CHECK(f.ui.ends.size() == 1);
    CHECK(f.ui.ends[0] == UIEndReason::Applied);
    CHECK(f.sc.last_end_reason() == mru::domain::SessionEndReason::Applied);
    CHECK(!f.sc.is_active());
}

// --- T-S-03: cancel ends without apply focus (restore_focus_on_cancel=false).
TEST(t_s_03_cancel_ends_without_focus) {
    Fixture f;
    f.candidates({ref(10), ref(20)});
    (void)f.sc.cycle(Direction::Next);

    const SessionController::CommandResult r = f.sc.cancel();
    CHECK(r.ok);
    CHECK(f.fg.focused.empty());
    CHECK(f.ui.ends.size() == 1);
    CHECK(f.ui.ends[0] == UIEndReason::Cancelled);
    CHECK(f.sc.last_end_reason() == mru::domain::SessionEndReason::UserCancel);
    CHECK(!f.sc.is_active());
}

// --- T-S-04: empty candidates fail cycle without an Active session.
TEST(t_s_04_empty_candidates_fail_cycle) {
    Fixture f;
    const SessionController::CommandResult r = f.sc.cycle(Direction::Next);
    CHECK(!r.ok);
    CHECK(r.error == "no windows");
    CHECK(!f.sc.is_active());
    CHECK(f.ui.starts.empty());
    CHECK(f.source.candidates_calls == 1);
}

// --- T-S-05: restore_focus_on_cancel focuses a valid session_origin.
TEST(t_s_05_restore_cancels_to_valid_origin) {
    SessionPolicy policy;
    policy.restore_focus_on_cancel = true;
    Fixture f(policy);
    f.candidates({ref(10), ref(20)});
    f.source.focused_result = {ref(10)}; // origin captured at session start
    (void)f.sc.cycle(Direction::Next);

    const SessionController::CommandResult r = f.sc.cancel();
    CHECK(r.ok);
    CHECK(f.fg.focused.size() == 1);
    CHECK(f.fg.focused[0] == ref(10));
    CHECK(f.ui.ends.size() == 1);
    CHECK(f.ui.ends[0] == UIEndReason::Cancelled);
}

// --- T-S-06: restore_focus_on_cancel no-ops when origin is invalid (REQ-R-002).
TEST(t_s_06_restore_invalid_origin_noop) {
    SessionPolicy policy;
    policy.restore_focus_on_cancel = true;
    Fixture f(policy);
    f.candidates({ref(10), ref(20)});
    f.source.focused_result = {ref(99)}; // not in validity map -> invalid
    (void)f.sc.cycle(Direction::Next);

    const SessionController::CommandResult r = f.sc.cancel();
    CHECK(r.ok);
    CHECK(f.fg.focused.empty());
    CHECK(f.ui.ends.size() == 1);
    CHECK(f.ui.ends[0] == UIEndReason::Cancelled);
}

// --- T-S-07: Active cycle ignores a different scope token, still advances (REQ-S-010).
TEST(t_s_07_active_cycle_ignores_scope_override) {
    Fixture f;
    f.candidates({ref(1), ref(2), ref(3)}); // default scope = Global
    (void)f.sc.cycle(Direction::Next);      // index = 1
    const std::size_t before = f.sc.index();
    CHECK(f.sc.active_snapshot()->scope() == Scope::Global);

    const SessionController::CommandResult r = f.sc.cycle(Direction::Next, Scope::Monitor);
    CHECK(r.ok);
    CHECK(f.source.candidates_calls == 1); // ignored, no rebuild
    CHECK(f.sc.active_snapshot()->scope() == Scope::Global);
    CHECK(f.sc.index() != before); // selection continues inside existing snapshot
}

// --- T-S-08: start_offset=First begins the session on slot 0 (REQ-SEL-001).
TEST(t_s_08_start_offset_first_selects_slot_zero) {
    SessionPolicy p;
    p.start_offset = mru::domain::StartOffset::First;
    Fixture f(p);
    f.candidates({ref(10), ref(20)});

    (void)f.sc.cycle(Direction::Next);
    CHECK(f.sc.is_active());
    CHECK(f.sc.index() == 0);
    CHECK(f.sc.active_snapshot()->at(0) == ref(10));
    CHECK(f.ui.starts.size() == 1);
    CHECK(f.ui.starts[0].second == 0);
}

// --- T-SEL-01: wrap=false clamps Next at the last slot and Prev at slot 0.
TEST(t_sel_01_sc_wrap_false_clamps_at_edges) {
    SessionPolicy p;
    p.wrap = false;
    Fixture f(p);
    f.candidates({ref(10), ref(20), ref(30)});

    (void)f.sc.cycle(Direction::Next); // start_offset=Second -> slot 1
    (void)f.sc.cycle(Direction::Next); // slot 2
    (void)f.sc.cycle(Direction::Next); // clamped at 2
    CHECK(f.sc.index() == 2);

    (void)f.sc.cancel();

    (void)f.sc.cycle(Direction::Next); // new session -> slot 1
    (void)f.sc.cycle(Direction::Prev); // slot 0
    (void)f.sc.cycle(Direction::Prev); // clamped at 0
    CHECK(f.sc.index() == 0);
    CHECK(f.sc.active_snapshot()->at(0) == ref(10));
}

// --- T-F-01: cycle never calls FocusGateway (REQ-F-003).
TEST(t_f_01_cycle_never_focuses) {
    Fixture f;
    f.candidates({ref(1), ref(2), ref(3)});
    (void)f.sc.cycle(Direction::Next);
    (void)f.sc.cycle(Direction::Next);
    (void)f.sc.cycle(Direction::Prev);
    CHECK(f.fg.focused.empty());
}

// --- T-F-02: apply with a valid selection focuses exactly once.
TEST(t_f_02_apply_single_focus) {
    Fixture f;
    f.candidates({ref(10), ref(20)});
    (void)f.sc.cycle(Direction::Next); // index 1
    const WindowRef selected = f.sc.active_snapshot()->at(f.sc.index());

    (void)f.sc.apply();
    CHECK(f.fg.focused.size() == 1);
    CHECK(f.fg.focused[0] == selected);
}

// --- T-F-03a: invalid selection -> prune+clamp -> focus survivor (ADR-014).
TEST(t_f_03_invalid_selection_prunes_to_survivor) {
    Fixture f;
    f.candidates({ref(10), ref(20), ref(30)});
    (void)f.sc.cycle(Direction::Next); // index 1 -> ref(20)
    f.set_valid(ref(20), false);       // selected invalid at apply time

    const SessionController::CommandResult r = f.sc.apply();
    CHECK(r.ok);
    CHECK(f.fg.focused.size() == 1);   // clamp to same slot survivor
    CHECK(f.fg.focused[0] == ref(30)); // survivors {10,30}, index min(1,1)=1
    CHECK(f.ui.ends.size() == 1);
    CHECK(f.ui.ends[0] == UIEndReason::Applied);
    CHECK(!f.sc.is_active());
}

// --- T-F-03b: SPEC §2.8 step 3 — if the selection is *still* invalid right after
// prune+clamp (validity can race with window.close between the calls), the second
// prune+clamp pass runs; focus lands on the final survivor.
struct RefLess {
    bool operator()(const WindowRef &a, const WindowRef &b) const {
        if (a.address != b.address)
            return a.address < b.address;
        return a.generation < b.generation;
    }
};

struct FlakyWindowSource : WindowSource {
    mutable std::map<WindowRef, std::size_t, RefLess> consults;
    std::vector<WindowRef> candidates_result;

    std::vector<WindowRef> candidates(Scope) const override { return candidates_result; }

    bool is_valid(const WindowRef &r) const override {
        const std::size_t n = consults[r]++;
        if (r == ref(20) && n == 0)
            return false; // selected pruned in the first pass
        if (r == ref(10) && n == 1)
            return false; // first-pass survivor dies before the probe
        return true;
    }

    std::optional<WindowRef> focused() const override { return std::nullopt; }
};

TEST(t_f_03_second_pass_prune_and_clamp) {
    FlakyWindowSource source;
    source.candidates_result = {ref(10), ref(20)};
    FakeClock clock;
    mru::test::MockFocusGateway fg;
    mru::test::MockUIPort ui;
    HistoryTracker tracker(clock, [&](const WindowRef &r) { return source.is_valid(r); }, 50);
    SessionController sc(source, fg, ui, tracker, {});

    (void)sc.cycle(Direction::Next); // start_offset=Second -> index 1 = ref(20)
    // 1st prune: ref(20) n==0 -> invalid, pruned; ref(10) n==0 valid, survives.
    // clamp: index 1 -> 0.
    // probe: ref(10) n==1 -> invalid -> 2nd prune: ref(10) n==2 valid, survives.
    // 2nd clamp keeps index 0; focus ref(10).
    const SessionController::CommandResult r = sc.apply();
    CHECK(r.ok);
    CHECK(fg.focused.size() == 1);
    CHECK(fg.focused[0] == ref(10));
    CHECK(ui.ends.size() == 1);
    CHECK(ui.ends[0] == UIEndReason::Applied);
    CHECK(!sc.is_active());
}

// --- T-F-03c: second pass empties the snapshot -> Cancelled "no windows".
TEST(t_f_03_second_pass_empties_session_cancelled) {
    struct FlickerSource : WindowSource {
        mutable std::size_t ten_calls = 0;
        std::vector<WindowRef> candidates(Scope) const override { return {ref(10)}; }
        bool is_valid(const WindowRef &r) const override {
            if (r == ref(10) && ten_calls == 0) {
                ++ten_calls;
                return true; // passes the first prune consult...
            }
            return false; // ...then dies, so every later pass empties the snapshot
        }
        std::optional<WindowRef> focused() const override { return std::nullopt; }
    };

    FlickerSource source;
    FakeClock clock;
    mru::test::MockFocusGateway fg;
    mru::test::MockUIPort ui;
    HistoryTracker tracker(clock, [&](const WindowRef &r) { return source.is_valid(r); }, 50);
    SessionController sc(source, fg, ui, tracker, {});

    (void)sc.cycle(Direction::Next); // single candidate -> index 0
    const SessionController::CommandResult r = sc.apply();
    CHECK(!r.ok);
    CHECK(r.error == "no windows");
    CHECK(fg.focused.empty());
    CHECK(ui.ends.size() == 1);
    CHECK(ui.ends[0] == UIEndReason::Cancelled);
}

// --- T-F-04: apply when all snapshot refs became invalid -> fail "no windows".
TEST(t_f_04_apply_empty_after_prune) {
    Fixture f;
    f.candidates({ref(10), ref(20)});
    (void)f.sc.cycle(Direction::Next);
    f.set_valid(ref(10), false);
    f.set_valid(ref(20), false);

    const SessionController::CommandResult r = f.sc.apply();
    CHECK(!r.ok);
    CHECK(r.error == "no windows");
    CHECK(f.fg.focused.empty());
    CHECK(f.ui.ends.size() == 1);
    CHECK(f.ui.ends[0] == UIEndReason::Cancelled);
    CHECK(!f.sc.is_active());
}

// --- T-F-05a: FocusResult::InvalidTarget -> bounded retry; repeated InvalidTarget
// ends InvalidSelection (REQ-F-008 §2.8 symmetry, no successful focus, FM-10).
TEST(t_f_05_invalid_target_retry_then_ends_cancelled) {
    Fixture f;
    f.candidates({ref(10), ref(20)});
    (void)f.sc.cycle(Direction::Next);
    f.fg.result = FocusResult::InvalidTarget;

    const SessionController::CommandResult r = f.sc.apply();
    CHECK(!r.ok);
    CHECK(r.error == "selection invalid"); // MEDIUM-8: truthful InvalidSelection diagnosis
    CHECK(f.fg.focused.size() == 2);       // bounded: one retry, still no success
    CHECK(f.fg.focused[0] == f.fg.focused[1]);
    CHECK(f.ui.ends.size() == 1);
    CHECK(f.ui.ends[0] == UIEndReason::Cancelled);
}

// --- T-F-05b: InvalidTarget race resolved by the retry -> survivor is focused
// exactly once (success), session ends Applied (REQ-F-006 at-most-one success).
TEST(t_f_05_invalid_target_retry_succeeds) {
    Fixture f;
    f.candidates({ref(10), ref(20)});
    (void)f.sc.cycle(Direction::Next); // index 1 -> ref(20)
    const WindowRef selected = f.sc.active_snapshot()->at(f.sc.index());
    f.fg.script = {FocusResult::InvalidTarget, FocusResult::Applied};

    const SessionController::CommandResult r = f.sc.apply();
    CHECK(r.ok);
    CHECK(f.fg.focused.size() == 2); // first attempt died, retry landed
    CHECK(f.fg.focused[0] == selected);
    CHECK(f.fg.focused[1] == selected);
    CHECK(f.ui.ends.size() == 1); // exactly one UI end (REQ-F-007)
    CHECK(f.ui.ends[0] == UIEndReason::Applied);
    CHECK(f.sc.last_end_reason() == mru::domain::SessionEndReason::Applied);
}

// --- T-F-05c: FocusResult::Failed -> FocusFailed, definitive, no retry (FM-10).
TEST(t_f_05_focus_failed_ends_cancelled) {
    Fixture f;
    f.candidates({ref(10), ref(20)});
    (void)f.sc.cycle(Direction::Next);
    f.fg.result = FocusResult::Failed;

    const SessionController::CommandResult r = f.sc.apply();
    CHECK(!r.ok);
    CHECK(r.error == "focus failed"); // MEDIUM-8: truthful FocusFailed diagnosis
    CHECK(f.fg.focused.size() == 1);
    CHECK(f.ui.ends.size() == 1);
    CHECK(f.ui.ends[0] == UIEndReason::Cancelled);
}

// --- T-RE-01: after apply, a synthetic window.active must not reopen the session
// and HistoryTracker resumes committing (lock released, REQ-RE-003).
TEST(t_re_01_apply_then_active_does_not_reopen) {
    Fixture f; // lock_history_on_session = true (default)
    f.candidates({ref(10), ref(20)});
    (void)f.sc.cycle(Direction::Next);
    (void)f.sc.apply();
    CHECK(!f.sc.is_active());

    f.sc.on_focus(ref(10)); // post-apply focus event
    CHECK(!f.sc.is_active());
    CHECK(f.tracker.pending_job() != mru::domain::kInvalidJobId);
    f.clock.advance(50); // debounce fires, commit resumes after unlock

    const auto &order = f.tracker.order();
    CHECK(order.size() == 1);
    CHECK(order[0] == ref(10));
}

// --- T-RE-04 (BLOCKER-2 regression): the applied window must reach the MRU head.
// FocusGateway::focus() emits a synchronous window.active that lock-in swallows
// (REQ-H-001) while Active, so complete_apply() promotes it explicitly once the
// session ends. Without the fix the applied window is absent from order_.
TEST(t_re_04_apply_promotes_applied_to_mru_head) {
    Fixture f; // lock_history_on_session = true (default)
    f.candidates({ref(10), ref(20)});
    (void)f.sc.cycle(Direction::Next); // start_offset=Second -> index 1 -> ref(20)
    const WindowRef applied = f.sc.active_snapshot()->at(f.sc.index());

    (void)f.sc.apply();
    CHECK(!f.sc.is_active());
    f.clock.advance(50); // debounce fires; applied window committed at head

    const auto &order = f.tracker.order();
    CHECK(order.size() == 1);
    CHECK(order.front() == applied);
}

// --- T-RE-05 (BLOCKER-2, invalidated apply): promotion happens for the *final*
// applied target (apply-after-invalidation surrogate), not the stale slot.
TEST(t_re_05_apply_after_invalidation_promotes_survivor) {
    Fixture f;
    f.candidates({ref(10), ref(20), ref(30)});
    (void)f.sc.cycle(Direction::Next); // index 1 -> ref(20)
    f.set_valid(ref(20), false);       // selected invalid at apply time

    (void)f.sc.apply(); // clamps to survivor ref(30)
    f.clock.advance(50);

    const auto &order = f.tracker.order();
    CHECK(order.size() == 1);
    CHECK(order.front() == ref(30));
}

// --- T-RE-06 (audit BLOCKER-2 adapter check): a compositor emitting a
// *synchronous* window.active from inside FocusGateway::focus() (as Hyprland
// does through fullWindowFocus) must be swallowed by lock-in and must not
// corrupt state: the applied window is still promoted to the MRU head once the
// controller ends the session, and the reentrant event does not reopen it.
TEST(t_re_06_reentrant_active_inside_focus_is_safe) {
    mru::test::ReentrantFocusGateway fg;
    mru::test::MockWindowSource source;
    mru::test::MockUIPort ui;
    FakeClock clock;
    HistoryTracker tracker(clock, [&](const WindowRef &r) { return source.is_valid(r); }, 50);

    source.candidates_result = {ref(10), ref(20)};
    source.validity.emplace_back(ref(10), true);
    source.validity.emplace_back(ref(20), true);

    SessionController scl(source, fg, ui, tracker, {}); // lock_history_on_session = true
    ui.ends.clear();

    (void)scl.cycle(Direction::Next); // index 1 -> ref(20)
    const WindowRef applied = scl.active_snapshot()->at(scl.index());
    CHECK(applied == ref(20));

    // Simulate the compositor: while we focus, it synchronously fires the
    // window.active event for the same window (re-enters through on_focus).
    fg.on_reentrant_focus = [&](const WindowRef &r) { scl.on_focus(r); };

    (void)scl.apply();
    CHECK(fg.reentered);           // the reentrancy path actually ran
    CHECK(fg.focused.size() == 1); // exactly one focus call
    CHECK(!scl.is_active());       // reentrant active did not reopen the session
    CHECK(scl.last_end_reason() == mru::domain::SessionEndReason::Applied);

    clock.advance(50); // debounce fires
    const auto &order = tracker.order();
    CHECK(order.size() == 1);
    CHECK(order.front() == applied); // promoted to MRU head despite swallowed sync event
}

// --- T-RE-07 removed: plugin_shutdown coverage already exists as
// bonus_plugin_shutdown_ends_active_session + bonus_plugin_shutdown_when_idle_is_noop
// (added with L-11); T-RE-06 is the unique audit-requested reentrancy test.

// --- Bonus: lock-in ignores focus events while a session is Active.
TEST(bonus_focus_during_active_session_is_ignored) {
    Fixture f; // lock_history_on_session = true
    f.candidates({ref(10), ref(20)});
    (void)f.sc.cycle(Direction::Next);

    f.sc.on_focus(ref(20)); // must be swallowed by lock-in
    CHECK(f.tracker.pending_job() == mru::domain::kInvalidJobId);
    f.clock.advance(200);
    CHECK(f.tracker.order().empty());
    CHECK(f.sc.is_active());
}

// --- Bonus (FM-04): on_window_invalid prunes mid-session and clamps the index.
TEST(bonus_on_window_invalid_prunes_and_clamps) {
    Fixture f;
    f.candidates({ref(10), ref(20), ref(30)});
    (void)f.sc.cycle(Direction::Next); // index 1 -> ref(20)
    f.set_valid(ref(20), false);

    f.sc.on_window_invalid(ref(20));
    CHECK(f.sc.is_active());
    CHECK(f.sc.active_snapshot()->size() == 2); // ref(20) pruned
    CHECK(f.sc.index() == 1);                   // clamp min(1, len-1=1)
    CHECK(f.sc.active_snapshot()->at(f.sc.index()) == ref(30));
    CHECK(f.ui.ends.empty()); // session still alive
}

// --- Bonus (FM-04, REQ-S-006): emptying the snapshot via close ends Cancelled.
TEST(bonus_on_window_invalid_empties_session_cancelled) {
    Fixture f;
    f.candidates({ref(10)});
    (void)f.sc.cycle(Direction::Next);
    f.set_valid(ref(10), false);

    f.sc.on_window_invalid(ref(10));
    CHECK(!f.sc.is_active());
    CHECK(f.ui.ends.size() == 1);
    CHECK(f.ui.ends[0] == UIEndReason::Cancelled);
}

// --- L-11: plugin_shutdown ends an active session before teardown: UI gets the
// end event, state clears, history unlocks, and no focus is applied (REQ-F-006).
TEST(bonus_plugin_shutdown_ends_active_session) {
    Fixture f; // lock_history_on_session = true (default)
    f.candidates({ref(10), ref(20)});
    CHECK(f.sc.cycle(Direction::Next).ok);
    CHECK(f.sc.is_active());

    f.sc.plugin_shutdown();
    CHECK(!f.sc.is_active());
    CHECK(!f.sc.active_snapshot().has_value());
    CHECK(f.ui.ends.size() == 1);
    CHECK(f.ui.ends[0] == UIEndReason::Cancelled);
    CHECK(f.sc.last_end_reason() == mru::domain::SessionEndReason::PluginShutdown);
    CHECK(f.fg.focused.empty()); // never focuses

    // lock released by shutdown: a focus event commits after debounce
    f.sc.on_focus(ref(20));
    f.clock.advance(50);
    CHECK(!f.tracker.order().empty());
}

// --- L-11: plugin_shutdown while Idle is a no-op.
TEST(bonus_plugin_shutdown_when_idle_is_noop) {
    Fixture f;
    f.sc.plugin_shutdown();
    CHECK(!f.sc.is_active());
    CHECK(f.ui.ends.empty());
    CHECK(!f.sc.last_end_reason().has_value());
}

// --- Bonus: apply while Idle is an idempotent success no-op (FM-12).
TEST(bonus_apply_and_cancel_idempotent_when_idle) {
    Fixture f;
    const SessionController::CommandResult a = f.sc.apply();
    CHECK(a.ok);
    CHECK(a.error.empty());
    CHECK(!f.sc.is_active());
    CHECK(f.ui.ends.empty());

    const SessionController::CommandResult c = f.sc.cancel();
    CHECK(c.ok);
    CHECK(f.ui.ends.empty());
}

// --- T-CFG-02: a policy refresh affects the *next* session only (REQ-CFG-002, REQ-S-009).
TEST(t_cfg_02_reload_policy_applies_to_next_session_only) {
    Fixture f; // defaults: start_offset = second, wrap = true
    f.candidates({ref(1), ref(2), ref(3)});

    CHECK(f.sc.cycle(Direction::Next).ok);
    EQ(f.sc.index(), 1u);
    (void)f.sc.cycle(Direction::Next);
    EQ(f.sc.index(), 2u);

    SessionPolicy reloaded;
    reloaded.start_offset = StartOffset::First;
    reloaded.wrap = false;
    f.sc.set_policy(reloaded);

    // Active session keeps the frozen policy: wrap still wraps 2 -> 0.
    (void)f.sc.cycle(Direction::Next);
    EQ(f.sc.index(), 0u);
    (void)f.sc.cancel();

    // Next session picks the reloaded policy: start_offset = first -> index 0.
    CHECK(f.sc.cycle(Direction::Next).ok);
    EQ(f.sc.index(), 0u);

    // wrap = false clamps at the end instead of wrapping.
    (void)f.sc.cycle(Direction::Next);
    EQ(f.sc.index(), 1u);
    (void)f.sc.cycle(Direction::Next);
    EQ(f.sc.index(), 2u);
    (void)f.sc.cycle(Direction::Next);
    EQ(f.sc.index(), 2u);
}

// --- T-S-09: a mid-session policy refresh does not change the frozen restore flag
// (REQ-S-009 with REQ-R-001); the *next* session applies the new value.
TEST(t_s_09_restore_flag_frozen_mid_session) {
    SessionPolicy policy;
    policy.restore_focus_on_cancel = true;
    Fixture f(policy);
    f.candidates({ref(10), ref(20)});
    f.source.focused_result = {ref(10)}; // session_origin
    CHECK(f.sc.cycle(Direction::Next).ok);

    SessionPolicy reloaded; // restores turned off mid-session
    reloaded.restore_focus_on_cancel = false;
    f.sc.set_policy(reloaded);

    CHECK(f.sc.cancel().ok);
    CHECK(f.fg.focused == std::vector<WindowRef>{ref(10)}); // frozen true -> origin restored
    CHECK(f.ui.ends.size() == 1);
    CHECK(f.ui.ends[0] == UIEndReason::Cancelled);

    // Next session starts with the reloaded policy: cancel must not move focus.
    CHECK(f.sc.cycle(Direction::Next).ok);
    f.source.focused_result = {ref(10)};
    CHECK(f.sc.cancel().ok);
    EQ(f.fg.focused.size(), 1u); // no second focus
    CHECK(f.ui.ends.size() == 2);
    CHECK(f.ui.ends[0] == UIEndReason::Cancelled);
    CHECK(f.ui.ends[1] == UIEndReason::Cancelled);
}

// --- T-S-10 (a): a session ending for a non-cancel reason never restores focus.
// REQ-S-006 / NoWindows (FM-02). The origin stays *valid* but outside the snapshot,
// so any focus on this path is observable (not masked by the validity guard).
TEST(t_s_10_empty_snapshot_never_restores) {
    SessionPolicy policy;
    policy.restore_focus_on_cancel = true;
    Fixture f(policy);
    f.candidates({ref(10)});             // snapshot = [10]
    f.set_valid(ref(99), true);          // origin is valid ...
    f.source.focused_result = {ref(99)}; // ... but is not a candidate
    CHECK(f.sc.cycle(Direction::Next).ok);

    f.set_valid(ref(10), false);
    f.sc.on_window_invalid(ref(10)); // snapshot empties -> end NoWindows

    CHECK(f.fg.focused.empty()); // no restore on a non-cancel end
    CHECK(f.sc.last_end_reason() == mru::domain::SessionEndReason::NoWindows);
    CHECK(!f.sc.is_active());
    CHECK(f.ui.ends.size() == 1);
    CHECK(f.ui.ends[0] == UIEndReason::Cancelled);
}

// --- T-S-10 (b): same guarantee for the plugin shutdown path (L-11, REQ-S-005).
TEST(t_s_10_plugin_shutdown_never_restores) {
    SessionPolicy policy;
    policy.restore_focus_on_cancel = true;
    Fixture f(policy);
    f.candidates({ref(10)});    // snapshot = [10]
    f.set_valid(ref(99), true); // origin valid but out of the snapshot
    f.source.focused_result = {ref(99)};
    CHECK(f.sc.cycle(Direction::Next).ok);

    f.sc.plugin_shutdown();

    CHECK(f.fg.focused.empty());
    CHECK(f.sc.last_end_reason() == mru::domain::SessionEndReason::PluginShutdown);
    CHECK(!f.sc.is_active());
}

// --- T-O-01: select_index moves the virtual selection and notifies UIPort (REQ-O-004).
TEST(t_o_01_select_index_moves_virtual_selection) {
    Fixture f;
    f.candidates({ref(10), ref(20), ref(30)});
    CHECK(f.sc.cycle(Direction::Next).ok); // index 1 -> ref(20)
    CHECK(f.sc.index() == 1);

    const SessionController::CommandResult r = f.sc.select_index(2);
    CHECK(r.ok);
    CHECK(f.sc.index() == 2);
    CHECK(f.sc.active_snapshot()->at(f.sc.index()) == ref(30));
    CHECK(!f.ui.changes.empty());
    CHECK(f.ui.changes.back() == 2); // UIPort notified (virtual only)
    CHECK(f.fg.focused.empty());     // REQ-F-003: never focuses
    CHECK(f.sc.is_active());         // session continues
}

// --- T-O-02: select_index out of range, empty snapshot, or Idle is a no-op (REQ-O-004).
TEST(t_o_02_select_index_noop_out_of_range_or_idle) {
    Fixture f;
    f.candidates({ref(10), ref(20)});
    CHECK(f.sc.cycle(Direction::Next).ok); // index 1

    CHECK(f.sc.select_index(2).ok); // out of range -> ignored
    CHECK(f.sc.index() == 1);
    CHECK(f.sc.select_index(99).ok);
    CHECK(f.sc.index() == 1);
    const std::size_t before_changes = f.ui.changes.size();
    (void)f.sc.select_index(2);
    CHECK(f.ui.changes.size() == before_changes); // no UIPort notification

    (void)f.sc.cancel(); // Idle from here
    const std::size_t before = f.sc.index();
    CHECK(f.sc.select_index(1).ok); // Idle -> no-op
    CHECK(f.sc.index() == before);
    CHECK(f.ui.changes.size() == before_changes);
}

// --- T-O-08: peer select-then-apply maps to the controller ops and focuses the
// chosen window exactly once (REQ-O-004, REQ-F-006).
TEST(t_o_08_peer_select_then_apply_focuses_chosen) {
    Fixture f;
    f.candidates({ref(10), ref(20), ref(30)});
    CHECK(f.sc.cycle(Direction::Next).ok); // index 1 -> ref(20)

    CHECK(f.sc.select_index(0).ok); // peer picks ref(10)
    CHECK(f.sc.active_snapshot()->at(f.sc.index()) == ref(10));

    CHECK(f.sc.apply().ok);
    CHECK(f.fg.focused.size() == 1);
    CHECK(f.fg.focused[0] == ref(10));
    CHECK(f.ui.ends.size() == 1);
    CHECK(f.ui.ends[0] == UIEndReason::Applied);
    CHECK(!f.sc.is_active());
}

} // namespace

int main() {
    return mru::test::run_all();
}