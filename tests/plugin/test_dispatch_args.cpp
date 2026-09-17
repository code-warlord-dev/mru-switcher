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

// T-SC-05 (M3-S3, S3-5): the unknown-token parse error is distinct from the
// grammar errors. A scope-position token that is neither a direction nor a
// valid scope token yields "unknown scope token"; the extra-token case still
// yields "too many arguments" (distinct errors, REQ-DISP-003, REQ-SC-003).
TEST(disp_05_unknown_scope_token) {
    const auto f = parse_cycle_args("bogus");
    CHECK(!f.ok);
    CHECK(f.error.find("unknown scope token") != std::string::npos);

    const auto h = parse_cycle_args("next bogus");
    CHECK(!h.ok);
    CHECK(h.error.find("unknown scope token") != std::string::npos);

    const auto m = parse_cycle_args("next workspace extra");
    CHECK(!m.ok);
    // REQ-SC-003: distinct from the T-SC-05 message — the extra-token error
    // does not name a scope token.
    CHECK(m.error.find("unknown scope token") == std::string::npos);
    CHECK(m.error != h.error);
}

} // namespace

int main() {
    return mru::test::run_all();
}
