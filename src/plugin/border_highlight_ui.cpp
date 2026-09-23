#include "border_highlight_ui.hpp"

#include <charconv>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <format>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace mru::plugin {
namespace {

std::string_view trim_ws(std::string_view s) {
    std::size_t begin = 0;
    while (begin < s.size() && s[begin] == ' ')
        ++begin;
    std::size_t end = s.size();
    while (end > begin && s[end - 1] == ' ')
        --end;
    return s.substr(begin, end - begin);
}

// Parse one 1- or 2-digit hex group into a byte. Returns -1 on a non-hex char.
int hex_byte(std::string_view s) {
    auto digit = [](char c) -> int {
        if (c >= '0' && c <= '9')
            return c - '0';
        if (c >= 'a' && c <= 'f')
            return c - 'a' + 10;
        if (c >= 'A' && c <= 'F')
            return c - 'A' + 10;
        return -1;
    };
    if (s.empty() || s.size() > 2)
        return -1;
    const int hi = digit(s[0]);
    if (hi < 0)
        return -1;
    if (s.size() == 1)
        return hi;
    const int lo = digit(s[1]);
    if (lo < 0)
        return -1;
    return (hi << 4) | lo;
}

// struct that carries the parsed / shifted RGB channels. `ok` distinguishes the
// "stay constant" fallback (unparseable input) from a successfully read colour.
struct Rgb {
    bool ok = false;
    int r = 0, g = 0, b = 0, a = 255;
    // scale RGB by half, keep alpha; always re-emitted as 0xAARRGGBB hex.
    std::string darker_hex() const { return std::format("0x{:02x}{:02x}{:02x}{:02x}", a, r / 2, g / 2, b / 2); }
};

// Hybrid grammar accepted by the pinned parseColor() (setprop colour values):
//   #RGB #RRGGBB #RRGGBBAA | 0x… | rgb(...) rgba(...) | plain number
// We only need the forms users document: 0xAARRGGBB / 0xRRGGBB hex, rgb(r,g,b),
// rgba(r,g,b,a) (comma form) and rgba(rrggbbaa) / rgb(rrggbb) hex-inside. Any
// other input is returned unchanged (constant-colour pulse fallback).
Rgb parse_hypr_color(std::string_view raw) {
    const std::string_view input = trim_ws(raw);
    Rgb out;

    // 0x…: ARGB for 8 hex digits, RGB for 6 (alpha ff).
    if (input.size() >= 3 && input.starts_with("0x")) {
        const std::string_view hex = input.substr(2);
        if (hex.size() == 6) {
            out = {true, hex_byte(hex.substr(0, 2)), hex_byte(hex.substr(2, 2)), hex_byte(hex.substr(4, 2)), 255};
        } else if (hex.size() == 8) {
            out = {true, hex_byte(hex.substr(0, 2)), hex_byte(hex.substr(2, 2)), hex_byte(hex.substr(4, 2)),
                   hex_byte(hex.substr(6, 2))};
        }
        if (out.ok && (out.r < 0 || out.g < 0 || out.b < 0 || out.a < 0))
            out.ok = false;
        return out;
    }

    // rgb(r,g,b) / rgba(r,g,b,a) — comma form.
    if (input.starts_with("rgb") && input.ends_with(')')) {
        const std::string_view inner = input.substr(input.find('(') + 1, input.size() - input.find('(') - 2);
        int comps[4] = {-1, -1, -1, 255};
        std::size_t count = 0;
        std::size_t pos = 0;
        while (count < 4 && pos != std::string_view::npos) {
            const std::size_t comma = inner.find(',', pos);
            const std::string_view token =
                trim_ws(inner.substr(pos, comma == std::string_view::npos ? std::string_view::npos : comma - pos));
            if (token.empty())
                break;
            int val = 0;
            std::from_chars(token.data(), token.data() + token.size(), val);
            if (val < 0 || val > 255)
                break;
            comps[count++] = val;
            if (comma == std::string_view::npos)
                break;
            pos = comma + 1;
        }
        const std::size_t expected = input.starts_with("rgba(") ? 4 : 3;
        if (count == expected)
            out = {true, comps[0], comps[1], comps[2], input.starts_with("rgba(") ? comps[3] : 255};
        return out;
    }

    // rgb(rrggbb) / rgba(rrggbbaa) — hex inside the parens.
    if ((input.starts_with("rgb(") || input.starts_with("rgba(")) && input.ends_with(')')) {
        const std::string_view inner = input.substr(input.find('(') + 1, input.size() - input.find('(') - 2);
        if (inner.size() == 6) {
            out = {true, hex_byte(inner.substr(0, 2)), hex_byte(inner.substr(2, 2)), hex_byte(inner.substr(4, 2)), 255};
        } else if (inner.size() == 8 && input.starts_with("rgba(")) {
            // RGBA byte order inside rgba(); alpha is the LAST byte.
            out = {true, hex_byte(inner.substr(0, 2)), hex_byte(inner.substr(2, 2)), hex_byte(inner.substr(4, 2)),
                   hex_byte(inner.substr(6, 2))};
        }
        if (out.ok && (out.r < 0 || out.g < 0 || out.b < 0 || out.a < 0))
            out.ok = false;
        return out;
    }

    return out;
}

} // namespace

std::string darker_color(std::string_view color) {
    const Rgb parsed = parse_hypr_color(color);
    if (!parsed.ok)
        return std::string(color); // constant-colour fallback (ADR-028)
    return parsed.darker_hex();
}

// Live pin 0.56.2 evidence (M4-S3 nest smoke, evidence 09-border-size-probe.txt in
// docs/agent-state/reports/2026-09-18-m4-s3-nest-smoke.md):
//   * getprop answers "<hex> <N>deg" (e.g. "ff44cc88 0deg") for colour slots and a
//     bare integer for border_size, while setprop accepts ONLY 0x…/rgb()/rgba()
//     forms WITHOUT the angle suffix — a suffix-echoed value written back verbatim
//     parses to an empty gradient, i.e. an invisible border;
//   * border_size IS exposed through getprop (the earlier "no getprop for
//     border_size" assumption in R0 F3 was wrong), so the prior size can be
//     restored by value;
//   * opacity / opacity_inactive (ADR-028 dim) getprop replies are plain decimals
//     ("1", "0.7"); setprop accepts them verbatim (padmon: alphaToString normal
//     format prints valueOrDefault().alpha, not the "overridden" flag).
// normalize_capture turns every BorderPropIo::get() reply into a value setprop
// accepts (REQ-UI-004 exact restore, REQ-UI-005 no stuck/invisible borders):
//   1. trimmed; empty stays empty (capture-failure path unchanged);
//   2. a trailing whitespace-separated token ending in "deg" is dropped;
//   3. colour values not already 0x-prefixed and not rgb(/rgba( form get "0x"
//      prepended (6- and 8-hex-digit forms; already-0x values keep any nonzero
//      hex-digit count, e.g. short seeded forms);
//   4. Size slot: pure digit string, empty otherwise;
//   5. Alpha/AlphaInactive slots: plain finite decimal in [0,1], else empty;
//   6. anything still not matching a 0x-prefixed hex value / rgb(...) / rgba(...)
//      returns empty -> existing read_slot/highlight fail-soft skip applies.
std::string normalize_capture(std::string raw, BorderSlot slot) {
    constexpr std::string_view kWhitespace = " \t\r\n";
    const auto first = raw.find_first_not_of(kWhitespace);
    if (first == std::string::npos)
        return {};
    const auto last = raw.find_last_not_of(kWhitespace);
    raw = raw.substr(first, last - first + 1);

    if (slot == BorderSlot::Size) {
        if (raw.find_first_not_of("0123456789") != std::string::npos)
            return {}; // not a pure integer -> treat as unreadable
        return raw;
    }

    if (slot == BorderSlot::Alpha || slot == BorderSlot::AlphaInactive) {
        // getprop normal format: e.g. "1", "0.7". Must round-trip through setprop,
        // which parses a float; reject anything that is not a finite value in
        // [0, 1] so a malformed/unknown reply degrades to "no dim on this window".
        double value = 0.0;
        const char *s = raw.c_str();
        char *end = nullptr;
        value = std::strtod(s, &end);
        if (end == s || *end != '\0' || !std::isfinite(value) || value < 0.0 || value > 1.0)
            return {};
        return raw;
    }

    // Colour slot: drop a trailing whitespace-separated token ending in "deg"
    // (getprop echoes the gradient angle; setprop must not receive it).
    const std::size_t space = raw.find_last_of(kWhitespace);
    if (space != std::string::npos) {
        const std::string_view tail = std::string_view{raw}.substr(space + 1);
        if (tail.size() >= 3 && tail.substr(tail.size() - 3) == "deg")
            raw = std::string{raw.substr(0, space)};
        else
            return {}; // extra tokens other than the angle are not restorable
    }

    // setprop grammar: 0xRRGGBB | 0xAARRGGBB | rgb(...) | rgba(...), 0x REQUIRED.
    const bool rgb_form = raw.starts_with("rgb(") || raw.starts_with("rgba(");
    if (!raw.starts_with("0x") && !rgb_form) {
        if (raw.size() != 6 && raw.size() != 8)
            return {}; // not a recognised hex length
        raw.insert(0, "0x");
    }
    if (rgb_form)
        return raw;

    // Already-0x-prefixed: accept any nonzero hex-digit count (existing seeded
    // short forms like "0xa1" must round-trip unchanged; getprop echoes the
    // configured width). A bare value must be 6 or 8 digits before prepending.
    const std::string_view hex_digits = std::string_view{raw}.substr(2);
    if (hex_digits.empty())
        return {};
    if (hex_digits.find_first_not_of("0123456789abcdefABCDEF") != std::string::npos)
        return {}; // non-hex digits are not restorable
    return raw;
}

BorderHighlightUI::BorderHighlightUI(BorderPropIo &io, Validator is_valid, BorderStyle style, std::string color,
                                     int size, Warn warn, WorkspaceNavigator &navigator, BorderStyleParams params,
                                     PulseTimerPort &pulse_timer)
    : io_(io), is_valid_(std::move(is_valid)), style_(style), color_(std::move(color)), size_(size),
      warn_(std::move(warn)), navigator_(navigator), pulse_timer_(pulse_timer), params_(params),
      half_cycle_ms_(params.pulse_period_ms / 2) {}

void BorderHighlightUI::on_session_start(const mru::domain::Snapshot &snapshot, std::size_t index) {
    restore_all(); // defensive: never leak a highlight from a previous session
    snapshot_ = snapshot;
    degraded_ = false; // REQ-UI-002: the degrade decision is per session
    // ADR-026: pair every begin() with an end() — a degraded session that never
    // highlights still ends cleanly with an empty capture set.
    navigator_.begin();
    if (!probe_available()) {
        degraded_ = true;
        warn_once("border API unavailable; highlight disabled for this session");
        return;
    }
    highlight(index); // also drives navigator_.ensure_visible (REQ-UI-012)
}

void BorderHighlightUI::on_selection_changed(std::size_t index) {
    restore_all(); // REQ-UI-004: clear the previous highlight before applying the new one
    highlight(index);
}

void BorderHighlightUI::on_session_end(mru::domain::UIEndReason reason) {
    restore_all(); // REQ-UI-005: full clear on every end path
    // ADR-026: Cancelled -> restore the monitors this session elevated; Applied ->
    // leave views as-is (the target workspace is already active; FocusGateway
    // focuses the selected window next). Fail-soft, like every other port below.
    try {
        navigator_.end(reason);
    } catch (...) {
        warn_once("workspace restore failed");
    }
    snapshot_.reset();
    degraded_ = false;
}

const std::string &BorderHighlightUI::style_color() const {
    switch (style_) {
    case BorderStyle::Solid:
    case BorderStyle::Pulse:
    case BorderStyle::Dim:
    default:
        return color_; // ADR-028: base colour is the constant (pulse toggles from it)
    }
}

bool BorderHighlightUI::probe_available() {
    if (!snapshot_)
        return true; // nothing to probe; no writes will happen anyway
    for (std::size_t i = 0; i < snapshot_->size(); ++i) {
        const mru::domain::WindowRef &ref = snapshot_->at(i);
        if (!is_valid_ || !is_valid_(ref))
            continue;
        try {
            return !io_.get(ref.address, BorderSlot::ActiveColor).empty();
        } catch (...) {
            return false;
        }
    }
    return true; // no valid target to probe
}

void BorderHighlightUI::highlight(std::size_t index) {
    if (degraded_ || !snapshot_ || index >= snapshot_->size())
        return;
    const mru::domain::WindowRef &ref = snapshot_->at(index);
    if (!is_valid_ || !is_valid_(ref))
        return; // REQ-UI-010: invalid target -> skip highlight, session continues

    // Restore safety (R0 F10): only override once BOTH prior colour values were
    // read back, otherwise a failed restore could leave an invisible border.
    WindowCapture cap;
    cap.ref = ref;
    cap.selected = true;
    if (!read_slot(ref.address, BorderSlot::ActiveColor, cap.active) ||
        !read_slot(ref.address, BorderSlot::InactiveColor, cap.inactive)) {
        warn_once("border property read failed; skipping highlight");
        return;
    }

    // ADR-026 / REQ-UI-012: keep the selected window visible (workspace elevation,
    // never window focus — REQ-F-003 / REQ-UI-006). Runs only when the highlight
    // is actually about to be drawn — a degraded/read-failed target never flips the
    // view without a highlight. Fail-soft like the border writes (REQ-UI-001).
    try {
        navigator_.ensure_visible(ref);
    } catch (...) {
        warn_once("workspace elevation failed");
    }

    const std::string &highlight = style_color();
    apply_slot(ref.address, BorderSlot::ActiveColor, highlight, cap.active.applied);
    apply_slot(ref.address, BorderSlot::InactiveColor, highlight, cap.inactive.applied);
    // border_size IS readable via getprop on the pin (live evidence: nest smoke,
    // evidence 09-border-size-probe.txt in
    // docs/agent-state/reports/2026-09-18-m4-s3-nest-smoke.md — the earlier R0 F3
    // claim that it was not exposed was wrong). Capture the prior integer BEFORE
    // the override write (a post-write read would see our own value) and restore
    // by value; `unset` is only the fallback when that read fails.
    if (size_ >= 0) {
        SlotCapture prior_size;
        const bool size_read = read_slot(ref.address, BorderSlot::Size, prior_size);
        apply_slot(ref.address, BorderSlot::Size, std::to_string(size_), cap.size_applied);
        if (cap.size_applied && size_read) {
            cap.size_captured = true;
            cap.size_value = prior_size.value;
        }
    }
    captures_.push_back(std::move(cap));

    // ADR-028 style side effects:
    //   * Pulse -> (re)arm the colour throb timer; a failed schedule keeps the
    //     constant colour (fail-soft, REQ-UI-001).
    //   * Dim -> dim every non-selected ring window (REQ-UI-014).
    if (style_ == BorderStyle::Pulse) {
        start_pulse();
    } else if (style_ == BorderStyle::Dim) {
        dim_ring(index);
    }
}

void BorderHighlightUI::dim_ring(std::size_t selected_index) {
    if (!snapshot_ || params_.dim_alpha >= 1.0)
        return; // dim_alpha >= 1.0 disables dimming (SPEC §4 REQ-UI-014)
    const std::string dim_value = std::format("{}", params_.dim_alpha);
    for (std::size_t i = 0; i < snapshot_->size(); ++i) {
        const mru::domain::WindowRef &ref = snapshot_->at(i);
        if (i == selected_index)
            continue;
        if (!is_valid_ || !is_valid_(ref))
            continue;
        WindowCapture cap;
        cap.ref = ref;
        // Restore-by-value safety: dim only when BOTH alpha channels were read
        // back, otherwise a failed restore could leave a stuck-transparent window
        // (REQ-UI-005, same rule as the colour slots).
        if (!read_slot(ref.address, BorderSlot::Alpha, cap.alpha) ||
            !read_slot(ref.address, BorderSlot::AlphaInactive, cap.alpha_inactive)) {
            warn_once("alpha read failed for a ring window; skipping its dim");
            continue;
        }
        apply_slot(ref.address, BorderSlot::Alpha, dim_value, cap.alpha.applied);
        apply_slot(ref.address, BorderSlot::AlphaInactive, dim_value, cap.alpha_inactive.applied);
        captures_.push_back(std::move(cap));
    }
}

// ADR-028/REQ-UI-013: colour throb timer. Half-cycle period = pulse_period_ms / 2
// (e.g. 1000ms pulse = 500ms per tick). schedule() returning false means the host
// has no timer (e.g. test null port) — keep the constant colour, fail-soft.
// Exceptions from the port are isolated exactly like every other io (REQ-UI-001):
// a throwing timer must never abort the session.
void BorderHighlightUI::start_pulse() {
    cancel_pulse();
    if (style_ != BorderStyle::Pulse || degraded_)
        return;
    try {
        if (!pulse_timer_.schedule(half_cycle_ms_, [this] { pulse_tick(); }))
            warn_once("pulse timer unavailable; keeping constant highlight colour");
    } catch (...) {
        warn_once("pulse timer failed; keeping constant highlight colour");
    }
}

void BorderHighlightUI::cancel_pulse() {
    try {
        pulse_timer_.cancel();
    } catch (...) {
        // REQ-UI-001: a failing cancel disarms nothing but must not throw out
        // (the captured latch below still drops the throb state).
    }
    pulse_dark_ = false;
}

void BorderHighlightUI::pulse_tick() {
    if (style_ != BorderStyle::Pulse || degraded_)
        return;
    // Apply the toggled colour to the highlighted window only (the ring windows
    // are dim targets, not pulse targets — styles are exclusive at config parse).
    if (captures_.empty())
        return;
    pulse_dark_ = !pulse_dark_;
    const std::string toggled = pulse_dark_ ? darker_color(color_) : color_;
    WindowCapture &head = captures_.front();
    // safe_set (not apply_slot): a failed tick write must NOT clear the `applied`
    // flag, or restore_window would skip this slot and strand the pulse colour.
    safe_set(head.ref.address, BorderSlot::ActiveColor, toggled);
    safe_set(head.ref.address, BorderSlot::InactiveColor, toggled);
}

bool BorderHighlightUI::read_slot(std::uint64_t address, BorderSlot slot, SlotCapture &out) {
    try {
        out.value = normalize_capture(io_.get(address, slot), slot);
    } catch (...) {
        return false;
    }
    return !out.value.empty();
}

void BorderHighlightUI::apply_slot(std::uint64_t address, BorderSlot slot, const std::string &value, bool &applied) {
    applied = safe_set(address, slot, value);
}

void BorderHighlightUI::restore_window(WindowCapture &cap) {
    // REQ-UI-005: restore only the slots we actually overrode, from captured values
    // (already normalized to the setprop grammar at capture time).
    if (cap.active.applied)
        safe_set(cap.ref.address, BorderSlot::ActiveColor, cap.active.value);
    if (cap.inactive.applied)
        safe_set(cap.ref.address, BorderSlot::InactiveColor, cap.inactive.value);
    // ADR-028/REQ-UI-014: dim ring windows restore their prior per-window alpha.
    if (cap.alpha.applied)
        safe_set(cap.ref.address, BorderSlot::Alpha, cap.alpha.value);
    if (cap.alpha_inactive.applied)
        safe_set(cap.ref.address, BorderSlot::AlphaInactive, cap.alpha_inactive.value);
    if (!cap.size_applied)
        return;
    if (cap.size_captured)
        safe_set(cap.ref.address, BorderSlot::Size, cap.size_value); // exact prior size
    else
        safe_set(cap.ref.address, BorderSlot::Size, "unset"); // read failed: drop the override
}

void BorderHighlightUI::restore_all() {
    cancel_pulse(); // REQ-UI-013: no tick behind a cleared/sessionless capture list
    for (WindowCapture &cap : captures_)
        restore_window(cap);
    captures_.clear();
}

bool BorderHighlightUI::safe_set(std::uint64_t address, BorderSlot slot, const std::string &value) {
    try {
        if (io_.set(address, slot, value))
            return true;
        warn_once("border property write failed");
    } catch (...) {
        warn_once("border property write threw");
    }
    return false;
}

void BorderHighlightUI::warn_once(std::string_view reason) {
    if (warned_)
        return;
    warned_ = true;
    if (!warn_)
        return;
    try {
        warn_(reason);
    } catch (...) {
        // never let the warn sink break the session (REQ-UI-001)
    }
}

} // namespace mru::plugin