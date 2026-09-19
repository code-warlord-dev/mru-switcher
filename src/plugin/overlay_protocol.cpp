#include "overlay_protocol.hpp"

#include <charconv>
#include <cstdint>
#include <system_error>

namespace mru::plugin::overlay_protocol {
namespace {

void append_json_string(std::string &out, std::string_view s) {
    out.push_back('"');
    for (const char c : s) {
        const unsigned char uc = static_cast<unsigned char>(c);
        switch (uc) {
        case '"':
            out += "\\\"";
            break;
        case '\\':
            out += "\\\\";
            break;
        case '\b':
            out += "\\b";
            break;
        case '\f':
            out += "\\f";
            break;
        case '\n':
            out += "\\n";
            break;
        case '\r':
            out += "\\r";
            break;
        case '\t':
            out += "\\t";
            break;
        default:
            if (uc < 0x20) {
                constexpr char kHex[] = "0123456789abcdef";
                out += "\\u00";
                out.push_back(kHex[(uc >> 4) & 0xF]);
                out.push_back(kHex[uc & 0xF]);
            } else {
                out.push_back(static_cast<char>(uc));
            }
        }
    }
    out.push_back('"');
}

std::string encode_addr(std::uint64_t address) {
    constexpr char kHex[] = "0123456789abcdef";
    std::string out = "0x";
    if (address == 0) {
        out.push_back('0');
        return out;
    }
    bool started = false;
    for (int shift = 60; shift >= 0; shift -= 4) {
        const unsigned nibble = static_cast<unsigned>((address >> shift) & 0xF);
        if (!started && nibble == 0)
            continue;
        started = true;
        out.push_back(kHex[nibble]);
    }
    return out;
}

// --- minimal flat-object reader -------------------------------------------------
// Peer commands are flat JSON objects (Appendix B), so a full document parser is
// not required: locate a `"key"` and read the value of the expected kind.
// Anything that does not fit is treated as malformed (REQ-O-005).

void skip_ws(std::string_view s, std::size_t &i) {
    while (i < s.size() && (s[i] == ' ' || s[i] == '\t' || s[i] == '\r' || s[i] == '\n'))
        ++i;
}

std::size_t value_after_key(std::string_view s, std::string_view key) {
    std::string needle;
    needle.reserve(key.size() + 2);
    needle.push_back('"');
    needle += key;
    needle.push_back('"');
    const std::size_t pos = s.find(needle);
    if (pos == std::string_view::npos)
        return std::string_view::npos;
    std::size_t i = pos + needle.size();
    skip_ws(s, i);
    if (i >= s.size() || s[i] != ':')
        return std::string_view::npos;
    ++i;
    skip_ws(s, i);
    return i < s.size() ? i : std::string_view::npos;
}

std::optional<std::int64_t> read_int(std::string_view s, std::string_view key) {
    const std::size_t i = value_after_key(s, key);
    if (i == std::string_view::npos)
        return std::nullopt;
    std::int64_t value = 0;
    const char *begin = s.data() + i;
    const char *end = s.data() + s.size();
    const auto res = std::from_chars(begin, end, value);
    if (res.ec != std::errc{} || res.ptr == begin)
        return std::nullopt;
    return value;
}

std::optional<std::string> read_string(std::string_view s, std::string_view key) {
    std::size_t i = value_after_key(s, key);
    if (i == std::string_view::npos || s[i] != '"')
        return std::nullopt;
    ++i;
    std::string out;
    while (i < s.size()) {
        const char c = s[i];
        if (c == '\\' && i + 1 < s.size()) {
            switch (s[i + 1]) {
            case '"':
                out.push_back('"');
                break;
            case '\\':
                out.push_back('\\');
                break;
            case '/':
                out.push_back('/');
                break;
            case 'b':
                out.push_back('\b');
                break;
            case 'f':
                out.push_back('\f');
                break;
            case 'n':
                out.push_back('\n');
                break;
            case 'r':
                out.push_back('\r');
                break;
            case 't':
                out.push_back('\t');
                break;
            default:
                return std::nullopt; // unsupported escape -> not a command
            }
            i += 2;
            continue;
        }
        if (c == '"')
            return out;
        out.push_back(c);
        ++i;
    }
    return std::nullopt; // unterminated string
}

std::string_view trim(std::string_view s) {
    while (!s.empty() && (s.front() == ' ' || s.front() == '\t' || s.front() == '\r' || s.front() == '\n'))
        s.remove_prefix(1);
    while (!s.empty() && (s.back() == ' ' || s.back() == '\t' || s.back() == '\r' || s.back() == '\n'))
        s.remove_suffix(1);
    return s;
}

} // namespace

std::string encode_session_start(const std::vector<OverlayWindowInfo> &windows, std::size_t index) {
    std::string out = "{\"v\":";
    out += std::to_string(kVersion);
    out += ",\"type\":\"session_start\",\"windows\":[";
    bool first = true;
    for (const OverlayWindowInfo &w : windows) {
        if (!first)
            out.push_back(',');
        first = false;
        out += "{\"addr\":\"";
        out += encode_addr(w.address);
        out += "\",\"title\":";
        append_json_string(out, w.title);
        out += ",\"class\":";
        append_json_string(out, w.window_class);
        out.push_back('}');
    }
    out += "],\"index\":";
    out += std::to_string(index);
    out.push_back('}');
    return out;
}

std::string encode_selection(std::size_t index) {
    return "{\"v\":" + std::to_string(kVersion) + ",\"type\":\"selection\",\"index\":" + std::to_string(index) + "}";
}

std::string encode_session_end(mru::domain::UIEndReason reason) {
    return "{\"v\":" + std::to_string(kVersion) + ",\"type\":\"session_end\",\"reason\":\"" +
           (reason == mru::domain::UIEndReason::Applied ? "applied" : "cancelled") + "\"}";
}

std::optional<Command> parse_command(std::string_view line) {
    if (line.size() > kMaxLineBytes)
        return std::nullopt;
    line = trim(line);
    if (line.size() < 2 || line.front() != '{' || line.back() != '}')
        return std::nullopt;

    const std::optional<std::int64_t> version = read_int(line, "v");
    if (!version || *version != kVersion)
        return std::nullopt;

    const std::optional<std::string> type = read_string(line, "type");
    if (!type)
        return std::nullopt;

    if (*type == "apply")
        return Command{CommandType::Apply, 0};
    if (*type == "cancel")
        return Command{CommandType::Cancel, 0};
    if (*type == "select") {
        const std::optional<std::int64_t> index = read_int(line, "index");
        if (!index || *index < 0)
            return std::nullopt;
        return Command{CommandType::Select, static_cast<std::size_t>(*index)};
    }
    return std::nullopt; // unknown type (REQ-O-005)
}

} // namespace mru::plugin::overlay_protocol
