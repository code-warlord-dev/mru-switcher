#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

#include "mru/domain/snapshot.hpp"
#include "mru/domain/window_source.hpp"
#include "test_framework.hpp"

namespace {

using mru::domain::Scope;
using mru::domain::Snapshot;
using mru::domain::WindowRef;
using mru::domain::WindowSource;

WindowRef ref(std::uint64_t address) { return WindowRef{address, 1}; }

struct MockSource : WindowSource {
  explicit MockSource(std::vector<WindowRef> valid) : valid_(std::move(valid)) {}

  std::vector<WindowRef> candidates(Scope) const override { return {}; }
  bool is_valid(const WindowRef& r) const override {
    for (const WindowRef& v : valid_) {
      if (v == r)
        return true;
    }
    return false;
  }
  std::optional<WindowRef> focused() const override { return std::nullopt; }

private:
  std::vector<WindowRef> valid_;
};

TEST(snapshot_empty) {
  const Snapshot s({}, Scope::Global);
  CHECK(s.empty());
  CHECK(s.size() == 0);
  CHECK(s.windows().empty());
}

TEST(snapshot_order_preserved) {
  const Snapshot s({ref(1), ref(2), ref(3)}, Scope::Workspace);
  CHECK(!s.empty());
  CHECK(s.size() == 3);
  CHECK(s.scope() == Scope::Workspace);
  CHECK((s.windows() == std::vector<WindowRef>{ref(1), ref(2), ref(3)}));
  CHECK(s.at(0) == ref(1));
  CHECK(s.at(2) == ref(3));
}

TEST(snapshot_at_is_const_access) {
  const Snapshot s({ref(7), ref(8)}, Scope::Global);
  const WindowRef& first = s.at(0);
  CHECK(first == ref(7));
}

TEST(snapshot_prune_keeps_relative_order) {
  // Drop ref(2) and ref(4): survivors keep their original relative order.
  MockSource source({ref(1), ref(3), ref(5)});
  const Snapshot s({ref(1), ref(2), ref(3), ref(4), ref(5)}, Scope::Global);

  const std::optional<Snapshot> out = mru::domain::pruned(s, source);
  CHECK(out.has_value());
  CHECK((out->windows() == std::vector<WindowRef>{ref(1), ref(3), ref(5)}));
  CHECK(out->size() == 3);
  CHECK(out->scope() == Scope::Global);
}

TEST(snapshot_prune_nothing_invalid_keeps_all) {
  MockSource source({ref(1), ref(2), ref(3)});
  const Snapshot s({ref(1), ref(2), ref(3)}, Scope::Workspace);

  const std::optional<Snapshot> out = mru::domain::pruned(s, source);
  CHECK(out.has_value());
  CHECK((out->windows() == std::vector<WindowRef>{ref(1), ref(2), ref(3)}));
}

TEST(snapshot_prune_none_survive_returns_nullopt) {
  MockSource source({});
  const Snapshot s({ref(1), ref(2)}, Scope::Global);

  const std::optional<Snapshot> out = mru::domain::pruned(s, source);
  CHECK(!out.has_value());
}

TEST(snapshot_prune_generation_mismatch_is_invalid) {
  // Same address, different generation must be pruned (REQ-ID-003, REQ-F-005).
  MockSource source({WindowRef{0x100, 2}});
  const Snapshot s({WindowRef{0x100, 1}}, Scope::Global);

  const std::optional<Snapshot> out = mru::domain::pruned(s, source);
  CHECK(!out.has_value());
}

} // namespace

int main() {
  return mru::test::run_all();
}