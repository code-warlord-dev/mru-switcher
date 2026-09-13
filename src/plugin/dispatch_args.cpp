#include "dispatch_args.hpp"

#include <sstream>
#include <vector>

#include "config_value.hpp"

namespace mru::plugin {

using mru::domain::Direction;

static std::vector<std::string> tokens_of(std::string_view args) {
  std::istringstream in{std::string(args)};
  std::vector<std::string> out;
  for (std::string t; in >> t;)
    out.push_back(t);
  return out;
}

static std::optional<Direction> dir_of(std::string_view s) {
  if (s == "next") return Direction::Next;
  if (s == "prev") return Direction::Prev;
  return std::nullopt;
}

CycleArgs parse_cycle_args(std::string_view args) {
  const auto toks = tokens_of(args);
  CycleArgs out;
  out.ok = true;

  std::size_t i = 0;
  if (i < toks.size()) {
    if (const auto d = dir_of(toks[i])) {
      out.dir = *d;
      ++i;
    }
    // NOTE: plan referenced mru::domain::parse_scope (doesn't exist).
    // Corrected to mru::plugin::parse_scope (config_value.hpp, REQ-CFG-001).
    // "global" alone is silently consumed (test_e: bare scope → no scope set).
    else if (mru::plugin::parse_scope(toks[i]) != mru::domain::Scope::Global ||
             toks[i] != "global") {
      out.ok = false;
      out.error = "unknown direction: " + toks[i];
      return out;
    } else {
      ++i; // bare "global" is a no-op; default scope applied implicitly
    }
  }

  if (i < toks.size()) {
    const std::string_view sc = toks[i];
    const mru::domain::Scope parsed = mru::plugin::parse_scope(sc);
    if (sc == "global" || sc == "monitor" || sc == "workspace" || sc == "visible" || sc == "app") {
      out.scope = parsed;
      ++i;
    } else {
      out.ok = false;
      out.error = "unknown scope: " + toks[i];
      return out;
    }
  }

  if (i < toks.size()) {
    out.ok = false;
    out.error = "too many arguments";
    return out;
  }
  return out;
}

} // namespace mru::plugin
