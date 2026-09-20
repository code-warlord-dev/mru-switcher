cmake_minimum_required(VERSION 3.20)
project(mru-switcher VERSION 0.4.0 LANGUAGES CXX)

# Single source of truth for the plugin version string (L-13): CMake's
# PROJECT_VERSION is configured into a header consumed by PLUGIN_INIT.
configure_file(cmake/mru-version.hpp.in ${CMAKE_CURRENT_BINARY_DIR}/generated/mru_version.hpp @ONLY)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Static libs are linked into the mru-switcher shared plugin; needs -fPIC.
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(MRU_BUILD_TESTS "Build domain unit tests" ON)
option(MRU_BUILD_PLUGIN "Build Hyprland plugin .so (needs Hyprland headers)" OFF)
option(MRU_WARNINGS_AS_ERRORS "Treat warnings as errors" ON)

function(mru_warnings target)
  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    if(MRU_WARNINGS_AS_ERRORS)
      target_compile_options(${target} PRIVATE -Werror)
    endif()
  endif()
endfunction()

# --- Domain library (no Hyprland) ---
add_library(mru_domain STATIC)
target_include_directories(mru_domain PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
file(GLOB_RECURSE MRU_DOMAIN_SOURCES CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/src/domain/*.cpp)
target_sources(mru_domain PRIVATE ${MRU_DOMAIN_SOURCES})
mru_warnings(mru_domain)

if(MRU_BUILD_TESTS)
  enable_testing()

  add_executable(domain_test_window_identity tests/domain/test_window_identity.cpp)
  target_link_libraries(domain_test_window_identity PRIVATE mru_domain)
  mru_warnings(domain_test_window_identity)
  add_test(NAME domain_window_identity COMMAND domain_test_window_identity)

  add_executable(domain_test_history_tracker tests/domain/test_history_tracker.cpp)
  target_link_libraries(domain_test_history_tracker PRIVATE mru_domain)
  mru_warnings(domain_test_history_tracker)
  add_test(NAME domain_history_tracker COMMAND domain_test_history_tracker)

  add_executable(domain_test_selection_helpers tests/domain/test_selection_helpers.cpp)
  target_link_libraries(domain_test_selection_helpers PRIVATE mru_domain)
  mru_warnings(domain_test_selection_helpers)
  add_test(NAME domain_selection_helpers COMMAND domain_test_selection_helpers)

  add_executable(domain_test_snapshot tests/domain/test_snapshot.cpp)
  target_link_libraries(domain_test_snapshot PRIVATE mru_domain)
  mru_warnings(domain_test_snapshot)
  add_test(NAME domain_snapshot COMMAND domain_test_snapshot)

  add_executable(domain_test_scope_predicate tests/domain/test_scope_predicate.cpp)
  target_link_libraries(domain_test_scope_predicate PRIVATE mru_domain)
  mru_warnings(domain_test_scope_predicate)
  add_test(NAME domain_scope_predicate COMMAND domain_test_scope_predicate)

  add_executable(domain_test_session_controller tests/domain/test_session_controller.cpp)
  target_link_libraries(domain_test_session_controller PRIVATE mru_domain)
  mru_warnings(domain_test_session_controller)
  add_test(NAME domain_session_controller COMMAND domain_test_session_controller)

  add_executable(domain_test_stress_window_close tests/domain/test_stress_window_close.cpp)
  target_link_libraries(domain_test_stress_window_close PRIVATE mru_domain)
  mru_warnings(domain_test_stress_window_close)
  add_test(NAME domain_stress_window_close COMMAND domain_test_stress_window_close)

  add_executable(domain_test_stress_focus_invalidation tests/domain/test_stress_focus_invalidation.cpp)
  target_link_libraries(domain_test_stress_focus_invalidation PRIVATE mru_domain)
  mru_warnings(domain_test_stress_focus_invalidation)
  add_test(NAME domain_stress_focus_invalidation COMMAND domain_test_stress_focus_invalidation)
endif()

# --- Plugin pure helpers (no Hyprland headers; unit-testable) ---
add_library(mru_plugin_core STATIC
  src/plugin/config_value.cpp
  src/plugin/dispatch_args.cpp
  src/plugin/status_format.cpp
  src/plugin/mru_merge.cpp
  src/plugin/border_highlight_ui.cpp
  src/plugin/session_ui.cpp
  src/plugin/overlay_protocol.cpp
  src/plugin/external_overlay_ui.cpp
  src/plugin/overlay_socket_server.cpp)
target_include_directories(mru_plugin_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/src/plugin)
target_link_libraries(mru_plugin_core PUBLIC mru_domain)
mru_warnings(mru_plugin_core)

if(MRU_BUILD_TESTS)
  add_executable(plugin_config_value tests/plugin/test_config_value.cpp)
  target_include_directories(plugin_config_value PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests/domain)
  target_link_libraries(plugin_config_value PRIVATE mru_plugin_core)
  mru_warnings(plugin_config_value)
  add_test(NAME plugin_config_value COMMAND plugin_config_value)

  add_executable(plugin_dispatch_args tests/plugin/test_dispatch_args.cpp)
  target_include_directories(plugin_dispatch_args PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests/domain)
  target_link_libraries(plugin_dispatch_args PRIVATE mru_plugin_core)
  mru_warnings(plugin_dispatch_args)
  add_test(NAME plugin_dispatch_args COMMAND plugin_dispatch_args)

  add_executable(plugin_status_format tests/plugin/test_status_format.cpp)
  target_include_directories(plugin_status_format PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests/domain)
  target_link_libraries(plugin_status_format PRIVATE mru_plugin_core)
  mru_warnings(plugin_status_format)
  add_test(NAME plugin_status_format COMMAND plugin_status_format)

  add_executable(plugin_mru_merge tests/plugin/test_mru_merge.cpp)
  target_include_directories(plugin_mru_merge PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests/domain)
  target_link_libraries(plugin_mru_merge PRIVATE mru_plugin_core)
  mru_warnings(plugin_mru_merge)
  add_test(NAME plugin_mru_merge COMMAND plugin_mru_merge)

  # T-UI-03..06: BorderHighlightUI lifecycle/restore over a fake BorderPropIo.
  add_executable(plugin_border_highlight_ui tests/plugin/test_border_highlight_ui.cpp)
  target_include_directories(plugin_border_highlight_ui PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests/domain)
  target_link_libraries(plugin_border_highlight_ui PRIVATE mru_plugin_core mru_domain)
  mru_warnings(plugin_border_highlight_ui)
  add_test(NAME plugin_border_highlight_ui COMMAND plugin_border_highlight_ui)

  # REQ-UI-009: backend swap takes effect on the next session, not mid-session.
  add_executable(plugin_session_ui tests/plugin/test_session_ui.cpp)
  target_include_directories(plugin_session_ui PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests/domain)
  target_link_libraries(plugin_session_ui PRIVATE mru_plugin_core mru_domain)
  mru_warnings(plugin_session_ui)
  add_test(NAME plugin_session_ui COMMAND plugin_session_ui)

  # T-O-03..05: Appendix B encode/decode over the frozen M5 protocol (ADR-018).
  add_executable(plugin_overlay_protocol tests/plugin/test_overlay_protocol.cpp)
  target_include_directories(plugin_overlay_protocol PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests/domain)
  target_link_libraries(plugin_overlay_protocol PRIVATE mru_plugin_core mru_domain)
  mru_warnings(plugin_overlay_protocol)
  add_test(NAME plugin_overlay_protocol COMMAND plugin_overlay_protocol)

  # T-O-06: ExternalOverlayUI over a fake transport (peer absent stays safe).
  add_executable(plugin_external_overlay_ui tests/plugin/test_external_overlay_ui.cpp)
  target_include_directories(plugin_external_overlay_ui PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests/domain)
  target_link_libraries(plugin_external_overlay_ui PRIVATE mru_plugin_core mru_domain)
  mru_warnings(plugin_external_overlay_ui)
  add_test(NAME plugin_external_overlay_ui COMMAND plugin_external_overlay_ui)

  # T-O-07: AF_UNIX server over a real loopback socket (no Hyprland needed).
  add_executable(plugin_overlay_socket_server tests/plugin/test_overlay_socket_server.cpp)
  target_include_directories(plugin_overlay_socket_server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests/domain)
  target_link_libraries(plugin_overlay_socket_server PRIVATE mru_plugin_core mru_domain)
  mru_warnings(plugin_overlay_socket_server)
  add_test(NAME plugin_overlay_socket_server COMMAND plugin_overlay_socket_server)
endif()

if(MRU_BUILD_PLUGIN)
  include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/Hyprland.cmake)

  # Adapters that touch Hyprland internals (PHLWINDOW, globals).
  add_library(mru_plugin_hypr STATIC
    src/plugin/hypr/identity_registry.cpp
    src/plugin/hypr/hypr_window_source.cpp
    src/plugin/hypr/hypr_focus_gateway.cpp
    src/plugin/hypr/hypr_scheduler.cpp
    src/plugin/hypr/hyprctl_border_prop_io.cpp
    src/plugin/hypr/hyprland_overlay_socket.cpp)
  target_include_directories(mru_plugin_hypr PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src/plugin
    ${HYPRLAND_INCLUDE_DIRS})
  # hypr_window_source.cpp calls mru_plugin_core's merge_mru_order; declaring the
  # static-library dependency makes CMake order the link line so the final .so
  # resolves it (otherwise the core archive is processed before it is needed).
  target_link_libraries(mru_plugin_hypr PUBLIC mru_plugin_core)
  # Facade: final shared library (hyprpm output build/mru-switcher.so)
  add_library(mru-switcher SHARED
    src/plugin/mru_plugin.cpp
    src/plugin/config_v2.cpp)
  target_include_directories(mru-switcher PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src/plugin
    ${CMAKE_CURRENT_BINARY_DIR}/generated
    ${HYPRLAND_INCLUDE_DIRS})
  target_link_libraries(mru-switcher PRIVATE mru_plugin_core mru_plugin_hypr mru_domain)
  set_target_properties(mru-switcher PROPERTIES PREFIX "" OUTPUT_NAME mru-switcher)
  mru_warnings(mru-switcher)
  # The Hyprland PluginAPI facade is extern "C" by contract (APICALL EXPORT) yet
  # must return C++ types (std::string / PLUGIN_DESCRIPTION_INFO); gcc accepts
  # this silently, system clang (22.x) flags it as -Wreturn-type-c-linkage. The
  # diagnostic is noise here: these symbols are only ever read by the plugin
  # loader in-process, never by C code. Gate it to the plugin target only.
  target_compile_options(mru-switcher PRIVATE $<$<CXX_COMPILER_ID:Clang>:-Wno-return-type-c-linkage>)
endif()
