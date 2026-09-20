# Risk profiles (narrative)

Three end-to-end stories of how the plugin reacts to failure. Each profile follows one risk from
trigger → guards → degradation → final user-visible result, and names the tests that pin the
behaviour. This is the way these scenarios are meant to be **read**; the exhaustive matrices remain
the reference: [FAILURE-MODES.md](FAILURE-MODES.md) (FM-xx), [THREAT-MODEL.md](THREAT-MODEL.md),
[COMPAT.md](COMPAT.md), [SPEC.md](SPEC.md) §9.

---

## 1. External overlay fails

**Trigger.** You run `ui = external` with an overlay process attached. It crashes or disconnects
mid-session, sends garbage or oversized lines, reconnects repeatedly — or the configured
`external_socket` path cannot be bound at all.

**Guards that engage.** The socket is strictly opt-in: with `external_socket` empty the feature does
not exist. When enabled, the plugin creates the socket in your runtime directory with mode `0600`
(permissive-ownership fixups are attempted, failures are non-fatal), accepts only the **first**
client (later clients are closed), and treats all I/O as non-blocking and best-effort — the plugin
never waits on the peer. Protocol input is line-framed JSON with a bounded per-message size; unknown
version, unknown type, malformed JSON and oversized lines are ignored and logged at debug.

**Degradation.** A dead or absent peer is not an error: switching behaves exactly like `ui = null`
(outgoing messages are dropped). A live peer sending garbage is ignored line-by-line; the running
session is never cancelled or restarted because of protocol errors. An unbindable socket path
degrades to `null` with a one-time warning.

**User result.** Switching keeps working; the overlay can reconnect at any time and resume.

**Pinned by.** `T-O-05` (bad lines ignored, session unaffected), `T-O-06` (absent/failing transport
never aborts the session), `T-O-07` (framing, first-client, non-blocking), `T-FUZZ-01`
(deterministic parse-path fuzz, runs under the ASan/UBSan job), `T-UI-01`/`T-UI-02` (backend
fallback/isolation pattern), FM-08 in [FAILURE-MODES.md](FAILURE-MODES.md).

---

## 2. Malicious / broken configuration

**Trigger.** A config pasted from the internet: `debounce_ms` far out of range, unknown values for
`ui` / `default_scope` / `border_style`, colour strings in a wrong format, unknown keys, an unknown
scope token on the command line.

**Guards that engage.** Every key is typed and registered once at plugin load under
`plugin:mru-switcher:`. Values are clamped or fall back to documented defaults; nothing is
interpreted — no eval-like behaviour, no shell-out, and colours are restricted to the compositor's
own `setprop` grammar. String paths are used only as a bind target for a socket the plugin itself
owns. Unknown **config** keys surface as compositor warnings; an unknown **scope** token fails the
`mru:cycle` dispatcher with a clear error string instead of opening a session.

**Degradation.** Out-of-range or unknown values produce one warning plus a safe default; the plugin
loads and switching works. Settings changed by a reload apply only to the **next** session — a
running session keeps its frozen policy.

**User result.** The realistic worst case is a wrong visual style plus a single warning — never a
broken compositor from configuration content.

**Pinned by.** `T-CFG-*` (clamp and enum fallback), `T-UI-07` (unknown `border_style` → solid, no
abort), `T-UI-03` (default `null` has no border side effects), `T-S-09` (mid-session policy freeze),
`T-SC-05` (unknown scope token fails with a clear error), `T-UI-02` (UI exceptions isolated).

---

## 3. Window-close storm during a session

**Trigger.** While an Alt+Tab session is open, windows close en masse — a workspace or monitor
disappears, a batch job finishes, the compositor tears down scratchpads.

**Guards that engage.** The session's snapshot is frozen at start and stores identity references,
not raw pointers; validity is re-checked at focus time. Focus application runs a **bounded**
apply-retry policy: after prune-and-repair, at most one successful focus is attempted; persistent
failure ends the session cleanly with a structured result and exactly one UI end — focus is never
moved twice and never moved at all on definitive failure.

**Degradation.** Each close prunes that window and clamps the selection with a single clamp (never
an unbounded scan). If the last snapshot window disappears, the session ends (NoWindows) and focus
stays where it is. Closing a window that is not in the snapshot is a no-op. Unload or teardown
mid-session cancels scheduled jobs and releases everything without use-after-free.

**User result.** No crash, no focus teleport, no stuck highlight; the next Alt+Tab starts from a
fresh, valid snapshot.

**Pinned by.** `T-H-06` (prune/clamp, NoWindows drain, dead origin, deterministic 300-step close
storm), `T-H-07` (§2.8 bounds: bounded InvalidTarget retry, definitive Failed, 500-apply storm,
monitor-disconnect drain), `T-F-03`/`T-F-04`, `T-S-10` (non-cancel session end never moves focus).

---

*Reference matrices: [FAILURE-MODES.md](FAILURE-MODES.md) · [THREAT-MODEL.md](THREAT-MODEL.md) ·
[COMPAT.md](COMPAT.md). Profiles review: `docs/agent-state/research/2026-09-20-external-docs-review.md`.*
