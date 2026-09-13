#include <cstdint>

#include "mru/domain/window_ref.hpp"
#include "test_framework.hpp"

namespace {
using mru::domain::WindowRef;

WindowRef ref(std::uint64_t address, std::uint64_t generation) {
    return WindowRef{address, generation};
}
} // namespace

TEST(id01_same_fields_equal) {
    const WindowRef a = ref(0x1000, 1);
    const WindowRef b = ref(0x1000, 1);
    CHECK(a == b);
    CHECK(!(a != b));
}

TEST(id01_address_mismatch_unequal) {
    CHECK(ref(0x1000, 1) != ref(0x2000, 1));
    CHECK(!(ref(0x1000, 1) == ref(0x2000, 1)));
}

TEST(id01_generation_mismatch_unequal) {
    CHECK(ref(0x1000, 1) != ref(0x1000, 2));
    CHECK(!(ref(0x1000, 1) == ref(0x1000, 2)));
}

TEST(id01_default_zero_ref) {
    const WindowRef d;
    CHECK(d.address == 0);
    CHECK(d.generation == 0);
    CHECK(d == WindowRef{});
}

int main() {
    return mru::test::run_all();
}