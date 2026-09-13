#pragma once

#include <string_view>

#include "mru/domain/scope.hpp"

namespace mru::plugin {

struct PluginConfig {
  int                              debounce_ms = 400;             // clamp [0,5000] (REQ-CFG-004)
  mru::domain::Scope               default_scope = mru::domain::Scope::Global;
  bool                             wrap = true;
  bool                             lock_history_on_session = true;
  bool                             restore_focus_on_cancel = false;
  bool                             ui_null = true;    // REQ-UI-003: solely null in M2
  bool                             ui_border = false; // parsed, falls back to null (REQ-UI-002)
  bool                             ui_external = false;
  bool                             ui_matched = true;
};

PluginConfig default_plugin_config();
mru::domain::Scope parse_scope(std::string_view s);
int clamp_debounce_ms(int raw);

struct ParsedUi {
  enum class Kind { Null, Border, External };
  Kind  kind = Kind::Null;
  bool  matched = false;
};
ParsedUi parse_ui_backend(std::string_view s);

} // namespace mru::plugin