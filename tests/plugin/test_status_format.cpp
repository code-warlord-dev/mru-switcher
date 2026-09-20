#include "config_value.hpp"
#include "status_format.hpp"
#include "test_framework.hpp"

namespace {
using mru::domain::Scope;
using mru::domain::SessionEndReason;
using namespace mru::plugin;

// SPEC §3.4: frozen payload `active= index= size= scope= session= last_end=` in order (M6-T1).
TEST(status_01_active_session) {
    const std::string s = format_status(true, 2, 5, scope_name(Scope::Global), 3, std::nullopt);
    EQ(s, std::string{"active=true index=2 size=5 scope=global session=3 last_end=none"});
}

TEST(status_02_idle_session_with_last_end) {
    const std::string s = format_status(false, 0, 0, scope_name(Scope::Workspace), 7, SessionEndReason::UserCancel);
    EQ(s, std::string{"active=false index=0 size=0 scope=workspace session=7 last_end=UserCancel"});
}

TEST(status_02b_unknown_suffix_is_tolerated) {
    // SPEC §3.4 additive rule: parsers MUST tolerate appended keys.
    const std::string s = format_status(false, 0, 0, scope_name(Scope::Global), 1, std::nullopt) + " extra=1";
    CHECK(s.find("active=false index=0 size=0 scope=global session=1 last_end=none") == 0);
}

TEST(status_03_scope_names_are_stable) {
    EQ(scope_name(Scope::Global), std::string_view{"global"});
    EQ(scope_name(Scope::Monitor), std::string_view{"monitor"});
    EQ(scope_name(Scope::Workspace), std::string_view{"workspace"});
    EQ(scope_name(Scope::Visible), std::string_view{"visible"});
    EQ(scope_name(Scope::App), std::string_view{"app"});
}

} // namespace

int main() {
    return mru::test::run_all();
}
