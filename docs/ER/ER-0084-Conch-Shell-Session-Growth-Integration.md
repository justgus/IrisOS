---
GitHub-Issue: N/A
AR-Dependencies: AR-0011, AR-0012
ER-Dependencies: ER-0066, ER-0081, ER-0082, ER-0083
---

# ER-0084 — Conch Shell Session-Growth Integration

## Roles

- Implementation Engineer: drafts and implements changes
- System Engineer: reviews, tests, and verifies
- Note: In this Autonomous AR Batch, Codex may verify only after all batch gates pass; the human records the required live pass.

## ER Metadata

- ER ID: ER-0084
- Title: Conch Shell Session-Growth Integration
- Status: Complete
- Date: 2026-10-07
- Owners: Mike
- Type: Enhancement

## Engineering Guidelines

- Implementation language baseline: C++20 or C++24.
- Avoid line compaction or formatting changes that risk obscuring or losing content.
- Keep source files reasonably small. If a file grows too large to be fully replaced in a change, split it into smaller local files.

## Context

- Problem statement: the Conch executable does not call `update_session_from_graph`; the verified ER-0066 library API therefore does not cause the running shell session to grow when commands create routable graph relationships.
- Background / constraints: AR-0012 says Conch subscribes to graph changes and spawns views, while ER-0066 provides a deterministic pull-based update API. This ER connects those accepted pieces without adding a background thread or event loop.

## Goals

- Give each running Conch process an active object-backed session with a starting graph cursor.
- Consume graph changes at command boundaries and attach newly routed Conchos to that session.
- Make created/reused views visible in the shell's textual interaction.

## Non-Goals

- Add a background watcher, asynchronous runtime, compositor, tiling, or focus behavior.
- Change ER-0066 routing or duplicate-suppression semantics.
- Implement interaction modes belonging only to AR-0025.

## Scope

- In scope: startup session creation, graph cursor lifecycle in the REPL, invoking `update_session_from_graph` after command handling, user-readable update results, and integration tests.
- Out of scope: persistent multi-user sessions, remote graph subscriptions, background threads, layout, and visual rendering.

## Requirements

- Functional: startup creates a Conch session using a cursor that excludes pre-existing graph history.
- Functional: after a command changes the graph, the running session consumes new changes and creates or reuses the corresponding Conchos.
- Functional: repeated read-only commands do not create duplicate Conchos for already observed relationships.
- Non-functional: the implementation remains pull-based and introduces no new dependency or background thread.

## Proposed Approach

- Summary: initialize one `SessionState` in the Conch process and invoke the existing update API at the end of each REPL command iteration, reporting created/reused view counts. Preserve the API's cursor and duplicate-suppression behavior.
- Alternatives considered: adding a watcher thread was rejected because the accepted ER-0066 design is pull-based and current event-loop ownership is unspecified. Leaving the helper API disconnected was rejected because the shell must exhibit the behavior in AR-0012.

## Acceptance Criteria

- An automated subprocess integration test exercises the actual `bin/conch` command loop, proves it consumes new graph relationships, and inspects the resulting session-to-Concho links.
- A command that produces routed artifacts causes a visible session-growth result.
- A subsequent command with no new routable graph changes does not add duplicate Conchos.
- Existing command-triggered artifact routing and session-growth library tests continue to pass.

## Risks / Open Questions

- Risk: command handlers use early `continue` paths; the update hook must run on every completed command path without running after process teardown.
- Question: none; the pull-based update API and command-boundary polling are the narrowest implementation supported by AR-0012 and ER-0066.

## Dependencies

- Dependency 1: ER-0066 Observer-Driven Conch Session Growth.
- Dependency 2: ER-0081 Referee Object Lookup NotFound Semantics.
- Dependency 3: ER-0082 Refract Lookup NotFound Semantics.
- Dependency 4: AR-0011 Vizier Interpretation Layer.
- Dependency 5: AR-0012 Conch Shell and Conchos.

## Implementation Notes

- Do not introduce threads or dependencies.
- Keep the initial graph cursor so existing historical artifacts are not retroactively spawned into a new process session.
- Use the existing `SessionState` and update APIs rather than duplicate routing logic.
- The Conch process now creates and reports one active session at startup, then applies the existing update API when each REPL loop iteration ends.
- Subprocess integration coverage seeds a historical routable relationship, verifies it is excluded from the new session, exercises `demo v1`, inspects session-to-Concho links, and confirms subsequent read-only commands create no duplicates.

## Implementation Validation

- `make -j AM_CXXFLAGS='-Wall -Wextra -Wpedantic -Werror -Wno-unused-const-variable -Wno-unused-private-field -Wno-dangling-gsl'` passed.
- `make -C tests test_conch_authoring test_conch_session_growth AM_CXXFLAGS='-Wall -Wextra -Wpedantic -Werror -Wno-unused-const-variable -Wno-unused-private-field -Wno-gnu-zero-variadic-macro-arguments -Wno-dangling-gsl'` passed.
- `DYLD_LIBRARY_PATH=/Users/justgus/Xcode-Projects/IrisOS/src/.libs TMPDIR=/private/tmp make check AM_CXXFLAGS='-Wall -Wextra -Wpedantic -Werror -Wno-unused-const-variable -Wno-unused-private-field -Wno-gnu-zero-variadic-macro-arguments -Wno-dangling-gsl'` passed all 29 tests.
- Independent review: Pass. Reviewer derived the AR/ER matrix independently, inspected command-loop control paths and startup cursor ordering, and freshly passed the focused two-test suite. The first subsequent read-only update is asserted to create zero Conchos.
- Live Pass (Human): Pass reported by the System Engineer on 2026-10-08.

## Verification Plan

- Tests to run:
  - `make -j`
  - `make -C tests test_conch_authoring test_conch_session_growth`
  - `make check`
- Manual checks: run `./bin/conch --db :memory:`, enter `demo v1`, inspect the session-growth message and the created Concho/session relationships with the shell's object/graph inspection commands, then issue a read-only command and confirm no duplicate view is reported.

### Live Pass (Human)

- Required: Yes
- If no, reason: N/A
- Build / environment / target: Built `bin/conch` in a terminal with an interactive TTY.
- Steps: Start `./bin/conch --db :memory:`; enter `demo v1`; inspect the Conch update output and object graph for the active session's Conchos; enter `objects` or `debug graph <session-id>` to inspect links; enter `objects` again and confirm no duplicate Conchos are reported; exit.
- Expected observations: the shell reports newly created Conchos after the demo produces routed artifacts; the active session links to those Conchos; a subsequent read-only command reports no newly created duplicates.
- Observed results: The System Engineer confirmed the required live pass is complete.
- Result: Pass
- Performed by: System Engineer
- Date: 2026-10-08
