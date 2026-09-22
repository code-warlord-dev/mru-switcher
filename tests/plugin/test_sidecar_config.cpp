#include "sidecar_config.hpp"
#include "test_framework.hpp"

#include <string>
#include <vector>

namespace {
using namespace mru::plugin;
using namespace mru::plugin::sidecar;

// REQ-CFG-005: full valid file overlays every key it sets.
TEST(sidecar_01_full_overlay) {
    PluginConfig base = default_plugin_config();
    const SidecarParse p = parse("ui = border\ndebounce_ms = 100\ndefault_scope = workspace\n"
                                 "start_offset = first\nwrap = false\nborder_style = solid\n"
                                 "border_color = 0xff112233\nborder_size = 3\n"
                                 "selection_follow_workspace = false\n"
                                 "restore_focus_on_cancel = true\nexternal_socket = /tmp/mru.sock\n");
    CHECK(p.warnings.empty());
    std::vector<std::string> warns;
    PluginConfig cfg = base;
    apply_overlay(cfg, p, [&](const std::string &w) { warns.push_back(w); });
    CHECK(warns.empty());
    CHECK(cfg.ui_border);
    CHECK(!cfg.ui_null);
    CHECK(cfg.debounce_ms == 100);
    CHECK(cfg.default_scope == mru::domain::Scope::Workspace);
    CHECK(cfg.start_offset == mru::domain::StartOffset::First);
    CHECK(!cfg.wrap);
    CHECK(cfg.border_color == "0xff112233");
    CHECK(cfg.border_size == 3);
    CHECK(!cfg.selection_follow_workspace); // ADR-026 / REQ-UI-012
    CHECK(cfg.restore_focus_on_cancel);
    CHECK(cfg.external_socket == "/tmp/mru.sock");
}

// Whitespace, comments (full-line + inline), CRLF tolerance.
TEST(sidecar_02_comments_whitespace) {
    const SidecarParse p = parse("# full line\n; another\r\n  ui   =   border   # inline\n"
                                 "wrap=false;trailing-kept? no -> stripped only with blank\n");
    CHECK(p.overrides.ui.has_value());
    CHECK(*p.overrides.ui == "border");
}

// Unknown keys + malformed lines warn and are ignored; base untouched.
TEST(sidecar_03_unknown_and_malformed_warn) {
    PluginConfig base = default_plugin_config();
    const SidecarParse p = parse("bogus_key = 1\nno-equals-here\nui = border\n");
    CHECK(p.warnings.size() == 2);
    std::vector<std::string> warns;
    PluginConfig cfg = base;
    apply_overlay(cfg, p, [&](const std::string &w) { warns.push_back(w); });
    CHECK(cfg.ui_border); // valid key still applies
    CHECK(warns.empty()); // parse-time warnings already in p.warnings
}

// Invalid values keep the base value and warn once each via the sink.
TEST(sidecar_04_invalid_values_keep_base) {
    PluginConfig base = default_plugin_config();
    const SidecarParse p = parse("debounce_ms = 12x\nwrap = maybe\nui = nope\ndefault_scope = nowhere\n"
                                 "start_offset = sideways\nborder_size = -5\nrestore_focus_on_cancel = yes\n");
    std::vector<std::string> warns;
    PluginConfig cfg = base;
    apply_overlay(cfg, p, [&](const std::string &w) { warns.push_back(w); });
    CHECK(cfg.debounce_ms == base.debounce_ms);
    CHECK(cfg.wrap == base.wrap);
    // Invalid ui value keeps the base config (ADR-025 default: border) — REQ-CFG-001.
    CHECK(cfg.ui_border && !cfg.ui_null);
    CHECK(cfg.default_scope == base.default_scope);
    CHECK(cfg.start_offset == base.start_offset);
    CHECK(cfg.border_size == base.border_size);
    CHECK(cfg.restore_focus_on_cancel == base.restore_focus_on_cancel);
    CHECK(warns.size() == 7);
}

// REQ-H-010 / ADR-021: reserved key stored but ignored; false warns.
TEST(sidecar_05_reserved_lock_key_ignored) {
    PluginConfig base = default_plugin_config();
    std::vector<std::string> warns;
    PluginConfig cfg = base;
    apply_overlay(cfg, parse("lock_history_on_session = false\n"), [&](const std::string &w) { warns.push_back(w); });
    CHECK(!cfg.lock_history_on_session); // stored...
    CHECK(warns.size() == 1);            // ...but warns (behaviour downstream ignores it)
    // policy derivation never sees it (mandatory lock-in): still true-default path
    PluginConfig cfg2 = base;
    std::vector<std::string> warns2;
    apply_overlay(cfg2, parse("lock_history_on_session = true\n"), [&](const std::string &w) { warns2.push_back(w); });
    CHECK(cfg2.lock_history_on_session);
    CHECK(warns2.empty());
}

// Missing file = absent, silent. default_path honours XDG then HOME.
TEST(sidecar_06_missing_file_silent_and_paths) {
    const SidecarParse p = load_file("/nonexistent-mru-sidecar-xyz/config");
    CHECK(!p.overrides.ui.has_value());
    CHECK(p.warnings.empty());
    CHECK(default_path("/xdg", "/home/u") == "/xdg/mru-switcher/config");
    CHECK(default_path("", "/home/u") == "/home/u/.config/mru-switcher/config");
    CHECK(default_path("", "") == "");
}

// debounce clamp still applies through the overlay (REQ-CFG-004).
TEST(sidecar_07_debounce_clamped) {
    PluginConfig cfg = default_plugin_config();
    std::vector<std::string> warns;
    apply_overlay(cfg, parse("debounce_ms = 99999\n"), [&](const std::string &w) { warns.push_back(w); });
    CHECK(cfg.debounce_ms == 5000);
    CHECK(warns.empty()); // clamped, not invalid
}

// ADR-026 / REQ-UI-012: the sidecar can set the view-follow toggle; an invalid
// value keeps the base (default true) and warns once (REQ-CFG-001).
TEST(sidecar_08_selection_follow_workspace_overlay) {
    PluginConfig base = default_plugin_config();
    auto overlay = [&](const char *text) {
        PluginConfig cfg = base;
        std::vector<std::string> warns;
        apply_overlay(cfg, parse(text), [&](const std::string &w) { warns.push_back(w); });
        return std::make_pair(cfg, warns);
    };

    const auto [off, warns_off] = overlay("selection_follow_workspace = false\n");
    CHECK(!off.selection_follow_workspace);
    CHECK(warns_off.empty());

    const auto [on, warns_on] = overlay("selection_follow_workspace = true\n");
    CHECK(on.selection_follow_workspace);
    CHECK(warns_on.empty());

    const auto [bad, warns_bad] = overlay("selection_follow_workspace = maybe\n");
    CHECK(bad.selection_follow_workspace); // invalid keeps the base value
    CHECK(warns_bad.size() == 1);
}

} // namespace

int main() {
    return mru::test::run_all();
}
