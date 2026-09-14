# Hyprland include discovery for the plugin build (MRU_BUILD_PLUGIN=ON).
#
# Prefers pkg-config (hyprland ships a .pc on the pinned install); falls back to
# the documented header install prefix /usr/include/hyprland.
#
# Outputs:
#   HYPRLAND_INCLUDE_DIRS - include dirs needed to compile plugin sources

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_check_modules(HYPRLAND_PC QUIET hyprland)
endif()

if(HYPRLAND_PC_FOUND)
  set(HYPRLAND_INCLUDE_DIRS ${HYPRLAND_PC_INCLUDE_DIRS})
  # transitively needed by Hyprland headers (drm, cairo, ...) on the pinned install
  set(HYPRLAND_EXTRA_CFLAGS ${HYPRLAND_PC_CFLAGS})
else()
  set(HYPRLAND_INCLUDE_DIRS
    /usr/include/hyprland/protocols
    /usr/include/hyprland
    /usr/include/hyprland/src)
endif()