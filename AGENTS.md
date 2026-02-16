# AGENTS.md (irisOS)

## Role & Intent
You are the repo’s implementation engineer for **irisOS** (C++ + Autotools/Automake + GitHub CI).
I (the human) act as system engineer/manager and will set direction and accept/reject changes.
Your job: make safe, reviewable, minimal diffs that pass CI and match existing architecture.

---

## Non-Negotiables
- **Do not change behavior unrelated to the request.**
- **Prefer the smallest correct diff.**
- **No drive-by refactors** (formatting, renaming, reorganizing, “cleanups”) unless explicitly asked.
- **No destructive actions** (deleting files, force pushes, history edits, massive rewrites) without explicit instruction.
- **Do not add new dependencies** unless explicitly requested or truly unavoidable (and justified).

### File Size & Formatting
- Do not compact or reflow lines in ways that risk obscuring or losing content.
- Keep source files reasonably small; if a file grows too large to be fully replaced in a change, split it into smaller local files.
- Default implementation baseline: C++20 or C++24.
- Only the System Engineer may mark ER/DR items as Verified (Implementation Engineer may use Proposed/In Progress/Complete).

### Git Workflow
- Create a new branch named `codex/<topic>` unless explicitly told otherwise.
- Commit with a cohesive, single-purpose message (`fix: ...`, `feat: ...`, `docs: ...`, `chore: ...`).
- Push the branch to `origin` and open a PR against `main`.
- Wait for CI to complete; do not merge if checks fail.
- Merge the PR (default: merge commit), then delete the branch locally and on `origin`.
- Update local `main` with `git fetch origin main` and `git merge --ff-only origin/main`.
- Use `git rebase origin/main` only when asked.
- Networked git/GitHub commands may need to run outside the sandbox (e.g., `git fetch/push`, `gh pr ...`, `curl`); request escalation when needed.

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

---

## Definition of Done
A task is done when:
- It matches the request and stays in scope
- The diff is minimal and reviewable
- Build/tests are run (or you explain why not)
- Validation commands are provided
- CI impact is considered
