#include "config_value.hpp"

#include <algorithm>
#include <string_view>

namespace mru::plugin {

PluginConfig default_plugin_config() {
  return {};
}

mru::domain::Scope parse_scope(std::string_view s) {
  using mru::domain::Scope;
  if (s == "monitor") return Scope::Monitor;
  if (s == "workspace") return Scope::Workspace;
  if (s == "visible") return Scope::Visible;
  if (s == "app") return Scope::App;
  return Scope::Global; // includes "global"; fallback for unknown (REQ-CFG-001)
}

int clamp_debounce_ms(int raw) {
  return std::clamp(raw, 0, 5000); // REQ-CFG-004
}

ParsedUi parse_ui_backend(std::string_view s) {
  if (s == "null") return {ParsedUi::Kind::Null, true};
  if (s == "border") return {ParsedUi::Kind::Border, true};
  if (s == "external") return {ParsedUi::Kind::External, true};
  return {ParsedUi::Kind::Null, false}; // REQ-CFG-001 fallback to default
}

} // namespace mru::plugin