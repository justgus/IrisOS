# AR-0024 Independent Verification — 2026-10-15

- Reviewer: separate read-only Codex context `review_machine`.
- Scope: canonical AR-0024 and ER-0070 through ER-0074, current implementation, tests, error paths,
  and independently compiled counterexamples. No files edited by the reviewer.
- Result: implementation review Pass after one bounded ER-0072 repair. Existing human gate remains
  Pending until the System Engineer supplies observation or changes its applicability.

## Findings and Repair

Before repair, a context with a resource-scoped grant could obtain an active lease from an aggregate
handle for a nonexistent or wrong-kind descriptor. An authorized transport accepted that lease.
An invalid access-mode enum also passed the factory with a matching malformed grant.

The repaired MachineLeaseRegistry requires inventory and reuses the handle factory before publishing
leases. Factory authorization rejects invalid modes. Constructor callers now supply
`MachineLeaseRegistry(inventory, contexts)` and keep both borrowed inputs alive.

Independently recompiled counterexample output:

```text
factory_missing=0 lease_missing=0 transport_missing=0 lease_wrong_kind=0 factory_invalid_mode=0
```

Exit 0. Before repair, the latter four values were 1. Regression tests cover the forged descriptor
paths and invalid modes in all three handle kinds. No open implementation findings remain.

## Criterion Evidence

| ER | Criterion | Verdict | Implementation and independent validation |
|---|---|---|---|
| ER-0070 | Primitive construction/equality | Pass | scalars.h, primitives.cc, buffers.cc; primitive value/equality tests |
| ER-0070 | Packet/slice bounds | Pass | Overflow and blob containment checked; span/slice bounds tests |
| ER-0070 | Existing Comms preserved | Pass | Four Comms primitive and thirteen transport/session checks |
| ER-0071 | Processor/memory/bus/device creation | Pass | definitions.cc, topology.cc, inventory.cc; representative descriptor tests |
| ER-0071 | Registry/query path | Pass | Inventory reference/query and registry field round-trip tests |
| ER-0071 | Descriptors expose no access operations | Pass | Facts/query APIs; registered descriptor operations asserted empty |
| ER-0072 | Known resource handles | Pass | Factory and repaired lease boundary validate inventory; missing/wrong-kind/RAM-as-IO tests |
| ER-0072 | Lease metadata round-trip | Pass | Retrieval preserves owner, context, handle, mode, resource, and lifecycle states |
| ER-0072 | Unauthorized access rejected | Pass | Exact grants/context/owner; terminal leases denied; invalid mode tests |
| ER-0073 | Machine-backed transports | Pass | Descriptor and authorized transport tests; independent missing-resource counterexample rejects |
| ER-0073 | Session state/metadata | Pass | Endpoint/transport references retained; invalid/terminal transitions rejected |
| ER-0073 | Existing Comms preserved | Pass | Existing loopback byte/datagram tests |
| ER-0074 | Compatible mappings | Pass | Semantics, resource-kind, and exact capability mapping tests |
| ER-0074 | Incompatible mappings deterministic | Pass | Reason assertions for semantics, kind, missing capabilities, and revoked leases |
| ER-0074 | Inspectable metadata | Pass | Identity/name/resource/requirement getters and normalization tests |
| All | Previously recorded human observation | Unverified | No human pass supplied; applicability ruling requested |

## Fresh Validation

- `env TMPDIR=/private/tmp make -C tests check TESTS='test_machine_authority test_comms_transport_session test_machine_primitives test_machine_inventory test_comms_primitives'`:
  passed 5/5 programs, 38 checks.
- `env TMPDIR=/private/tmp make check`: passed 31/31 programs against the repaired code.
- `git diff --check`: passed.
- Independently compiled counterexample: all invalid paths reject, exit 0.
- Tests ran outside the sandbox following concrete Check communication-file permission failures.
- Initial baseline full suite also encountered a stale core-test dylib loader path; normal relinking
  during parent build resolved it. The final independent full suite passed without a DYLD workaround.
- No Autotools regeneration occurred.

## Limits

The manual criteria inspect APIs, facts, metadata, and mapping decisions. Physical probing, device
access, drivers, and cross-host networking are explicitly out of scope and are not verified here.
Existing Pending human-gate language is preserved; code review does not attest a human observation.
