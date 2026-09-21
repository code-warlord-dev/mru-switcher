#!/usr/bin/env bash
# mru-switcher binding helper - writes ONLY the plugin-owned keybinding file
# (ADR-022; SPEC section 14.5-14.6: REQ-DIST-016/017/018 - examples + first-setup delivery).
#
# Principle: never rewrite someone else's config silently. This script:
#   - detects the config backend (Lua / Omarchy vs hyprlang)
#   - reports live Alt+Tab conflicts via `hyprctl binds -j` (never edits them)
#   - writes only its own file (conf.d/...conf on hyprlang, or
#     ~/.config/hypr/mru-switcher-bindings.lua on Lua) - verbatim from examples/
#   - refuses to overwrite an existing file unless --force (then backups first)
#   - prints the single source / dofile line you add yourself
#   - never touches hyprland.conf, bindings.lua, or Omarchy defaults
#
# Which apply model each backend gets (ADR-023):
#   hyprlang : bindrt on ALT_L applies when Alt is released (unchanged recipe;
#              still awaiting a nest re-verification on the pin - docs/COMPAT.md)
#   lua      : B1 explicit apply key (ALT + Return) - the default profile this
#              script installs. B2 "release Alt to apply" is an optional
#              copy-paste reference: examples/mru-switcher-bindings-poll.lua
#              (hl.timer + hl.is_key_down), because a release bind keyed on a
#              modifier token does not fire on the pinned Hyprland's Lua path.
#              apply-on-Tab-release is NOT a supported profile.
#
# Invocation (mirrors install.sh - git clone, never curl|bash):
#   ./scripts/setup-bindings.sh --dry-run
#   ./scripts/setup-bindings.sh
#
# Exit codes:
#   0  success (also for --help / --version / a clean --dry-run)
#   1  unexpected failure (the ERR trap prints the failing line and command)
#   2  usage error: unknown option, missing/invalid value, stray argument
#   3  refused: running as root (would write into a non-user HOME)
#   4  refused: cannot locate the shipped examples/ (run from a checkout)
#   5  write/backup failure
#   6  refused: target file exists without --force
set -euo pipefail

SCRIPT_NAME="scripts/setup-bindings.sh"
SCRIPT_VERSION="1.0.0"
REPO_URL="https://github.com/code-warlord-dev/mru-switcher.git"
HYPR_DIR="${XDG_CONFIG_HOME:-$HOME/.config}/hypr"

# Option state (defaults).
OPT_BACKEND="auto"   # auto | lua | hyprlang
OPT_FROM=""          # examples/ override
FORCE=0
DRY_RUN=0
RELOAD=0
QUIET=0
VERBOSE=0

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
  trap - ERR
  printf '%s\n' "[mru-switcher] ERROR: command failed (exit ${code}) at ${SCRIPT_NAME}:${BASH_LINENO[0]:-?}" >&2
  printf '%s\n' "[mru-switcher] Re-run with --verbose to trace, or --help for options." >&2
  exit "${code}"
}
trap on_error ERR

usage() {
  cat <<EOF
Usage: ${SCRIPT_NAME} [options]

Write the mru-switcher keybinding file for your Hyprland config backend, and
print the one line to add so the binds take effect (ADR-022/ADR-023). The
plugin does not capture keys, so without this step Alt+Tab does nothing.

The script only ever writes its OWN file:
    Lua / Omarchy : \${XDG_CONFIG_HOME:-\$HOME/.config}/hypr/mru-switcher-bindings.lua
    hyprlang      : \${XDG_CONFIG_HOME:-\$HOME/.config}/hypr/conf.d/mru-switcher-bindings.conf

Apply model depends on the backend:
    Lua      : explicit apply key (B1) - cycle on ALT+TAB, commit on ALT+Return.
               "Release Alt to apply" (B2) is an optional copy-paste reference
               (examples/mru-switcher-bindings-poll.lua: hl.timer + hl.is_key_down
               poll). apply-on-Tab-release is not supported: it moves real focus
               on every Tab press and breaks the browse-then-commit workflow.
    hyprlang : bindrt on ALT_L applies when Alt is released (unchanged recipe;
               still awaiting a nest re-verification on the pin - docs/COMPAT.md).

It never edits hyprland.conf / bindings.lua / Omarchy defaults, never touches
existing binds, and never runs hyprctl reload unless you pass --reload.

Live Alt+Tab conflicts are REPORTED from \`hyprctl binds -j\` (advisory only):
if something else already claims ALT+TAB, unbind it yourself (hyprlang 'unbind'
/ Lua hl.unbind) or both binds will fire together.

Options:
    -h, --help              Show this help and exit (exit 0).
        --version           Print script version, then exit 0.
        --backend TYPE      auto (default) | lua | hyprlang. 'auto' picks
                            'lua' when \${HYPR_DIR}/hyprland.lua exists,
                            otherwise 'hyprlang'.
        --from DIR          Read the example file from DIR instead of the
                            checkout's ./examples (DIR must contain
                            mru-switcher-bindings.lua|conf).
        --force             Overwrite an existing target file: back it up to
                            <file>.bak.<timestamp> first, then replace it.
        --dry-run           Print the full plan and the conflict report,
                            change nothing (exit 0).
        --reload            Run \`hyprctl reload\` after writing. Off by
                            default: you own the reload.
        --quiet             Suppress progress output (errors still print).
        --verbose           Print every command before it runs.

Environment:
    XDG_CONFIG_HOME         Base for the hypr config dir (default: \$HOME/.config).
    HYPRLAND_INSTANCE_SIGNATURE  Needed for \`hyprctl\` to reach the session.

Exit codes:
    0 success | 1 unexpected | 2 usage | 3 root refused
    4 examples not found | 5 write/backup | 6 existing file without --force

Docs: ADR-022/ADR-023, README.md ("Installation"), docs/USER.md (Quick start).
EOF
}

version_info() {
  printf '%s\n' "mru-switcher ${SCRIPT_NAME} ${SCRIPT_VERSION}"
  printf '%s\n' "backend default: auto -> lua if ${HYPR_DIR}/hyprland.lua exists, else hyprlang"
}

# --- option parsing --------------------------------------------------------

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
    --backend)
      (($# >= 2)) || die 2 "--backend requires a value: auto | lua | hyprlang (see --help)."
      case "$2" in
      auto | lua | hyprlang) OPT_BACKEND="$2" ;;
      *) die 2 "--backend expects auto | lua | hyprlang (got '$2') - see --help." ;;
      esac
      shift
      ;;
    --backend=*)
      case "${1#--backend=}" in
      auto | lua | hyprlang) OPT_BACKEND="${1#--backend=}" ;;
      *) die 2 "--backend expects auto | lua | hyprlang (got '${1#--backend=}') - see --help." ;;
      esac
      ;;
    --from)
      (($# >= 2)) || die 2 "--from requires a directory path (see --help)."
      OPT_FROM="$2"
      shift
      ;;
    --from=*)
      OPT_FROM="${1#--from=}"
      ;;
    --force)
      FORCE=1
      ;;
    --dry-run)
      DRY_RUN=1
      ;;
    --reload)
      RELOAD=1
      ;;
    --quiet)
      QUIET=1
      ;;
    --verbose)
      VERBOSE=1
      ;;
    --)
      shift
      (($# == 0)) || die 2 "unexpected argument '$1' - this helper takes no positional arguments (see --help)."
      ;;
    -*)
      die 2 "unknown option '$1' (see --help for the full option list)."
      ;;
    *)
      die 2 "unexpected argument '$1' - this helper takes no positional arguments (see --help)."
      ;;
    esac
    shift
  done
}

# --- backend detection -----------------------------------------------------

detect_backend() {
  if [[ -f "$HYPR_DIR/hyprland.lua" ]]; then
    printf '%s\n' "lua"
  else
    printf '%s\n' "hyprlang"
  fi
}

# --- conflict report (advisory only) ---------------------------------------

# Prints one line per clashing live binding, if hyprctl + python3 are usable.
report_conflicts() {
  if ! command -v hyprctl >/dev/null 2>&1; then
    warn "hyprctl not found - skipping the live conflict check (only our own file is written; a stale Alt+Tab owner may still win)."
    return 0
  fi
  if [[ -z "${HYPRLAND_INSTANCE_SIGNATURE:-}" ]]; then
    warn "HYPRLAND_INSTANCE_SIGNATURE is unset - the running session is not reachable; skipping the live conflict check."
    return 0
  fi
  local binds
  if ! binds="$(hyprctl binds -j 2>/dev/null)"; then
    warn "hyprctl binds -j failed - skipping the live conflict check."
    return 0
  fi
  if ! command -v python3 >/dev/null 2>&1; then
    warn "python3 not found - cannot parse 'hyprctl binds -j' for the conflict report; skipping it."
    return 0
  fi

  local conflicts
  conflicts="$(printf '%s' "$binds" | python3 -c '
import json, sys
try:
    binds = json.load(sys.stdin)
except Exception:
    print("_parse_error")
    raise SystemExit(0)
seen = set()
rows = []
for b in binds:
    key = str(b.get("key") or "").upper()
    mod = int(b.get("modmask", 0) or 0)
    desc = str(b.get("description") or b.get("dispatcher") or "?")
    # Skip MRU own binds (registered from examples/*), so re-runs are calm.
    if desc.startswith("MRU") or (desc == "__lua" and not b.get("description")):
        continue
    if key == "TAB" and (mod & 8):
        rows.append((mod, key, bool(b.get("release")), desc))
    elif key in ("ALT_L", "ALT_R") and bool(b.get("release")):
        rows.append((mod, key, True, desc))
print("\n".join(f"modmask={m} key={k} release={str(r).lower()} <- {d}" for m, k, r, d in rows if (m, k, r, d) not in seen and not seen.add((m, k, r, d))))
' 2>/dev/null || printf '%s\n' "_parse_error")"

  if [[ "$conflicts" == "_parse_error" ]]; then
    warn "could not parse 'hyprctl binds -j' - skipping the conflict report."
    return 0
  fi
  if [[ -n "$conflicts" ]]; then
    warn "found keybindings that will fire together with MRU until unbind:"
    printf '%s\n' "$conflicts" | sed 's/^/    /' >&2
    warn "unbind them in your own config (hyprlang: unbind=ALT,TAB,...; Lua: hl.unbind(\"ALT + TAB\")) so only MRU fires."
  fi
}

# --- write ------------------------------------------------------------------

backend_layout() {  # prints "FILE DESTDIR" for the selected backend
  if [[ "$OPT_BACKEND" == "lua" ]]; then
    printf '%s %s\n' "mru-switcher-bindings.lua" "$HYPR_DIR"
  else
    printf '%s %s\n' "mru-switcher-bindings.conf" "$HYPR_DIR/conf.d"
  fi
}

examples_dir() {
  if [[ -n "$OPT_FROM" ]]; then
    printf '%s\n' "$OPT_FROM"
    return 0
  fi
  local self
  self="$(cd -P -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
  printf '%s\n' "$self/../examples"
}

print_next_step() {
  if [[ "$OPT_BACKEND" == "lua" ]]; then
    printf '%s\n' "  In an already-loaded Lua config (bindings.lua), before the plugin binds:"
    printf '    dofile(os.getenv("HOME") .. "/.config/hypr/mru-switcher-bindings.lua")\n'
    printf '%s\n' "  That fragment is model B1: cycle on ALT+TAB, commit on ALT+Return."
    printf '%s\n' "  Want \"release Alt to apply\" (B2) instead? Copy examples/mru-switcher-"
    printf '%s\n' "  bindings-poll.lua here and dofile that one instead (hl.timer +"
    printf '%s\n' "  hl.is_key_down poll); load one of the two, never both. This helper"
    printf '%s\n' "  only ever installs the B1 fragment."
  else
    printf '%s\n' "  In hyprland.conf, with the other source= lines:"
    printf '    source = ~/.config/hypr/conf.d/mru-switcher-bindings.conf\n'
    printf '%s\n' "  That fragment commits on Alt release (bindrt); Lua/Omarchy users want"
    printf '%s\n' "  examples/mru-switcher-bindings.lua (explicit apply) or its poll variant."
  fi
  printf '%s\n' "  Then reload: hyprctl reload"
}

write_bindings() {
  local file dest_dir dest ex src_dir bak
  read -r file dest_dir < <(backend_layout)

  if ((EUID == 0)); then
    die 3 "refusing to run as root: the helper writes into a user HOME and never into system config. Run it as your desktop user."
  fi

  src_dir="$(examples_dir)"
  ex="${src_dir}/${file}"
  if [[ ! -f "$ex" ]]; then
    die 4 "shipped example ${ex} not found. Run this from a mru-switcher checkout (git clone ${REPO_URL}, cd, then ./scripts/setup-bindings.sh), or point --from at a directory holding ${file}."
  fi

  dest="${dest_dir}/${file}"
  # Defense in depth: the destination must stay inside the hypr config dir
  # and keep the mru-switcher prefix.
  case "$dest" in
  "$HYPR_DIR"/*) ;;
  *) die 1 "refusing to write outside the hypr config dir: ${dest}" ;;
  esac
  case "$dest" in
  *mru-switcher*) ;;
  *) die 1 "refusing to write a non-mru-switcher path: ${dest}" ;;
  esac

  if ((DRY_RUN)); then
    printf '%s\n' "[mru-switcher] --dry-run: nothing is written."
    printf '    backend   : %s\n' "$OPT_BACKEND"
    printf '    source    : %s\n' "$ex"
    printf '    target    : %s\n' "$dest"
    printf '    overwrite : %s\n' "$([[ -e "$dest" ]] && printf 'existing file -> --force needed (backs up, then replaces)' || printf 'new file')"
    printf '%s\n' "  Next step you would be told after a real run:"
    print_next_step
    return 0
  fi

  mkdir -p "$dest_dir" || die 5 "cannot create ${dest_dir} (check XDG_CONFIG_HOME and directory permissions)."

  if [[ -e "$dest" ]]; then
    if ((FORCE)); then
      bak="${dest}.bak.$(date +%Y%m%d-%H%M%S)"
      cp -p -- "$dest" "$bak" || die 5 "cannot back up ${dest} to ${bak}."
      info "backed up ${dest} -> ${bak}"
      rm -f -- "$dest" || die 5 "cannot remove ${dest} before rewriting it."
    else
      die 6 "refusing to overwrite ${dest}: it already exists. Re-run with --force to back it up (<file>.bak.<timestamp>) and replace it, or edit it manually; the original stays at ${ex}."
    fi
  fi

  cp -- "$ex" "$dest" || die 5 "cannot write ${dest} (permissions?)."
  info "wrote ${dest} (verbatim copy of ${ex})"

  if ((RELOAD)); then
    if command -v hyprctl >/dev/null 2>&1 && [[ -n "${HYPRLAND_INSTANCE_SIGNATURE:-}" ]]; then
      (hyprctl reload >/dev/null 2>&1 && info "hyprctl reload: ok") || warn "hyprctl reload reported a problem - check the config error output manually."
    else
      warn "--reload given but the Hyprland session is not reachable - reload manually with: hyprctl reload"
    fi
  fi
}

# --- entry point -----------------------------------------------------------

main() {
  parse_args "$@"

  if [[ "$OPT_BACKEND" == "auto" ]]; then
    OPT_BACKEND="$(detect_backend)"
  fi

  # --verbose traces every command as it runs (matches --help and the ERR-trap hint).
  ((${VERBOSE:-0})) && set -x

  info "config backend: ${OPT_BACKEND} (dir: ${HYPR_DIR})"

  report_conflicts

  write_bindings

  if ((!DRY_RUN)); then
    printf '%s\n' ""
    printf '%s\n' "  Done. One more step for the binds to take effect:"
    print_next_step
    printf '%s\n' ""
  fi
}

main "$@"