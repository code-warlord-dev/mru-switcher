#include "mru/domain/selection.hpp"

#include <algorithm>

namespace mru::domain {

std::size_t initial_index(StartOffset offset, std::size_t len) {
  if (len == 0)
    return 0;
  switch (offset) {
  case StartOffset::First:
    return 0;
  case StartOffset::Second:
    return std::min<std::size_t>(1, len - 1);
  }
  return 0;
}

std::size_t advance_index(std::size_t index, Direction direction, bool wrap, std::size_t len) {
  if (len == 0)
    return 0;

  index = std::min(index, len - 1);

  if (!wrap) {
    if (direction == Direction::Next)
      return std::min(index + 1, len - 1);
    return index == 0 ? 0 : index - 1;
  }

  if (direction == Direction::Next)
    return (index + 1) % len;
  return (index + len - 1) % len;
}

} // namespace mru::domain