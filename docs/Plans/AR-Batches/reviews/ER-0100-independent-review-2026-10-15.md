# ER-0100 Independent Review — 2026-10-15

- Separate read-only reviewer context: `review_allocation`; no files edited by reviewer.
- Result: Pass after one repair; no open findings.
- Finding: selector/CBOR parsing originally preceded the bad_alloc exception boundary. Parent
  moved both inside try. Reviewer inspected the repair and rebuilt/reran the focused suite.

| Criterion | Verdict | Independently inspected evidence |
|---|---|---|
| Owned zero-initialized allocation | Pass | Allocation owns Bytes(size, 0); IPC identity/size/owner/distinct-ID tests pass. Zero initialization established by source inspection; no buffer-read endpoint is claimed. |
| Owner-only inspect/release and envelope preservation | Pass | Stored owner checked before mutation; cross-owner tests reject; responses assert recipient/correlation/sandbox/type. |
| Total/subject/count bounds and release | Pass | Subtraction avoids overflow; tests cover each quota, disabled limits, restoration and repeated release NotFound. |
| Invalid inputs/selectors atomic | Pass | Strict object/argument shape, positive representable integers, hex IDs, matching endpoint/message selectors; invalid requests preserve the sole allocation slot. |
| Recovery empty and stopped/unbound | Pass | File-backed reopen requires bind/start; old allocation absent and fresh full quota available. |
| Allocation failure and lifetime | Pass | Parsing/response/buffer/map allocation inside try; publication/counters follow successful construction. One-shot operator-new failure returns IoError; subsequent allocate/release/reallocate passes. RAII map/vector cleanup. |
| Build/distribution | Pass | Library sources/internal header, installed public header, TESTS/check_PROGRAMS integration inspected; fresh rebuild passes. |

Independent commands:

- `make -j4`: exit 0 after repair.
- `env TMPDIR=/private/tmp make -C tests check TESTS='test_core_services test_service_host test_service_ipc test_capability_context'`:
  sandbox failed before assertions with Check communication temp-file permission error; outside
  sandbox passed 4/4 programs, 24 checks (allocator 5).
- `git diff --check`: exit 0.

Counterexample inspection covered overflow, subject accumulation, quota drift after failure,
owner override payloads, conflicting selectors, invalid IDs, double release, descriptor recovery,
and response construction before release mutation. Trusted local envelopes and serialized calls
remain the contract. Parent full suite passed 33/33; reviewer does not claim that as an independent run.

## Status reconciliation review

| Transition | Verdict | Evidence |
|---|---|---|
| ER-0070–0074 -> Verified | Pass | Existing AR-0024 report covers every criterion, forged-handle/mode counterexamples and repair, fresh focused/full tests; System Engineer explicitly accepted automated evidence. |
| AR-0024 -> Implemented | Pass | All listed ER dependency documents Verified; four tracks covered by committed report; applicability ruling supersedes prior human gate without claiming observation. |
| AR-0023 -> Implemented | Pass | All dependency documents Verified; ER-0103 report covers decoder, compatibility, reopen/inheritance and regressions; System Engineer accepts required_capabilities scope. |

Reviewer independently read back successful CI run 38751271580 for
982379e414cd4a887520f8bff1f22a1d3770e708 and 38759330650 for
a3ee08ac40ebc8e34547c936e8544d704723ce31. New ER-0100 CI is a separate pending publication gate.
