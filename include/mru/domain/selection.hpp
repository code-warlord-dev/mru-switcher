#pragma once

#include <cstddef>

#include "mru/domain/scope.hpp"

namespace mru::domain {

std::size_t initial_index(StartOffset offset, std::size_t len);
std::size_t advance_index(std::size_t index, Direction direction, bool wrap, std::size_t len);

} // namespace mru::domain