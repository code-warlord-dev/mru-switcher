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
# Sources added as M1 lands; placeholder object keeps target valid
file(GLOB_RECURSE MRU_DOMAIN_SOURCES CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/src/domain/*.cpp)
if(MRU_DOMAIN_SOURCES)
  target_sources(mru_domain PRIVATE ${MRU_DOMAIN_SOURCES})
else()
  target_sources(mru_domain PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/domain/placeholder.cpp)
endif()
mru_warnings(mru_domain)

if(MRU_BUILD_TESTS)
  enable_testing()
  add_executable(mru_domain_tests tests/domain/smoke_placeholder.cpp)
  target_link_libraries(mru_domain_tests PRIVATE mru_domain)
  mru_warnings(mru_domain_tests)
  add_test(NAME domain_smoke COMMAND mru_domain_tests)
endif()

if(MRU_BUILD_PLUGIN)
  # M2 will define add_library(mru-switcher SHARED ...). Until then, fail closed so
  # hyprpm/CI cannot report a green build without a .so.
  message(FATAL_ERROR
    "MRU_BUILD_PLUGIN=ON is not supported until M2 wires the shared library target.\n"
    "  See docs/ROADMAP.md M2 and docs/COMPAT.md.\n"
    "  Configure with -DMRU_BUILD_PLUGIN=OFF (default) for domain tests.")
endif()
