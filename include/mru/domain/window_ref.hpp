#pragma once

#include <cstdint>
#include <functional>

namespace mru::domain {

struct WindowRef {
    std::uint64_t address{};
    std::uint64_t generation{};

    friend bool operator==(const WindowRef &a, const WindowRef &b) {
        return a.address == b.address && a.generation == b.generation;
    }
    friend bool operator!=(const WindowRef &a, const WindowRef &b) { return !(a == b); }
};

} // namespace mru::domain
