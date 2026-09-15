#include "status_format.hpp"

namespace mru::plugin {
namespace {

std::string_view end_reason_name(mru::domain::SessionEndReason reason) {
    using mru::domain::SessionEndReason;
    switch (reason) {
    case SessionEndReason::Applied:
        return "Applied";
    case SessionEndReason::UserCancel:
        return "UserCancel";
    case SessionEndReason::NoWindows:
        return "NoWindows";
    case SessionEndReason::InvalidSelection:
        return "InvalidSelection";
    case SessionEndReason::FocusFailed:
        return "FocusFailed";
    case SessionEndReason::PluginShutdown:
        return "PluginShutdown";
    }
    return "unknown";
}

} // namespace

std::string format_status(bool active, std::size_t index, std::size_t size, std::string_view scope,
                          std::uint64_t session_id, std::optional<mru::domain::SessionEndReason> last_end_reason) {
    std::string out = std::string{"active="} + (active ? "true" : "false");
    out += " index=" + std::to_string(index);
    out += " size=" + std::to_string(size);
    out += " scope=" + std::string{scope};
    out += " session=" + std::to_string(session_id);
    out += " last_end=" + std::string{last_end_reason ? end_reason_name(*last_end_reason) : "none"};
    return out;
}

} // namespace mru::plugin
