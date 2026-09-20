# Session State
Updated: 2026-09-21
Human goal: keybindings as first-class install step; fix and document the Lua apply-on-release host bug (ADR-022)
Active milestone: M6 (release-prep); v1.0.0 tag stays human-gated
Branch: feat/bindings-first-class
PR: #85 (open; self-review fixes applied, pending CI + merge)
Next action: verify CI green -> squash-merge #85 -> update PROGRESS checkbox
Blocked: none
State: user confirmed working on host (release-on-Tab); setup-bindings.sh sandbox-tested (dry-run, force+backup, exit 0/2/4/6, conflict filter, --verbose live); review fixes: --verbose wired to set -x, CHANGELOG --no-python claim removed, ADR-022 amended re opt-in --reload, ADR tag scrubbed from README/USER/API (REQ-DIST-019)
SPEC focus: §14.5-14.6 REQ-DIST-016/017/018 — no normative SPEC change (ADR-022 documented only)
Open questions: none blocking
Last artifact: docs/agent-state/research/2026-09-20-lua-modifier-release-bisect.md; PR https://github.com/code-warlord-dev/mru-switcher/pull/85