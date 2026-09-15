#include "mru_merge.hpp"

#include <algorithm>

namespace mru::plugin {

std::vector<mru::domain::WindowRef> merge_mru_order(const std::vector<mru::domain::WindowRef> &primary,
                                                    const std::vector<mru::domain::WindowRef> &fallback) {
    std::vector<mru::domain::WindowRef> out;
    out.reserve(primary.size() + fallback.size());

    const auto push_unique = [&out](const mru::domain::WindowRef &ref) {
        // std::find here is O(n²) over the merged list; the input is tens of
        // windows at most, so an unordered_set would be blind optimization (L-17).
        if (std::find(out.begin(), out.end(), ref) == out.end())
            out.push_back(ref);
    };

    for (const auto &ref : primary)
        push_unique(ref);
    for (const auto &ref : fallback)
        push_unique(ref);

    return out;
}

} // namespace mru::plugin