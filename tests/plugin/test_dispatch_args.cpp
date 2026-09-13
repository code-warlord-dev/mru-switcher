#include <cassert>
#include <string_view>

#include "dispatch_args.hpp"

int main() {
    using mru::domain::Direction;
    using mru::domain::Scope;
    using namespace mru::plugin;

    // REQ-DISP-001: omitted direction -> next
    auto a = parse_cycle_args("");
    assert(a.ok && a.dir == Direction::Next && !a.scope);

    auto b = parse_cycle_args("next");
    assert(b.ok && b.dir == Direction::Next && !b.scope);

    auto c = parse_cycle_args("prev");
    assert(c.ok && c.dir == Direction::Prev && !c.scope);

    auto d = parse_cycle_args("next workspace");
    assert(d.ok && d.scope && *d.scope == Scope::Workspace);

    auto e = parse_cycle_args("global");
    assert(e.ok && e.dir == Direction::Next && !e.scope); // scope alone invalid

    auto f = parse_cycle_args("nonsense");
    assert(!f.ok && !f.error.empty());

    auto g = parse_cycle_args("next bogus");
    assert(!g.ok && !g.error.empty());

    return 0;
}
