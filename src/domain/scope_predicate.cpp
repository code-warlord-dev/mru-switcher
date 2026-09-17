#include "mru/domain/scope_predicate.hpp"

#include <algorithm>

namespace mru::domain {

bool scope_matches(Scope scope, const WindowMeta &candidate, const FocusContext &focus) {
    // Uniform special-workspace rule (ADR-016 __3__, REQ-SC-002a): a hidden
    // scratchpad window is excluded from every scope. `hidden == true` exactly
    // means "on a special workspace not currently shown" (see WindowMeta), so
    // this single gate covers all five scopes including `visible`.
    if (!candidate.mapped || candidate.hidden)
        return false;

    switch (scope) {
    case Scope::Global:
        return true;
    case Scope::Monitor:
        return !focus.has_focus || candidate.monitor_id == focus.monitor_id;
    case Scope::Workspace:
        return !focus.has_focus || candidate.workspace_id == focus.workspace_id;
    case Scope::Visible:
        return std::find(focus.visible_workspaces.cbegin(), focus.visible_workspaces.cend(), candidate.workspace_id) !=
               focus.visible_workspaces.cend();
    case Scope::App:
        // Empty focus (or empty focused class) degrades to global (REQ-SC-002b);
        // otherwise byte-exact class equality (ADR-016 __4__). A candidate with
        // an empty class never matches a non-empty focused class, and never
        // matches via initialClass (the domain carries only app_class; S2-6).
        if (!focus.has_focus || focus.app_class.empty())
            return true;
        return candidate.app_class == focus.app_class;
    }
    return false;
}

} // namespace mru::domain
