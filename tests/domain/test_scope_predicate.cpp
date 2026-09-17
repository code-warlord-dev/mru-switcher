#include <cstdint>
#include <vector>

#include "mru/domain/focus_context.hpp"
#include "mru/domain/scope.hpp"
#include "mru/domain/scope_predicate.hpp"
#include "mru/domain/window_meta.hpp"
#include "test_framework.hpp"

namespace {

using mru::domain::FocusContext;
using mru::domain::Scope;
using mru::domain::WindowMeta;

constexpr Scope kScopes[] = {Scope::Global, Scope::Monitor, Scope::Workspace, Scope::Visible, Scope::App};

WindowMeta win(std::uint64_t monitor, std::uint64_t workspace, const char *app_class, bool mapped = true,
               bool hidden = false) {
    WindowMeta m;
    m.monitor_id = monitor;
    m.workspace_id = workspace;
    m.app_class = app_class;
    m.mapped = mapped;
    m.hidden = hidden;
    return m;
}

// Focus on monitor 1 / workspace 10 with class "Firefox"; workspaces 10 and 20 shown.
FocusContext plain_focus() {
    FocusContext f;
    f.has_focus = true;
    f.monitor_id = 1;
    f.workspace_id = 10;
    f.app_class = "Firefox";
    f.visible_workspaces = {10, 20};
    return f;
}

auto matches = mru::domain::scope_matches;

// --- T-SC-01: each scope filters correctly on plain (non-special) windows.
TEST(t_sc_01_global_takes_any_live_window) {
    const FocusContext f = plain_focus();
    CHECK(matches(Scope::Global, win(1, 10, "Firefox"), f));
    CHECK(matches(Scope::Global, win(2, 99, "kitty"), f));
}

TEST(t_sc_01_global_rejects_unmapped_and_hidden) {
    const FocusContext f = plain_focus();
    CHECK(!matches(Scope::Global, win(1, 10, "Firefox", /*mapped=*/false), f));
    CHECK(!matches(Scope::Global, win(1, 10, "Firefox", true, /*hidden=*/true), f));
}

TEST(t_sc_01_monitor_filters_by_focus_monitor) {
    const FocusContext f = plain_focus(); // monitor 1
    CHECK(matches(Scope::Monitor, win(1, 99, "kitty"), f));
    CHECK(!matches(Scope::Monitor, win(2, 99, "kitty"), f));
}

TEST(t_sc_01_workspace_filters_by_focus_workspace) {
    const FocusContext f = plain_focus(); // workspace 10
    CHECK(matches(Scope::Workspace, win(2, 10, "kitty"), f));
    CHECK(!matches(Scope::Workspace, win(2, 11, "kitty"), f));
}

TEST(t_sc_01_visible_filters_by_visible_workspace_set) {
    const FocusContext f = plain_focus(); // {10, 20}
    CHECK(matches(Scope::Visible, win(2, 20, "kitty"), f));
    CHECK(!matches(Scope::Visible, win(2, 30, "kitty"), f)); // mapped but not shown
}

TEST(t_sc_01_app_requires_class_match) {
    const FocusContext f = plain_focus(); // app_class "Firefox"
    CHECK(matches(Scope::App, win(3, 40, "Firefox"), f));
    CHECK(!matches(Scope::App, win(3, 40, "kitty"), f));
}

// --- T-SC-02: `app` uses class only; the focused window is itself a candidate.
TEST(t_sc_02_app_focused_window_is_candidate) {
    // REQ-SC-002b: with start_offset=second the in-app toggle needs the focused
    // window to be a candidate in app scope.
    const FocusContext f = plain_focus();
    CHECK(matches(Scope::App, win(1, 10, "Firefox"), f));
}

TEST(t_sc_02_no_focus_anchor_scopes_degrade_to_global) {
    // Documented decision (FocusContext comment): with no focused window the
    // anchor scopes have no reference to compare against and degrade to global.
    FocusContext f; // has_focus == false
    f.visible_workspaces = {10, 20};
    CHECK(matches(Scope::Monitor, win(9, 33, "kitty"), f));
    CHECK(matches(Scope::Workspace, win(9, 33, "kitty"), f));
}

// initialClass is not consulted at all (S2-6): WindowMeta exposes only
// `app_class`, so `app` can never match on a stored initial class.

// --- T-SC-03 (REQ-SC-002a, ADR-016 __3__): special workspace, table-driven
// over all five scopes with an otherwise identical snapshot.
TEST(t_sc_03_special_ws_shown_candidate_in_all_scopes) {
    // Focus sits on the shown scratchpad: its workspace is the current one, so
    // the workspace scope admits it exactly like a regular window (ADR-016 __3__).
    FocusContext f = plain_focus();
    f.workspace_id = 99;
    f.visible_workspaces = {10, 99}; // special ws 99 shown -> window not hidden
    const WindowMeta scratch = win(1, 99, "Firefox");

    for (const Scope s : kScopes)
        CHECK(matches(s, scratch, f));
}

TEST(t_sc_03_special_ws_hidden_excluded_in_all_scopes) {
    // The scratchpad is toggled away, so focus returned to a regular workspace
    // and ws 99 left the visible set; the adapter marks the window hidden. app
    // and global (which do NOT depend on the workspace) would wrongly admit it
    // without the uniform hidden gate (REQ-SC-002a).
    FocusContext f = plain_focus();
    f.workspace_id = 10;
    f.visible_workspaces = {10}; // special ws 99 hidden
    const WindowMeta scratch = win(1, 99, "Firefox", true, /*hidden=*/true);

    for (const Scope s : kScopes)
        CHECK(!matches(s, scratch, f));
}

TEST(t_sc_03_hidden_bit_wins_over_visible_set_membership) {
    // Guard against a "visible" scope implemented as bare set-membership: even
    // with the hidden workspace id in the visible set, a hidden scratchpad must
    // stay excluded in every scope (the uniform rule lives in the validity gate).
    FocusContext f = plain_focus();
    f.visible_workspaces = {10, 99};
    const WindowMeta scratch = win(1, 99, "Firefox", true, /*hidden=*/true);

    for (const Scope s : kScopes)
        CHECK(!matches(s, scratch, f));
}

// --- T-SC-04 (REQ-SC-002b, ADR-016 __4__): byte-exact class + empty-class fold.
TEST(t_sc_04_app_class_is_byte_exact_case_sensitive) {
    const FocusContext f = plain_focus(); // app_class "Firefox"
    CHECK(matches(Scope::App, win(1, 10, "Firefox"), f));
    CHECK(!matches(Scope::App, win(1, 10, "firefox"), f));
    CHECK(!matches(Scope::App, win(1, 10, "FireFox"), f));
}

TEST(t_sc_04_focus_empty_class_degrades_app_to_global) {
    FocusContext f = plain_focus();
    f.app_class = "";
    CHECK(matches(Scope::App, win(2, 20, "any-other-class"), f));
}

TEST(t_sc_04_no_focus_degrades_app_to_global) {
    FocusContext f; // has_focus == false
    f.visible_workspaces = {10, 20};
    CHECK(matches(Scope::App, win(2, 20, "anything"), f));
}

TEST(t_sc_04_candidate_empty_class_never_matches_nonempty_focus) {
    const FocusContext f = plain_focus(); // app_class "Firefox"
    CHECK(!matches(Scope::App, win(1, 10, ""), f));
}

} // namespace

int main() {
    return mru::test::run_all();
}
