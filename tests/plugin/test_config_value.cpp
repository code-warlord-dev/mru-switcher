#include <cassert>

#include "config_value.hpp"

int main() {
  using mru::domain::Scope;
  using namespace mru::plugin;

  // REQ-CFG-004: debounce clamped into [0,5000]
  assert(clamp_debounce_ms(-1) == 0);
  assert(clamp_debounce_ms(0) == 0);
  assert(clamp_debounce_ms(400) == 400);
  assert(clamp_debounce_ms(5000) == 5000);
  assert(clamp_debounce_ms(9999) == 5000);

  // REQ-CFG-001: unknown scope falls back to default; known ones parse
  assert(parse_scope("global") == Scope::Global);
  assert(parse_scope("monitor") == Scope::Monitor);
  assert(parse_scope("workspace") == Scope::Workspace);
  assert(parse_scope("visible") == Scope::Visible);
  assert(parse_scope("app") == Scope::App);
  assert(parse_scope("bogus") == Scope::Global);

  // REQ-UI-002/003: null matches; border/external match but not implemented in M2
  assert(parse_ui_backend("null").matched);
  assert(parse_ui_backend("null").kind == ParsedUi::Kind::Null);
  assert(parse_ui_backend("border").matched);
  assert(parse_ui_backend("border").kind == ParsedUi::Kind::Border);
  assert(parse_ui_backend("external").matched);
  assert(parse_ui_backend("nope").kind == ParsedUi::Kind::Null);
  assert(!parse_ui_backend("nope").matched);

  auto cfg = default_plugin_config();
  assert(cfg.debounce_ms == 400);
  assert(cfg.default_scope == Scope::Global);
  assert(cfg.wrap);
  assert(cfg.lock_history_on_session);
  assert(!cfg.restore_focus_on_cancel);
  assert(cfg.ui_null);
  assert(!cfg.ui_border);
  assert(!cfg.ui_external);

  return 0;
}