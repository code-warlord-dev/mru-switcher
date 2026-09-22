# Session State
Updated: 2026-09-22 (late)
Human goal: (1) 3 new skills integrated into docs ✅ PR #96; (2) stale build* dirs purged (canonical build/ kept) ✅; (3) PROJECT_VERSION 0.4.0→0.5.0 ✅ PR #96; (4) ui=border default + bug fix ✅ PR #97 (ADR-025). РЕЛИЗ ТОЛЬКО ПО ЯВНОЙ КОМАНДЕ
Active milestone: M6 (release-prep); v1.0.0 tag strictly human-gated — NOT executed
Branch: main @ 1efbe77
PR: #96 (skills+version+cleanup) and #97 (ADR-025 ui=border) both merged, branches deleted
Blocked: none
Next action: idle — await human: live-host hyprpm reload to pick up 0.5.0 (border default will apply without any config); then M6-T9 tag ONLY on explicit command
SPEC focus: REQ-CFG-002, REQ-UI-002/003 (ADR-025); ADR-024 sidecar unchanged
Open questions: ui_matched=true on default (border recognized) — verified; unknown-token fallback still → null (REQ-CFG-001)
Last artifact: PR #97 (squash 1efbe77) — ADR-025, SPEC §4/§5.2, 19/19 ctest, clang-format clean; ui default flip is user-visible, CHANGELOG Changed has both bullets