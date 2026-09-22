#include <array>
#include <cstddef>
#include <cstdint>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

#include "border_highlight_ui.hpp"
#include "config_value.hpp"
#include "hypr/null_ui.hpp"
#include "mru/domain/fake_clock.hpp"
#include "mru/domain/history_tracker.hpp"
#include "mru/domain/session_controller.hpp"
#include "mru/domain/snapshot.hpp"
#include "mru/domain/window_ref.hpp"

#include "test_fakes.hpp"
#include "test_framework.hpp"

namespace {

using mru::domain::Direction;
using mru::domain::FakeClock;
using mru::domain::HistoryTracker;
using mru::domain::Scope;
using mru::domain::SessionController;
using mru::domain::SessionPolicy;
using mru::domain::Snapshot;
using mru::domain::StartOffset;
using mru::domain::UIEndReason;
using mru::domain::WindowRef;
using mru::plugin::BorderHighlightUI;
using mru::plugin::BorderPropIo;
using mru::plugin::BorderSlot;
using mru::plugin::BorderStyle;

const std::string kHighlight = "0xffffd9a0";
constexpr std::uint64_t A = 0xA;
constexpr std::uint64_t B = 0xB;
constexpr std::uint64_t C = 0xC;

std::size_t idx(BorderSlot slot) {
    return static_cast<std::size_t>(slot);
}

WindowRef ref(std::uint64_t address, std::uint64_t generation = 1) {
    return WindowRef{address, generation};
}

// Records every read/write and can inject soft/fatal failures to prove the
// backend is fail-soft (REQ-UI-001).
struct FakeBorderPropIo : BorderPropIo {
    std::map<std::uint64_t, std::array<std::string, 3>> values; // "" = unknown/unsupported
    std::vector<std::tuple<std::uint64_t, BorderSlot, std::string>> writes;
    std::vector<std::pair<std::uint64_t, BorderSlot>> reads;
    bool fail_sets = false;
    bool fail_gets = false;
    bool fail_get_inactive = false; // probe (active) passes, inactive read fails
    bool throw_sets = false;
    bool throw_gets = false;
    int set_calls = 0;
    int get_calls = 0;

    std::string get(std::uint64_t address, BorderSlot slot) override {
        ++get_calls;
        reads.emplace_back(address, slot);
        if (throw_gets)
            throw std::runtime_error("fake get");
        if (fail_gets)
            return {};
        if (fail_get_inactive && slot == BorderSlot::InactiveColor)
            return {};
        const auto it = values.find(address);
        if (it == values.end())
            return {};
        return it->second[idx(slot)];
    }

    bool set(std::uint64_t address, BorderSlot slot, const std::string &value) override {
        ++set_calls;
        writes.emplace_back(address, slot, value);
        if (throw_sets)
            throw std::runtime_error("fake set");
        if (fail_sets)
            return false;
        values[address][idx(slot)] = value;
        return true;
    }
};

void seed(FakeBorderPropIo &io, std::uint64_t address, std::string active, std::string inactive) {
    io.values[address][idx(BorderSlot::ActiveColor)] = std::move(active);
    io.values[address][idx(BorderSlot::InactiveColor)] = std::move(inactive);
}

// Seeds a RAW live getprop-format value (e.g. "ff44cc88 0deg" for colours, "2" for
// size) without normalization, mimicking what the pinned compositor actually
// reports (M4-S3 nest smoke, evidence 09-border-size-probe.txt).
void seed_raw(FakeBorderPropIo &io, std::uint64_t address, BorderSlot slot, std::string raw) {
    io.values[address][idx(slot)] = std::move(raw);
}

bool any_highlight_left(const FakeBorderPropIo &io) {
    for (const auto &[address, slots] : io.values) {
        (void)address;
        if (slots[idx(BorderSlot::ActiveColor)] == kHighlight || slots[idx(BorderSlot::InactiveColor)] == kHighlight)
            return true;
    }
    return false;
}

// R0 F10: a colour slot must never be restored with a bare `-1`/`unset`.
bool color_slot_write_is_bare_clear(const FakeBorderPropIo &io) {
    for (const auto &[address, slot, value] : io.writes) {
        (void)address;
        if (slot != BorderSlot::ActiveColor && slot != BorderSlot::InactiveColor)
            continue;
        if (value == "-1" || value == "unset")
            return true;
    }
    return false;
}

// --- T-UI-03: ui=null -> no border side effects ---------------------------------
TEST(t_ui_03_null_backend_no_border_io) {
    mru::plugin::PluginConfig cfg = mru::plugin::default_plugin_config();
    cfg.ui_null = true; // ADR-025: default is border, so opt out explicitly here
    cfg.ui_border = false;
    EQ(mru::plugin::effective_ui_backend(cfg), mru::plugin::UiBackend::Null);

    FakeBorderPropIo io; // must stay untouched: the null backend has no adapter
    mru::plugin::NullUI ui;
    const Snapshot snap({ref(A), ref(B)}, Scope::Global);
    ui.on_session_start(snap, 0);
    ui.on_selection_changed(1);
    ui.on_session_end(UIEndReason::Applied);
    EQ(io.set_calls, 0);
    EQ(io.get_calls, 0);
}

// --- T-UI-04: selection change restores previous, highlights new ----------------
TEST(t_ui_04_selection_change_restores_previous) {
    FakeBorderPropIo io;
    seed(io, A, "0xaa000001", "0xaa000002");
    seed(io, B, "0xbb000001", "0xbb000002");
    const auto valid = [](const WindowRef &r) { return r.address == A || r.address == B || r.address == C; };
    BorderHighlightUI ui(io, valid, BorderStyle::Solid, kHighlight, -1);
    const Snapshot snap({ref(A), ref(B), ref(C)}, Scope::Global);

    ui.on_session_start(snap, 0);
    CHECK(io.values[A][idx(BorderSlot::ActiveColor)] == kHighlight);
    CHECK(io.values[A][idx(BorderSlot::InactiveColor)] == kHighlight);

    ui.on_selection_changed(1);
    CHECK(io.values[A][idx(BorderSlot::ActiveColor)] == "0xaa000001"); // previous restored (REQ-UI-004)
    CHECK(io.values[A][idx(BorderSlot::InactiveColor)] == "0xaa000002");
    CHECK(io.values[B][idx(BorderSlot::ActiveColor)] == kHighlight); // new highlighted
    CHECK(io.values[B][idx(BorderSlot::InactiveColor)] == kHighlight);
    CHECK(io.values[C][idx(BorderSlot::ActiveColor)] == ""); // untouched

    ui.on_session_end(UIEndReason::Applied);
    CHECK(io.values[B][idx(BorderSlot::ActiveColor)] == "0xbb000001"); // restored
    CHECK(io.values[B][idx(BorderSlot::InactiveColor)] == "0xbb000002");
    CHECK(!any_highlight_left(io));
}

// --- T-UI-05: session controller apply/cancel/unload -> no stuck highlight ------
struct Fixture {
    FakeClock clock;
    mru::test::MockWindowSource source;
    mru::test::MockFocusGateway fg;
    FakeBorderPropIo io;
    BorderHighlightUI ui;
    HistoryTracker tracker;
    SessionController sc;

    static SessionPolicy first_policy() {
        SessionPolicy p;
        p.start_offset = StartOffset::First;
        return p;
    }

    Fixture()
        : ui(
              io, [this](const WindowRef &r) { return source.is_valid(r); }, BorderStyle::Solid, kHighlight, -1),
          tracker(
              clock, [this](const WindowRef &r) { return source.is_valid(r); }, 50),
          sc(source, fg, ui, tracker, first_policy()) {}

    void candidates(std::vector<WindowRef> windows) {
        source.candidates_result = std::move(windows);
        for (const WindowRef &w : source.candidates_result)
            source.validity.emplace_back(w, true);
    }
};

TEST(t_ui_05_apply_restores_all_no_stuck) {
    Fixture f;
    seed(f.io, A, "0xa1", "0xa2");
    seed(f.io, B, "0xb1", "0xb2");
    f.candidates({ref(A), ref(B)});

    CHECK(f.sc.cycle(Direction::Next).ok); // A highlighted
    CHECK(f.io.values[A][idx(BorderSlot::ActiveColor)] == kHighlight);
    CHECK(f.sc.cycle(Direction::Next).ok); // B highlighted, A restored
    CHECK(f.io.values[A][idx(BorderSlot::ActiveColor)] == "0xa1");

    CHECK(f.sc.apply().ok);
    CHECK(!f.sc.is_active());
    CHECK(f.io.values[A][idx(BorderSlot::ActiveColor)] == "0xa1");
    CHECK(f.io.values[B][idx(BorderSlot::ActiveColor)] == "0xb1");
    CHECK(f.io.values[B][idx(BorderSlot::InactiveColor)] == "0xb2");
    CHECK(!any_highlight_left(f.io));
}

TEST(t_ui_05b_cancel_restores_all_no_stuck) {
    Fixture f;
    seed(f.io, A, "0xa1", "0xa2");
    seed(f.io, B, "0xb1", "0xb2");
    f.candidates({ref(A), ref(B)});

    CHECK(f.sc.cycle(Direction::Next).ok);
    CHECK(f.sc.cycle(Direction::Next).ok);
    CHECK(f.sc.cancel().ok);
    CHECK(!f.sc.is_active());
    CHECK(f.io.values[A][idx(BorderSlot::ActiveColor)] == "0xa1");
    CHECK(f.io.values[B][idx(BorderSlot::ActiveColor)] == "0xb1");
    CHECK(!any_highlight_left(f.io));
}

TEST(t_ui_05c_unload_restores_all_no_stuck) {
    Fixture f;
    seed(f.io, A, "0xa1", "0xa2");
    seed(f.io, B, "0xb1", "0xb2");
    f.candidates({ref(A), ref(B)});

    CHECK(f.sc.cycle(Direction::Next).ok);
    CHECK(f.sc.cycle(Direction::Next).ok);
    f.sc.plugin_shutdown(); // PLUGIN_EXIT path (REQ-UI-005)
    CHECK(!f.sc.is_active());
    CHECK(f.io.values[A][idx(BorderSlot::ActiveColor)] == "0xa1");
    CHECK(f.io.values[B][idx(BorderSlot::ActiveColor)] == "0xb1");
    CHECK(!any_highlight_left(f.io));
}

// REQ-UI-008 / R0 F3+F10: border_size >= 0 overrides the size for the highlighted
// window; with no readable prior size (empty getprop) restore drops the SET_PROP
// override via `unset`, while a readable prior size is restored by exact value
// (see t_ui_010_size_restored_by_value_when_readable). Colours are always restored
// by captured value (never `unset`/`-1` on a colour — the detector below only
// scans colour slots).
TEST(t_ui_05d_size_override_restored_via_unset) {
    FakeBorderPropIo io;
    seed(io, A, "0xa1", "0xa2");
    const auto valid = [](const WindowRef &) { return true; };
    BorderHighlightUI ui(io, valid, BorderStyle::Solid, kHighlight, 2); // size >= 0
    const Snapshot snap({ref(A)}, Scope::Global);

    ui.on_session_start(snap, 0);
    bool size_write_configured = false;
    for (const auto &[address, slot, value] : io.writes)
        if (slot == BorderSlot::Size && address == A && value == "2")
            size_write_configured = true;
    CHECK(size_write_configured); // (a) the configured size was written

    ui.on_session_end(UIEndReason::Applied);
    bool size_write_unset = false;
    for (const auto &[address, slot, value] : io.writes)
        if (slot == BorderSlot::Size && address == A && value == "unset")
            size_write_unset = true;
    CHECK(size_write_unset); // (b) restore drops the SET_PROP override via `unset`

    CHECK(io.values[A][idx(BorderSlot::ActiveColor)] == "0xa1");   // (c) colours restored
    CHECK(io.values[A][idx(BorderSlot::InactiveColor)] == "0xa2"); //     by captured value
    CHECK(!color_slot_write_is_bare_clear(io));                    // (d) Size `unset` must not trip the colour detector
    CHECK(!any_highlight_left(io));
}

// --- T-UI-06: invalid WindowRef skipped, session continues ----------------------
TEST(t_ui_06_invalid_ref_skipped_session_continues) {
    FakeBorderPropIo io;
    seed(io, A, "0xa1", "0xa2");
    const auto valid = [](const WindowRef &r) { return r.address != B; }; // B invalid (REQ-UI-010)
    BorderHighlightUI ui(io, valid, BorderStyle::Solid, kHighlight, -1);
    const Snapshot snap({ref(A), ref(B)}, Scope::Global);

    ui.on_session_start(snap, 1); // selects invalid B -> skip, no crash
    EQ(io.set_calls, 0);
    CHECK(io.values[B][idx(BorderSlot::ActiveColor)] == "");

    ui.on_selection_changed(0); // session continues; valid A highlighted
    CHECK(io.values[A][idx(BorderSlot::ActiveColor)] == kHighlight);
    ui.on_session_end(UIEndReason::Cancelled);
    CHECK(io.values[A][idx(BorderSlot::ActiveColor)] == "0xa1");
}

// --- REQ-UI-002: runtime probe degrades to null for the session ---------------
TEST(t_ui_002_runtime_probe_degrades_no_writes) {
    FakeBorderPropIo io;
    io.fail_gets = true; // border API unavailable at session start
    int warnings = 0;
    const auto valid = [](const WindowRef &) { return true; };
    BorderHighlightUI ui(io, valid, BorderStyle::Solid, kHighlight, -1, [&](std::string_view) { ++warnings; });
    const Snapshot snap({ref(A), ref(B)}, Scope::Global);

    ui.on_session_start(snap, 0);
    ui.on_selection_changed(1);
    ui.on_session_end(UIEndReason::Applied);

    EQ(io.set_calls, 0); // degraded to null: no border writes at all
    EQ(warnings, 1);     // exactly one warn-once
    CHECK(!color_slot_write_is_bare_clear(io));
}

// REQ-UI-002: the degrade decision is per session; the instance-level warn latch
// keeps the warning count stable for one backend instance (cross-session dedupe
// lives in the plugin's warn sink).
TEST(t_ui_002_degrade_resets_next_session) {
    FakeBorderPropIo io;
    seed(io, A, "0xa1", "0xa2");
    io.fail_gets = true;
    int warnings = 0;
    const auto valid = [](const WindowRef &) { return true; };
    BorderHighlightUI ui(io, valid, BorderStyle::Solid, kHighlight, -1, [&](std::string_view) { ++warnings; });
    const Snapshot snap({ref(A)}, Scope::Global);

    ui.on_session_start(snap, 0); // API down -> no writes
    EQ(io.set_calls, 0);
    ui.on_session_end(UIEndReason::Cancelled);

    io.fail_gets = false;         // API available again
    ui.on_session_start(snap, 0); // recovered for THIS session
    CHECK(io.values[A][idx(BorderSlot::ActiveColor)] == kHighlight);
    ui.on_session_end(UIEndReason::Applied);
    CHECK(io.values[A][idx(BorderSlot::ActiveColor)] == "0xa1");
    EQ(warnings, 1); // instance-latch: this instance already warned on the degraded session
}

// --- REQ-UI-005 / R0 F10: restore safety over a fake BorderPropIo --------------
// get-failure off the probe path: either colour capture fails -> skip, no override.
TEST(t_ui_01_partial_capture_skips_window) {
    FakeBorderPropIo io;
    seed(io, A, "0xa1", "0xa2");
    io.fail_get_inactive = true; // probe (active) passes, inactive read fails
    int warnings = 0;
    const auto valid = [](const WindowRef &) { return true; };
    BorderHighlightUI ui(io, valid, BorderStyle::Solid, kHighlight, -1, [&](std::string_view) { ++warnings; });
    const Snapshot snap({ref(A)}, Scope::Global);

    ui.on_session_start(snap, 0);
    EQ(io.set_calls, 0); // no override without BOTH prior colour values
    EQ(io.values[A][idx(BorderSlot::ActiveColor)], "0xa1");
    EQ(io.values[A][idx(BorderSlot::InactiveColor)], "0xa2");
    EQ(warnings, 1);
    CHECK(!color_slot_write_is_bare_clear(io));
}

// set-failure: not "applied" -> restore is a no-op and no bare clear is written.
TEST(t_ui_01_set_failure_not_applied_and_no_bare_clear) {
    FakeBorderPropIo io;
    seed(io, A, "0xa1", "0xa2");
    io.fail_sets = true;
    int warnings = 0;
    const auto valid = [](const WindowRef &) { return true; };
    BorderHighlightUI ui(io, valid, BorderStyle::Solid, kHighlight, -1, [&](std::string_view) { ++warnings; });
    const Snapshot snap({ref(A)}, Scope::Global);

    ui.on_session_start(snap, 0);
    ui.on_selection_changed(0);
    ui.on_session_end(UIEndReason::Applied);

    CHECK(io.set_calls > 0);                                // attempts happened
    CHECK(!color_slot_write_is_bare_clear(io));             // but never -1/unset on a colour
    EQ(io.values[A][idx(BorderSlot::ActiveColor)], "0xa1"); // untouched
    EQ(warnings, 1);                                        // warn-once
}

// --- REQ-UI-001: io exceptions are isolated, never thrown ----------------------
TEST(t_ui_01_io_exception_is_swallowed) {
    FakeBorderPropIo io;
    io.throw_gets = true;
    io.throw_sets = true;
    int warnings = 0;
    const auto valid = [](const WindowRef &) { return true; };
    BorderHighlightUI ui(io, valid, BorderStyle::Solid, kHighlight, -1, [&](std::string_view) { ++warnings; });
    const Snapshot snap({ref(A)}, Scope::Global);

    ui.on_session_start(snap, 0); // throwing io must not abort the session
    ui.on_selection_changed(0);
    ui.on_session_end(UIEndReason::Cancelled);
    CHECK(!color_slot_write_is_bare_clear(io));
}

TEST(t_ui_01_warn_sink_throw_is_swallowed) {
    FakeBorderPropIo io;
    seed(io, A, "0xa1", "0xa2");
    io.fail_sets = true;
    const auto valid = [](const WindowRef &) { return true; };
    BorderHighlightUI ui(io, valid, BorderStyle::Solid, kHighlight, -1,
                         [](std::string_view) { throw std::runtime_error("warn sink"); });
    const Snapshot snap({ref(A)}, Scope::Global);

    ui.on_session_start(snap, 0); // a broken warn sink must not escape either
    CHECK(io.set_calls > 0);
    CHECK(!color_slot_write_is_bare_clear(io));
}

// --- M4-S3 D2: captured values must be normalized to the setprop grammar -------
// getprop answers "<hex> <N>deg" but setprop only accepts 0x.../rgb()/rgba()
// WITHOUT the suffix; writing the raw echo back yields an empty gradient =
// invisible border (REQ-UI-004 exact restore, REQ-UI-005 no stuck borders).
TEST(t_ui_010_capture_normalized_to_setprop_grammar) {
    FakeBorderPropIo io;
    seed_raw(io, A, BorderSlot::ActiveColor, "ff44cc88 0deg");
    seed_raw(io, A, BorderSlot::InactiveColor, "ff44ccff 10deg");
    int warnings = 0;
    const auto valid = [](const WindowRef &) { return true; };
    BorderHighlightUI ui(io, valid, BorderStyle::Solid, kHighlight, -1, [&](std::string_view) { ++warnings; });
    const Snapshot snap({ref(A)}, Scope::Global);

    ui.on_session_start(snap, 0);
    CHECK(io.values[A][idx(BorderSlot::ActiveColor)] == kHighlight);
    EQ(warnings, 0); // live-format replies are restorable -> no skip warning

    ui.on_session_end(UIEndReason::Cancelled);
    bool restore_active = false, restore_inactive = false, bare_clear = false;
    for (const auto &[address, slot, value] : io.writes) {
        if (address != A)
            continue;
        if (slot == BorderSlot::ActiveColor && value == "0xff44cc88")
            restore_active = true; // EXACTLY: suffix stripped, 0x prepended
        if (slot == BorderSlot::InactiveColor && value == "0xff44ccff")
            restore_inactive = true;
        if ((slot == BorderSlot::ActiveColor || slot == BorderSlot::InactiveColor) &&
            (value == "-1" || value == "unset"))
            bare_clear = true;
    }
    CHECK(restore_active);
    CHECK(restore_inactive);
    CHECK(!bare_clear);
    CHECK(io.values[A][idx(BorderSlot::ActiveColor)] == "0xff44cc88"); // applied by the fake
    CHECK(io.values[A][idx(BorderSlot::InactiveColor)] == "0xff44ccff");
}

TEST(t_ui_010_rgb_capture_round_trips) {
    FakeBorderPropIo io;
    seed_raw(io, A, BorderSlot::ActiveColor, "rgb(44cc88)");
    seed_raw(io, A, BorderSlot::InactiveColor, "rgba(44,cc,88,ff)");
    const auto valid = [](const WindowRef &) { return true; };
    BorderHighlightUI ui(io, valid, BorderStyle::Solid, kHighlight, -1);
    const Snapshot snap({ref(A)}, Scope::Global);

    ui.on_session_start(snap, 0);
    CHECK(io.values[A][idx(BorderSlot::ActiveColor)] == kHighlight);

    ui.on_session_end(UIEndReason::Applied);
    bool restore_active = false, restore_inactive = false;
    for (const auto &[address, slot, value] : io.writes) {
        if (address != A)
            continue;
        if (slot == BorderSlot::ActiveColor && value == "rgb(44cc88)")
            restore_active = true; // verbatim: rgb()/rgba() pass through, no 0x added
        if (slot == BorderSlot::InactiveColor && value == "rgba(44,cc,88,ff)")
            restore_inactive = true;
    }
    CHECK(restore_active);
    CHECK(restore_inactive);
    CHECK(!any_highlight_left(io));
}

TEST(t_ui_010_garbage_capture_skips_highlight) {
    FakeBorderPropIo io;
    seed_raw(io, A, BorderSlot::ActiveColor, "garbage");
    seed_raw(io, A, BorderSlot::InactiveColor, "0xa2");
    int warnings = 0;
    const auto valid = [](const WindowRef &) { return true; };
    BorderHighlightUI ui(io, valid, BorderStyle::Solid, kHighlight, -1, [&](std::string_view) { ++warnings; });
    const Snapshot snap({ref(A)}, Scope::Global);

    ui.on_session_start(snap, 0); // normalized capture empty -> fail-soft skip
    EQ(io.set_calls, 0);
    EQ(warnings, 1);
    CHECK(io.values[A][idx(BorderSlot::ActiveColor)] == "garbage"); // live value untouched
    CHECK(!color_slot_write_is_bare_clear(io));

    ui.on_session_end(UIEndReason::Applied);
    EQ(io.set_calls, 0); // nothing was applied -> nothing to restore, no bare clear
}

TEST(t_ui_010_size_restored_by_value_when_readable) {
    FakeBorderPropIo io;
    seed(io, A, "0xa1", "0xa2");
    seed_raw(io, A, BorderSlot::Size, "2"); // border_size getprop IS available
    const auto valid = [](const WindowRef &) { return true; };
    BorderHighlightUI ui(io, valid, BorderStyle::Solid, kHighlight, 4);
    const Snapshot snap({ref(A)}, Scope::Global);

    ui.on_session_start(snap, 0);
    bool size_write_configured = false;
    for (const auto &[address, slot, value] : io.writes)
        if (slot == BorderSlot::Size && address == A && value == "4")
            size_write_configured = true;
    CHECK(size_write_configured);

    ui.on_session_end(UIEndReason::Applied);
    bool size_restore_by_value = false, size_unset = false;
    for (const auto &[address, slot, value] : io.writes) {
        if (slot != BorderSlot::Size || address != A)
            continue;
        if (value == "2")
            size_restore_by_value = true; // exact prior integer, not "unset"
        if (value == "unset")
            size_unset = true;
    }
    CHECK(size_restore_by_value);
    CHECK(!size_unset);
    CHECK(io.values[A][idx(BorderSlot::Size)] == "2");
}

TEST(t_ui_010_size_falls_back_to_unset_when_read_fails) {
    FakeBorderPropIo io;
    seed(io, A, "0xa1", "0xa2"); // no size seed: getprop empty/unavailable
    const auto valid = [](const WindowRef &) { return true; };
    BorderHighlightUI ui(io, valid, BorderStyle::Solid, kHighlight, 3);
    const Snapshot snap({ref(A)}, Scope::Global);

    ui.on_session_start(snap, 0);
    bool size_write_configured = false;
    for (const auto &[address, slot, value] : io.writes)
        if (slot == BorderSlot::Size && address == A && value == "3")
            size_write_configured = true;
    CHECK(size_write_configured);

    ui.on_session_end(UIEndReason::Applied);
    bool size_unset = false, size_zero_restore = false;
    for (const auto &[address, slot, value] : io.writes) {
        if (slot != BorderSlot::Size || address != A)
            continue;
        if (value == "unset")
            size_unset = true; // only remaining way to drop the override
        if (value == "0")
            size_zero_restore = true; // "0" would render a borderless window
    }
    CHECK(size_unset);
    CHECK(!size_zero_restore);
    CHECK(!color_slot_write_is_bare_clear(io));
}

} // namespace

int main() {
    return mru::test::run_all();
}
