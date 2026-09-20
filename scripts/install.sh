#!/usr/bin/env bash
# mru-switcher source installer - optional secondary helper.
#
# ADR-020 section 4 / SPEC section 14.4 (REQ-DIST-011..013). hyprpm is the
# primary channel (SPEC REQ-DIST-001/002); this script only automates a source
# build into the canonical layout (SPEC REQ-DIST-007). It never downloads
# anything, never needs sudo, and never touches hyprland.conf without the
# explicit --write-conf opt-in flag.
#
# Documented invocation (SPEC REQ-DIST-012 - no curl|bash):
#   git clone https://github.com/code-warlord-dev/mru-switcher.git ~/.local/src/mru-switcher
#   cd ~/.local/src/mru-switcher
#   ./scripts/install.sh
#
# Pinned Hyprland version lives in docs/COMPAT.md and hyprpm.toml
# (commit_pins) and is mirrored here as the default.
set -euo pipefail

REPO_URL="https://github.com/code-warlord-dev/mru-switcher.git"
PIN_VERSION="${MRU_HYPRLAND_PIN:-0.56.2}"    # docs/COMPAT.md matrix + hyprpm.toml commit_pins
PIN_DOC="docs/COMPAT.md"
CONF_D_DIR="${XDG_CONFIG_HOME:-$HOME/.config}/hypr/conf.d"
MODELINE="cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DMRU_BUILD_PLUGIN=ON"

info() { printf '%s\n' "[mru-switcher] $*"; }
die()  { printf '%s\n' "[mru-switcher] ERROR: $*" >&2; exit 1; }

usage() {
  cat <<EOF
Usage: install.sh [options]

Build and install the mru-switcher Hyprland plugin from this source tree
into the canonical source layout (SPEC REQ-DIST-007):

    $HOME/.local/src/mru-switcher/build/mru-switcher.so

Configure/build flags match hyprpm.toml and CI:

    $MODELINE
    cmake --build build -j

The script detects the Hyprland headers and verifies them against the pinned
version ($PIN_VERSION, $PIN_DOC). It never edits your Hyprland config unless
you pass --write-conf.

Options:
    -h, --help             Show this help and exit.
        --write-conf       Write a ready-made fragment under
                           $CONF_D_DIR/mru-switcher.conf
                           (opt-in only; never done silently).
        --hyprland-version X.Y.Z
                           Expect Hyprland headers version X.Y.Z instead of
                           the pin $PIN_VERSION. Use when deliberately building
                           against a non-pinned revision; the result only
                           loads on a compositor built from those headers
                           (Hyprland's fail-closed hash check).

Environment:
    MRU_HYPRLAND_PIN       Override the expected pinned version (same effect
                           as --hyprland-version).

Documented invocation (SPEC REQ-DIST-012):
    git clone $REPO_URL ~/.local/src/mru-switcher
    cd ~/.local/src/mru-switcher
    ./scripts/install.sh
EOF
}

version_ge() {
  [[ "$(printf '%s\n%s\n' "$1" "$2" | sort -V | tail -n1)" == "$1" ]]
}

resolve_root() {
  local source="$1"
  while [[ -L "$source" ]]; do
    source="$(readlink "$source")"
  done
  printf '%s\n' "$(cd -P -- "$(dirname -- "$source")/.." && pwd)"
}

check_arch() {
  local arch
  arch="$(uname -m)"
  if [[ "$arch" != "x86_64" ]]; then
    die "unsupported architecture '$arch': the plugin is built and tested on x86_64 only (see $PIN_DOC). Expected an x86_64 Hyprland host."
  fi
  info "Architecture: x86_64 OK"
}

check_toolchain() {
  if ! command -v git >/dev/null 2>&1; then
    die "git not found - a git checkout is required for a source install (install git, e.g. 'sudo apt install git' or 'sudo pacman -S git')."
  fi

  if ! command -v cmake >/dev/null 2>&1; then
    die "cmake not found - install cmake (>= 3.20), e.g. 'sudo apt install cmake' or 'sudo pacman -S cmake', then re-run."
  fi
  local cmake_ver
  cmake_ver="$(cmake --version | awk '/^cmake version/{print $3}')"
  if ! version_ge "$cmake_ver" "3.20"; then
    die "cmake $cmake_ver is too old - CMakeLists.txt requires >= 3.20 (install a newer cmake and re-run)."
  fi
  info "cmake: $cmake_ver OK"

  local cxx="${CXX:-}"
  if [[ -z "$cxx" ]]; then
    local cand
    for cand in c++ g++ clang++; do
      if command -v "$cand" >/dev/null 2>&1; then cxx="$cand"; break; fi
    done
  fi
  if [[ -z "$cxx" ]]; then
    die "no C++ compiler found - install one (e.g. 'sudo apt install g++' or 'sudo pacman -S base-devel') and re-run; set CXX if it is not on PATH."
  fi
  info "C++ compiler: $cxx OK"
}

check_headers() {
  local version="" vh
  if command -v pkg-config >/dev/null 2>&1 && pkg-config --exists hyprland; then
    version="$(pkg-config --modversion hyprland)"
  elif [[ -d /usr/include/hyprland ]]; then
    info "pkg-config did not report a 'hyprland' module (or pkg-config is absent) - falling back to headers at /usr/include/hyprland ..." >&2
    vh="/usr/include/hyprland/src/version.h"
    if [[ -r "$vh" ]]; then
      version="$(sed -n 's/^#define GIT_TAG[[:space:]]*"v\(.*\)"$/\1/p' "$vh")"
    fi
    if [[ -z "$version" ]]; then
      info "WARNING: found /usr/include/hyprland but could not read its version from $vh; the pinned-version check is skipped (Hyprland's fail-closed load-time hash check remains the authority)." >&2
      version="unknown"
    fi
  else
    die "Hyprland headers not found - neither the 'hyprland' pkg-config module nor /usr/include/hyprland is present. Install the headers for Hyprland v$PIN_VERSION (Arch: 'sudo pacman -S hyprland'; other distros: install or extract the matching headers), then re-run. See $PIN_DOC."
  fi
  printf '%s\n' "$version"
}

verify_version() {
  local have="$1" want="$2"
  if [[ "$have" == "$want" ]]; then
    info "Hyprland headers: $have (matches the $PIN_DOC pin)"
    return 0
  fi
  cat >&2 <<EOF
[mru-switcher] ERROR: Hyprland headers $have do not match the pinned version $want ($PIN_DOC).
The pinned compositor computes a header hash that this build will NOT match, so
the resulting .so fails closed on Hyprland's hash check at load time (fail-closed
policy, $PIN_DOC). Build anyway only against a compositor built from the same headers.
EOF
  die "version skew: headers $have vs pin $want - re-run with --hyprland-version $have to build against these headers deliberately."
}

write_fragment() {
  local file="$CONF_D_DIR/mru-switcher.conf"
  if [[ -e "$file" ]]; then
    info "Not overwriting existing $file (left untouched)."
    return 0
  fi
  mkdir -p "$CONF_D_DIR"
  cat > "$file" <<EOF
# mru-switcher - source-build fragment (written by scripts/install.sh --write-conf).
# Plugin built from: $ROOT
# Hyprland headers: $DETECTED_VERSION
plugin = $PLUGIN_PATH

# Recommended keybindings (apply fires on modifier release).
bind = ALT, TAB, mru:cycle, next
bind = ALT SHIFT, TAB, mru:cycle, prev
bindrt = ALT, ALT_L, mru:apply
bind = ALT, Escape, mru:cancel
EOF
  info "Wrote $file"
  info "If your config does not already source $CONF_D_DIR, add the line: source = $file"
}

success_summary() {
  echo
  echo "----------------------------------------------------------------"
  echo " mru-switcher build complete  (source channel, SPEC section 14.4)"
  echo "----------------------------------------------------------------"
  echo " Source:          $ROOT"
  echo " Plugin (.so):    $PLUGIN_PATH"
  echo " Hyprland:        $DETECTED_VERSION (headers)"
  echo
  echo " Load it into the running compositor:"
  echo "   hyprctl plugin load \"$PLUGIN_PATH\""
  echo
  echo " Recommended keybindings (add to your Hyprland config):"
  echo "   bind = ALT, TAB, mru:cycle, next"
  echo "   bind = ALT SHIFT, TAB, mru:cycle, prev"
  echo "   bindrt = ALT, ALT_L, mru:apply"
  echo "   bind = ALT, Escape, mru:cancel"
  if [[ "$WRITE_CONF" == 1 ]]; then
    echo
    echo " Conf fragment written: $CONF_D_DIR/mru-switcher.conf"
  else
    echo
    echo " Tip: re-run with --write-conf to write the fragment under"
    echo "   $CONF_D_DIR/"
  fi
  echo " Docs: installation channels and configuration in docs/USER.md and README."
  echo "----------------------------------------------------------------"
}

main() {
  WRITE_CONF=0
  expected_version="$PIN_VERSION"

  while (($#)); do
    case "$1" in
      -h|--help)
        usage
        exit 0
        ;;
      --write-conf)
        WRITE_CONF=1
        ;;
      --hyprland-version)
        if [[ $# -lt 2 ]]; then die "--hyprland-version needs a value, e.g. --hyprland-version $PIN_VERSION"; fi
        expected_version="$2"
        shift
        ;;
      --*)
        die "unknown option '$1' (see --help)"
        ;;
      *)
        die "unexpected argument '$1' (see --help)"
        ;;
    esac
    shift
  done

  ROOT="$(resolve_root "${BASH_SOURCE[0]}")"
  if [[ ! -f "$ROOT/CMakeLists.txt" ]]; then
    die "scripts/install.sh must be run from a mru-switcher checkout (no CMakeLists.txt found at $ROOT)."
  fi
  if [[ "$ROOT" != "$HOME/.local/src/mru-switcher" ]]; then
    info "WARNING: running from '$ROOT', not the canonical '$HOME/.local/src/mru-switcher' (SPEC REQ-DIST-007). The plugin is built here and the paths below reflect it; the docs use the canonical path."
  fi

  check_arch
  check_toolchain
  info "Detecting Hyprland headers ..."
  DETECTED_VERSION="$(check_headers)"
  if [[ "$DETECTED_VERSION" == "unknown" ]]; then
    info "Skipping pinned-version verification (headers version unknown)."
  else
    verify_version "$DETECTED_VERSION" "$expected_version"
  fi

  PLUGIN_PATH="$ROOT/build/mru-switcher.so"

  info "Configuring (Release, plugin only; same flags as hyprpm.toml / CI): $MODELINE"
  cmake -S "$ROOT" -B "$ROOT/build" -DCMAKE_BUILD_TYPE=Release -DMRU_BUILD_PLUGIN=ON
  info "Building ..."
  cmake --build "$ROOT/build" -j

  if [[ ! -f "$PLUGIN_PATH" || ! -s "$PLUGIN_PATH" ]]; then
    die "the build did not produce $PLUGIN_PATH."
  fi
  if command -v file >/dev/null 2>&1; then
    if ! file "$PLUGIN_PATH" | grep -q 'ELF.*shared object'; then
      die "$PLUGIN_PATH is not an ELF shared object: $(file "$PLUGIN_PATH")"
    fi
  else
    local magic
    magic="$(od -An -tx1 -N4 "$PLUGIN_PATH" 2>/dev/null | tr -d ' \n')"
    if [[ "$magic" != "7f454c46" ]]; then
      die "$PLUGIN_PATH is not an ELF file (magic ${magic:-none})."
    fi
  fi
  info "Verified: $PLUGIN_PATH (ELF shared object, $(du -h "$PLUGIN_PATH" | cut -f1))"

  if [[ "$WRITE_CONF" == 1 ]]; then
    write_fragment
  fi

  success_summary
}

WRITE_CONF=0
ROOT=""
DETECTED_VERSION=""
PLUGIN_PATH=""
main "$@"