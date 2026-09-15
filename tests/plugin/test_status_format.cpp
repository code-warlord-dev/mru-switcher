#include "config_value.hpp"
#include "status_format.hpp"
#include "test_framework.hpp"

namespace {
using mru::domain::Scope;
using mru::domain::SessionEndReason;
using namespace mru::plugin;

// SPEC §3.4: active=true index=2 size=5 scope=global (+ informative extras)
TEST(status_01_active_session) {
    const std::string s = format_status(true, 2, 5, scope_name(Scope::Global), 3, std::nullopt);
    CHECK(s.find("active=true") == 0);
    CHECK(s.find("index=2") != std::string::npos);
    CHECK(s.find("size=5") != std::string::npos);
    CHECK(s.find("scope=global") != std::string::npos);
    CHECK(s.find("session=3") != std::string::npos);
}

TEST(status_02_idle_session_with_last_end) {
    const std::string s = format_status(false, 0, 0, scope_name(Scope::Workspace), 7, SessionEndReason::UserCancel);
    CHECK(s.find("active=false") == 0);
    CHECK(s.find("size=0") != std::string::npos);
    CHECK(s.find("scope=workspace") != std::string::npos);
    CHECK(s.find("last_end=UserCancel") != std::string::npos);
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
