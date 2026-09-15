#include "mru_merge.hpp"

#include <algorithm>

namespace mru::plugin {

std::vector<mru::domain::WindowRef> merge_mru_order(const std::vector<mru::domain::WindowRef> &primary,
                                                    const std::vector<mru::domain::WindowRef> &fallback) {
    std::vector<mru::domain::WindowRef> out;
    out.reserve(primary.size() + fallback.size());

    const auto push_unique = [&out](const mru::domain::WindowRef &ref) {
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