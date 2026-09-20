#!/usr/bin/env bash
# mru-switcher source installer - optional helper for the source channel
# (ADR-020 section 4; SPEC section 14.4, REQ-DIST-011..013 and REQ-DIST-026..027).
#
# hyprpm stays the primary channel (SPEC REQ-DIST-001/002); this script only
# automates a source build into the canonical layout (SPEC REQ-DIST-007).
# It never downloads anything, never needs sudo, and never touches
# hyprland.conf: the only writes it can make are the cmake build directory and
# - with the explicit --write-conf opt-in - verbatim copies of examples/*.conf
# under ~/.config/hypr/conf.d/.
#
# Documented invocation (SPEC REQ-DIST-012 - no curl|bash):
#   git clone https://github.com/code-warlord-dev/mru-switcher.git ~/.local/src/mru-switcher
#   cd ~/.local/src/mru-switcher
#   ./scripts/install.sh
#
# Exit codes:
#   0  success (also for --help / --version / a clean --dry-run)
#   1  unexpected failure (the ERR trap prints the failing line and command)
#   2  usage error: unknown option, missing/invalid value, stray argument
#   3  refused: not the canonical source layout (see --dir / --allow-non-canonical)
#   4  preflight failure: architecture, toolchain, Hyprland headers, pin skew
#   5  build failure: configure, compile, or .so verification
#   6  refused: --write-conf would replace an existing file without --force
set -euo pipefail

SCRIPT_NAME="scripts/install.sh"
SCRIPT_VERSION="1.0.0"
REPO_URL="https://github.com/code-warlord-dev/mru-switcher.git"
PIN_DOC="docs/COMPAT.md"
CMAKE_MIN="3.20"
CANONICAL_ROOT="${HOME}/.local/src/mru-switcher"
EXAMPLES=(mru-switcher.conf mru-switcher-bindings.conf)
CONF_D_DIR="${XDG_CONFIG_HOME:-$HOME/.config}/hypr/conf.d"
# Pinned Hyprland version; mirrored from docs/COMPAT.md + hyprpm.toml commit_pins.
PIN_VERSION="${MRU_HYPRLAND_PIN:-0.56.2}"

# Option state (defaults).
OPT_DIR=""
ALLOW_NON_CANONICAL=0
WRITE_CONF=0
FORCE=0
DRY_RUN=0
QUIET=0
VERBOSE=0
JOBS=""
EXPECTED_VERSION="$PIN_VERSION"
ROOT=""
DETECTED_VERSION=""
PLUGIN_PATH=""

info() { if ((!QUIET)); then printf '%s\n' "[mru-switcher] $*"; fi; }
warn() { printf '%s\n' "[mru-switcher] WARNING: $*" >&2; }
err() { printf '%s\n' "[mru-switcher] ERROR: $*" >&2; }

die() {
  local code="$1"
  shift
  err "$*"
  exit "$code"
}

on_error() {
  local code=$?
  local line="${BASH_LINENO[0]:-?}"
  local cmd="${BASH_COMMAND:-?}"
  trap - ERR
  printf '%s\n' "[mru-switcher] ERROR: command failed (exit ${code}) at ${SCRIPT_NAME}:${line}" >&2
  printf '%s\n' "[mru-switcher]   failing command: ${cmd}" >&2
  printf '%s\n' "[mru-switcher] Re-run with --verbose to trace every command, or --help for options." >&2
  exit "${code}"
}
trap on_error ERR


usage() {
  cat <<EOF
Usage: ${SCRIPT_NAME} [options]

Build the mru-switcher Hyprland plugin from this source tree and verify the
result, targeting the canonical layout (SPEC REQ-DIST-007):

    ${CANONICAL_ROOT}/build/mru-switcher.so

Configure/build commands (identical to hyprpm.toml and CI):

    cmake -S "\$ROOT" -B "\$ROOT/build" -DCMAKE_BUILD_TYPE=Release -DMRU_BUILD_PLUGIN=ON
    cmake --build "\$ROOT/build" -j N

No sudo, no downloads, no curl|bash. hyprland.conf is never modified: the only
config write is --write-conf, which copies the shipped examples verbatim
(checkout examples/ -> ${CONF_D_DIR}/).

Options:
    -h, --help              Show this help and exit (exit 0).
        --version           Print script version and the expected Hyprland pin,
                            then exit 0.
        --dir DIR           Build the checkout at DIR (absolute path) instead of
                            the tree this script lives in. Selecting DIR
                            explicitly marks a non-canonical location as
                            deliberate.
        --allow-non-canonical
                            Accept a checkout outside ${CANONICAL_ROOT}.
        --write-conf        Copy examples/${EXAMPLES[0]} and
                            examples/${EXAMPLES[1]} (verbatim) into
                            ${CONF_D_DIR}/.
        --force             With --write-conf: back up an existing file to
                            <file>.bak and replace it.
        --dry-run           Print the full plan (paths, options, commands) and
                            change nothing. Preflight problems are reported as
                            warnings instead of errors, so --dry-run exits 0 on
                            a readable checkout.
        --quiet             Suppress progress output (errors and the final
                            summary still print).
        --verbose           Print every command before it runs.
        --jobs N            Parallel build jobs; positive integer
                            (default: all available cores).
        --hyprland-version X.Y.Z
                            Expect Hyprland headers X.Y.Z instead of the pin
                            ${PIN_VERSION}. Only for a deliberate build against a
                            non-pinned revision: the resulting .so loads only on
                            a compositor built from the same headers (Hyprland's
                            fail-closed hash check).

Environment:
    MRU_HYPRLAND_PIN        Same effect as --hyprland-version.
    XDG_CONFIG_HOME         Base for the hypr/conf.d target of --write-conf
                            (default: \$HOME/.config).

Exit codes:
    0 success | 1 unexpected | 2 usage | 3 non-canonical layout
    4 preflight (arch/toolchain/headers/pin) | 5 build | 6 --write-conf refused

Documented invocation (SPEC REQ-DIST-012):
    git clone ${REPO_URL} ~/.local/src/mru-switcher
    cd ~/.local/src/mru-switcher
    ./${SCRIPT_NAME}

Docs: docs/USER.md (installation + configuration), README.md (quick start).
EOF
}

version_info() {
  printf '%s\n' "mru-switcher ${SCRIPT_NAME} ${SCRIPT_VERSION}"
  printf '%s\n' "expected Hyprland pin: ${PIN_VERSION} (${PIN_DOC}; override: --hyprland-version or MRU_HYPRLAND_PIN)"
  printf '%s\n' "canonical source layout: ${CANONICAL_ROOT}"
  printf '%s\n' "repository: ${REPO_URL}"
}


# --- helpers ---------------------------------------------------------------

version_ge() {
  [[ "$(printf '%s\n%s\n' "$1" "$2" | sort -V | tail -n1)" == "$1" ]]
}

resolve_root() {
  local source="$1" dir
  while [[ -L "$source" ]]; do
    dir="$(cd -P -- "$(dirname -- "$source")" && pwd)"
    source="$(readlink -- "$source")"
    [[ "$source" == /* ]] || source="${dir}/${source}"
  done
  cd -P -- "$(dirname -- "$source")/.." && pwd
}

# pf_fail: hard failure during a real run, advisory warning during --dry-run.
pf_fail() {
  local code="$1" msg="$2"
  if ((DRY_RUN)); then
    warn "$msg (dry run: reported, not fatal)"
  else
    die "$code" "$msg"
  fi
}

run_cmd() {
  if ((VERBOSE)); then
    printf '%s\n' "[mru-switcher] + $*" >&2
  fi
  if ((QUIET)); then
    # --quiet: hide progress chatter on stdout; compiler/cmake diagnostics
    # (stderr) still reach the user, and a failure still trips `|| die`.
    "$@" >/dev/null
  else
    "$@"
  fi
}

# --- preflight -------------------------------------------------------------

check_arch() {
  local arch
  arch="$(uname -m)"
  if [[ "$arch" != "x86_64" ]]; then
    pf_fail 4 "unsupported architecture '${arch}': the plugin is built and tested on x86_64 only (see ${PIN_DOC}). Run this on an x86_64 Hyprland host."
    return 0
  fi
  info "arch: x86_64 OK"
}

check_toolchain() {
  local missing=0

  if ! command -v git >/dev/null 2>&1; then
    pf_fail 4 "git not found - the source channel starts from a git checkout (Debian/Ubuntu: 'sudo apt install git'; Arch: 'sudo pacman -S git'), then re-run."
    missing=1
  fi

  if ! command -v cmake >/dev/null 2>&1; then
    pf_fail 4 "cmake not found - install cmake >= ${CMAKE_MIN} (Debian/Ubuntu: 'sudo apt install cmake'; Arch: 'sudo pacman -S cmake'), then re-run."
    missing=1
  elif ((!missing)); then
    local cmake_ver
    cmake_ver="$(cmake --version | awk '/^cmake version/{print $3}')"
    if ! version_ge "${cmake_ver:-0}" "$CMAKE_MIN"; then
      pf_fail 4 "cmake ${cmake_ver:-unknown} is too old - CMakeLists.txt requires >= ${CMAKE_MIN}; install a newer cmake and re-run."
      missing=1
    else
      info "cmake: ${cmake_ver} OK (>= ${CMAKE_MIN})"
    fi
  fi

  local cxx="${CXX:-}"
  if [[ -z "$cxx" ]]; then
    local cand
    for cand in c++ g++ clang++; do
      if command -v "$cand" >/dev/null 2>&1; then
        cxx="$cand"
        break
      fi
    done
  fi
  if [[ -z "$cxx" ]]; then
    pf_fail 4 "no C++ compiler found - install one (Debian/Ubuntu: 'sudo apt install g++'; Arch: 'sudo pacman -S base-devel') and re-run; set CXX if it is not on PATH."
  else
    info "C++ compiler: ${cxx} OK"
  fi

  if command -v ninja >/dev/null 2>&1; then
    info "build tool: ninja OK"
  elif command -v make >/dev/null 2>&1; then
    info "build tool: make OK"
  else
    pf_fail 4 "neither ninja nor make found - install one (Debian/Ubuntu: 'sudo apt install ninja-build'; Arch: 'sudo pacman -S ninja') and re-run, or configure the tree with an explicit -G generator."
  fi
}

# Sets DETECTED_VERSION (globals, not stdout: a hard failure must exit the
# script itself, not a command-substitution subshell).
check_headers() {
  local version="" vh=""
  DETECTED_VERSION=""
  if command -v pkg-config >/dev/null 2>&1 && pkg-config --exists hyprland 2>/dev/null; then
    version="$(pkg-config --modversion hyprland 2>/dev/null || true)"
  elif [[ -d /usr/include/hyprland ]]; then
    info "pkg-config does not report a 'hyprland' module (or pkg-config is absent) - falling back to the headers at /usr/include/hyprland ..."
    vh="/usr/include/hyprland/src/version.h"
    if [[ -r "$vh" ]]; then
      version="$(sed -n 's/^#define GIT_TAG[[:space:]]*"v\(.*\)"$/\1/p' "$vh")"
    fi
    if [[ -z "$version" ]]; then
      warn "found /usr/include/hyprland but could not read its version from ${vh}; the pinned-version check is skipped (Hyprland's fail-closed load-time hash check remains the authority)."
      version="unknown"
    fi
  else
    pf_fail 4 "Hyprland headers not found - neither the 'hyprland' pkg-config module nor /usr/include/hyprland is present. Install the headers for Hyprland v${PIN_VERSION} (Arch: 'sudo pacman -S hyprland'; other distros: install or extract the matching headers) and re-run. See ${PIN_DOC}."
    version="unknown"
  fi
  DETECTED_VERSION="$version"
}

verify_version() {
  local have="$1" want="$2"
  if [[ "$have" == "$want" ]]; then
    info "Hyprland headers: ${have} (matches the pin in ${PIN_DOC})"
    return 0
  fi
  pf_fail 4 "version skew: headers ${have} vs pin ${want} (${PIN_DOC}). The pinned compositor computes a header hash this build will not match, so the .so fails closed on Hyprland's hash check at load time. Re-run with --hyprland-version ${have} to build against these headers deliberately, or install the pinned headers."
}

report_running_hyprland() {
  if ! command -v hyprctl >/dev/null 2>&1 || [[ -z "${HYPRLAND_INSTANCE_SIGNATURE:-}" ]]; then
    info "running Hyprland: not detected (hyprctl unavailable or HYPRLAND_INSTANCE_SIGNATURE unset) - continuing."
    return 0
  fi
  local line
  line="$(hyprctl version 2>/dev/null | head -n1 || true)"
  if [[ -n "$line" ]]; then
    info "running Hyprland: ${line}"
  else
    info "running Hyprland: hyprctl present but the compositor did not answer - continuing."
  fi
}


# --- plan / layout ---------------------------------------------------------

enforce_layout() {
  if [[ "$ROOT" == "$CANONICAL_ROOT" ]]; then
    info "layout: canonical (${CANONICAL_ROOT})"
    return 0
  fi
  if ((ALLOW_NON_CANONICAL)) || [[ -n "$OPT_DIR" ]]; then
    warn "building outside the canonical layout ${CANONICAL_ROOT}: README.md and docs/USER.md document every path from that location, so copy the commands below with the paths printed here."
    return 0
  fi
  err "refusing to install from '${ROOT}': the documented source layout is ${CANONICAL_ROOT} (SPEC REQ-DIST-007), and every command in README.md / docs/USER.md uses that path."
  err "Fix it one of two ways:"
  err "  1. use the canonical checkout:"
  err "       git clone ${REPO_URL} ${CANONICAL_ROOT}"
  err "       cd ${CANONICAL_ROOT} && ./${SCRIPT_NAME}"
  err "  2. keep this location deliberately (explicit opt-in):"
  err "       ./${SCRIPT_NAME} --dir ${ROOT}"
  err "       ./${SCRIPT_NAME} --allow-non-canonical"
  err "Exit code 3; run ./${SCRIPT_NAME} --help for all options."
  exit 3
}

print_plan() {
  local layout="canonical"
  [[ "$ROOT" == "$CANONICAL_ROOT" ]] || layout="non-canonical (explicit opt-in)"
  local wc_desc="no"
  if ((WRITE_CONF)); then
    wc_desc="yes -> ${CONF_D_DIR}/{${EXAMPLES[0]}, ${EXAMPLES[1]}}"
  fi
  printf '\n'
  printf '%s\n' "[mru-switcher] --dry-run: nothing below is executed and no file is written."
  printf '%s\n' "[mru-switcher] plan:"
  printf '    %-19s %s\n' "source root" "$ROOT"
  printf '    %-19s %s\n' "build dir" "${ROOT}/build"
  printf '    %-19s %s\n' "plugin output" "${ROOT}/build/mru-switcher.so"
  printf '    %-19s %s\n' "canonical root" "$CANONICAL_ROOT"
  printf '    %-19s %s\n' "layout" "$layout"
  printf '    %-19s %s\n' "expected pin" "$EXPECTED_VERSION"
  printf '    %-19s %s\n' "detected headers" "$DETECTED_VERSION"
  printf '    %-19s %s\n' "jobs" "$JOBS"
  printf '    %-19s %s\n' "write-conf" "$wc_desc"
  printf '    %-19s %s\n' "quiet / verbose" "${QUIET} / ${VERBOSE}"
  printf '%s\n' "[mru-switcher] commands, in order:"
  printf '    %s\n' "cmake -S \"${ROOT}\" -B \"${ROOT}/build\" -DCMAKE_BUILD_TYPE=Release -DMRU_BUILD_PLUGIN=ON"
  printf '    %s\n' "cmake --build \"${ROOT}/build\" -j ${JOBS}"
  if ((WRITE_CONF)); then
    local f
    for f in "${EXAMPLES[@]}"; do
      printf '    %s\n' "cp ${ROOT}/examples/${f} ${CONF_D_DIR}/${f}"
    done
    if ((FORCE)); then
      printf '    %s\n' "(existing files are backed up to <file>.bak first: --force)"
    fi
  fi
  printf '%s\n' "[mru-switcher] verification after the build: the .so exists, is non-empty, and is an ELF shared object."
  printf '\n'
}

# --- build -----------------------------------------------------------------

build_plugin() {
  PLUGIN_PATH="${ROOT}/build/mru-switcher.so"
  info "configuring (Release, plugin only; same flags as hyprpm.toml and CI) ..."
  run_cmd cmake -S "$ROOT" -B "$ROOT/build" -DCMAKE_BUILD_TYPE=Release -DMRU_BUILD_PLUGIN=ON ||
    die 5 "configure failed: cmake -S \"${ROOT}\" -B \"${ROOT}/build\" -DCMAKE_BUILD_TYPE=Release -DMRU_BUILD_PLUGIN=ON (see the cmake output above; missing or mismatched Hyprland headers are the usual cause - see ${PIN_DOC})."
  info "building with ${JOBS} job(s) ..."
  run_cmd cmake --build "$ROOT/build" -j "$JOBS" ||
    die 5 "build failed: cmake --build \"${ROOT}/build\" -j ${JOBS} (see the compiler output above)."
}

verify_plugin() {
  if [[ ! -f "$PLUGIN_PATH" || ! -s "$PLUGIN_PATH" ]]; then
    die 5 "the build did not produce a non-empty ${PLUGIN_PATH} (hyprpm.toml expects build/mru-switcher.so as the output)."
  fi
  if command -v file >/dev/null 2>&1; then
    if ! file "$PLUGIN_PATH" | grep -q 'ELF.*shared object'; then
      die 5 "${PLUGIN_PATH} is not an ELF shared object: $(file "$PLUGIN_PATH")"
    fi
    info "verified: ${PLUGIN_PATH} ($(file -b "$PLUGIN_PATH"), $(du -h "$PLUGIN_PATH" | cut -f1))"
  else
    local magic
    magic="$(od -An -tx1 -N4 "$PLUGIN_PATH" 2>/dev/null | tr -d ' \n')"
    if [[ "$magic" != "7f454c46" ]]; then
      die 5 "${PLUGIN_PATH} is not an ELF file (magic ${magic:-none})."
    fi
    info "verified: ${PLUGIN_PATH} (ELF magic, $(du -h "$PLUGIN_PATH" | cut -f1))"
  fi
}

# --- examples -> hypr/conf.d (verbatim copies, opt-in only) ----------------

write_conf() {
  local src_dir="${ROOT}/examples" f
  for f in "${EXAMPLES[@]}"; do
    if [[ ! -f "${src_dir}/${f}" ]]; then
      die 4 "shipped example ${src_dir}/${f} is missing - this checkout is incomplete; re-clone ${REPO_URL}."
    fi
  done

  if ((DRY_RUN)); then
    for f in "${EXAMPLES[@]}"; do
      info "[dry-run] would copy ${src_dir}/${f} -> ${CONF_D_DIR}/${f}"
    done
    return 0
  fi

  mkdir -p "$CONF_D_DIR" || die 5 "cannot create ${CONF_D_DIR} (check XDG_CONFIG_HOME and directory permissions)."

  local dest
  for f in "${EXAMPLES[@]}"; do
    dest="${CONF_D_DIR}/${f}"
    if [[ -e "$dest" ]]; then
      if ((FORCE)); then
        cp -p -- "$dest" "${dest}.bak" || die 5 "cannot back up ${dest} to ${dest}.bak."
        info "backed up ${dest} -> ${dest}.bak"
      else
        die 6 "refusing to overwrite ${dest} without --force. Re-run with --force (backs the file up as ${dest}.bak, then replaces it) or edit it manually; the shipped original stays at examples/${f}."
      fi
    fi
    cp -- "${src_dir}/${f}" "$dest" || die 5 "cannot write ${dest} (permissions?)."
    info "wrote ${dest} (verbatim copy of examples/${f})"
  done

  info "if your config does not already include that directory, add to hyprland.conf:"
  info "  source = ${CONF_D_DIR}/${EXAMPLES[0]}"
  info "  source = ${CONF_D_DIR}/${EXAMPLES[1]}"
}


# --- summary ---------------------------------------------------------------

success_summary() {
  local layout="canonical"
  [[ "$ROOT" == "$CANONICAL_ROOT" ]] || layout="non-canonical (accepted explicitly)"
  local f
  printf '\n'
  printf '%s\n' "----------------------------------------------------------------"
  printf '%s\n' " mru-switcher build complete (source channel, SPEC section 14.4)"
  printf '%s\n' "----------------------------------------------------------------"
  printf ' %-16s %s\n' "Source root:" "$ROOT"
  printf ' %-16s %s\n' "Plugin (.so):" "$PLUGIN_PATH"
  printf ' %-16s %s\n' "Size:" "$(du -h "$PLUGIN_PATH" | cut -f1)"
  printf ' %-16s %s\n' "Hyprland:" "${DETECTED_VERSION} (headers; expected pin ${EXPECTED_VERSION})"
  printf ' %-16s %s\n' "Layout:" "$layout"
  printf ' %-16s %s\n' "Examples:" "${ROOT}/examples/"
  printf '\n'
  printf '%s\n' " Next steps:"
  printf '%s\n' "   1. Load the plugin into the running compositor:"
  printf '        hyprctl plugin load "%s"\n' "$PLUGIN_PATH"
  if ((WRITE_CONF)); then
    printf '%s\n' "   2. Config fragments written (verbatim copies of the shipped examples):"
    for f in "${EXAMPLES[@]}"; do
      printf '        %s\n' "${CONF_D_DIR}/${f}"
    done
    printf '%s\n' "      Make sure hyprland.conf includes them, and keep the load line for a source build:"
    printf '        source = %s\n' "${CONF_D_DIR}/${EXAMPLES[0]}"
    printf '        source = %s\n' "${CONF_D_DIR}/${EXAMPLES[1]}"
    printf '        plugin = %s\n' "$PLUGIN_PATH"
    printf '%s\n' "      Then: hyprctl reload"
  else
    printf '%s\n' "   2. Want the config fragments written for you? Re-run with --write-conf:"
    printf '        %s --write-conf\n' "$SCRIPT_NAME"
    printf '%s\n' "      (copies examples/*.conf verbatim into ${CONF_D_DIR}; --force adds a .bak backup before replacing an existing file)"
  fi
  printf '%s\n' "   3. Docs: docs/USER.md (configuration, scopes, troubleshooting), README.md (quick start)."
  printf '%s\n' "----------------------------------------------------------------"
}

# --- option parsing --------------------------------------------------------

set_dir() {
  local dir="$1"
  [[ -n "$dir" ]] || die 2 "--dir requires a non-empty path (see --help)."
  [[ "$dir" == /* ]] || die 2 "--dir requires an absolute path (got '${dir}'); expand it explicitly, e.g. --dir \"\${HOME}/src/mru-switcher\" (see --help)."
  [[ -d "$dir" ]] || die 2 "--dir '${dir}' is not a directory (see --help)."
  OPT_DIR="$dir"
}

set_jobs() {
  local n="$1"
  [[ "$n" =~ ^[1-9][0-9]*$ ]] || die 2 "--jobs requires a positive integer (got '${n}') - see --help."
  JOBS="$n"
}

set_version() {
  local v="$1"
  v="${v#v}"
  [[ "$v" =~ ^[0-9]+\.[0-9]+(\.[0-9]+)?$ ]] || die 2 "--hyprland-version expects a version such as ${PIN_VERSION} (got '${1}') - see --help."
  EXPECTED_VERSION="$v"
}

parse_args() {
  while (($#)); do
    case "$1" in
    -h | --help)
      usage
      exit 0
      ;;
    --version)
      version_info
      exit 0
      ;;
    --dir)
      (($# >= 2)) || die 2 "--dir requires an absolute path argument (see --help)."
      set_dir "$2"
      shift
      ;;
    --dir=*)
      set_dir "${1#--dir=}"
      ;;
    --allow-non-canonical)
      ALLOW_NON_CANONICAL=1
      ;;
    --write-conf)
      WRITE_CONF=1
      ;;
    --force)
      FORCE=1
      ;;
    --dry-run)
      DRY_RUN=1
      ;;
    --quiet)
      QUIET=1
      ;;
    --verbose)
      VERBOSE=1
      ;;
    --jobs)
      (($# >= 2)) || die 2 "--jobs requires a value, e.g. --jobs 4 (see --help)."
      set_jobs "$2"
      shift
      ;;
    --jobs=*)
      set_jobs "${1#--jobs=}"
      ;;
    --hyprland-version)
      (($# >= 2)) || die 2 "--hyprland-version requires a value, e.g. --hyprland-version ${PIN_VERSION} (see --help)."
      set_version "$2"
      shift
      ;;
    --hyprland-version=*)
      set_version "${1#--hyprland-version=}"
      ;;
    --)
      shift
      (($# == 0)) || die 2 "unexpected argument '$1' - this installer takes no positional arguments (see --help)."
      ;;
    -*)
      die 2 "unknown option '$1' (see --help for the full option list)."
      ;;
    *)
      die 2 "unexpected argument '$1' - this installer takes no positional arguments (see --help)."
      ;;
    esac
    shift
  done
}

# --- entry point -----------------------------------------------------------

main() {
  parse_args "$@"

  if ((FORCE)) && ((!WRITE_CONF)); then
    warn "--force only affects --write-conf; ignoring it for this run."
  fi

  if [[ -z "$JOBS" ]]; then
    JOBS="$(nproc 2>/dev/null || getconf _NPROCESSORS_ONLN 2>/dev/null || printf '%s\n' 1)"
  fi

  if [[ -n "$OPT_DIR" ]]; then
    ROOT="$(cd -P -- "$OPT_DIR" && pwd)"
  else
    ROOT="$(resolve_root "${BASH_SOURCE[0]}")"
  fi
  [[ -f "${ROOT}/CMakeLists.txt" ]] ||
    die 3 "no CMakeLists.txt at ${ROOT} - this is not a mru-switcher checkout; point --dir at one (see --help)."

  enforce_layout

  info "source root: ${ROOT}"
  info "expected Hyprland pin: ${EXPECTED_VERSION} (${PIN_DOC})"

  check_arch
  check_toolchain
  info "detecting Hyprland headers ..."
  check_headers
  if [[ "$DETECTED_VERSION" == "unknown" ]]; then
    info "Hyprland headers version unknown: skipping the pin comparison (the fail-closed hash check at load time stays authoritative)."
  else
    verify_version "$DETECTED_VERSION" "$EXPECTED_VERSION"
  fi
  report_running_hyprland

  if ((DRY_RUN)); then
    print_plan
    exit 0
  fi

  build_plugin
  verify_plugin

  if ((WRITE_CONF)); then
    write_conf
  fi

  success_summary
}

main "$@"

