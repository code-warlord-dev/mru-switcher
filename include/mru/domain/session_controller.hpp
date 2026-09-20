#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "mru/domain/focus_gateway.hpp"
#include "mru/domain/history_tracker.hpp"
#include "mru/domain/scope.hpp"
#include "mru/domain/snapshot.hpp"
#include "mru/domain/ui_port.hpp"
#include "mru/domain/window_ref.hpp"
#include "mru/domain/window_source.hpp"

namespace mru::domain {

// Internal reason recorded for diagnostics/logs (REQ-F-009). UIPort continues
// to receive only the coarse UI reason (Applied | Cancelled).
enum class SessionEndReason {
    Applied,
    UserCancel,
    NoWindows,
    InvalidSelection,
    FocusFailed,
    PluginShutdown,
};

// State machine Idle <-> Active (SPEC §2.1, docs/TRANSITION-TABLE.md).
// One active session at a time (REQ-S-001); cycle never focuses (REQ-F-003);
// apply issues at most one *successful* focus (REQ-F-006) and exactly one UI end
// (REQ-F-007). On FocusGateway::InvalidTarget a single bounded re-attempt runs
// apply-after-invalidation once more (REQ-F-008, §2.8 symmetry).
// History lock-in while Active is mandatory and unconditional (REQ-H-001/010,
// ADR-021); at session start a still-pending debounced promotion is flushed
// before the snapshot is built (REQ-H-011).
class SessionController {
  public:
    struct CommandResult {
        bool ok = false;
        std::string error;
    };

    // `source`, `fg`, `ui`, `tracker` and `policy` must outlive the controller.
    SessionController(WindowSource &source, FocusGateway &fg, UIPort &ui, HistoryTracker &tracker,
                      SessionPolicy policy);

    // First cycle when Idle builds the Snapshot; subsequent cycles only move the
    // selection inside the existing Snapshot (REQ-S-003).
    [[nodiscard]] CommandResult cycle(Direction dir, std::optional<Scope> scope_override = std::nullopt);
    // Peer-driven virtual selection (REQ-O-004, ADR-018): move the selection to a
    // bounds-checked index of the active Snapshot. Never focuses (REQ-F-003). No-op
    // while Idle or for an out-of-range index (T-O-02).
    [[nodiscard]] CommandResult select_index(std::size_t i);
    // FocusGateway at most once/success, then Idle (REQ-S-004, REQ-F-006, ADR-014).
    [[nodiscard]] CommandResult apply();
    // Transition to Idle without applying the selection (REQ-S-005).
    [[nodiscard]] CommandResult cancel();
    // PLUGIN_EXIT path (L-11): end an active session so the UI still receives
    // on_session_end(Cancelled) and active_/snapshot_ are cleared while the
    // controller and its ports are alive. Never focuses (REQ-F-006). No-op Idle.
    void plugin_shutdown();

    // Focus events from the facade: ignored while Active (mandatory lock-in,
    // REQ-H-001/010, REQ-RE-003); forwarded to HistoryTracker when Idle.
    void on_focus(const WindowRef &ref);
    // Prune the active snapshot when a window closes (FM-04); empty ends session.
    void on_window_invalid(const WindowRef &ref);
    // REQ-CFG-002 / REQ-S-009: refresh the live policy applied to *new* sessions
    // (scope, start_offset, wrap, restore_focus_on_cancel). The active session
    // keeps the snapshot policy frozen at session start. There is no lock-in flag:
    // lock-in is mandatory (REQ-H-001/010, ADR-021).
    void set_policy(SessionPolicy policy);

    bool is_active() const { return active_; }
    std::uint64_t session_id() const { return session_id_; } // monotonic (REQ-S-008)
    const std::optional<Snapshot> &active_snapshot() const { return snapshot_; }
    std::size_t index() const { return index_; }
    const std::optional<WindowRef> &session_origin() const { return session_origin_; }
    // Reason the last session ended, for introspection/diagnostics (REQ-F-009).
    // std::nullopt when no session has ended yet.
    std::optional<SessionEndReason> last_end_reason() const { return last_end_reason_; }

  private:
    CommandResult begin_session(Scope scope);
    CommandResult end_session(SessionEndReason reason, std::string_view error);
    // Promote the applied window to the MRU head after the session ends.
    // The FocusGateway emits a synchronous window.active during the focus call,
    // but lock-in (REQ-H-001) swallows it while Active, so without this the
    // applied window would never reach the head (BLOCKER-2, REQ-RE-003). The
    // promotion stays debounced; a session started before the timer fires commits
    // it through HistoryTracker::flush_pending() (REQ-H-011, ADR-021).
    CommandResult complete_apply();
    void prune_active();
    // Apply-after-invalidation (§2.8): prune, defensive re-prune + clamp, then a
    // single focus attempt. std::nullopt when the snapshot emptied (no focus).
    std::optional<FocusResult> resolve_and_focus();

    WindowSource &source_;
    FocusGateway &fg_;
    UIPort &ui_;
    HistoryTracker &tracker_;
    SessionPolicy policy_;          // live config reference for new sessions
    SessionPolicy snapshot_policy_; // frozen at session start (REQ-S-009)
    bool active_ = false;
    std::optional<Snapshot> snapshot_;
    std::size_t index_ = 0;
    std::optional<WindowRef> session_origin_;
    std::uint64_t session_id_ = 0;
    std::optional<SessionEndReason> last_end_reason_;
};

} // namespace mru::domain