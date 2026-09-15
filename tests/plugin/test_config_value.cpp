#include "config_value.hpp"
#include "test_framework.hpp"

namespace {
using mru::domain::Scope;
using namespace mru::plugin;

// REQ-CFG-004: debounce clamped into [0,5000]
TEST(cfg_01_debounce_clamped) {
    EQ(clamp_debounce_ms(-1), 0);
    EQ(clamp_debounce_ms(0), 0);
    EQ(clamp_debounce_ms(400), 400);
    EQ(clamp_debounce_ms(5000), 5000);
    EQ(clamp_debounce_ms(9999), 5000);
}

// MEDIUM-9: clamp on the full 64-bit value BEFORE narrowing to int. A value like
// 4294967296 (2^32) would previously be narrowed to 0 (UB/implementation-defined)
// and then pass the int clamp as a valid zero.
TEST(cfg_01b_debounce_wide_value_clamped) {
    EQ(clamp_debounce_ms(4294967296LL), 5000);
    EQ(clamp_debounce_ms(-4294967296LL), 0);
}

// REQ-CFG-001: unknown scope falls back to default; known ones parse
TEST(cfg_02_scope_parse) {
    EQ(parse_scope("global"), Scope::Global);
    EQ(parse_scope("monitor"), Scope::Monitor);
    EQ(parse_scope("workspace"), Scope::Workspace);
    EQ(parse_scope("visible"), Scope::Visible);
    EQ(parse_scope("app"), Scope::App);
    EQ(parse_scope("bogus"), Scope::Global); // fallback, no failure
}

// REQ-DISP-003: strict scope token for dispatcher args
TEST(cfg_03_scope_token_strict) {
    CHECK(parse_scope_token("global").has_value());
    CHECK(parse_scope_token("app").value() == Scope::App);
    CHECK(!parse_scope_token("bogus").has_value());
}

// REQ-UI-002/003: null matches; border/external match but are not implemented in M2
TEST(cfg_04_ui_backend_parse) {
    CHECK(parse_ui_backend("null").matched);
    EQ(parse_ui_backend("null").kind, ParsedUi::Kind::Null);
    CHECK(parse_ui_backend("border").matched);
    EQ(parse_ui_backend("border").kind, ParsedUi::Kind::Border);
    CHECK(parse_ui_backend("external").matched);
    EQ(parse_ui_backend("nope").kind, ParsedUi::Kind::Null);
    CHECK(!parse_ui_backend("nope").matched);
}

// REQ-SEL-002: start_offset parses first|second, unknown falls back to second
TEST(cfg_05_start_offset_parse) {
    EQ(parse_start_offset("first"), mru::domain::StartOffset::First);
    EQ(parse_start_offset("second"), mru::domain::StartOffset::Second);
    EQ(parse_start_offset("bogus"), mru::domain::StartOffset::Second);
}

TEST(cfg_06_defaults) {
    const auto cfg = default_plugin_config();
    EQ(cfg.debounce_ms, 400);
    EQ(cfg.default_scope, Scope::Global);
    CHECK(cfg.wrap);
    CHECK(cfg.lock_history_on_session);
    CHECK(!cfg.restore_focus_on_cancel);
    CHECK(cfg.ui_null);
    CHECK(!cfg.ui_border);
    CHECK(!cfg.ui_external);
}

} // namespace

int main() {
    return mru::test::run_all();
}
