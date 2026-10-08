# Autonomous AR Batch Progress

- Requested ARs: AR-0001, AR-0011, AR-0012
- Started: 2026-10-07
- Current state: ER-0081, ER-0082, ER-0083, and ER-0087 are merged; ER-0084 is the active implementation item
- Goal/thread reference: 01a1120b-578d-7563-8735-2de39aeeee44

## AR Queue

| AR | Eligibility | AR status | ERs | State | Blocker / evidence |
|---|---|---|---|---|---|
| AR-0001 | Accepted at batch start | In Progress | ER-0081, ER-0082, ER-0083, ER-0086 | Planning | Typed NotFound lookup gaps plus an unverified public error-boundary conformance requirement. |
| AR-0011 | Accepted at batch start | In Progress | ER-0009, ER-0011, ER-0012, ER-0013, ER-0014, ER-0052, ER-0063, ER-0064, ER-0065, ER-0066, ER-0083, ER-0084, ER-0087 | Planning | ER-0087 adds the missing Vizier interpretation of operation Emits metadata. |
| AR-0012 | Accepted at batch start | In Progress | ER-0008, ER-0011, ER-0012, ER-0013, ER-0014, ER-0021, ER-0030, ER-0047, ER-0047.1, ER-0047.2, ER-0047.3, ER-0047.4, ER-0052, ER-0054, ER-0063, ER-0064, ER-0065, ER-0066, ER-0084, ER-0085 | Planning | ER-0084 covers missing shell integration; ER-0085 adds the currently uncovered tiling workspace data model and requires a human live pass. |

## ER Queue

| ER | Parent AR | Dependencies | Branch / PR | State | Last verified checkpoint |
|---|---|---|---|---|---|
| ER-0081 | AR-0001 | None | [PR #332](https://github.com/justgus/IrisOS/pull/332) | In Progress | Merged 2026-10-08 as `7c1ee212d5793c1625493f287fe12ab825bab89b`; independent review and local validation passed; fresh CI runs 37809738189 and 37809744722 passed. Awaiting batch reconciliation. |
| ER-0082 | AR-0001 | ER-0081 | [PR #333](https://github.com/justgus/IrisOS/pull/333) | In Progress | Merged 2026-10-08 as `73aef8f41715af4be37ab34080819408bdb43e2f`; independent review and validation passed; current-head CI runs 37826234939 and 37826228791 passed. Awaiting batch reconciliation. |
| ER-0083 | AR-0001, AR-0011 | ER-0081, ER-0082 | [PR #334](https://github.com/justgus/IrisOS/pull/334) | Complete | Merged 2026-10-08 as `0448d974b4bd66921f1cb91a53dbe5277bcf92ea`; independent review and local full suite passed 29/29; current-head CI runs 37834406987 and 37834412357 passed. |
| ER-0084 | AR-0011, AR-0012 | ER-0066, ER-0081, ER-0082, ER-0083 | PR #330 | Approved | Independent planning review passed; shell subprocess integration and human live pass specified. |
| ER-0085 | AR-0012 | ER-0084 | PR #330 | Approved | Independent planning review passed; tiling/nesting model, Automake target, and human live pass specified. |
| ER-0086 | AR-0001 | ER-0081, ER-0082, ER-0083 | PR #330 | Approved | Independent planning review passed; API ownership groups and focused test mappings enumerated. |
| ER-0087 | AR-0011 | ER-0009, ER-0064 | [PR #331](https://github.com/justgus/IrisOS/pull/331) | In Progress | Merged 2026-10-08 as `a484bd3e279ee9592dc7b71c2a0abcc6c91fca60`; independent review, local validation, and CI runs 37792280473 and 37792290137 passed. Awaiting batch reconciliation. |

## Current Item

- Next action: Inspect ER-0084 implementation context and baseline, then implement its shell integration and automated subprocess coverage.
- Last completed action: PR #334 merged at 2026-10-08T19:55:45Z as `0448d974b4bd66921f1cb91a53dbe5277bcf92ea`; current-head CI runs 37834406987 and 37834412357 passed (build, unit tests, and distcheck). The ER-0083 branch was deleted and local `main` was synchronized. Created `codex/er-0084-conch-shell-integration` from merged `main`. ER-0084 remains approved pending implementation; its required human live pass remains pending. Earlier PR #333 merged at 2026-10-08T18:51:56Z as `73aef8f41715af4be37ab34080819408bdb43e2f`; its current-head CI runs 37826234939 and 37826228791 passed.
- Exact validation results: `make -j AM_CXXFLAGS='-Wall -Wextra -Wpedantic -Werror -Wno-unused-const-variable -Wno-unused-private-field -Wno-dangling-gsl'` passed. Focused targets `make -C tests test_service_ipc test_capability_context test_ceo_tasks test_vizier_routing test_exec_integration test_exec_waitables` built successfully; all six executables ran successfully (41 checks, 0 failures) before the no-datagram test was added. Post-fix `make -C tests test_ceo_io_reactor` built successfully, and the test passed under the full Autotools harness. Fresh post-fix `DYLD_LIBRARY_PATH=/Users/justgus/Xcode-Projects/IrisOS/src/.libs TMPDIR=/private/tmp make check AM_CXXFLAGS='-Wall -Wextra -Wpedantic -Werror -Wno-unused-const-variable -Wno-unused-private-field -Wno-gnu-zero-variadic-macro-arguments -Wno-dangling-gsl'` passed all 29 executables (0 failures); the independent reviewer freshly reran it with the same result. Direct sandbox execution could not create Check temp files. To resolve the stale local wrapper, `make -B -C tests test_referee_core` was run; forced refresh invoked `aclocal-1.19 -I m4`, `automake-1.19 --foreign Makefile`, `autoconf`, and `config.status --recheck`. Generated local artifacts `libtool` and `tests/test_referee_core` will be restored before commit; no source-of-truth Autotools input changed.
- Independent review result: ER-0081 and ER-0082 passed. ER-0083 re-review passed all criteria, including the no-datagram assertion; reviewer freshly ran the full suite, 29/29 passed.
- CI result: PR #330 merged; run 37688293301 succeeded. PR #331 runs 37792280473 and 37792290137 passed; PR #332 fresh runs 37809738189 and 37809744722 passed. PR #333 current-head runs 37826234939 and 37826228791 passed. PR #334 current-head runs 37834406987 and 37834412357 passed. Earlier run 37673963672 was canceled after 104 minutes at `Install dependencies`.
- Live pass requirement and human-reported result: ER-0084 and ER-0085 require human live passes; both are pending. Other drafted ERs mark live pass not required.
- Merge result: PR #330 merged at 2026-10-07T21:52:33Z; merge commit `e42f3a9e5cd404534fbac51a8d10687f232f4da7`. PR #331 merged at 2026-10-08T15:00:51Z; merge commit `a484bd3e279ee9592dc7b71c2a0abcc6c91fca60`. PR #332 merged at 2026-10-08T16:44:38Z; merge commit `7c1ee212d5793c1625493f287fe12ab825bab89b`. PR #333 merged at 2026-10-08T18:51:56Z; merge commit `73aef8f41715af4be37ab34080819408bdb43e2f`. PR #334 merged at 2026-10-08T19:55:45Z; merge commit `0448d974b4bd66921f1cb91a53dbe5277bcf92ea`.

## Decisions and Blockers

- The user's selected set is exactly AR-0001, AR-0011, and AR-0012; intervening AR IDs are out of scope.
- GitHub issue synchronization for AR issues #65, #75, and #76 failed with HTTP 403 `Resource not accessible by integration`. The connected GitHub integration can read these issues but does not have permission to update them. Local AR statuses are In Progress; issue labels and bodies remain stale.
- Creating the first planned ER issue (#ER-0081) failed with HTTP 403 `Resource not accessible by integration`; no new ER issues were created. The integration permission block still prevents issue synchronization; retry only if credential or app permissions have changed.
- The local Git credential helper had previously failed with `System.OverflowException` in `GitCredentialManager` (`Interop.Sys.GetGroups`). PR #330 has since been merged by the user and local `main` fast-forwarded.
- GitHub issue synchronization remains blocked: connector writes returned HTTP 403 `Resource not accessible by integration`, and `gh auth status` reported an invalid token. The CI workflow fix was included in merged PR #330 and its required checks passed.
- Publishing ER-0081 was previously blocked locally: normal and escalated pushes failed to resolve `github.com`; using the known address override reached GitHub but Git Credential Manager crashed in `Interop.Sys.GetGroups` with `System.OverflowException`, then Git could not read the HTTPS username. On 2026-10-08 the branch pushed successfully and PR #332 was opened after `gh auth status` recovered.
- SSH transport retry previously failed before authentication with `No user exists for uid 502`; no remote branch or PR was created by that attempt. ER-0081 remains In Progress and dependent ERs remain unstarted pending publication and CI.
- ER-0081 through ER-0083 implement AR-0001's documented lookup-absence gap in ordered storage, Refract, and runtime slices. Normal empty datagram and no-route results remain optional because they are not missing-object lookups.
- AR-0011 and AR-0012's existing ER-0063 through ER-0066 are already Verified. ER-0084 is needed because `bin/conch` does not call the existing `update_session_from_graph` API.
- First independent reviewer found gaps: parent mismatch for ER-0083, no shell-level automated verification in ER-0084, no AR-0012 layout coverage, and missing parent-requirement evidence mapping. The revised plan now links ER-0083 to AR-0011, adds it as an AR-0011 dependency, uses the existing subprocess-capable `test_conch_authoring` target for ER-0084 shell integration coverage, and adds ER-0085.
- Requirement-to-evidence map: AR-0001's `Result<T>` template, `ErrorCode`, and `ErrorCode::NotFound` are defined in `src/referee/referee.h`; focused result/error assertions are in `tests/test_referee_core.cc`. ER-0081 through ER-0083 close public lookup absence gaps. ER-0086 contains the API/failure/test inventory for those public boundaries and requires extending it for any additional cross-module error boundary found during the full named-layer audit.
- Requirement-to-evidence map: AR-0011 type routing and preferred renderer lookup are implemented in `src/vizier/routing.cc` and tested by `test_vizier_routing`; graph relationship routing is covered by ER-0064; task route selection by ER-0065; graph/session materialization by ER-0066; missing artifact errors by ER-0083; shell invocation by ER-0084. Refract serializes operation definitions and effects in `src/refract/schema_registry.cc`, and `test_refract_registry` exercises operation metadata round trips. Since Vizier did not read this metadata, ER-0087 adds a read-only interpretation API with deterministic tests.
- Requirement-to-evidence map: AR-0012 graph observation, routing, and session-to-Concho object creation are covered by ER-0063 through ER-0066; ER-0084 integrates the helper into `bin/conch`; ER-0085 covers the accepted tiling and nested-Concho layout recommendation.

## Completion Summary

- Completed ARs: None.
- Blocked ARs and reasons: None identified yet.
- Completed ERs: None; ER-0081, ER-0082, ER-0083, and ER-0087 are merged and queued for batch reconciliation.
- Outstanding CI, PR, status, or cleanup actions: ER-0084, ER-0085, and ER-0086 implementation and review; human live passes for ER-0084/85; final batch reconciliation.

## ER-0083 Requirement-to-Test Matrix

| Acceptance criterion | Evidence |
|---|---|
| Missing named/type service, memory region, capability context/sandbox, and task return `NotFound`; present records resolve | `tests/test_service_ipc.cc`, `tests/test_capability_context.cc`, and `tests/test_ceo_tasks.cc` cover missing and present cases; implementation in the corresponding registry/store methods. |
| Invalid input remains distinct from absence; wrong stored object kind remains `InvalidArgument` | Empty service name and malformed memory ID assertions in `tests/test_service_ipc.cc`; wrong capability object kind in `tests/test_capability_context.cc`. |
| Missing artifact returns `NotFound`, while valid artifact with no route remains a successful empty optional | `test_spawn_concho_not_found_and_unroutable_artifact` in `tests/test_vizier_routing.cc`. |
| No-datagram receive remains a successful empty optional | `test_conduit_datagram_flow` in `tests/test_ceo_io_reactor.cc` asserts a successful empty result before sending a packet, then verifies the later payload receive. |
| Consumers propagate or handle typed `NotFound` | `await_task` missing-task assertion in `tests/test_exec_integration.cc`; `TaskComms::open_channel` missing-task assertion in `tests/test_ceo_tasks.cc`; service IPC memory lookup assertion in `tests/test_service_ipc.cc`; machine authority callers return the typed context error. |
| Existing service, capability, task, and Vizier behavior stays valid | Focused service, capability, task, await, waitable, and Vizier tests passed; fresh full 29-executable `make check` and independent review passed. |
