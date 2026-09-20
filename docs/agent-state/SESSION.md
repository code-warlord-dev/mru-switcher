# Session State
Updated: 2026-09-21
Human goal: keybindings as first-class install step; fix and document the Lua apply-on-release host bug (ADR-022)
Active milestone: M6 (release-prep); v1.0.0 tag stays human-gated
Branch: (uncommitted on main) -> feat/bindings-first-class (to create)
PR: none yet
Next action: create branch, commit docs+packaging, open PR; human applies the Lua fix to the user's own ~/.config/hypr (Tab-release already live + working there)
Blocked: none
State: user confirmed working on host (release-on-Tab); scripts/setup-bindings.sh sandbox-tested (dry-run, force+backup, exit codes 0/2/4/6, conflict filter); ci.yml installer job extended; ./packages/scripts bash -n OK (shellcheck n/a on this host)
SPEC focus: §14.6/REQ-DIST-018 (verify script header citation) — no normative SPEC change (ADR-022 documented only)
Open questions: none blocking
Last artifact: ADR-022 in docs/DECISIONS.md; bisect memo docs/agent-state/research/2026-09-20-lua-modifier-release-bisect.md