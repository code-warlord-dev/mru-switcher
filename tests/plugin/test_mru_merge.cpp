#include "mru_merge.hpp"
#include "test_framework.hpp"

namespace {
using mru::domain::WindowRef;
using mru::plugin::merge_mru_order;

WindowRef ref(std::uint64_t address, std::uint64_t generation = 1) {
    return WindowRef{address, generation};
}

// ADR-015: the plugin-owned MRU prefix keeps its order, the enumeration is appended.
TEST(merge_01_primary_first_fallback_appended) {
    const auto out = merge_mru_order({ref(3), ref(2)}, {ref(2), ref(1)});
    CHECK(out.size() == 3);
    EQ(out[0].address, 3u);
    EQ(out[1].address, 2u);
    EQ(out[2].address, 1u);
}

TEST(merge_02_dedup_keeps_first_occurrence) {
    const auto out = merge_mru_order({ref(1), ref(1)}, {ref(1)});
    CHECK(out.size() == 1);
    EQ(out[0].address, 1u);
}

// REQ-ID-003: identity is address AND generation.
TEST(merge_03_generation_is_part_of_identity) {
    const auto out = merge_mru_order({ref(1, 2)}, {ref(1, 1)});
    CHECK(out.size() == 2);
    EQ(out[0].generation, 2u);
    EQ(out[1].generation, 1u);
}

TEST(merge_04_empty_and_single_sources) {
    CHECK(merge_mru_order({}, {}).empty());

    const auto only_fallback = merge_mru_order({}, {ref(7)});
    CHECK(only_fallback.size() == 1);
    EQ(only_fallback[0].address, 7u);

    const auto only_primary = merge_mru_order({ref(7)}, {});
    CHECK(only_primary.size() == 1);
    EQ(only_primary[0].address, 7u);
}

} // namespace

int main() {
    return mru::test::run_all();
}