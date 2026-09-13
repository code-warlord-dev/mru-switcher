#include "mru/domain/snapshot.hpp"

#include "mru/domain/window_source.hpp"

namespace mru::domain {

Snapshot::Snapshot(std::vector<WindowRef> windows, Scope scope) : windows_(std::move(windows)), scope_(scope) {}

const std::vector<WindowRef> &Snapshot::windows() const {
    return windows_;
}

std::size_t Snapshot::size() const {
    return windows_.size();
}

bool Snapshot::empty() const {
    return windows_.empty();
}

const WindowRef &Snapshot::at(std::size_t i) const {
    return windows_.at(i);
}

Scope Snapshot::scope() const {
    return scope_;
}

std::optional<Snapshot> pruned(const Snapshot &snapshot, const WindowSource &source) {
    std::vector<WindowRef> survivors;
    survivors.reserve(snapshot.size());
    for (const WindowRef &ref : snapshot.windows()) {
        if (source.is_valid(ref))
            survivors.push_back(ref);
    }

    if (survivors.empty())
        return std::nullopt;

    return Snapshot(std::move(survivors), snapshot.scope());
}

} // namespace mru::domain
