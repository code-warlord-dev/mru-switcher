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
    if (s == "next")
        return Direction::Next;
    if (s == "prev")
        return Direction::Prev;
    return std::nullopt;
}

CycleArgs parse_cycle_args(std::string_view args) {
    const auto toks = tokens_of(args);
    CycleArgs out;
    out.ok = true; // no tokens == "next" with the default scope (REQ-DISP-001)

    std::size_t i = 0;
    if (i < toks.size()) {
        if (const auto dir = dir_of(toks[i])) { // direction is optional
            out.dir = *dir;
            ++i;
        }
    }

    if (i < toks.size()) {
        // REQ-DISP-003: a scope token may be given without an explicit direction.
        if (const auto scope = mru::plugin::parse_scope_token(toks[i])) {
            out.scope = *scope;
            ++i;
        } else {
            // T-SC-05: a scope-position token that is neither a direction nor a
            // valid scope token is distinct from a grammar error (REQ-SC-003).
            out.ok = false;
            out.error = "unknown scope token: " + toks[i];
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
