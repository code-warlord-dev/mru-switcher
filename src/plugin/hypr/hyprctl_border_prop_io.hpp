#pragma once

#include "border_highlight_ui.hpp"

namespace mru::plugin {

// Adapter-private implementation of BorderPropIo (REQ-UI-011) over the pinned
// Hyprland public API `HyprlandAPI::invokeHyprctlCommand` (docs/COMPAT.md):
//   set -> "dispatch" "setprop address:0x<ptr> <prop> <value>"
//   get -> "getprop"  "address:0x<ptr> <prop>"
// The call-string spelling is R0-uncertain (`"dispatch"` vs `"dispatch/"`); the
// exact grammar is verified in the M4-S3 nest smoke. Failures are returned as an
// empty get() / false set() and never throw (REQ-UI-001).
class HyprctlBorderPropIo : public BorderPropIo {
  public:
    std::string get(std::uint64_t address, BorderSlot slot) override;
    bool set(std::uint64_t address, BorderSlot slot, const std::string &value) override;

  private:
    static std::string selector(std::uint64_t address);
    static std::string prop_name(BorderSlot slot);
};

} // namespace mru::plugin
