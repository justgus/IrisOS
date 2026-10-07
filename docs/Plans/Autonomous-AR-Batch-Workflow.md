# Autonomous AR Batch Workflow

## Purpose

This workflow lets Codex carry a user-selected range of already Accepted Architecture
Recommendations (ARs) through Engineering Request (ER) planning, implementation, independent
review, validation, GitHub delivery, and architecture status updates. It is intended to continue
across conversation, context, quota, and calendar-day boundaries.

The user starts a batch by explicitly naming its inclusive AR range. That request delegates the
per-ER approval and verification steps described here for those ARs. It does not authorize changing
the accepted architecture, working on unrelated ARs, bypassing checks, or concealing blockers.

## Starting a Batch

Use a request such as:

> Run an Autonomous AR Batch for AR-0017 through AR-0019. Follow
> `docs/Plans/Autonomous-AR-Batch-Workflow.md`. Derive the ERs, implement and validate eligible
> work, obtain independent review, update statuses, and complete the GitHub workflow. Continue
> without asking me at routine milestones. Stop only for the documented blocked conditions.

If Codex Goals are available, create one Goal for this bounded batch. Its completion condition is:

> Every eligible AR in the requested range is Implemented, or has a specific documented blocker;
> every ER completed by the batch has independent review and passing required validation; and all
> mergeable work has completed the GitHub workflow.

The active batch file is `docs/Plans/AR-Batch-Progress.md`. Create it at batch start and treat it as
the durable checkpoint across sessions. Do not overwrite an unfinished batch. If one already
exists, resume it and do not start a different range until it is complete or explicitly abandoned.

## Eligibility and Scope

1. Parse the requested range as inclusive. Enumerate existing AR IDs in that range.
2. ARs currently marked `Accepted` are eligible. Mark ARs already `Implemented` or `Done` as
   complete; report missing IDs and all other statuses in the progress file. Do not silently treat
   Proposed ARs as approved work.
3. Read each eligible AR, its stated dependencies, linked ERs, acceptance criteria, known
   conformance gaps, and relevant existing code before defining work.
4. Keep implementation inside the selected AR range. A dependency outside the range may be read and
   reported, but is not authority to implement that other AR. Mark only dependent work blocked and
   continue independent in-range work.
5. Resolve routine implementation details in the narrowest way consistent with the accepted AR,
   existing architecture, and tests. Record assumptions. If two materially different designs remain
   and neither is supported by the accepted direction, mark the affected work blocked with the
   precise decision needed; continue unrelated work.

## ER Planning

Before implementation begins:

1. Inspect existing ERs and the ER Status Ledger to find related, incomplete, or already delivered
   work. Do not duplicate an existing ER.
2. Break the eligible AR work into the smallest independently implementable ERs that can be
   reviewed and validated. Order them by dependencies and avoid bundling unrelated changes.
3. Draft each ER from `docs/ER/ER-Template.md`. Include the parent AR, concrete acceptance criteria,
   exact local validation commands, dependency IDs, and any required documentation or migration.
4. The user-selected AR range authorizes these derived ERs. Set them to `Approved` after checking
   that their scope implements the accepted AR without extending it. Update parent AR dependency
   lists and the ER Status Ledger.
5. Run a separate, read-only Codex planning review against the parent ARs. It must check coverage,
   scope, dependencies, acceptance criteria, and whether the validation commands can prove those
   criteria. Fix its findings before implementation. If no independent review context is available,
   leave the batch in Planning and record the blocker; do not treat self-review as equivalent.
6. Synchronize each ER with GitHub using the repository's established issue workflow. Record issue
   IDs and links in the progress file.
7. Commit the coherent ER-planning changes, open a planning PR, wait for required CI, and merge it
   before starting implementation. This preserves the authorized work definition if a session
   ends or implementation needs to resume later.

## Implementation and Validation Loop

Process one ER at a time, in dependency order:

1. Create a new `codex/<topic>` branch from current `main` for the ER.
2. Update the progress file with the active ER, branch, acceptance criteria, and next checkpoint.
3. Inspect the affected code and run a relevant baseline check when practical. Implement only the
   ER's scope. Add or update automated tests for changed behavior.
4. Run the exact commands in the ER verification plan and the repository-required build/test
   workflow. Record command, exit status, and relevant output in the progress file. Do not describe
   a check as passing unless it actually completed successfully.
5. If a check fails, make at most three focused repair attempts. If it still fails, leave an
   unstarted ER Proposed or an active ER In Progress, record the blocker and evidence in the batch
   ledger, and move to independent eligible work. Never invent a `Blocked` ER status or weaken a
   test to make the batch pass.

## Independent Review Gate

After local validation and before opening the PR, run a separate Codex review in a fresh reviewer
context. Use a reviewer/subagent with read-only access to source and documentation; it may run the
specified validation commands but must not edit implementation files. Give it the ER, acceptance
criteria, full diff, and test output. Do not supply the implementer's reasoning as a substitute for
the artifact evidence. If the active Codex surface cannot provide an independent reviewer context,
do not claim the gate passed or mark work Verified; record the limitation in the batch ledger.

The review must report concrete findings with file and line references, or explicitly report no
findings. It checks:

- every acceptance criterion against the resulting code and docs;
- scope and consistency with the parent AR and existing architecture;
- correctness risks, regressions, error paths, and security/capability boundaries;
- whether tests exercise changed behavior and meaningful failure cases;
- whether validation evidence corresponds to this branch and current diff.

Fix every valid finding and repeat independent review and affected tests. A disagreement is resolved
by evidence from the AR, ER, code, and validation. After three repair cycles, leave the ER blocked
with the unresolved finding and continue independent work. A review with unresolved findings never
passes the gate.

## GitHub Completion and Status Changes

1. Commit the implementation, tests, review fixes, and implementation-progress status (normally
   `Complete`) together as a cohesive change, following repository conventions.
2. Push the branch and open a PR against `main`. Include the AR/ER issue references, change summary,
   exact local validation results, and independent review result.
3. Wait until every required CI check reaches a final state. A pending, missing, failed, cancelled,
   or unqueryable required check is not a pass. Record its state and stop merging that ER.
4. Merge only when local validation passed, independent review has no open findings, all required
   CI checks passed, and the PR targets the expected base. Then delete the branch locally and on
   origin and fast-forward local `main` as prescribed in `AGENTS.md`.
5. After the implementation PR is merged, queue its ER for final verification; do not yet mark it
   `Verified`. Keep its review and validation evidence in the progress file.
6. At the end of the batch, open one reconciliation PR that marks eligible merged ERs `Verified`,
   updates `docs/ER/ER-Status.md`, and marks an AR `Implemented` only when its recommendation is
   realized and every ER in its `ER-Dependencies` list is Verified. Include the progress ledger and
   evidence references in this PR. This is the only batch-authorized docs-only status PR. Run
   required CI and an independent read-only review of the reconciliation; merge only when both
   pass. If any required verification gate is missing, keep those ERs Complete and leave the
   affected AR Accepted. Do not mark an AR Implemented if a required dependency is blocked,
   incomplete, or outside the authorized batch.

If a required GitHub action cannot be completed because credentials, permissions, network, CI, or
repository policy prevents it, record exact evidence and leave affected statuses honest. Continue
other in-range ERs that do not depend on the blocked action.

## Checkpointing and Resume

Update `docs/Plans/AR-Batch-Progress.md` before and after each material transition: ER drafting,
approval, branch creation, implementation, validation, review, commit, push, PR creation, CI result,
merge, and cleanup. Write the checkpoint before waiting on a long operation when possible. A
checkpoint must be understandable without the original conversation.

At the start of every session, including after quota renewal or a context reset:

1. Read `AGENTS.md` and this workflow.
2. Read `docs/Plans/AR-Batch-Progress.md`.
3. Inspect Git status, current branch, latest commits, and any recorded GitHub issue/PR/CI state.
4. Reconcile the file with actual repository and GitHub state. GitHub and Git are authoritative for
   whether external actions completed; update the checkpoint if an action completed just before
   interruption.
5. Resume the first eligible unfinished ER or transition. Do not repeat completed work or assume a
   command succeeded merely because it was started.

The user may resume with `Continue`. A Codex Goal helps maintain the objective within a continuing
thread; the progress file is the recovery source when a new conversation is needed. No work can
execute while Codex is unable to run due to quota exhaustion; after access returns, `Continue`
resumes from the last reconciled checkpoint.

## Progress File Template

Create `docs/Plans/AR-Batch-Progress.md` at batch start with:

```md
# Autonomous AR Batch Progress

- Requested range:
- Started:
- Current state: Planning | In Progress | Waiting | Complete | Partially Blocked
- Goal/thread reference:

## AR Queue

| AR | Eligibility | ERs | State | Blocker / evidence |
|---|---|---|---|---|

## ER Queue

| ER | Parent AR | Dependencies | Branch / PR | State | Last verified checkpoint |
|---|---|---|---|---|---|

## Current Item

- Next action:
- Last completed action:
- Exact validation results:
- Independent review result:
- CI result:
- Merge result:

## Decisions and Blockers

- Record each resolved assumption or blocker with links and evidence.

## Completion Summary

- Completed ARs:
- Blocked ARs and reasons:
- Completed ERs:
- Outstanding CI, PR, status, or cleanup actions:
```

When a batch is complete, preserve this file as the final audit record. A later batch archives the
completed record under `docs/Plans/AR-Batches/` before creating a fresh active progress file.
