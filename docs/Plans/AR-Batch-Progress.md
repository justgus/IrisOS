# Autonomous AR Batch Progress

## Current Follow-on Audit — 2026-10-20

- Resumed from `docs/NOTE-FOR-CODEX.md`: reassess every AR/ER completion claim against architecture.
- [Audit report](AR-Implementation-Audit-2026-10-20.md) inventories 27 ARs and 115 ERs, separates
  concrete gaps from accepted narrow scope, and records executable storage/identity/generic/dispatch
  counterexamples plus actual Conch output. This is not a fresh verification of every ER.
- The System Engineer directed AR-0001–0004, AR-0009, AR-0014, AR-0016–0018, and AR-0020–0021
  to Pending Review on 2026-10-20. Canonical documents and architecture index agree; review of
  remaining work is pending. ER statuses and prior verification evidence are preserved.
  No implementation repair started.
- Baseline AR-0027 implementation CI 39052974745 is now confirmed completed/success for 278ae38.
- The completed AR-0022–0024 batch record below remains historical evidence for its accepted scope.

- Requested range: AR-0022 through AR-0024; implementation and independent verification.
- Started: 2026-10-14.
- Current state: Complete; all selected ARs Implemented and all listed dependency ERs Verified.
- Process: Temporary Initial Development Mode; main, cohesive commits, direct pushes, advisory CI.
- Goal: complete eligible accepted scope and independent verification; preserve architectural and human-observation gates.
- Prior reconciliation is preserved in [the archive](AR-Batches/AR-reconciliation-2026-10-14-before-0022-0024.md).
- The System Engineer explicitly requested this batch, including resuming AR-0024's unfinished verification.

## Context-Renewal Handoff (Historical)

- Paused explicitly by the System Engineer on 2026-10-15 to exit and renew context. The Codex Goal
  is Paused, not Complete. Do no additional batch implementation until the System Engineer resumes.
- Work directly on main under Temporary Initial Development Mode. No branches, worktrees, or PRs.
- Repository was clean at a3ee08ac40ebc8e34547c936e8544d704723ce31 before this handoff update.
  Both implementation commits below were pushed to origin/main. This checkpoint is documentation only.
- Original scope remains full implementation and independent verification of remaining work in
  AR-0022 through AR-0024. All three ARs remain In Progress; do not shrink the objective to the
  two repairs already delivered or infer architectural approval from a generic Continue.

### First Actions on Resume

1. Read AGENTS.md, docs/Plans/Autonomous-AR-Batch-Workflow.md, this ledger, the three canonical ARs,
   and the relevant ERs. Inspect git status/current branch/log and reconcile origin/main before edits.
2. Query the existing implementation CI run, rather than starting another run:
   `gh run view 38759330650 --json headSha,status,conclusion,url,jobs`.
   Execute gh outside the sandbox. Verify headSha is a3ee08ac40ebc8e34547c936e8544d704723ce31.
3. At the last check, run 38759330650 was in_progress: bootstrap/configure passed, Build running,
   Unit tests and Distcheck pending. URL: https://github.com/justgus/IrisOS/actions/runs/38759330650.
   No failure or pass is inferred before terminal evidence. A transient observation failure does not
   stop the remote run. The prior local watcher session 20329 is optional and not required to resume.
4. If CI fails, inspect the actual failed step/log and apply the documented bounded repair policy.
   If it succeeds, reconcile ER-0103 verification using its criterion-level independent report,
   passing local/full tests, Not Required live-pass decision, and exact-head CI evidence. Temporary
   Mode permits the resulting reconciliation directly on main; no separate PR is required.
5. Keep ER-0070–0074 Complete until the System Engineer resolves their existing human-observed gate.
   Do not interpret ER-0057's prior confirmation as verification of these unrelated ERs.
6. Resolve the three pending architectural/human-gate rulings below. ER-0100–0102 remain Proposed;
   their linked issues do not mean approval. Then implement eligible ERs in dependency order with
   independent review for security/lifetime/public API changes, and reconcile only proven statuses.

### Published Work and Evidence

- 982379e414cd4a887520f8bff1f22a1d3770e708: validates known descriptor kind/existence at Machine lease
  creation and rejects invalid access modes. MachineLeaseRegistry now requires `(inventory, contexts)`.
  Independent focused suite 5/5, 38 checks; full suite 31/31. Linux CI run 38751271580 passed bootstrap,
  configure, build, unit tests, and distcheck. Review: AR-Batches/reviews/AR-0024-independent-review-2026-10-15.md.
- a3ee08ac40ebc8e34547c936e8544d704723ce31: ER-0103 rejects malformed persisted capability containers
  and tests omitted/empty/valid requirements, file-backed reopen, and inherited listing. Local build
  passed; full suite 32/32; independent focused suite 4/4 programs, 55 checks, no findings.
  Review: AR-Batches/reviews/ER-0103-independent-review-2026-10-15.md. ER-0103 is now Verified; see the resume reconciliation below.
- Exact commands are preserved in the ERs/review reports. For ER-0103:
  `make -j4`,
  `env TMPDIR=/private/tmp make -C tests check TESTS='test_refract_capabilities test_refract_registry test_refract_bootstrap test_conch_authoring'`,
  and `env TMPDIR=/private/tmp make check`.
- ER-0103 added tests/test_refract_capabilities.cc and its tests/Makefile.am target. Local
  `./bootstrap.sh` and `./configure` succeeded. Generated tracked libtool/tests/test_referee_core
  were restored before publication; no generated artifacts or unfinished source changes remain.
- Check tests fail before assertions in the sandbox when communication files cannot be created;
  prior successful escalated executions are recorded. Do not misreport this as an assertion failure.
- Independent reviewer agents may not survive context renewal. Their committed reports are the
  evidence; do not claim new reviews from remembered conversations or merely rerun all passed gates.

### Historical Rulings Requested (Resolved 2026-10-15)

- AR-0022: approve or revise initial Stage E scope: in-process owned-buffer allocation with total,
  per-subject, and count limits plus owner checks; local Comms loopback network service; read-only
  caller-filtered capability/sandbox inspection. No cross-host transport, raw memory addresses,
  grant issuance, or new OS sandbox enforcement is assumed. Drafts ER-0100/#354, ER-0101/#355,
  ER-0102/#356 contain concrete criteria. These proposals have NOT been approved.
- AR-0023: does persisted operation required_capabilities satisfy the undefined extended policy
  metadata requirement, or is a broader model intended? ER-0103 fixes the existing format only;
  it does not answer this architectural question or complete the parent AR.
- AR-0024: may independent API/code review and executable tests replace the blanket Pending human
  live-pass language in ER-0070–0074? Original criteria are locally testable and hardware is excluded,
  but AGENTS.md preserves existing human gates until a ruling or reported human observation arrives.
- These were the original unanswered questions. The explicit System Engineer response recorded
  below resolved all three; do not request these rulings again.

## AR Queue

| AR | Eligibility | Status | Remaining work | Evidence / blocker |
|---|---|---|---|---|
| AR-0022 | Accepted; selected | Implemented | None | All dependencies Verified; approved Stage E and corrected-head CI pass |
| AR-0023 | Accepted; selected | Implemented | None | All dependencies Verified; policy scope accepted |
| AR-0024 | Existing In Progress; explicitly resumed | Implemented | None | ER-0070–0074 Verified; automated evidence accepted |

## ER Queue

| ER | Parent | State | Next step |
|---|---|---|---|
| ER-0100 | AR-0022 | Verified | Published a53cf93; exact-head CI 38811469029 success |
| ER-0101 | AR-0022/0024 | Verified | Corrected-head CI 38831614893 success; independent review Pass |
| ER-0102 | AR-0022 | Verified | Corrected-head CI 38831614893 success; independent review Pass |
| ER-0103 | AR-0023 | Verified | None; exact-head CI passed |

- ER-0057–0062, ER-0055/0078–0080, and ER-0070–0074 are Verified.
- Separate read-only review is required for each Stage E service's public/security/lifetime boundary.

## Current Item

- Implementation is published on main at `6e5677ae522e4b22de42931fae7e0604d6f503cc`.
- Corrected-head CI [38831614893](https://github.com/justgus/IrisOS/actions/runs/38831614893)
  completed success on 2026-10-16, including build, unit tests and distcheck.
- ER-0100–0103 Verified; AR-0022–0024 Implemented. Independent reviews have no open findings;
  no architectural ruling, human gate, or implementation blocker remains for the selected scope.
- Final documentation/issue reconciliation completes this batch directly on main. No source or
  generated output changed, no local tests repeated, and no Autotools regeneration performed.

## Decisions and Blockers

- No unresolved architectural ruling. Stage E scope is approved; broader remote networking,
  physical memory, grant mutation, and OS sandbox enforcement remain outside the selected scope.
- AR-0024 Independent Live Pass is Not Required per System Engineer acceptance of automated evidence.

## Completion Summary

- AR-0024 repair 982379e, independent implementation review, and CI passed. ER-0070–0074 are
  Verified following the System Engineer acceptance of automated evidence.
- ER-0103 Verified, published as a3ee08a; parent validation, independent review, and exact-head CI passed.
- AR-0022–0024 are Implemented; ER-0101/0102 verification closes approved Stage E using
  completed corrected-head CI and the existing independent/local validation evidence.

## Independent Findings and Repair (2026-10-15)

- AR-0024 fresh independent baseline: five programs, 37 checks passed. Reviewer then reproduced
  forged aggregate handles yielding leases/transports for absent or wrong-kind descriptors, and
  invalid mode acceptance with a matching malformed grant. ER-0072/0073 review fails pending repair.
- ER-0072 repair plan and acceptance criteria updated before implementation. Registry construction
  now requires inventory and uses the existing handle factory before publishing a lease; factory
  rejects invalid modes. Regression tests cover absent descriptor, wrong kind, RAM-as-IO, and all
  invalid handle modes. Validation and independent re-review pending.
- AR-0023 independent baseline: three programs, 52 checks passed. New policy-specific gaps are
  reopened-store capability assertions and malformed capability-container rejection. Scope ruling pending.
- Issue sync encountered `scripts/issue_sync.sh: line 180: dr_deps[@]: unbound variable` after
  updating AR-0022 title/body/labels. Final issue state must be checked before claiming sync complete.

### Repair validation checkpoint

- `make -j4`: exit 0; no Autotools regeneration needed.
- `TMPDIR=/private/tmp make -C tests check TESTS='test_machine_authority test_comms_transport_session test_machine_primitives test_machine_inventory test_comms_primitives'`:
  sandbox exit 2 before assertions, Check communication temp-file error. Escalated rerun with
  `env TMPDIR=/private/tmp` passed 5/5 programs (38 checks after the new regression).
- `env TMPDIR=/private/tmp make check`: passed 31/31 programs after normal relinking fixed the
  baseline core-test dylib loader failure. No system library installation or DYLD workaround used.
- Independent reviewer recompiled its separate counterexample: missing descriptor factory, missing
  descriptor lease, unauthorized transport from that lease, wrong-kind lease, invalid factory mode
  all reject. Final independent criterion review passed; see [report](AR-Batches/reviews/AR-0024-independent-review-2026-10-15.md).
- GitHub read-back confirms AR-0022 #290 and AR-0023 #291 are OPEN with status:in-progress despite
  issue_sync's empty-array Bash failure. No unrelated script repair is included.

### Independent review completion

- AR-0024 implementation criteria all Pass after the bounded repair. Fresh reviewer suite passes
  5/5 programs (38 checks), full suite passes 31/31. No open implementation findings.
- [Criterion-level review](AR-Batches/reviews/AR-0024-independent-review-2026-10-15.md) preserves
  counterexamples, repair evidence, commands, and limits. This is not a human live pass.
- ER-0070–0074 remain Complete until the pending human-gate ruling/observation and CI state are
  resolved. No AR completion or ER Verified transition is inferred.
- Source files touched: src/machine/authority.{h,cc}; tests/test_machine_authority.cc and
  tests/test_comms_transport_session.cc. The public registry constructor now requires inventory.
- Generated tracked tests/test_referee_core will be restored before committing; no generated
  artifacts or Autotools regeneration are part of this repair.

### Planning publication checkpoint

- Proposed Stage E issue sync completed: ER-0100 [#354](https://github.com/justgus/IrisOS/issues/354),
  ER-0101 [#355](https://github.com/justgus/IrisOS/issues/355), ER-0102
  [#356](https://github.com/justgus/IrisOS/issues/356). Proposed scope remains unapproved pending
  the architectural ruling; do not implement dependent work or infer approval from elapsed time.
- Three System Engineer questions pending: AR-0023 required_capabilities scope; bounded Stage E
  allocation/loopback/inspection scope; AR-0024 human live-pass applicability.
- Next action: incorporate rulings, approve eligible ERs, implement in dependency order, and
  independently review security/lifetime changes. While waiting, publish the validated AR-0024 fix.

## Continuation — 2026-10-15

- Prior turn classification: Progress (982379e published the independent-review repair and ER plans).
- Current worktree clean at 982379e414cd4a887520f8bff1f22a1d3770e708 on main; instructions and ledger reloaded.
- Exact-head advisory CI run [38751271580](https://github.com/justgus/IrisOS/actions/runs/38751271580)
  is in_progress at this check. Pending is not a pass.
- Architectural questions remain unanswered; no response or approval is inferred from continuation.
- Available independent work: ER-0103 is fully drafted and Approved by batch delegation to repair
  the existing capabilities encoding boundary. It does not decide the disputed policy profile.
- Requirement/test mapping: raw non-array/non-string metadata -> typed CorruptData; absent/empty/valid
  arrays -> compatibility; file-backed reopen -> exact multiple-grant preservation and inherited
  listing; focused Refract/Conch suites -> existing bootstrap/dispatch regression coverage.
- Next action: implement ER-0103, validate, independently review, publish; then revisit CI and rulings.

### ER-0103 implementation checkpoint

- Independent planning review approved the existing-format repair and requirement/test mapping.
- Issue created: ER-0103 [#357](https://github.com/justgus/IrisOS/issues/357).
- Dedicated tests reproduce the defect before the fix: three checks, one failure accepting null
  capabilities; omitted/empty/string-array compatibility and file-backed reopen tests already pass.
- Decoder now rejects non-array containers and non-string entries with CorruptData. No grant
  naming or authorization semantics change, and no policy scope ruling is inferred.
- New test target required `./bootstrap.sh` and `./configure`; both passed. Tracked generated
  libtool output will be restored; generated Autotools outputs are not included in the commit.
- Previous repair CI [38751271580](https://github.com/justgus/IrisOS/actions/runs/38751271580)
  completed success for 982379e, including bootstrap/configure/build/unit tests/distcheck.
- ER-0070–0074 still await the existing human-gate applicability ruling; green CI does not supply it.

### ER-0103 implementation handoff

- Parent build passed; focused suite 4/4 programs; full suite 32/32. Independent focused rerun
  passes 4/4 programs and 55 checks, with all ER criteria Pass and no findings. See
  [review](AR-Batches/reviews/ER-0103-independent-review-2026-10-15.md).
- ER-0103 transitions In Progress -> Complete with this implementation commit. Verified awaits
  implementation CI. AR-0023 remains In Progress pending both this dependency and the policy ruling.
- Exact local commands are in ER-0103; no unrelated source behavior changed. New target integration
  required `./bootstrap.sh` and `./configure`; both passed, generated tracked output excluded.
- Architectural and existing human-gate questions remain pending. ER-0100–0102 stay Proposed.

## Resume Reconciliation — 2026-10-15

- Read current instructions, workflow, ledger, selected ARs, Stage E proposals, ER-0103 and its
  independent review. The explicit continuation ends the prior context-renewal pause.
- `git fetch origin main` passed; `git merge --ff-only origin/main` reported Already up to date
  after escalation for a sandbox-blocked ORIG_HEAD update. Worktree was clean at 719d4c2.
- `gh run view 38759330650 --json headSha,status,conclusion,url,jobs` confirmed completed/success
  for a3ee08ac40ebc8e34547c936e8544d704723ce31. All implementation checks passed.
- Files changed by reconciliation: this ledger, ER-0103, and ER-Status. No implementation or
  generated artifacts changed; no Autotools regeneration performed. Prior completed validation
  is retained without claiming a new test run.
- Remaining architectural and human-gate decisions were presented together through async input.
  Pending answers do not authorize the dependent implementation or verification transitions.
- `bash scripts/issue_sync.sh docs/ER/ER-0103-Refract-Capability-Metadata-Validation.md`
  completed successfully; existing issue #357 is synchronized and remains closed.
- `git diff --check` passed for the three documentation files. Reconciliation is ready for
  direct publication on main under Temporary Initial Development Mode.

## System Engineer Rulings and Execution — 2026-10-15

- Explicit response: "AR-0022 approve. AR-0023 I believe it does. AR-0024 accept."
- Stage E ER-0100–0102 are Approved; the proposed bounded scope is unchanged.
- Persisted required_capabilities satisfies the accepted extended policy scope. AR-0023 is
  Implemented with all listed dependencies Verified.
- AR-0024 automated independent review/tests accepted in place of the earlier human gate.
  ER-0070–0074 are Verified; AR-0024 Implemented. No human observation is claimed.
- Earlier pending-ruling records are historical and superseded by this explicit response.
- Next: ER-0100 owned allocations, then ER-0101 network service and ER-0102 inspection.
  Separate read-only review is required for each new public/security/lifetime service boundary.
- Allocation criterion mapping: owned zero buffers -> API plus IPC; owner/correlation -> IPC;
  byte/count quotas -> boundary/release tests; numeric/selectors -> malformed requests;
  recovery -> file-backed host reopen and explicit rebind.

### ER-0100 publication checkpoint

- Focused independent tests 4/4 programs, 24 checks; full parent suite 33/33. Review Pass after
  one exception-boundary repair and added allocation-failure test. Detailed report committed.
- ER-0100 In Progress -> Complete; CI pending, no Verified claim. AR-0023/0024 reconciliation
  independently reviewed Pass using prior exact-head CI and explicit System Engineer rulings.
- Stage E issues #354–356 synchronized; ER-0070–0074 issues closed as Verified; AR-0023/#291
  and AR-0024/#292 synchronized and closed. AR-0022/#290 remains open with expanded dependencies.
- Files: allocation service/header, internal request validation, core service tests, Automake inputs,
  Service-Hosts usage, review report, affected AR/ER/status/index/ledger documents.
- Regeneration: `./bootstrap.sh`, `./configure` for new library sources/test target; generated
  tracked libtool and tests/test_referee_core restored. Next implementation: ER-0101.

### ER-0101 implementation start

- ER-0100 plus accepted AR reconciliation published directly on main as a53cf93.
- ER-0101 Approved -> In Progress: constructor-bound copied protocol/transport plus borrowed
  schemas/leases, owner/lease checks on every request, bounded retained session records, and
  local registered packet round trip. Closed records remain inspectable and count toward the bound.
- Requirement mapping: host-backed IPC/open/inspect -> metadata and envelope tests; packet
  empty/binary/max-size -> exact round trips; close/double-close/count -> lifecycle tests;
  cross-owner/revocation/descriptor/incompatible -> authority tests; malformed selectors/payloads
  -> no session publication and retained open state. New dedicated network test target added.
- Independent Live Pass Not Required under the approved bounded local scope. Independent
  public API/security/lifetime review required after local validation.

### ER-0101 publication checkpoint

- New network service/header, dedicated test, Automake integration, Service-Hosts usage, ER/status,
  ledger, and independent report. Governance notes corrected for the accepted AR-0023/0024 rulings.
- Build and focused independent 6/6 programs (39 checks) passed; full parent suite 34/34 passed.
  Reviewer `review_network` reports all six criteria Pass, no open findings.
- ER-0101 In Progress -> Complete; no Verified transition before implementation CI. Next ER-0102.
- a53cf93 allocation CI run 38811469029: build and unit tests passed, distcheck still running at
  last observation. Reconciliation commit fe9beba CI run 38806640740 completed success.
- Local `./bootstrap.sh` and `./configure` required for new module/test target; generated tracked
  libtool and tests/test_referee_core restored before commit.

### ER-0102 implementation start

- ER-0101 published as cc54489. ER-0102 Approved -> In Progress on main.
- Read-only service uses persisted caller-filtered store queries on every request. Empty object
  payloads only; sandbox output contains the caller's membership, never other subject IDs.
- Requirement mapping: contexts/sandbox queries -> two-subject privacy and ordering; selector and
  subject override -> malformed IPC; recovery -> file-backed host reopen/rebind/start; corruption
  and closed store -> propagated errors; persisted record counts/values -> no mutation.
- Independent public API/privacy/persistence/lifetime review required; live pass Not Required.

### ER-0102 publication checkpoint

- Files: new inspection service/header, dedicated privacy/recovery/error tests, Automake inputs,
  Service-Hosts usage, ER/status/ledger and independent review report. ER-0100 CI reconciliation
  included using successful exact head a53cf93593279cfd14e4fcd9f6d340a652bc328c.
- Build passed; focused independent suite 5/5 programs; full parent suite 35/35 passed.
  `review_inspection`: every criterion Pass, no findings. No human observation required.
- ER-0102 In Progress -> Complete; implementation CI remains pending before Verified.
- `./bootstrap.sh` and `./configure` required for new module/test target; generated artifacts
  restored before publication. No dependencies added. Next action: observe existing CI runs.

### ER-0101 CI repair cycle 1

- Network run 38813348896 for cc544898878833f5d59cc4ccacdfa14c37fd547c failed unit tests:
  33/34 programs passed; network lifecycle helper reports Timeout. Build passed; distcheck skipped.
- Exact CI command `make check V=1`; downloaded artifact test_network_service.log identifies
  test_network_host_session_and_packet_lifecycle at success helper, not a payload mismatch.
- Minimal repair: test-only IPC deadline 10 ms -> 5 seconds for the 1 MiB round trip. No latency
  requirement exists; all size/equality/authority/lifecycle assertions retained. Production unchanged.
- ER-0101 remains Complete; inspection run 38816782208 may encounter the same unchanged test.
  Do not restart stale-head CI or claim a pass; publish repair after focused/full validation.

### CI repair validation and restart recovery

- Server restart interrupted the relink command before execution; no .batch-core-relink.log existed.
  Resumed by inspecting Git/log/CI state rather than repeating a completed operation.
- Inspection run 38816782208 for 2175222bebe3ae5e00367cacfbd5bfd48f08fdf1 completed failure:
  34/35 programs passed; downloaded artifact confirms the identical network success-helper Timeout.
  Build passed; inspection tests passed; distcheck skipped. No distinct implementation defect found.
- Focused network repair test passed. Full rerun initially 34/35 due to restored generated core
  launcher referencing /usr/local/lib/libreferee.0.dylib. Normal relink command
  `make -C tests -W test_referee_core.cc test_referee_core` passed; no system install or DYLD workaround.
- `env TMPDIR=/private/tmp make check` then passed 35/35, exit 0, after the test deadline change.
  `git diff --check` passed. Production service source unchanged; existing independent reviews retained.
- Repair changes only test_network_service.cc, ER-0101 evidence, and this ledger. No regeneration.
  New-head CI must pass before ER-0101/0102 verification. Generated launcher restored before commit.

## Final Resume Reconciliation — 2026-10-17

- Reloaded instructions, ledger, canonical ARs/ERs, and committed independent criterion reports.
  Preserved the unfinished documentation corrections and final independent audit from the prior
  session. No new independent review is claimed.
- `git fetch origin main` passed; HEAD and origin/main both resolve to
  `6e5677ae522e4b22de42931fae7e0604d6f503cc`. Existing work is on main as authorized.
- `gh run view 38831614893 --json headSha,status,conclusion,url,jobs` returned completed/success
  for that exact head. Bootstrap/configure/build/unit tests/distcheck all passed. This is the
  terminal evidence missing at the prior audit, not an assumption from the old watcher.
- ER-0101/0102 Complete -> Verified; AR-0022 In Progress -> Implemented in its canonical document
  and architecture index. ER-Status and current queue tables reconciled with those transitions.
- Preserved prior AR-0023/0024 System Engineer rulings and corrected stale pending statements.
  No additional architecture scope, human observation, test execution, or DR verification claimed.
- `git diff --check` passed. Architecture JSON parses; all three canonical AR statuses match
  the index and every listed ER dependency is Verified (9 for AR-0022, 5 for AR-0023, 13 for
  AR-0024). ER-Status matches the final two ER transitions.
- `bash scripts/issue_sync.sh` succeeded for ER-0101/#355 and ER-0102/#356. AR-0022 sync updated
  body/labels but hit the previously recorded Bash empty-array failure at line 180; closure is
  completed directly with `gh issue close 290` after checking every dependency. No script change.
- GitHub read-back confirms AR #290/#291/#292 and ER #355/#356 are closed with status:done;
  ER issue bodies say Verified. Removed AR-0024's stale status:accepted label.
- Final files: AR-0022/0023/0024, ER-0100/0101/0102/0103, ER-Status, architecture index, this
  ledger, and the preserved final independent audit report. Documentation only; no regeneration.
