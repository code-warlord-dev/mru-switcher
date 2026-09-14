cmake_minimum_required(VERSION 3.20)
project(mru-switcher VERSION 0.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
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

  add_executable(domain_test_session_controller tests/domain/test_session_controller.cpp)
  target_link_libraries(domain_test_session_controller PRIVATE mru_domain)
  mru_warnings(domain_test_session_controller)
  add_test(NAME domain_session_controller COMMAND domain_test_session_controller)
endif()

# --- Plugin pure helpers (no Hyprland headers; unit-testable) ---
add_library(mru_plugin_core STATIC src/plugin/config_value.cpp src/plugin/dispatch_args.cpp)
target_include_directories(mru_plugin_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/src/plugin)
target_link_libraries(mru_plugin_core PUBLIC mru_domain)
mru_warnings(mru_plugin_core)

if(MRU_BUILD_TESTS)
  add_executable(plugin_config_value tests/plugin/test_config_value.cpp)
  target_link_libraries(plugin_config_value PRIVATE mru_plugin_core)
  mru_warnings(plugin_config_value)
  add_test(NAME plugin_config_value COMMAND plugin_config_value)

  add_executable(plugin_dispatch_args tests/plugin/test_dispatch_args.cpp)
  target_link_libraries(plugin_dispatch_args PRIVATE mru_plugin_core)
  mru_warnings(plugin_dispatch_args)
  add_test(NAME plugin_dispatch_args COMMAND plugin_dispatch_args)
endif()

if(MRU_BUILD_PLUGIN)
  include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/Hyprland.cmake)

  # Adapters that touch Hyprland internals (PHLWINDOW, globals).
  add_library(mru_plugin_hypr STATIC
    src/plugin/hypr/identity_registry.cpp)
  target_include_directories(mru_plugin_hypr PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src/plugin
    ${HYPRLAND_INCLUDE_DIRS})
  target_link_libraries(mru_plugin_hypr PUBLIC mru_plugin_core)
  mru_warnings(mru_plugin_hypr)
endif()
