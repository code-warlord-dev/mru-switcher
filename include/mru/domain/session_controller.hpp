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
// apply issues at most one focus (REQ-F-006) and exactly one UI end (REQ-F-007).
class SessionController {
public:
  struct CommandResult {
    bool ok = false;
    std::string error;
  };

  SessionController(WindowSource& source, FocusGateway& fg, UIPort& ui,
                    HistoryTracker& tracker, SessionPolicy policy);

  // First cycle when Idle builds the Snapshot; subsequent cycles only move the
  // selection inside the existing Snapshot (REQ-S-003).
  CommandResult cycle(Direction dir, std::optional<Scope> scope_override = std::nullopt);
  CommandResult apply();  // FocusGateway once, then Idle (REQ-S-004, ADR-014)
  CommandResult cancel(); // Transition to Idle without applying the selection (REQ-S-005)

  // Focus events from the facade: ignored while Active (lock-in, REQ-H-001,
  // REQ-RE-003); forwarded to HistoryTracker when Idle.
  void on_focus(const WindowRef& ref);
  // Prune the active snapshot when a window closes (FM-04); empty ends session.
  void on_window_invalid(const WindowRef& ref);

  bool is_active() const { return active_; }
  std::uint64_t session_id() const { return session_id_; } // monotonic (REQ-S-008)
  const std::optional<Snapshot>& active_snapshot() const { return snapshot_; }
  std::size_t index() const { return index_; }
  const std::optional<WindowRef>& session_origin() const { return session_origin_; }

private:
  CommandResult begin_session(Scope scope);
  CommandResult end_session(SessionEndReason reason, std::string_view error);
  void prune_active();

  WindowSource& source_;
  FocusGateway& fg_;
  UIPort& ui_;
  HistoryTracker& tracker_;
  SessionPolicy policy_;          // live config reference for new sessions
  SessionPolicy snapshot_policy_; // frozen at session start (REQ-S-009)
  bool active_ = false;
  std::optional<Snapshot> snapshot_;
  std::size_t index_ = 0;
  std::optional<WindowRef> session_origin_;
  std::uint64_t session_id_ = 0;
};

} // namespace mru::domain