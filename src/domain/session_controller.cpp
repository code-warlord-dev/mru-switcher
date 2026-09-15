#include "mru/domain/session_controller.hpp"

#include <algorithm>
#include <utility>

#include "mru/domain/selection.hpp"

namespace mru::domain {

SessionController::SessionController(WindowSource &source, FocusGateway &fg, UIPort &ui, HistoryTracker &tracker,
                                     SessionPolicy policy)
    : source_(source), fg_(fg), ui_(ui), tracker_(tracker), policy_(policy), snapshot_policy_(policy) {}

SessionController::CommandResult SessionController::cycle(Direction dir, std::optional<Scope> scope_override) {
    if (!active_)
        return begin_session(scope_override.value_or(policy_.default_scope));

    // REQ-S-010: a scope token that differs from the frozen session scope is
    // ignored; the existing Snapshot is reused and selection still advances.
    // (SPEC 2.1, T-S-07; plan Task 7 early-return corrected to SPEC behaviour.)
    index_ = advance_index(index_, dir, snapshot_policy_.wrap, snapshot_->size());
    ui_.on_selection_changed(index_);
    return {true, ""};
}

SessionController::CommandResult SessionController::begin_session(Scope scope) {
    std::vector<WindowRef> candidates = source_.candidates(scope);
    if (candidates.empty())
        return {false, "no windows"}; // REQ-S-002 guard, FM-11

    snapshot_ = Snapshot(std::move(candidates), scope);              // REQ-SNAP-001 (MRU-first from source)
    index_ = initial_index(policy_.start_offset, snapshot_->size()); // REQ-SEL-001/002
    session_origin_ = source_.focused();                             // REQ-S-007
    ++session_id_;                                                   // REQ-S-008
    snapshot_policy_ = policy_;                                      // REQ-S-009
    if (snapshot_policy_.lock_history_on_session)
        tracker_.set_session_locked(true); // lock-in (REQ-H-001)

    active_ = true;
    ui_.on_session_start(*snapshot_, index_);
    return {true, ""};
}

SessionController::CommandResult SessionController::apply() {
    if (!active_)
        return {true, ""}; // idempotent no-op, FM-12

    std::optional<FocusResult> first = resolve_and_focus(); // §2.8
    if (!first)
        return end_session(SessionEndReason::NoWindows, "no windows"); // step 2
    if (*first == FocusResult::Applied)
        return complete_apply();
    if (*first == FocusResult::Failed)
        return end_session(SessionEndReason::FocusFailed, "no windows"); // FM-10

    // InvalidTarget: the resolved window died between validation and focus.
    // Continue apply-after-invalidation once more (REQ-F-008, §2.8 step-3
    // symmetry). Bounded: a repeated InvalidTarget ends the session.
    std::optional<FocusResult> second = resolve_and_focus();
    if (!second)
        return end_session(SessionEndReason::NoWindows, "no windows");
    switch (*second) {
    case FocusResult::Applied:
        return complete_apply();
    case FocusResult::Failed:
        return end_session(SessionEndReason::FocusFailed, "no windows");
    case FocusResult::InvalidTarget:
        break;
    }
    return end_session(SessionEndReason::InvalidSelection, "no windows");
}

SessionController::CommandResult SessionController::complete_apply() {
    const WindowRef applied = snapshot_->at(index_); // capture before end_session
    const CommandResult result = end_session(SessionEndReason::Applied, "");
    // REQ-RE-003 / BLOCKER-2: the compositor emits a synchronous window.active
    // inside FocusGateway::focus(), but lock-in (REQ-H-001) swallows it while the
    // session is still Active — the applied window would never reach the MRU head.
    // Promote it explicitly once history is unlocked; a later duplicate window.active
    // for the same identity is idempotent in HistoryTracker::commit.
    tracker_.on_focus(applied);
    return result;
}

std::optional<FocusResult> SessionController::resolve_and_focus() {
    prune_active();
    if (!snapshot_ || snapshot_->empty())
        return std::nullopt;

    // Step 3: defensive repeat prune+clamp if the selection somehow stays invalid.
    if (!source_.is_valid(snapshot_->at(index_))) {
        prune_active();
        if (!snapshot_ || snapshot_->empty())
            return std::nullopt;
    }
    index_ = std::min(index_, snapshot_->size() - 1); // clamp toward same slot (ADR-014)
    return fg_.focus(snapshot_->at(index_));          // at most one success (REQ-F-006)
}

SessionController::CommandResult SessionController::cancel() {
    if (!active_)
        return {true, ""}; // idempotent no-op, FM-13

    const std::optional<WindowRef> origin = session_origin_;
    const bool restore = snapshot_policy_.restore_focus_on_cancel;
    end_session(SessionEndReason::UserCancel, ""); // UI Cancelled + unlock history

    if (restore && origin && source_.is_valid(*origin))
        fg_.focus(*origin); // REQ-R-001; invalid origin is a no-op (REQ-R-002, T-S-06)

    return {true, ""};
}

SessionController::CommandResult SessionController::end_session(SessionEndReason reason, std::string_view error) {
    active_ = false;
    snapshot_.reset();
    index_ = 0;
    session_origin_.reset();
    last_end_reason_ = reason; // diagnostics (REQ-F-009, mru:status)

    if (snapshot_policy_.lock_history_on_session)
        tracker_.set_session_locked(false); // unlock (REQ-H-001)

    ui_.on_session_end(reason == SessionEndReason::Applied ? UIEndReason::Applied
                                                           : UIEndReason::Cancelled); // REQ-F-007/009
    return {reason == SessionEndReason::Applied, std::string(error)};
}

void SessionController::prune_active() {
    if (!snapshot_)
        return;

    // Unify with the free helper: same stable-order, scope-preserving prune
    // (REQ-SNAP-003/004).
    const Scope scope = snapshot_->scope();
    std::optional<Snapshot> next = pruned(*snapshot_, source_);
    snapshot_ = next ? std::move(*next) : Snapshot(std::vector<WindowRef>{}, scope);

    if (snapshot_->empty())
        index_ = 0;
    else if (index_ >= snapshot_->size())
        index_ = snapshot_->size() - 1; // REQ-SNAP-004 clamp
}

void SessionController::on_focus(const WindowRef &ref) {
    if (active_)
        return; // lock-in (REQ-H-001, REQ-RE-003)
    tracker_.on_focus(ref);
}

void SessionController::on_window_invalid(const WindowRef &ref) {
    if (!active_ || !snapshot_)
        return;

    const std::vector<WindowRef> &windows = snapshot_->windows();
    if (std::find(windows.begin(), windows.end(), ref) == windows.end())
        return;

    prune_active(); // stable order, clamp (FM-04)
    if (snapshot_->empty())
        end_session(SessionEndReason::NoWindows, "no windows"); // REQ-S-006 -> Cancelled
}

void SessionController::set_policy(SessionPolicy policy) {
    policy_ = policy;
}

} // namespace mru::domain