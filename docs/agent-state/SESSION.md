# Session State
Updated: 2026-09-21
Human goal: correction stream — drop Tab-release as the Lua product recipe; ship explicit-apply and/or modifier-hold poll (ADR-022 framing is wrong as product model)
Active milestone: M6 (release-prep); v1.0.0 tag stays human-gated (NO tag without explicit human command)
Branch: fix/alt-release-model-correction
PR: ADR-023 correction PR open — CI + spec-compliance self-review, then self-merge
Next action: merge PR -> update PROGRESS.md; bindrt nest check + v1.0.0 tag remain human-gated
Blocked: none
State: ADR-023 delivered on the branch: Tab-release rejected as product model; B1 explicit ALT+Return apply default in examples/mru-switcher-bindings.lua; B2 poll reference shipped (examples/mru-switcher-bindings-poll.lua); README/USER/API/COMPAT/CHANGELOG/SPEC §11 note/setup-bindings.sh aligned; Omarchy unbind mandated; ctest 18/18 green; luac -p + bash -n clean; release NOT tagged
SPEC focus: REQ-DIST-016/017/018; REQ-F-002 (apply); ADR-023 product model — no normative REQ change
Open questions: does hyprlang bindrt fire apply on Alt release on efb5099? (empirical nest check — deferred, human-gated, before 1.0)
Last artifact: docs/agent-state/plans/2026-09-21-alt-release-correction.md (all core items done)