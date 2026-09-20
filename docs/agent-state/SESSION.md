# Session State
Updated: 2026-09-20
Human goal: integrate external docs review (user/dev split + risk narratives); v1.0.0 tag stays human-gated
Active milestone: M6 closed — release-prep merged; docs-review integration PR in review
Branch: docs/external-review-integration
PR: docs-review PR (this branch) — user/dev split, RISK-PROFILES.md, hash-mismatch recovery guide
Next action: await human review of the PR + explicit release command (tag v1.0.0 per #54)
Blocked: v1.0.0 tag + GitHub Release + manual nest gate — explicit human release command only
State: ctest 18/18 gcc + ASan/UBSan green @ 4afcf21; this PR is docs-only (no build surface)
SPEC focus: unchanged — SPEC/REQ-TRACE untouched; user docs scrubbed of REQ-*/ADR-*/M-markers
Open questions: behavioral review items filed as issues (overlay auth, mru:status ext, profiles, hash-mismatch notification, snapshot limit, Nix/AUR)
Last artifact: docs/external-review-integration — review archived at docs/agent-state/research/2026-09-20-external-docs-review.md
; T-H-06..11 normative in SPEC §9