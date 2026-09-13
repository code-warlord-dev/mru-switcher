#include <cstddef>

#include "mru/domain/selection.hpp"
#include "test_framework.hpp"

namespace {
using mru::domain::Direction;
using mru::domain::StartOffset;
} // namespace

TEST(initial_index_second) {
  // REQ-SEL-001: min(1, len-1)
  EQ(mru::domain::initial_index(StartOffset::Second, std::size_t{0}), std::size_t{0});
  EQ(mru::domain::initial_index(StartOffset::Second, std::size_t{1}), std::size_t{0});
  EQ(mru::domain::initial_index(StartOffset::Second, std::size_t{2}), std::size_t{1});
  EQ(mru::domain::initial_index(StartOffset::Second, std::size_t{9}), std::size_t{1});
}

TEST(initial_index_first) {
  // REQ-SEL-002: 0
  EQ(mru::domain::initial_index(StartOffset::First, std::size_t{0}), std::size_t{0});
  EQ(mru::domain::initial_index(StartOffset::First, std::size_t{1}), std::size_t{0});
  EQ(mru::domain::initial_index(StartOffset::First, std::size_t{9}), std::size_t{0});
}

TEST(advance_next_wrap) {
  // REQ-SEL-003: (index + 1) % len
  EQ(mru::domain::advance_index(0, Direction::Next, true, std::size_t{5}), std::size_t{1});
  EQ(mru::domain::advance_index(3, Direction::Next, true, std::size_t{5}), std::size_t{4});
  EQ(mru::domain::advance_index(4, Direction::Next, true, std::size_t{5}), std::size_t{0});
  EQ(mru::domain::advance_index(0, Direction::Next, true, std::size_t{1}), std::size_t{0});
  EQ(mru::domain::advance_index(0, Direction::Next, true, std::size_t{0}), std::size_t{0});
}

TEST(advance_prev_wrap) {
  // REQ-SEL-004: (index + len - 1) % len
  EQ(mru::domain::advance_index(0, Direction::Prev, true, std::size_t{5}), std::size_t{4});
  EQ(mru::domain::advance_index(4, Direction::Prev, true, std::size_t{5}), std::size_t{3});
  EQ(mru::domain::advance_index(1, Direction::Prev, true, std::size_t{5}), std::size_t{0});
  EQ(mru::domain::advance_index(0, Direction::Prev, true, std::size_t{0}), std::size_t{0});
}

TEST(advance_next_no_wrap) {
  // REQ-SEL-005: clamp at end
  EQ(mru::domain::advance_index(3, Direction::Next, false, std::size_t{5}), std::size_t{4});
  EQ(mru::domain::advance_index(4, Direction::Next, false, std::size_t{5}), std::size_t{4});
  EQ(mru::domain::advance_index(0, Direction::Next, false, std::size_t{1}), std::size_t{0});
}

TEST(advance_prev_no_wrap) {
  // REQ-SEL-005: clamp at start
  EQ(mru::domain::advance_index(2, Direction::Prev, false, std::size_t{5}), std::size_t{1});
  EQ(mru::domain::advance_index(0, Direction::Prev, false, std::size_t{5}), std::size_t{0});
  EQ(mru::domain::advance_index(0, Direction::Prev, false, std::size_t{0}), std::size_t{0});
}

int main() {
  return mru::test::run_all();
}