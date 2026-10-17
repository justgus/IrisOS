---
GitHub-Issue: N/A
ER-Dependencies: ER-0104, ER-0105, ER-0106, ER-0107
---

# AR-0027 — Standard Operation Contracts and Discovery

- Status: Implemented
- Date: 2026-10-06
- Owners: Mike

## Context

IrisOS has an implemented operation and dispatch model in [AR-0016](../accepted/AR-0016-Operations-Dispatch.md).
Refract records operation definitions, Conduit resolves and dispatches calls, and types can expose
operations for introspection. The model establishes a foundation, but it does not yet define a
system-wide catalog of standard operation contracts or a complete account of how types advertise
conformance to those contracts.

IrisOS needs common operations for mathematical values, collections, object storage, services,
graphics, processes, and hardware-facing resources. The system should provide a consistent
object-oriented way to describe and discover those capabilities. C++ prototypes and pure virtual
functions, and Java interfaces, are useful analogies: they specify callable contracts separately
from the implementations that fulfill them. IrisOS should express that idea through its own
first-class objects and metadata rather than depend on language-specific declarations.

Vector and matrix operations are an initial motivating case, not the boundary of this architecture.
For example, evaluating `C = A + B` requires a well-defined operation contract, compatible input
types, a result type, an implementation binding, and a check that the result can be assigned to C.

## Recommendation

Define a system-wide catalog of versioned operation contracts. A type or object may advertise
support for a contract by providing a compatible operation definition and implementation binding.
Refract is authoritative for the machine-readable definitions and conformance metadata. Human
readable names, documentation, and inspection output are views of that canonical metadata.

Build this architecture on AR-0016 and the existing Refract and Conduit mechanisms. Extend their
discovery surfaces where needed; do not require a separate operation-discovery protocol unless
implementation experience demonstrates that the current Refract interfaces cannot serve that role.

## Concepts

- **Operation contract**: a stable catalog identity and version, with a name, scope, ordered typed
  inputs, ordered typed outputs, and documented behavior.
- **Conformance**: a type or object declares that its operation definition satisfies a catalog
  contract. Conformance is explicit and introspectable.
- **Binding**: associates a conforming operation definition with an implementation callable by
  Conduit. One contract may have multiple bindings for different types or execution services.
- **Operation category**: organizes contracts for discovery and documentation, such as arithmetic,
  comparison, collection, conversion, persistence, rendering, process control, and resource access.
- **Effect declaration**: describes observable behavior such as pure computation, mutation,
  object creation or deletion, task creation, IO, or remote state change. Effects do not grant
  authority; applicable capability checks remain independently enforced.
- **Discovery view**: a queryable Refract/Conduit view of contracts and of the operations a type
  or object advertises, with machine-readable signatures and human-readable documentation.

## Goals

- Define stable identities and versioning rules for standard operation contracts.
- Make operation contracts independent of any one implementation or programming language.
- Let types and objects advertise supported contracts with explicit, inspectable signatures.
- Define deterministic resolution for calls with multiple typed inputs, including unambiguous
  failure when no unique compatible implementation exists.
- Distinguish an operation's returned result from assignment or mutation of a caller-provided
  destination.
- Make effects, required authority, and local or remote execution boundaries inspectable.
- Provide discoverability suitable for both programs and people, using Refract as the canonical
  metadata source.
- Establish operation categories that can support an IrisOS standard library, with vector and
  matrix math as an early domain.

## Non-Goals (v1)

- Defining every standard operation or completing a C++-scale standard library catalog.
- Specifying vector, matrix, or tensor algorithms and optimized kernels.
- Mandating a particular human-facing language syntax.
- Treating an operation's advertised effects as an authorization grant.
- Replacing AR-0016's dispatch engine or reopening its completed implementation scope.
- Requiring network protocol details for remote operation discovery before the Refract discovery
  requirements are established.

## Proposed Model

### Contract and implementation separation

Catalog contracts define operation identity and call shape independently of implementation code.
Concrete type definitions declare conformance and bind compatible implementations. Contracts may
be implemented by local code or delegated to an authorized service or remote object system; the
binding and invocation route must remain discoverable.

The catalog should distinguish an operation's stable identity from its display name. Names such as
`add` or `area` aid people, but identity and version rules must prevent accidental equivalence
between unrelated contracts that happen to share a name.

### Signatures, dispatch, and results

An operation signature contains ordered inputs and outputs. Dispatch considers the operation
contract and all input types, including generic type arguments where relevant. The selection rules
must be deterministic and report ambiguity rather than select an arbitrary implementation.

An expression such as `C = A + B` invokes the addition contract with A and B, obtains a result, and
then checks whether that result can be assigned to C. Assignment may be an explicit operation of
its own; it is not implied to mutate A or B. Mutation, if supported, is stated in the signature and
effects.

### Effects, authority, and locality

Operation definitions declare relevant effects, including mutation, object lifecycle changes, task
creation, IO, rendering, and remote state changes. The runtime uses effect metadata for inspection
and planning. Capability and policy checks independently determine whether the caller may perform
the operation.

Objects and services may be local or remote to the executing program. An operation binding may
identify the service or object system that performs it. Invocation must preserve the operation's
input/output contract and report transport, authority, and execution failures through the existing
IrisOS error model.

### Versioning and compatibility

Contracts and implementations are versioned independently. A newer type may satisfy an older
contract when it explicitly advertises a compatible signature and semantics. Compatibility must
not be inferred solely from matching names. Replacing a contract, such as `area` with a broader
`space` operation, requires an explicit compatibility or adaptation rule if older callers are to
continue working.

### Discovery and presentation

Refract provides the canonical machine-readable contract, version, signature, category, effect,
conformance, and binding metadata. Discovery queries should support finding contracts by identity,
category, or capability, and listing the contracts supported by a type or object. Human-facing
surfaces can render the same metadata as names, signatures, and documentation.

The first implementation should evaluate and extend existing operation listing and introspection
APIs. A separate wire protocol is a follow-on only if remote discovery cannot be expressed
adequately through the established Refract and service interfaces.

## Relationship to Existing Architecture

- Extends [AR-0016 — Operations and Dispatch Model](../accepted/AR-0016-Operations-Dispatch.md)
  with shared contracts, explicit conformance, and broader discovery requirements.
- Uses [AR-0018 — Generic Type System](../accepted/AR-0018-Generic-Type-System.md) for operations
  whose signatures include parameterized types.
- Builds on [AR-0020 — Core Type Operations and Rendering](../accepted/AR-0020-Core-Type-Ops-and-Rendering.md)
  for existing core operation metadata and bindings.
- Does not change the completed status or historical scope of those records.

## Open Questions

- What immutable or versioned identifier format should standard operation contracts use?
- Which type compatibility rules are sufficient for initial multi-input dispatch?
- How should conformance be recorded: inline on operation definitions, as explicit relationships,
  or both?
- Which categories and initial contract set are needed for a useful first catalog?
- Which Refract listing and query APIs need extension for category and contract discovery?
- How should operation bindings describe delegated execution across mounted object stores and
  system services?

## Next Steps

- Define the contract identity, version, conformance, and compatibility rules.
- Inventory existing Refract operation metadata and discovery APIs against the proposed model.
- Draft ERs for catalog metadata, conformance and discovery, and an initial set of core contracts.
- Plan vector and matrix operations as an early catalog domain after generic signature and dispatch
  rules are settled.

## Implementation Progress — 2026-10-17

- System Engineer approved [the implementation plan](../../Plans/AR-0027-Implementation-Plan.md)
  and ER-0104–0107, resolving the open questions for the initial implementation.
- ER-0104–0107 are Verified by System Engineer acceptance: immutable catalog, persisted conformance/discovery, typed contract
  invocation, standard primitive implementations and Conch inspection are implemented on main.
- Final local build passed and full suite passed 39/39 programs. Earlier catalog/conformance CI
  passed; final implementation CI is advisory and its result must be recorded when observed.
- The System Engineer accepted completion on 2026-10-17. All four dependency ERs are Verified.
  No independent review, human observation, remote transport or vector/matrix kernels claimed.
- API/usage: [Operation Contracts](../../Operation-Contracts.md).

### System Engineer Completion Acceptance — 2026-10-17

- Explicit instruction: "mark AR-0027 as complete." Recorded as Implemented, matching the AR
  status vocabulary, and as acceptance/verification of its completed ER-0104–0107 scope.
- Implementation head: 278ae380e3c066cbd7a5b8449e628939b0f787b2. Local build, full 39/39
  programs and installed-header checks passed as recorded in the implementation plan.
- Advisory [CI run 39052974745](https://github.com/justgus/IrisOS/actions/runs/39052974745)
  remains in_progress at acceptance: bootstrap/configure passed; build running, tests/distcheck
  pending. No final CI pass inferred; Temporary Initial Development Mode permits acceptance
  without waiting for advisory CI. Any subsequent failure still requires investigation.
