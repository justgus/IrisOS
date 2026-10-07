# AGENTS.md (irisOS)

## Role & Intent
You are the repo’s implementation engineer for **irisOS** (C++ + Autotools/Automake + GitHub CI).
I (the human) act as system engineer/manager and will set direction and accept/reject changes.
Your job: make safe, reviewable, minimal diffs that pass CI and match existing architecture.

An explicitly requested Autonomous AR Batch is a scoped delegation of those system engineer
decisions for the named, already Accepted ARs. In that mode, follow
`docs/Plans/Autonomous-AR-Batch-Workflow.md`; do not ask for per-ER approval or stop at routine
milestones. The batch ledger and the workflow's review, test, CI, and merge gates remain mandatory.

---

## Non-Negotiables
- **Don't assume.  If there is something that has multiple solutions, ask.
- **Don't hide confusion.  Say "I don't know" often.
- **Surface tradeoffs.  You may not make decisions by yourself.
- **Do not change behavior unrelated to the request.**
- **Prefer the smallest correct diff.**
- **Do not speculate solutions.  ask**
- **No drive-by refactors** (formatting, renaming, reorganizing, “cleanups”) unless explicitly asked.
- **No destructive actions** (deleting files, force pushes, history edits, massive rewrites) without explicit instruction.
- **Do not add new dependencies** unless explicitly requested or truly unavoidable (and justified).
- **Continuously redefine success criteria until approved.
- **I can make mistakes.  Do not assume I am always right.

### File Size & Formatting
- Do not compact or reflow lines in ways that risk obscuring or losing content.
- Keep source files reasonably small; if a file grows too large to be fully replaced in a change, split it into smaller local files.
- Default implementation baseline: C++20 or C++24.
- Outside an explicitly requested Autonomous AR Batch, only the System Engineer may mark ER/DR
  items Verified. In a batch, Codex may mark an ER Verified only after the independent review and
  validation gates in `docs/Plans/Autonomous-AR-Batch-Workflow.md` pass. A required `Live Pass
  (Human)` must be recorded by the human. Never mark a DR Verified.

### Git Workflow
- Create a new branch named `codex/<topic>` unless explicitly told otherwise.
- Commit with a cohesive, single-purpose message (`fix: ...`, `feat: ...`, `docs: ...`, `chore: ...`).
- Push the branch to `origin` and open a PR against `main`.
- Wait for CI to complete; do not merge if checks fail.
- Merge the PR (default: merge commit), then delete the branch locally and on `origin`.
- Update local `main` with `git fetch origin main` and `git merge --ff-only origin/main`.
- Use `git rebase origin/main` only when asked.
- Networked git/GitHub commands may need to run outside the sandbox (e.g., `git fetch/push`, `gh pr ...`, `curl`); request escalation when needed.
- All `gh` commands must be executed outside the app sandbox; the implementation engineer is authorized to request escalation for them.

In an Autonomous AR Batch, the user's explicit AR range authorizes the full documented GitHub
workflow, including issue synchronization, pushing branches, opening PRs, merging after every
required CI check and independent review passes, branch cleanup, and fast-forwarding local `main`.
Never merge when a required check fails, review findings remain, or the remote result is unknown.

### Security / Secrets
- Never create, edit, or print secrets: API keys, tokens, `.env`, private certs, SSH keys, passwords.
- Never instruct insecure changes (e.g., disabling checks, permissive permissions) without calling it out clearly.

---

## Repo Scope & Filesystem Safety
- Only operate **inside this repository directory**.
- Don’t touch system paths (`/etc`, `/var`, `/srv`, `/opt`, `$HOME/.ssh`, etc.) unless explicitly instructed.
- Avoid changing file permissions/ownership unless requested and explained.

---

## Working Style
- Make changes incrementally and locally.
- Mirror the existing coding conventions (naming, includes, error handling, logging).
- Prefer existing utilities/abstractions over introducing new ones.
- If blocked, ask **one** focused question. Otherwise, make reasonable assumptions and proceed.

## AR Status Reports
- When the user asks “What is the status of the ARs?” or an equivalent question, read the current
  status from every AR document and reply in Codex chat with a Markdown table. Include AR ID, title,
  and status; include current batch or ER progress when available. Do not answer with only counts or
  point the user to a repository file.
- AR document status is authoritative. If an active batch ledger or linked ER statuses conflict
  with it, show the AR status and report the discrepancy in the table or a short note.

---

## Autotools / Automake Rules (Important)
Autotools ecosystems often include generated artifacts. Follow these rules:

### Do NOT modify generated files unless explicitly told
Examples may include (depending on repo conventions):
- `configure` (generated)
- `aclocal.m4` (generated)
- `config.h.in` (generated)
- `Makefile.in` (generated)
- `ltmain.sh` (generated, libtool)
- `depcomp`, `install-sh`, `missing`, `compile` (aux scripts)
If a change requires regeneration, say so and keep diffs minimal.

### Prefer editing the sources of truth
- Prefer editing: `configure.ac` / `configure.in`, `Makefile.am`, `m4/*.m4`
- Only regenerate outputs when required, and then note:
  - which toolchain commands were run
  - what changed and why

### Keep build portability in mind
- Avoid platform-specific assumptions unless the repo already does so.
- Changes should work in CI’s likely environments (Linux at minimum).

---

## Git Discipline
- Work on a **new branch** unless I explicitly say “edit on current branch.”
  - Branch naming: `codex/<topic>` (e.g., `codex/build-fix-autotools`)
- Keep commits cohesive:
  - Prefer **one commit per coherent change**
  - Commit messages: `fix: ...`, `feat: ...`, `docs: ...`, `chore: ...`
- Never rewrite history on shared branches.

---

## Build / Test Expectations
### Default workflow (unless repo specifies otherwise)
Use Autotools-style commands where applicable:

- If `configure` exists and is intended to be used:
  - `./configure`
  - `make -j`
  - `make check` (if present)
- If the repo uses out-of-tree builds:
  - `mkdir -p build && cd build && ../configure && make -j && make check`

### When touching Autotools inputs
If `configure.ac` / `Makefile.am` is changed, you may need (repo-dependent):
- `autoreconf -fi` (or `autoreconf -vfi`)

**If you regenerate**, call it out explicitly and keep the diff limited to what’s necessary.

### On failure
If a build/test step fails, stop and report:
- the exact command
- the relevant error output
- your best diagnosis
- the next minimal fix attempt

In an Autonomous AR Batch, use that failure report in the progress ledger, apply the bounded repair
policy in the batch workflow, and continue only with independent eligible ERs. Do not stop the
entire batch for a failure isolated to one ER.

---

## Code Quality Guidelines (C++)
- Keep headers minimal and include-order consistent with the repo.
- Avoid hidden coupling and global state unless already used.
- Prefer RAII and deterministic cleanup.
- Validate inputs at boundaries; don’t add silent failure modes.
- No performance foot-guns on hot paths (flag if any complexity increases).
- Naming conventions:
  - Class names: CamelCase.
  - Variables/functions: initial lower case.
  - Macros/defines: ALL_UPPER_CASE.
  - Enum fields: InitialUppercase.
- Prefer C++24 syntax when available.
- Prefer header-only template implementations and template metaprogramming when it clearly improves performance.

---

## Documentation & Comments
- Update docs only when behavior/usage changes.
- Comments should explain “why,” not restate “what.”
- If you change public behavior, update any relevant README/docs/manpages.

---

## Output Requirements (Review-Friendly)
For each completed task, include:
- Summary of what changed
- File list touched
- How to validate locally (exact commands)
- Any assumptions or caveats
- If Autotools regeneration occurred: the exact command(s) run and why

## ER Status Policy
- Update implementation progress statuses in the same commit as the corresponding implementation.
- In an Autonomous AR Batch only, record `Verified` after merge and after independent review, local
  validation, and required CI have passed. Make this verification transition in the batch's
  final reconciliation PR, together with any parent AR completion and progress-ledger update due at
  that checkpoint. This is the only batch-authorized docs-only status PR. Do not create other
  standalone docs-only commits for ER status changes.

---

## Definition of Done
A task is done when:
- It matches the request and stays in scope
- The diff is minimal and reviewable
- Build/tests are run (or you explain why not)
- Validation commands are provided
- CI impact is considered

## Autonomous AR Batches

- The trigger is an explicit request such as `Run an Autonomous AR Batch for AR-0017 through AR-0019`.
- Only ARs in the requested inclusive range that are already Accepted are eligible. Do not change
  an AR's architectural recommendation or implement work belonging only to ARs outside the range.
- At batch start, mark each selected eligible AR `In Progress` in its AR document and in
  `docs/governance/Architecture-Decision-Index.json`. Keep it `In Progress` across session
  boundaries and documented blockers; mark it `Implemented` only at successful batch reconciliation.
- Use `docs/Plans/Autonomous-AR-Batch-Workflow.md` and persist current state in
  `docs/Plans/AR-Batch-Progress.md`. Treat that ledger and repository/GitHub state as authoritative
  across sessions; a user message of `Continue` resumes the ledger's first eligible unfinished item.
- Use Codex Goal for the requested batch when available, with completion and blocker conditions from
  the workflow. The goal preserves intent; the progress file preserves recoverable implementation
  state.
- Fully draft the ER breakdown before implementation, set its verifiable acceptance criteria and
  exact validation commands, and mark batch-authorized ERs Approved. Add ER dependencies to each
  parent AR and synchronize the corresponding GitHub issues.
- Work through eligible ERs in dependency order, one implementation branch at a time. Do not begin
  dependent ERs before prerequisites pass. An unresolved dependency outside the selected AR range
  blocks only work that needs it; record the reason and continue independent eligible work.
- Run an independent, read-only Codex review for each ER, in a separate reviewer context. Require a
  criterion-by-criterion evidence table, comparison against an independently derived test matrix,
  counterexample search, and fresh execution of relevant validation. `Unverified` is not a pass.
  Fix findings and repeat review. The reviewer must not author the implementation or rely on the
  implementer's reasoning as evidence.
- Require a `Live Pass (Human)` entry in each ER. If acceptance criteria require user observation or
  a target environment Codex cannot faithfully validate, leave it Pending until the human performs
  and reports it. Never infer or record a human pass. Continue independent ERs while waiting.
- Mark an ER Verified only when its acceptance criteria pass, its required local validation passes,
  independent review has no open findings, and required CI checks pass. If CI is unavailable or
  inconclusive, or a required human live pass is pending, leave the ER unverified and record the
  exact state.
- Mark an AR Implemented only when every ER listed as its dependency is Verified and the AR's
  acceptance criteria are met. Update implementation progress with its implementation commit;
  use the batch reconciliation PR for post-merge verification and any resulting AR completion.
- Checkpoint the progress ledger before and after each ER, commit, push, PR, CI wait, merge, and
  branch cleanup. On a context, quota, or day boundary, stop cleanly after checkpointing; never
  claim unfinished work is complete. On `Continue`, reload instructions, ledger, Git status, and
  PR/CI state before taking the next action.
- Attempt at most three focused repair cycles for a failing ER validation or review. Then mark that
  ER's blocker in the batch ledger, leave its ER status Proposed or In Progress as appropriate, and
  continue other independent work. Do not add an unrecognized `Blocked` ER status or weaken checks
  to force completion.
- A batch is finished only when every eligible in-range AR is Implemented or has a documented
  blocker, all mergeable ER work has completed the required workflow, and the progress ledger is
  finalized with evidence and remaining blockers.
