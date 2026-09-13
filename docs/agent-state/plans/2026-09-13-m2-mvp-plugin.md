# M2 — Hyprland MVP Plugin Implementation Plan

> **For agentic workers:** use the orchestrator flow (AGENTS.md §4): each Task is delivered by an implementer subagent, reviewed against SPEC, then merged. Steps use checkbox (`- [ ]`) syntax.

**Goal:** First loadable `mru-switcher.so` with real focus switching, Null UI only, on a pinned Hyprland master commit.

**Architecture:** ports-and-adapters (ADR-007). Domain (`mru_domain`) stays Hyprland-free (already merged M1). This milestone adds `adapters/` (HyprlandWindowSource, HyprlandFocusGateway, HyprlandSchedulerPort, NullUI, WindowIdentityRegistry) and a thin facade (`PLUGIN_INIT`/`PLUGIN_EXIT` with hash check, dispatchers, Event::bus subscriptions). Pure logic that must be unit-testable lives in domain-adjacent modules under `src/plugin/` that do **not** include Hyprland headers (`config_value.hpp`, `dispatch_args.hpp`); everything that touches `PHLWINDOW`/globals stays in `.cpp` files compiled only when `MRU_BUILD_PLUGIN=ON`.

**Tech Stack:** C++23, CMake, Hyprland master (pinned), Event::bus, `addDispatcherV2`, hyprlang config values.

**Spec:** `docs/SPEC.md` §3 (dispatchers), §4 (config), §5 (UI), REQ-DISP-001/002, REQ-CFG-001-004, REQ-UI-001-003, REQ-S-011; `docs/ARCHITECTURE.md` §8-11; `docs/COMPAT.md`.

## Global Constraints

- Native plugins are **C++ only** (Align with `-std=c++23`; same compiler family as pinned Hyprland).
- **Hash check in `PLUGIN_INIT`**: `__hyprland_api_get_hash()` vs `__hyprland_api_get_client_hash()`; abort load on mismatch.
- Config values registered **only in `PLUGIN_INIT`**, keys under `plugin:mru-switcher:` (REQ-CFG keys table in SPEC §4).
- Dispatchers via **`addDispatcherV2`**, prefixed names `mru:*`, return `SDispatchResult`.
- Events via **`Event::bus()`**; keep listener objects alive.
- No compositor-touching threads (Wayland loop is single-threaded).
- Domain headers must stay free of Hyprland types (`PHLWINDOW`, `g_p*`).
- `ui = null` only; other backends fall back to `null` (REQ-UI-002/003).
- `default_scope = global` only in M2; other scopes return `no windows` (from M3).
- Plugin binary output `build/mru-switcher.so` (hyprpm).

---

- [ ] **Baseline:** `git fetch origin && git checkout main && git pull --ff-only origin main`

Task 0 in this plan is the environment; it is the **only** task a human must run. All other tasks run inside `feat/m2-plugin`.

---

### Task 1: Environment — install and build pinned Hyprland (HUMAN)

**Reason:** the machine needs Hyprland headers + a hash-identical binary before the plugin can be compiled or nested-smoked. `paru` needs sudo (interactive), which the orchestrator cannot provide from a non-interactive shell.

**Files:**
- Modify: `docs/COMPAT.md` (fill the first matrix row real values)
- Modify: `docs/VERSION-MAP.md` (0.2.0 row: pin, tag, notes)
- Modify: `CHANGELOG.md` (Unreleased → Added M2)

**Interfaces:** produces the pinned Hyprland build used by every later compile/smoke task.

- [ ] **Step 1: Human installs `hyprland-git`**

Run in a terminal (or let the human run it):

```bash
paru -S hyprland-git --noconfirm
```

Or, if the human declines a system install, the orchestrator builds Hyprland from `/tmp/hyprland-src` and installs headers via the documented path in `docs/COMPAT.md` §"Minimal build recipe".

- [ ] **Step 2: Record the exact pinned commit in COMPAT.md**

```bash
git -C <hyprland-src> rev-parse HEAD
# current: c31b90c5fc87b6bfc494f4e66d5acc3b0ba5b0ad
```

Fill into `docs/COMPAT.md` matrix:

| Plugin version | Hyprland commit | Hyprland tag | CI/nest tested | Notes |
|----------------|-----------------|--------------|----------------|-------|
| 0.2.0 (planned) | `c31b90c5fc87b6bfc494f4e66d5acc3b0ba5b0ad` | master | pending (Task 7) | First `.so` |

- [ ] **Step 3: Verify headers available**

```bash
ls /usr/include/hyprland/plugins/PluginAPI.hpp  # or the install path used
```

Expected: file exists.

- [ ] **Step 4: Branch**

```bash
git checkout -b feat/m2-plugin
```

- [ ] **Step 5: Commit docs**

```bash
git add docs/COMPAT.md docs/VERSION-MAP.md CHANGELOG.md
git commit -m "docs(compat): pin Hyprland for M2 first .so"
```

---

### Task 2: Config value model (pure, unit-tested)

**Files:**
- Create: `src/plugin/config_value.hpp`
- Create: `src/plugin/config_value.cpp`
- Create: `tests/plugin/test_config_value.cpp`
- Modify: `CMakeLists.txt` (add `mru_plugin_core` static lib + `plugin_config_value` test, no Hyprland includes)
- Modify: `docs/agent-state/PROGRESS.md` (M2 checkbox)

**Interfaces:**
- Consumes: nothing (domain `mru/domain/scope.hpp` for `Scope`).
- Produces:
  - `struct PluginConfig { int debounce_ms; Scope default_scope; bool wrap; bool lock_history_on_session; bool restore_focus_on_cancel; bool ui_null; bool ui_border; bool ui_external; }` with a canonical `ui` string.
  - `PluginConfig default_plugin_config();`
  - `Scope parse_scope(std::string_view s);`
  - `int clamp_debounce_ms(int raw);`
  - `struct ParsedUi { enum Kind { Null, Border, External } kind; bool matched; }` ; `ParsedUi parse_ui_backend(std::string_view s);`

**Global constraint:** this module compiles in the **test-only** and **no-plugin** builds too, so it may NOT include any Hyprland header.

- [ ] **Step 1: Write the failing test** `tests/plugin/test_config_value.cpp`

```cpp
#include <cassert>

#include "config_value.hpp"

int main() {
  using mru::domain::Scope;

  // REQ-CFG-004: debounce clamped into [0,5000]
  assert(clamp_debounce_ms(-1) == 0);
  assert(clamp_debounce_ms(0) == 0);
  assert(clamp_debounce_ms(400) == 400);
  assert(clamp_debounce_ms(5000) == 5000);
  assert(clamp_debounce_ms(9999) == 5000);

  // REQ-CFG-001: unknown scope falls back to default; known ones parse
  assert(parse_scope("global") == Scope::Global);
  assert(parse_scope("monitor") == Scope::Monitor);
  assert(parse_scope("workspace") == Scope::Workspace);
  assert(parse_scope("visible") == Scope::Visible);
  assert(parse_scope("app") == Scope::App);
  assert(parse_scope("bogus") == Scope::Global);

  // REQ-UI-002/003: null matches; border/external match but not implemented in M2
  assert(parse_ui_backend("null").matched);
  assert(parse_ui_backend("null").kind == ParsedUi::Kind::Null);
  assert(parse_ui_backend("border").matched);
  assert(parse_ui_backend("border").kind == ParsedUi::Kind::Border);
  assert(parse_ui_backend("external").matched);
  assert(parse_ui_backend("nope").kind == ParsedUi::Kind::Null);
  assert(!parse_ui_backend("nope").matched);

  auto cfg = default_plugin_config();
  assert(cfg.debounce_ms == 400);
  assert(cfg.default_scope == Scope::Global);
  assert(cfg.wrap);
  assert(cfg.lock_history_on_session);
  assert(!cfg.restore_focus_on_cancel);
  assert(cfg.ui_null);
  assert(!cfg.ui_border);
  assert(!cfg.ui_external);

  return 0;
}
```

- [ ] **Step 2: Run — expect FAIL (no such header)**

```bash
cmake -S . -B build -DMRU_BUILD_PLUGIN=OFF
cmake --build build --target plugin_config_value 2>&1 | grep -q 'No such file\|fatal error' && echo FAIL
```

- [ ] **Step 3: Implement `config_value.hpp`**

```cpp
#pragma once

#include <string_view>

#include "mru/domain/scope.hpp"

namespace mru::plugin {

struct PluginConfig {
  int                              debounce_ms = 400;             // clamp [0,5000] (REQ-CFG-004)
  mru::domain::Scope               default_scope = mru::domain::Scope::Global;
  bool                             wrap = true;
  bool                             lock_history_on_session = true;
  bool                             restore_focus_on_cancel = false;
  bool                             ui_null = true;    // REQ-UI-003: solely null in M2
  bool                             ui_border = false; // parsed, falls back to null (REQ-UI-002)
  bool                             ui_external = false;
  bool                             ui_matched = true;
};

PluginConfig default_plugin_config();
mru::domain::Scope parse_scope(std::string_view s);
int clamp_debounce_ms(int raw);

struct ParsedUi {
  enum class Kind { Null, Border, External };
  Kind  kind = Kind::Null;
  bool  matched = false;
};
ParsedUi parse_ui_backend(std::string_view s);

} // namespace mru::plugin
```

- [ ] **Step 4: Implement `config_value.cpp`**

```cpp
#include "config_value.hpp"

#include <algorithm>
#include <string_view>

namespace mru::plugin {

PluginConfig default_plugin_config() {
  return {};
}

mru::domain::Scope parse_scope(std::string_view s) {
  using mru::domain::Scope;
  if (s == "monitor") return Scope::Monitor;
  if (s == "workspace") return Scope::Workspace;
  if (s == "visible") return Scope::Visible;
  if (s == "app") return Scope::App;
  return Scope::Global; // includes "global"; fallback for unknown (REQ-CFG-001)
}

int clamp_debounce_ms(int raw) {
  return std::clamp(raw, 0, 5000); // REQ-CFG-004
}

ParsedUi parse_ui_backend(std::string_view s) {
  if (s == "null") return {ParsedUi::Kind::Null, true};
  if (s == "border") return {ParsedUi::Kind::Border, true};
  if (s == "external") return {ParsedUi::Kind::External, true};
  return {ParsedUi::Kind::Null, false}; // REQ-CFG-001 fallback to default
}

} // namespace mru::plugin
```

- [ ] **Step 5: Wire CMake**

Add to `CMakeLists.txt` (outside `MRU_BUILD_PLUGIN`, test-only friendly):

```cmake
# --- Plugin pure helpers (no Hyprland headers; unit-testable) ---
add_library(mru_plugin_core STATIC src/plugin/config_value.cpp)
target_include_directories(mru_plugin_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/src/plugin)
target_link_libraries(mru_plugin_core PUBLIC mru_domain)
mru_warnings(mru_plugin_core)

if(MRU_BUILD_TESTS)
  add_executable(plugin_config_value tests/plugin/test_config_value.cpp)
  target_link_libraries(plugin_config_value PRIVATE mru_plugin_core)
  mru_warnings(plugin_config_value)
  add_test(NAME plugin_config_value COMMAND plugin_config_value)
endif()
```

- [ ] **Step 6: Run — expect PASS**

```bash
cmake --build build --target plugin_config_value && ctest --test-dir build -R plugin_config_value
```

- [ ] **Step 7: Commit**

```bash
git add src/plugin/config_value.hpp src/plugin/config_value.cpp tests/plugin/test_config_value.cpp CMakeLists.txt docs/agent-state/PROGRESS.md
git commit -m "feat(plugin): pure config value model with clamped debounce (REQ-CFG-001/004, REQ-UI-002/003)"
```

---

### Task 3: Dispatcher argument parser (pure, unit-tested)

**Files:**
- Create: `src/plugin/dispatch_args.hpp`
- Create: `src/plugin/dispatch_args.cpp`
- Create: `tests/plugin/test_dispatch_args.cpp`
- Modify: `CMakeLists.txt` (add to `mru_plugin_core`, add test)

**Interfaces:**
- Consumes: `mru/domain/scope.hpp`.
- Produces:
  - `struct CycleArgs { mru::domain::Direction dir; std::optional<mru::domain::Scope> scope; bool ok; std::string error; };`
  - `CycleArgs parse_cycle_args(std::string_view args);` — grammar `mru:cycle [next|prev] [global|monitor|workspace|visible|app]`; omitted direction → `next` (REQ-DISP-001); invalid token → `{ok=false, error="..."}`.

- [ ] **Step 1: failing test** `tests/plugin/test_dispatch_args.cpp`

```cpp
#include <cassert>
#include <string_view>

#include "dispatch_args.hpp"

int main() {
  using mru::domain::Direction;
  using mru::domain::Scope;

  // REQ-DISP-001: omitted direction -> next
  auto a = parse_cycle_args("");
  assert(a.ok && a.dir == Direction::Next && !a.scope);

  auto b = parse_cycle_args("next");
  assert(b.ok && b.dir == Direction::Next && !b.scope);

  auto c = parse_cycle_args("prev");
  assert(c.ok && c.dir == Direction::Prev && !c.scope);

  auto d = parse_cycle_args("next workspace");
  assert(d.ok && d.scope && *d.scope == Scope::Workspace);

  auto e = parse_cycle_args("global");
  assert(e.ok && e.dir == Direction::Next && !e.scope); // scope alone invalid

  auto f = parse_cycle_args("nonsense");
  assert(!f.ok && !f.error.empty());

  auto g = parse_cycle_args("next bogus");
  assert(!g.ok && !g.error.empty());

  return 0;
}
```

- [ ] **Step 2: Run — expect FAIL**

```bash
cmake --build build --target plugin_dispatch_args 2>&1 | grep -q 'fatal error' && echo FAIL
```

- [ ] **Step 3: Implement `dispatch_args.hpp`**

```cpp
#pragma once

#include <optional>
#include <string>
#include <string_view>

#include "mru/domain/scope.hpp"

namespace mru::plugin {

struct CycleArgs {
  mru::domain::Direction              dir = mru::domain::Direction::Next;
  std::optional<mru::domain::Scope>   scope;
  bool                                ok = false;
  std::string                         error;
};

// Grammar: mru:cycle [next|prev] [global|monitor|workspace|visible|app]
// Direction omitted -> next (REQ-DISP-001). Scope without direction is invalid.
CycleArgs parse_cycle_args(std::string_view args);

} // namespace mru::plugin
```

- [ ] **Step 4: Implement `dispatch_args.cpp`**

```cpp
#include "dispatch_args.hpp"

#include <sstream>
#include <vector>

namespace mru::plugin {

using mru::domain::Direction;

static std::vector<std::string> tokens_of(std::string_view args) {
  std::istringstream in{std::string(args)};
  std::vector<std::string> out;
  for (std::string t; in >> t;)
    out.push_back(t);
  return out;
}

static std::optional<Direction> dir_of(std::string_view s) {
  if (s == "next") return Direction::Next;
  if (s == "prev") return Direction::Prev;
  return std::nullopt;
}

CycleArgs parse_cycle_args(std::string_view args) {
  const auto toks = tokens_of(args);
  CycleArgs out;
  out.ok = true;

  std::size_t i = 0;
  if (i < toks.size()) {
    if (const auto d = dir_of(toks[i])) {
      out.dir = *d;
      ++i;
    }
    // if first token is a scope (gap left open for scripts) treat as invalid:
    else if (mru::domain::parse_scope(toks[i]) != mru::domain::Scope::Global ||
             toks[i] != "global") {
      out.ok = false;
      out.error = "unknown direction: " + toks[i];
      return out;
    }
  }

  if (i < toks.size()) {
    const std::string_view sc = toks[i];
    const mru::domain::Scope parsed = mru::domain::parse_scope(sc);
    if (sc == "global" || sc == "monitor" || sc == "workspace" || sc == "visible" || sc == "app") {
      out.scope = parsed;
      ++i;
    } else {
      out.ok = false;
      out.error = "unknown scope: " + toks[i];
      return out;
    }
  }

  if (i < toks.size()) {
    out.ok = false;
    out.error = "too many arguments";
    return out;
  }
  return out;
}

} // namespace mru::plugin
```

- [ ] **Step 5: Run — expect PASS**

```bash
cmake --build build --target plugin_dispatch_args && ctest --test-dir build -R plugin_dispatch_args
```

- [ ] **Step 6: Commit**

```bash
git add src/plugin/dispatch_args.hpp src/plugin/dispatch_args.cpp tests/plugin/test_dispatch_args.cpp CMakeLists.txt
git commit -m "feat(plugin): mru:cycle args parser (REQ-DISP-001)"
```

---

### Task 4: WindowIdentityRegistry (generation bookkeeping)

**Files:**
- Create: `src/plugin/hypr/identity_registry.hpp`
- Create: `src/plugin/hypr/identity_registry.cpp`
- Modify: `CMakeLists.txt` (plugin-only target `mru_plugin_hypr` — compiled only when `MRU_BUILD_PLUGIN=ON`)

**Interfaces:**
- Consumes: `mru/domain/window_ref.hpp`.
- Produces:
  - `class WindowIdentityRegistry { public: WindowRef register_window(PHLWINDOW w); bool contains(const WindowRef&) const; std::optional<PHLWINDOW> resolve(const WindowRef&) const; void on_window_close(PHLWINDOW w); bool is_known(void* addr); };`
  - `std::uint64_t address_of(PHLWINDOW w);` (raw pointer value).
  - `WindowRef` uses `{address, generation}`; a fresh `generation` is assigned per registration (ADR-013).

**Constraint:** this file includes `hyprland/src/...` headers; it is compiled ONLY under `MRU_BUILD_PLUGIN`. Generation bookkeeping must be deterministic and testable; the heavy lookup stays thin over `std::unordered_map`.

- [ ] **Step 1: create header**

```cpp
#pragma once

#include <cstdint>
#include <optional>
#include <unordered_map>
#include <vector>

#include "mru/domain/window_ref.hpp"

class CWindow;
using PHLWINDOW = WP<CWindow>;

namespace mru::plugin {

std::uint64_t address_of(PHLWINDOW w);

class WindowIdentityRegistry {
public:
  WindowRef register_window(PHLWINDOW w);           // increments generation per (re)registration
  std::optional<WindowRef> last_ref(PHLWINDOW w) const;
  std::optional<PHLWINDOW> resolve(const WindowRef& ref) const;
  void  on_window_close(PHLWINDOW w);               // marks entry invalid for future resolve
  bool  is_known(std::uint64_t addr) const;

private:
  struct Entry {
    WindowRef          ref;
    std::uint64_t      next_generation = 1;
    bool               closed = false;
  };
  std::unordered_map<std::uint64_t, Entry> by_address_;
};

} // namespace mru::plugin
```

Include `"hyprland.hpp"` at the top of the header if the build uses a single grouped include; otherwise include `src/.../Window.hpp` and `src/.../DesktopTypes.hpp` in the `.cpp` only. Keep the non-Hyprland parts (`by_address_`, generation logic) in the header so generation rules are visible.

- [ ] **Step 2: implement `.cpp`**

```cpp
#include "identity_registry.hpp"

#include <hyprland/src/desktop/view/window/Window.hpp>

namespace mru::plugin {

std::uint64_t address_of(PHLWINDOW w) {
  return reinterpret_cast<std::uint64_t>(w.get()); // no id field on master; use object address
}

WindowRef WindowIdentityRegistry::register_window(PHLWINDOW w) {
  const std::uint64_t addr = address_of(w);
  auto& entry = by_address_[addr];
  if (entry.ref.address == addr && !entry.closed) {
    // same live window: keep identity stable (no generation bump)
    return entry.ref;
  }
  // new generation for new/re-opened window at same address (ADR-013)
  entry.ref = WindowRef{addr, entry.next_generation++};
  entry.closed = false;
  return entry.ref;
}

std::optional<WindowRef> WindowIdentityRegistry::last_ref(PHLWINDOW w) const {
  const auto it = by_address_.find(address_of(w));
  if (it == by_address_.end())
    return std::nullopt;
  return it->second.ref;
}

std::optional<PHLWINDOW> WindowIdentityRegistry::resolve(const WindowRef& ref) const {
  const auto it = by_address_.find(ref.address);
  if (it == by_address_.end() || it->second.closed || it->second.ref.generation != ref.generation)
    return std::nullopt; // invalid identity (REQ-F-005)
  return it->second.window;
}

void WindowIdentityRegistry::on_window_close(PHLWINDOW w) {
  const auto it = by_address_.find(address_of(w));
  if (it != by_address_.end())
    it->second.closed = true;
}

bool WindowIdentityRegistry::is_known(std::uint64_t addr) const {
  return by_address_.contains(addr);
}

} // namespace mru::plugin
```

Note: `PHLWINDOW` is internally `WP<CWindow>`; storing it for resolve. Generation bookkeeping is deterministic and kept in the header for auditability.

- [ ] **Step 3: CMake plugin target** (guarded)

```cmake
if(MRU_BUILD_PLUGIN)
  # full Hyprland source include path set by the environment (Task 1)
  add_library(mru_plugin_hypr STATIC src/plugin/hypr/identity_registry.cpp)
  target_include_directories(mru_plugin_hypr PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/src/plugin ${HYPRLAND_INCLUDE_DIRS})
  target_link_libraries(mru_plugin_hypr PUBLIC mru_plugin_core)
  mru_warnings(mru_plugin_hypr)
endif()
```

`HYPRLAND_INCLUDE_DIRS` is defined in a new `cmake/Hyprland.cmake` (Task 6) or passed `-DHYPRLAND_SRC=/path/to/hyprland` and derived there.

- [ ] **Step 4: Compile check**

```bash
cmake --build build --target mru_plugin_hypr
```

Expected: builds without errors against pinned headers.

- [ ] **Step 5: Commit**

```bash
git add src/plugin/hypr/identity_registry.hpp src/plugin/hypr/identity_registry.cpp CMakeLists.txt
git commit -m "feat(plugin): WindowIdentityRegistry with address+generation (ADR-013, REQ-F-005)"
```

---

### Task 5: Adapters — WindowSource, FocusGateway, SchedulerPort, NullUI

**Files:**
- Create: `src/plugin/hypr/hypr_window_source.hpp` / `.cpp`
- Create: `src/plugin/hypr/hypr_focus_gateway.hpp` / `.cpp`
- Create: `src/plugin/hypr/hypr_scheduler.hpp` / `.cpp`
- Create: `src/plugin/hypr/null_ui.hpp` / `.cpp` (trivial, may be inline)
- Modify: `CMakeLists.txt`

**Interfaces:**
- Consumes: `mru_domain` ports (`WindowSource`, `FocusGateway`, `SchedulerPort`, `UIPort`), `WindowIdentityRegistry`, `PluginConfig`.
- Produces (each concrete adapter):

```cpp
// hypr_window_source.hpp
class HyprlandWindowSource : public mru::domain::WindowSource {
public:
  HyprlandWindowSource(WindowIdentityRegistry& registry, const PluginConfig& cfg);
  std::vector<mru::domain::WindowRef> candidates(mru::domain::Scope) const override; // only Global in M2
  bool is_valid(const mru::domain::WindowRef&) const override;
  std::optional<mru::domain::WindowRef> focused() const override;
};

// hypr_focus_gateway.hpp
class HyprlandFocusGateway : public mru::domain::FocusGateway {
public:
  explicit HyprlandFocusGateway(WindowIdentityRegistry& registry);
  mru::domain::FocusResult focus(const mru::domain::WindowRef&) override;
};

// hypr_scheduler.hpp
class HyprlandSchedulerPort : public mru::domain::SchedulerPort {
public:
  mru::domain::JobId schedule_after(std::uint32_t delay_ms, std::function<void()> cb) override;
  void cancel(mru::domain::JobId id) override;
  ~HyprlandSchedulerPort() override;
private:
  std::unordered_map<mru::domain::JobId, SP<CEventLoopTimer>> timers_;
  mru::domain::JobId next_id_ = 1;
};

// null_ui.hpp (inline in header)
class NullUI : public mru::domain::UIPort {
  void on_session_start(const mru::domain::Snapshot&, std::size_t) override {}
  void on_selection_changed(std::size_t) override {}
  void on_session_end(mru::domain::UIEndReason) override {}
};
```

- [ ] **Step 1: HyprlandWindowSource** uses `Desktop::History::windowTracker()->fullHistory()` (old→new), iterates **reversed** (MRU-first), and includes only windows that are known + not closed in the registry; filtered by scope helper (M2: `Scope::Global` only; other scopes → empty). `focused()` returns registry ref for `Desktop::focusState()->window()`.

```cpp
#include "hypr_window_source.hpp"
#include <hyprland/src/desktop/history/WindowHistoryTracker.hpp>
#include <hyprland/src/desktop/DesktopTypes.hpp>
#include <hyprland/src/desktop/View.hpp> // focusState() as available on pin

std::vector<mru::domain::WindowRef> HyprlandWindowSource::candidates(mru::domain::Scope scope) const {
  if (scope != mru::domain::Scope::Global)
    return {}; // M2: global only; other scopes wired in M3
  std::vector<mru::domain::WindowRef> out;
  const auto& history = Desktop::History::windowTracker()->fullHistory();
  for (auto it = history.rbegin(); it != history.rend(); ++it) {
    const auto  w = it->lock();
    if (!w || !registry_.is_known(address_of(w)))
      continue;
    if (const auto ref = registry_.last_ref(w))
      out.push_back(*ref);
  }
  return out;
}

bool HyprlandWindowSource::is_valid(const mru::domain::WindowRef& ref) const {
  const auto w = registry_.resolve(ref); // address+generation+closed check
  return w.has_value();
}

std::optional<mru::domain::WindowRef> HyprlandWindowSource::focused() const {
  const auto w = Desktop::focusState()->window();
  if (!w) return std::nullopt;
  return registry_.last_ref(w);
}
```

- [ ] **Step 2: HyprlandFocusGateway** resolves the ref and calls the compositor focus API exactly once (ADR-006); never called from `mru:cycle`.

```cpp
#include "hypr_focus_gateway.hpp"
#include <hyprland/src/desktop/state/FocusState.hpp>

mru::domain::FocusResult HyprlandFocusGateway::focus(const mru::domain::WindowRef& ref) {
  const auto w = registry_.resolve(ref);
  if (!w)
    return mru::domain::FocusResult::InvalidTarget;
  Desktop::focusState()->fullWindowFocus(*w, Desktop::FOCUS_REASON_DISPATCH_FOCUSWINDOW);
  return mru::domain::FocusResult::Applied;
}
```

- [ ] **Step 3: HyprlandSchedulerPort** wraps `g_pEventLoopManager->addTimer(SP<CEventLoopTimer>)`. Note: `CEventLoopTimer` runs on the compositor thread (safe), and `cancel()` must remove/expire the timer so unload never fires into freed memory (M2 exit criterion: no use-after-unload).

```cpp
#include "hypr_scheduler.hpp"
#include <hyprland/src/managers/eventLoop/EventLoopManager.hpp>
#include <hyprland/src/managers/eventLoop/EventLoopTimer.hpp>

mru::domain::JobId HyprlandSchedulerPort::schedule_after(std::uint32_t delay_ms, std::function<void()> cb) {
  const mru::domain::JobId id = next_id_++;
  auto timer = makeShared<CEventLoopTimer>(std::chrono::milliseconds(delay_ms),
      [this, cb](SP<CEventLoopTimer> self, void* data) { cb(); },
      nullptr);
  timers_[id] = timer;
  g_pEventLoopManager->addTimer(timer);
  return id;
}

void HyprlandSchedulerPort::cancel(mru::domain::JobId id) {
  auto it = timers_.find(id);
  if (it == timers_.end()) return;
  it->second->cancel();                       // disarm; no callback on fire
  g_pEventLoopManager->removeTimer(it->second);
  timers_.erase(it);
}

HyprlandSchedulerPort::~HyprlandSchedulerPort() {
  for (auto& [id, t] : timers_) {
    t->cancel();
    g_pEventLoopManager->removeTimer(t);
  }
  timers_.clear();
}
```

- [ ] **Step 4: NullUI inline header** (Step 0 header) — no side effects (REQ-UI-001).

- [ ] **Step 5: Compile + verify domain boundary**

```bash
grep -rn 'PHLWINDOW\|g_p' src/plugin/config_value.* src/plugin/dispatch_args.* || true  # must print nothing
cmake --build build --target mru_plugin_hypr
```

Expected: no Hyprland types in pure modules; plugin target builds.

- [ ] **Step 6: Commit**

```bash
git add src/plugin/hypr/ CMakeLists.txt
git commit -m "feat(plugin): Hyprland adapters (WindowSource/FocusGateway/SchedulerPort) + NullUI (REQ-F-005, ADR-006/007)"
```

---

### Task 6: Facade — PLUGIN_INIT/EXIT, hash check, config, dispatchers, event wiring

**Files:**
- Create: `src/plugin/mru_plugin.cpp`
- Create: `src/plugin/mru_plugin.hpp`
- Create: `cmake/Hyprland.cmake` (`HYPRLAND_INCLUDE_DIRS` discovery)
- Modify: `CMakeLists.txt` (final `add_library(mru-switcher SHARED ...)` replacing the FATAL_ERROR)
- Modify: `hyprpm.toml` (build output already `build/mru-switcher.so`; fill authors if needed)

**Hyprland.cmake** discovers `HYPRLAND_SRC` (cache var) → sets `HYPRLAND_INCLUDE_DIRS` to `src`, plus dependency include dirs (hyprlang etc.) once installed by Task 1.

**Step 1: plugin header**

```cpp
#pragma once

#include <memory>

#include "mru/domain/fake_clock.hpp" // shared with SchedulerPort? no — facade uses real scheduler
#include "mru/domain/history_tracker.hpp"
#include "mru/domain/session_controller.hpp"
#include "hypr/hypr_window_source.hpp"
#include "hypr/hypr_focus_gateway.hpp"
#include "hypr/hypr_scheduler.hpp"
#include "hypr/null_ui.hpp"
#include "config_value.hpp"

namespace mru::plugin {

struct PluginState {
  std::optional<PluginConfig>                 config;
  std::unique_ptr<WindowIdentityRegistry>     registry;
  std::unique_ptr<HyprlandWindowSource>       source;
  std::unique_ptr<HyprlandFocusGateway>       fg;
  std::unique_ptr<HyprlandSchedulerPort>      scheduler;
  std::unique_ptr<NullUI>                     ui;
  std::unique_ptr<mru::domain::HistoryTracker> tracker;
  std::unique_ptr<mru::domain::SessionController> controller;
};
} // namespace mru::plugin
```

Note on `HistoryTracker` — it needs a debounce SchedulerPort. `HyprlandSchedulerPort` provides that. It must never outlive the scheduler. The facade keeps `scheduler_` above `tracker_` and destroys `tracker_` first in `PLUGIN_EXIT` (or relies on unique_ptr member order: declaration order = destruction order reversed; put `controller`/`tracker` declared before `scheduler`).

- [ ] **Step 2: `mru_plugin.cpp` — hash check + init**

```cpp
#include "mru_plugin.hpp"

#include <hyprland/src/plugins/PluginAPI.hpp>
#include <hyprland/src/plugins/PluginSystem.hpp>

HANDLE PHANDLE = nullptr;

APICALL EXPORT std::string PLUGIN_API_VERSION() { return HYPRLAND_API_VERSION; }

static bool hash_ok() {
  const char* h = __hyprland_api_get_hash();
  const char* c = __hyprland_api_get_client_hash();
  if (h && c && std::string_view(h) == std::string_view(c)) return true;
  HyprlandAPI::addNotification(PHANDLE, "mru-switcher: header hash mismatch, refusing to load", CHyprColor{1,0,0,1}, 5000);
  return false;
}

static bool register_config(PLUGIN_FULL_CFG_PTR /* or per key */) { /* register keys in Task 6 */ }

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle) {
  PHANDLE = handle;
  if (!hash_ok())
    return {}; // empty struct aborts init
  // … register config keys, build adapters + controller, subscribe, addDispatcherV2 …
  return {"mru-switcher", "Niri-style MRU Alt+Tab (snapshot, apply-on-release, lock-in)", "mru", "0.2.0"};
}

APICALL EXPORT void PLUGIN_EXIT() {
  // drop listeners first, then scheduler before tracker/controller (no use-after-unload)
  g_pPluginSystem->... // optional; Hyprland cleans up listeners on unload
  PHANDLE = nullptr;
}
```

- [ ] **Step 3: config registration** — read cached pointers after registering defaults (REQ-CFG-002). M2 uses the deprecated-but-present `getConfigValue` path (documented as such; migration to V2 in M3):

```cpp
static auto* const CFG_DEBOUNCE        = &HyprlandAPI::getConfigValue(PHANDLE, "plugin:mru-switcher:debounce_ms")->intValue;
static auto* const CFG_DEFAULT_SCOPE   = &HyprlandAPI::getConfigValue(PHANDLE, "plugin:mru-switcher:default_scope")->stringValue;
static auto* const CFG_START_OFFSET    = &HyprlandAPI::getConfigValue(PHANDLE, "plugin:mru-switcher:start_offset")->stringValue;
static auto* const CFG_WRAP            = &HyprlandAPI::getConfigValue(PHANDLE, "plugin:mru-switcher:wrap")->intValue;
static auto* const CFG_UI              = &HyprlandAPI::getConfigValue(PHANDLE, "plugin:mru-switcher:ui")->stringValue;
static auto* const CFG_LOCK_HISTORY    = &HyprlandAPI::getConfigValue(PHANDLE, "plugin:mru-switcher:lock_history_on_session")->intValue;
static auto* const CFG_RESTORE_CANCEL  = &HyprlandAPI::getConfigValue(PHANDLE, "plugin:mru-switcher:restore_focus_on_cancel")->intValue;
```

Registration uses `HyprlandAPI::addConfigValue(handle, "plugin:mru-switcher:debounce_ms", Hyprlang::CConfigValue(400))` etc., matching SPEC defaults.

- [ ] **Step 4: dispatchers**

```cpp
static SDispatchResult dispatch_cycle(std::string args) {
  auto parsed = mru::plugin::parse_cycle_args(args);
  if (!parsed.ok) return {.success = false, .error = parsed.error};
  auto r = gState.controller->cycle(parsed.dir, parsed.scope);
  if (!r.ok) return {.success = false, .error = r.error};
  return {.success = true};
}
static SDispatchResult dispatch_apply(std::string) {
  auto r = gState.controller->apply();
  if (!r.ok) return {.success = false, .error = r.error};
  return {.success = true};
}
static SDispatchResult dispatch_cancel(std::string) {
  auto r = gState.controller->cancel();
  return {.success = r.ok, .error = r.error};
}
```

Register in `PLUGIN_INIT`:

```cpp
HyprlandAPI::addDispatcherV2(PHANDLE, "mru:cycle",  dispatch_cycle);
HyprlandAPI::addDispatcherV2(PHANDLE, "mru:apply",  dispatch_apply);
HyprlandAPI::addDispatcherV2(PHANDLE, "mru:cancel", dispatch_cancel);
```

- [ ] **Step 5: Event::bus subscriptions** — listeners stored as static members so they outlive init (ARCHITECTURE §9):

```cpp
static auto L_ACTIVE = Event::bus()->m_events.window.active.listen(
    [](PHLWINDOW w, Desktop::eFocusReason reason) {
      gState.controller->on_focus(mru::plugin::registry.ref_for(w));
    });
static auto L_CLOSE  = Event::bus()->m_events.window.close.listen(
    [](PHLWINDOW w) {
      gState.registry->on_window_close(w);
      gState.controller->on_window_invalid(mru::domain::WindowRef{address_of(w), /*generation*/ gState.registry->last_ref(w)->generation});
    });
static auto L_DESTROY = Event::bus()->m_events.window.destroy.listen(
    [](PHLWINDOWREF w) { /* if known, same invalid handling */ });
static auto L_RELOAD = Event::bus()->m_events.config.reloaded.listen(
    [](std::any) { /* refresh cached config values in gState.config (REQ-CFG-002) */ });
```

- [ ] **Step 6: wire controller**

```cpp
gState.config = default_plugin_config();            // from Task 2
gState.registry = makeUP<WindowIdentityRegistry>();
gState.scheduler = makeUP<HyprlandSchedulerPort>();
gState.source = makeUP<HyprlandWindowSource>(*gState.registry, *gState.config);
gState.fg = makeUP<HyprlandFocusGateway>(*gState.registry);
gState.ui = makeUP<NullUI>();                       // REQ-UI-003
mru::domain::SessionPolicy policy = policy_from_config(*gState.config); // Task 2 extension
gState.tracker = makeUP<mru::domain::HistoryTracker>(*gState.scheduler, /*valid*/ [&](auto ref){ return gState.source->is_valid(ref); }, gState.config->debounce_ms);
gState.controller = makeUP<mru::domain::SessionController>(*gState.source, *gState.fg, *gState.ui, *gState.tracker, policy);
```

Note: `history_tracker` seeds MRU from the adapter source (Task 5) on demand; see `HistoryTracker` interface — the facade passes a seed callback if the domain API exposes one. If `HistoryTracker` requires seeding at construction, seed here with `source->candidates(global)`.

Also note: the singleton `gState` must be a function-local static with explicit teardown in `PLUGIN_EXIT` (Needs no global destructor order pitfalls).

- [ ] **Step 7: CMake final shared lib**

Replace the `FATAL_ERROR` block in `CMakeLists.txt` with:

```cmake
if(MRU_BUILD_PLUGIN)
  include(cmake/Hyprland.cmake)
  add_library(mru-switcher SHARED src/plugin/mru_plugin.cpp src/plugin/hypr/*.cpp)
  target_include_directories(mru-switcher PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/src/plugin ${HYPRLAND_INCLUDE_DIRS})
  target_link_libraries(mru-switcher PRIVATE mru_plugin_core mru_domain)
  set_target_properties(mru-switcher PROPERTIES PREFIX "" OUTPUT_NAME mru-switcher)  # .so without lib prefix
  mru_warnings(mru-switcher)
endif()
```

- [ ] **Step 8: build**

```bash
cmake -S . -B build -DMRU_BUILD_PLUGIN=ON -DHYPRLAND_SRC=<path>
cmake --build build --target mru-switcher -j
ls build/mru-switcher.so
```

Expected: `.so` exists with no unresolved plugin API symbols (they resolve at load).

- [ ] **Step 9: commit**

```bash
git add src/plugin/mru_plugin.cpp src/plugin/mru_plugin.hpp cmake/Hyprland.cmake CMakeLists.txt hyprpm.toml docs/agent-state/PROGRESS.md
git commit -m "feat(plugin): facade PLUGIN_INIT/EXIT with hash check, dispatchers, Event::bus (REQ-DISP-001/002, REQ-CFG-002, REQ-UI-003)"
```

---

### Task 7: Nested smoke + human verification (checklist)

**Files:**
- Modify: `docs/USER.md` (validate recommended binds)
- Modify: `docs/COMPAT.md` (nest-tested row)
- Modify: `docs/agent-state/PROGRESS.md` (M2 complete)
- Modify: `CHANGELOG.md` (M2 section)

**Checklist** (from `hyprland-nested-dev` skill):

- [ ] `hyprctl plugin load <abs>/mru-switcher.so` succeeds; `hyprctl plugin list` shows it
- [ ] `hyprctl dispatch mru:cycle next` returns ok; again advances; `mru:apply` focuses the selected window; `mru:cancel` ends without focus
- [ ] `mru:cycle` with no args = next (REQ-DISP-001); `mru:cycle bogus` returns error
- [ ] `mru:apply` when Idle is a success no-op
- [ ] Window close mid-session: selected closes → prune → clamp or cancel (SPEC §2.8)
- [ ] No `mru:cycle` focus side-effects (REQ-F-003)
- [ ] Hash mismatch test: rename headers, rebuild → load refuses
- [ ] `hyprctl plugin unload` clean; no crash, no stuck timers (M2 exit: no use-after-unload)
- [ ] `config.reloaded` refresh works for next session (REQ-CFG-002)

Record results in the MR body.

- [ ] **Commit**

```bash
git add docs/USER.md docs/COMPAT.md docs/agent-state/PROGRESS.md CHANGELOG.md
git commit -m "docs(m2): validate smoke checklist, fill COMPAT+nest row, close M2"
```

---

## Self-review

**1. Spec coverage (check)**
- REQ-DISP-001 (Task 3), REQ-DISP-002 (Task 6 dispatch_apply maps `no windows`), REQ-CFG-001 (Task 2 parse fallback), REQ-CFG-002 (Task 6 reload listener), REQ-CFG-004 (Task 2 clamp), REQ-UI-001 (NullUI noexcept / UI failure isolation), REQ-UI-002 (Task 2/6 fallback), REQ-UI-003 (Task 6 NullUI only), REQ-S-011 (no timer-based apply anywhere — SchedulerPort only for debounce/in HistoryTracker), REQ-F-003 (Task 5 gateway only in apply), REQ-F-005 (Task 4 registry), REQ-S-001..008 (controller from M1), REQ-R-001/002 (restore-on-cancel via policy, M1), REQ-S-010 (cycle scope ignore, M1).
- Missing in M2 by design: `mru:status` (REQ-DISP-003, M3), all scopes except global (M3), `border`/`external` UI (M4/M5).

**2. Placeholder scan**: all code blocks are concrete; the only environment-dependent details are `HYPRLAND_SRC` path and exact internal header include names, recorded as "as available on pin" where the API may differ (COMPAT.md §Internal API expectations).

**3. Type consistency**: `PluginConfig`, `CycleArgs`, `WindowIdentityRegistry` names/signatures are identical across the tasks that use them; `SessionPolicy` comes from `mru_domain` M1; `FocusResult`/`SchedulerPort`/`UIPort` ports from M1.

## Execution Handoff

Plan complete and saved to `docs/agent-state/plans/2026-09-13-m2-mvp-plugin.md`.

Two execution options:

1. **Subagent-Driven (recommended)** — orchestrator dispatches a fresh implementer subagent per Task 2–6 with the brief from the plan; reviews against SPEC between tasks; Task 1 and 7 are human-gated.
2. **Inline Execution** — orchestrator executes tasks directly in this session.

The orchestrator will use option 1 (per AGENTS.md), starting with Task 1 (environment) once the human has completed the `paru -S hyprland-git` step.