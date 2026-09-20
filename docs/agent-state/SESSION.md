# Session State
Updated: 2026-09-20
Human goal: prepare the repo for the first stable-facing release; M6 close-out + installation/distribution pass (ADR-020)
Active milestone: M6 HARDENING → v1.0 — T2/T3 closed; packaging pass merged; only M6-T9 (tag, human gate) remains
Branch: main @ c70e6b9
PR: #69/#68 (T3/T2), #70 (ADR-020 + SPEC §14), #71 (examples + README/USER), #72 (scripts/install.sh) — all MERGED
Next action: await human review + release command → M6-T9 (v1.0.0 tag, finalize commit_pins, run manual nest gate incl. T-DIST-01/02/03)
Blocked: M6-T9 release tag (human gate); pre-release bug triage open: #67 (lock_history_on_session=false no-op), #65 (chained sessions), #58 (clang 22 build), #59 (THREAT-MODEL controls), #55 (M6-B1 reload)
State: ctest 17/17; CI green incl. new release-guard job; DIST docs+assets+installer merged; live nest smokes documented in manual gate
SPEC focus: M6-T2/T3 closed; ADR-020 Accepted; SPEC §14 REQ-DIST-001..024 (+T-DIST-01..04 non-unit checks)
Last artifact: PR #72 scripts/install.sh; packaging gate checklist in docs/agent-state/reports/2026-09-19-m6-manual-nest-gate.md