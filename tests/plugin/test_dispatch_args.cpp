#include "dispatch_args.hpp"
#include "test_framework.hpp"

namespace {
using mru::domain::Direction;
using mru::domain::Scope;
using namespace mru::plugin;

// REQ-DISP-001: omitted direction -> next
TEST(disp_01_empty_and_directions) {
    const auto a = parse_cycle_args("");
    CHECK(a.ok);
    EQ(a.dir, Direction::Next);
    CHECK(!a.scope);

    const auto b = parse_cycle_args("next");
    CHECK(b.ok);
    EQ(b.dir, Direction::Next);
    CHECK(!b.scope);

    const auto c = parse_cycle_args("prev");
    CHECK(c.ok);
    EQ(c.dir, Direction::Prev);
}

TEST(disp_02_direction_and_scope) {
    const auto d = parse_cycle_args("next workspace");
    CHECK(d.ok);
    EQ(d.dir, Direction::Next);
    CHECK(d.scope.has_value());
    EQ(*d.scope, Scope::Workspace);

    const auto e = parse_cycle_args("prev app");
    CHECK(e.ok);
    EQ(e.dir, Direction::Prev);
    EQ(*e.scope, Scope::App);
}

// REQ-DISP-003: scope without direction is legal; direction defaults to next
TEST(disp_03_scope_only) {
    const auto g = parse_cycle_args("global");
    CHECK(g.ok);
    EQ(g.dir, Direction::Next);
    CHECK(g.scope.has_value());
    EQ(*g.scope, Scope::Global);

    const auto m = parse_cycle_args("monitor");
    CHECK(m.ok);
    EQ(m.dir, Direction::Next);
    EQ(*m.scope, Scope::Monitor);
}

TEST(disp_04_invalid_args) {
    const auto f = parse_cycle_args("nonsense");
    CHECK(!f.ok);
    CHECK(!f.error.empty());

    const auto h = parse_cycle_args("next bogus");
    CHECK(!h.ok);

    const auto i = parse_cycle_args("next next");
    CHECK(!i.ok);

    const auto j = parse_cycle_args("next workspace extra");
    CHECK(!j.ok);
}

} // namespace

int main() {
    return mru::test::run_all();
}
