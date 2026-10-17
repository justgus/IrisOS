# AR-0027 Implementation Plan

- Date: 2026-10-17
- State: Approved; implementation in progress.
- Request: plan and implement AR-0027 on main under Temporary Initial Development Mode.
- Existing local change: regenerated macOS tests/test_referee_core launcher. Preserve it locally;
  do not restore the Linux launcher after tests or include generated platform paths in commits.
- AR-0022–0024 batch remains complete. This plan does not change its scope or evidence.

## Existing implementation inventory

- SchemaRegistry persists TypeDefinition operation signatures, scope, effects, capabilities, and
  documentation. Definitions have versions; old records need to remain readable.
- OperationRegistry lists declared/inherited operations by type and scope. It does not retain
  declaring-owner information in its returned operation list or expose catalog identities.
- DispatchEngine resolves name/scope over all supplied argument TypeIDs using exact/subtype
  matches, optional-count preference, then declaring depth. Ties fail. Supplying an inconsistent
  type count currently bypasses type checks; the new contract API must require full typed inputs.
- GenericRegistry already derives concrete instance TypeIDs from canonical generic arguments.
- Conch implementations are selected by operation name and receiver type. There is no general
  persisted implementation binding or contract invocation registry to reuse directly.
- Existing core to_string and compare metadata/implementations provide initial adoption candidates.
  Existing effects/capability checks must retain their independent authority semantics.

## Approved architectural decisions

1. Identity/version: use a stable 128-bit ObjectID plus a positive integer contract version,
   independent of display name and implementation version. Each identity/version pair is
   immutable; identical registration is idempotent and conflicting registration fails.
   Built-in identities are fixed constants. Compatibility is explicit conformance to each
   supported version, with no inferred name or latest-version substitution.
2. Signatures/compatibility: ordered inputs/outputs and scope; contract type parameters can be
   explicitly bound by conformance (including Self). Substitute concrete TypeIDs and require
   equal parameter/output types and optionality when declaring conformance. Instantiated generic
   TypeIDs preserve all existing generic arguments; no new generic inference or numeric coercion.
   Invocation considers every supplied input, reusing existing exact/subtype ranking and ambiguity
   failure after filtering by contract identity/version. Reject missing/inconsistent type vectors.
3. Conformance: inline references on OperationDefinition containing contract key and explicit
   type-parameter substitutions. Validate against the catalog; persist with the type definition.
   Plain existing operations remain valid and do not acquire conformance by name matching.
   Initial object support follows the object's stored concrete type; no per-object overrides.
4. Bindings: persist implementation identity/version and execution route on conforming operations.
   Local callbacks are registered separately in memory and must be explicitly rebound after reopen.
   Delegated routes identify service/object-system destination and are inspectable; invocation of
   an unbound/unsupported route fails with a typed error. No new remote transport is implied.
   Invocation checks required capabilities independently of effects before executing a binding,
   validates returned count/types, and returns results without assignment to a caller destination.
5. First catalog: conversion/to_string, comparison/compare, and arithmetic/add. Use Self-parameterized
   contracts with explicit concrete conformance for existing primitive to_string/compare and
   U64/F64 addition. Addition has two ordered inputs and a separate result. U64 overflow is a
   typed failure; proposed F64 addition accepts only finite inputs and fails if the result is
   non-finite. Document this in the versioned contract rather than changing legacy comparison.
   Vector/matrix kernels and a complete standard library remain outside this first catalog.
6. Discovery: extend Refract APIs for exact contract key, category, required capability, and
   type/object conformance listing, retaining declaring type and binding details. Add human
   inspection in Conch using the same metadata. Preserve existing name-based calls. An explicit
   contract-based invocation API supplies the new behavior without introducing expression syntax.

## Approved ER order

| ER | Work | Depends on |
|---|---|---|
| ER-0104 | Persisted immutable contract catalog, signatures and validation | Existing Refract/Referee |
| ER-0105 | Explicit operation conformance and discovery views | ER-0104 |
| ER-0106 | Typed contract resolution, bindings and invocation results | ER-0105 |
| ER-0107 | Initial standard contracts, concrete bindings and Conch inspection | ER-0106 |

The System Engineer approved this plan and ER scope explicitly on 2026-10-17.
ER-0104–0107 are Approved. Verification still requires completed evidence.

## Delivery and validation

- Implement sequentially with small cohesive commits directly on main and push origin/main.
- Add dedicated catalog/conformance/dispatch/bootstrap tests, including file-backed reopen,
  malformed persisted metadata, name collisions, mismatched versions/outputs, ambiguity,
  generic argument mismatch, unauthorized/unbound calls, and result/assignment separation.
- Run make -j4, targeted make check per ER, and full make check at integration.
- New Automake sources/targets require local ./bootstrap.sh and ./configure. Record commands;
  do not publish generated platform artifacts. Preserve the working macOS test launcher locally.
- CI after pushes is advisory; inspect actual failures and record completed evidence.
- Temporary Mode does not mandate separate reviewer runs. The implementation will record code
  review and tests; no independent review or human observation will be claimed unless performed.
- Proposed Independent Live Pass applicability: Not Required for deterministic local APIs and
  automated Conch inspection tests. No physical/remote environment is claimed.

## Current checkpoint

Architecture approved on 2026-10-17. Implement ER-0104 first on main; preserve the local launcher.

## Requirement-to-test mapping

| AR requirement | Planned evidence |
|---|---|
| Stable identity independent of name/version | Same-name distinct keys; conflicting immutable registration; old-version lookup after reopen |
| Canonical metadata and explicit conformance | Raw CBOR corruption cases, legacy record decode, incompatible scope/output/substitution rejection |
| All-input resolution and ambiguity | Two-input exact/subtype examples, conflicting overloads, wrong second input, full type-vector enforcement |
| Generic arguments | Distinct registered vector shapes/element types cannot satisfy each other's concrete signatures |
| Result versus assignment | Addition returns a value; source records and unrelated destination remain unchanged |
| Effects versus authority | A pure-looking call with unmet required capabilities never reaches the callback |
| Locality and bindings | Inspect local/delegated routes; missing runtime binding after reopen fails until explicitly rebound |
| Human/program discovery | Conch inspection and Refract queries describe the same catalog records and declaring owners |
| Existing compatibility | Refract registry/bootstrap and Conch name-based authoring/call regression suites |

No baseline test run was needed during planning: the immediately preceding local repair completed
35/35 programs, including all five RefereeCore checks. New tests are future targets, not claimed
completed validation. `git diff --check` passes for the planning documents.

## Execution checkpoints

- 2026-10-17: explicit System Engineer approval received; ER-0104 In Progress, ER-0105–0107 Approved.

- ER-0104 Complete: build and focused suite 4/4 pass (3 new catalog checks); no verification claim.
  New target integration required ./bootstrap.sh and ./configure; both passed. Next ER-0105.
