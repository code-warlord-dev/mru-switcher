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

// REQ-UI-002/003: null matches; border selects the M4 border backend; external
// still folds back to null.
TEST(cfg_04_ui_backend_parse) {
    CHECK(parse_ui_backend("null").matched);
    EQ(parse_ui_backend("null").kind, ParsedUi::Kind::Null);
    CHECK(parse_ui_backend("border").matched);
    EQ(parse_ui_backend("border").kind, ParsedUi::Kind::Border);
    CHECK(parse_ui_backend("external").matched);
    EQ(parse_ui_backend("nope").kind, ParsedUi::Kind::Null);
    CHECK(!parse_ui_backend("nope").matched);
}

// REQ-UI-003 / REQ-UI-002: effective backend selection (border vs null/external).
TEST(t_ui_03_effective_backend_selection) {
    PluginConfig cfg = default_plugin_config();
    EQ(effective_ui_backend(cfg), UiBackend::Null); // default ui=null

    cfg.ui_border = true;
    EQ(effective_ui_backend(cfg), UiBackend::Border);

    cfg.ui_border = false;
    cfg.ui_external = true; // M5: external overlay backend (ADR-018)
    EQ(effective_ui_backend(cfg), UiBackend::External);
}

// REQ-UI-007: unknown/reserved border styles behave as solid and signal a warning.
TEST(t_ui_07_border_style_solid_fallback) {
    const ParsedBorderStyle solid = parse_border_style("solid");
    EQ(solid.style, BorderStyle::Solid);
    CHECK(!solid.should_warn);

    for (const char *reserved : {"pulse", "dim", "bogus"}) {
        const ParsedBorderStyle parsed = parse_border_style(reserved);
        EQ(parsed.style, BorderStyle::Solid);
        CHECK(parsed.should_warn);
    }
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
    CHECK(cfg.lock_history_on_session); // reserved key: default stays true (REQ-H-010)
    CHECK(!cfg.restore_focus_on_cancel);
    CHECK(cfg.ui_null);
    CHECK(!cfg.ui_border);
    CHECK(!cfg.ui_external);
}

// REQ-UI-008: M4 border config defaults.
TEST(cfg_07_border_defaults) {
    const auto cfg = default_plugin_config();
    EQ(cfg.border_style, BorderStyle::Solid);
    CHECK(cfg.border_color == "0xffffd9a0");
    EQ(cfg.border_size, -1);
}

} // namespace

int main() {
    return mru::test::run_all();
}
