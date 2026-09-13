#pragma once
// Forward declarations for M1 domain types.
// WindowRef, Snapshot, Selection, Session — see docs/SPEC.md and ADR-013.

namespace mru::domain {

struct WindowRef;
class Snapshot;
struct Selection;
class SessionController;
class WindowSource;
class FocusGateway;
class UIPort;
class HistoryTracker;

enum class FocusResult;
enum class SessionEndReason;
enum class UIEndReason;

} // namespace mru::domain
