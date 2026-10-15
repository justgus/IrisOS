# Autonomous AR Batch Progress

- Requested range: AR-0022 through AR-0024; implementation and independent verification.
- Started: 2026-10-14.
- Current state: In Progress; AR-0024 repair published and CI passed; ER-0103 Complete pending publication/CI; architectural rulings pending.
- Process: Temporary Initial Development Mode; main, cohesive commits, direct pushes, advisory CI.
- Goal: complete eligible accepted scope and independent verification; preserve architectural and human-observation gates.
- Prior reconciliation is preserved in [the archive](AR-Batches/AR-reconciliation-2026-10-14-before-0022-0024.md).
- The System Engineer explicitly requested this batch, including resuming AR-0024's unfinished verification.

## AR Queue

| AR | Eligibility | Status | Remaining work | Evidence / blocker |
|---|---|---|---|---|
| AR-0022 | Accepted; selected | In Progress | Stage E allocation, network-oriented and capability/sandbox service | Planning review complete; ER-0100–0102 Proposed pending architectural ruling |
| AR-0023 | Accepted; selected | In Progress | Extended policy metadata scope and independent verification | System Engineer ruling requested on whether required_capabilities satisfies the scope |
| AR-0024 | Existing In Progress; resumed by explicit request | In Progress | ER-0070–0074 verification | Implementation review, local tests, and CI pass after 982379e repair; existing human gates remain Pending |

## ER Queue

- ER-0057–0062 and ER-0055/0078–0080 are already Verified; do not repeat or revoke completed gates without a concrete finding.
- ER-0070–0074 are Complete; independent implementation review and CI pass, human-gate ruling pending.

| ER | Parent | State | Dependencies / next step |
|---|---|---|---|
| ER-0100 | AR-0022 | Proposed | Owned-buffer allocation; Stage E architectural ruling pending |
| ER-0101 | AR-0022/0024 | Proposed | Local network service; Stage E ruling and ER-0073/0074 verification pending |
| ER-0102 | AR-0022 | Proposed | Read-only capability/sandbox service; Stage E ruling pending |
| ER-0103 | AR-0023 | Complete | Existing capabilities encoding repair; focused/full validation and independent review passed; implementation CI pending |

- New ERs are fully drafted before implementation; Proposed does not authorize unresolved architecture.
- Independent planning reviewer inspected the accepted records and service/Machine APIs and identified
  the architectural gates plus conflicting endpoint selector risk. It did not run tests during planning.
- Stage E proposal sent to System Engineer: bounded owned buffers with quotas/ownership, local
  Comms-backed loopback network service, and read-only capability/sandbox inspection.

## Current Item

- Last completed action: ER-0103 independent review passed all ER criteria; no findings.
- Current implementation: ER-0103; src/refract/schema_registry.cc, tests/Makefile.am,
  tests/test_refract_capabilities.cc, linked ER/AR/index records and this ledger.
- Next action: publish cohesive ER-0103 implementation on main, check CI, and await architectural rulings.
- Validation: `make -j4` passed; focused four-program suite 4/4; full suite 32/32.
- Independent reviewers: review_scope (ER-0103), review_machine (completed AR-0024).
- CI: 982379e passed run 38751271580; ER-0103 not yet published.
- GitHub: AR-0022/0023 In Progress; Stage E proposals #354–356; ER-0103 #357.
- Existing human gates and architecture rulings remain unanswered; do not infer approval.

## Decisions and Blockers

- AR-0023: undefined extended policy metadata requires a System Engineer ruling; question pending.
- AR-0024: previous records explicitly leave Independent Live Pass Pending; automated review cannot attest to that human observation.
- AR-0022: allocation policy and network service extent must be supported by accepted designs or receive a System Engineer ruling.

## Completion Summary

- Published AR-0024 repair 982379e; independent implementation review and CI passed. ERs remain
  Complete pending the existing human-gate ruling.
- ER-0103 Complete; parent validation and independent review passed; publication/CI pending.
- No selected AR is Implemented; architectural/human gates remain explicit.

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
