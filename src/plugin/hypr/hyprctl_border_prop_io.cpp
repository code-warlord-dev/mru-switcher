#include "hyprctl_border_prop_io.hpp"

#include <cstddef>
#include <format>
#include <string>

#include <hyprland/src/managers/KeybindManager.hpp>
#include <hyprland/src/plugins/PluginAPI.hpp>

namespace mru::plugin {
namespace {

bool is_space(unsigned char c) {
    return c == ' ' || c == '\n' || c == '\r' || c == '\t';
}

// hyprctl replies are newline-terminated; setprop joins the remaining tokens, so a
// trailing newline would end up part of the restored value.
std::string trim_ws(std::string s) {
    std::size_t start = 0;
    while (start < s.size() && is_space(static_cast<unsigned char>(s[start])))
        ++start;
    std::size_t end = s.size();
    while (end > start && is_space(static_cast<unsigned char>(s[end - 1])))
        --end;
    return s.substr(start, end - start);
}

// hyprctl error replies (F1/F8); anything else is treated as a successful value.
bool looks_like_error(const std::string &out) {
    return out.find("Invalid") != std::string::npos || out.find("invalid") != std::string::npos ||
           out.find("Error") != std::string::npos || out.find("error") != std::string::npos ||
           out.find("unknown") != std::string::npos || out.find("not found") != std::string::npos ||
           out.find("not enough") != std::string::npos;
}

} // namespace

std::string HyprctlBorderPropIo::selector(std::uint64_t address) {
    return std::format("address:0x{:x}", address); // lowercase, no padding (R0 F9)
}

std::string HyprctlBorderPropIo::prop_name(BorderSlot slot) {
    switch (slot) {
    case BorderSlot::ActiveColor:
        return "active_border_color";
    case BorderSlot::InactiveColor:
        return "inactive_border_color";
    case BorderSlot::Size:
        return "border_size";
    case BorderSlot::Alpha:
        return "opacity"; // ADR-028/REQ-UI-014: per-window alpha (active-focus channel)
    case BorderSlot::AlphaInactive:
        return "opacity_inactive"; // ADR-028/REQ-UI-014: per-window alpha (others)
    }
    return {};
}

std::string HyprctlBorderPropIo::get(std::uint64_t address, BorderSlot slot) {
    try {
        const std::string out = HyprlandAPI::invokeHyprctlCommand("getprop", selector(address) + " " + prop_name(slot));
        if (out.empty() || looks_like_error(out))
            return {};
        return trim_ws(out);
    } catch (...) {
        return {};
    }
}

bool HyprctlBorderPropIo::set(std::uint64_t address, BorderSlot slot, const std::string &value) {
    // On Lua config backends (0.56.2 / efb5099) the hyprctl `dispatch` command is
    // re-evaluated as Lua (hl.dispatch(<raw args>)), so `setprop address:0x…` from
    // invokeHyprctlCommand("dispatch", …) is a parse error and the write never runs.
    // Call the compositor's `setprop` dispatcher in-process instead — the legacy
    // dispatcher translator is backend-agnostic (see docs/COMPAT.md Known host limits).
    try {
        const auto it = g_pKeybindManager->m_dispatchers.find("setprop");
        if (it == g_pKeybindManager->m_dispatchers.end())
            return false;
        const SDispatchResult res = it->second(selector(address) + " " + prop_name(slot) + " " + value);
        return res.success && res.error.empty();
    } catch (...) {
        return false;
    }
}

} // namespace mru::plugin
