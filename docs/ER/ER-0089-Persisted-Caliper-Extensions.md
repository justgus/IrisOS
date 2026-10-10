---
GitHub-Issue: N/A
AR-Dependencies: AR-0019
ER-Dependencies: ER-0075, ER-0076, ER-0077
---

# ER-0089 — Persisted Caliper Extensions and Overrides

## Roles

- Implementation Engineer: drafts and implements changes
- System Engineer: reviews, tests, and verifies
- Note: In this Autonomous AR Batch, Codex may mark this ER Verified only after all documented gates pass.

## ER Metadata

- ER ID: ER-0089
- Title: Persisted Caliper Extensions and Overrides
- Status: Proposed
- Date: 2026-10-10
- Owners: Mike
- Type: Enhancement

## Engineering Guidelines

- Implementation language baseline: C++20 or C++24.
- Avoid line compaction or formatting changes that risk obscuring or losing content.
- Keep source files reasonably small; do not add dependencies.

## Context

- Problem statement: AR-0019 requires user-defined unit extensions and explicit overrides over an immutable shipped base catalog. `compose_caliper_catalog` currently validates this precedence only for in-memory vectors and is called only by its unit test. Persisted `Caliper::Unit` records do not store explicit override intent, and `load_caliper_catalog` reads all unit records without composing an extension layer.
- Background / constraints: The shipped `Caliper::Catalog` is versioned and immutable. `CaliperCatalogUnit` already has an `override_base` flag, and the accepted AR requires base immutability, extension layering, and explicit overrides. Existing Conch list/inspect/convert operations use `load_caliper_catalog`.

## Goals

- Persist user unit extensions separately from the immutable shipped base catalog while using the existing Referee object model.
- Compose base and extension entries deterministically when loading the effective runtime catalog.
- Persist and enforce explicit override intent using the existing `override_base` semantics.

## Non-Goals

- Change the shipped catalog's versioning or mutate its canonical entries.
- Add a new Conch authoring UI or change unit conversion mathematics.

## Scope

- In scope: the System Engineer-selected extension write boundary, persisted extension identification and override intent, effective catalog loading/composition, backwards-compatible treatment of existing base records, and API/Conch integration tests.
- Out of scope: expansion of the shipped unit set (ER-0075), conversion algorithm behavior (ER-0076), and command syntax/output redesign (ER-0077).

## Requirements

- Functional: a user-added unit with a unique symbol is persisted, loaded, and usable by catalog consumers after store reopen.
- Functional: an extension may replace a base unit only when explicit override intent is persisted; overrides cannot change the base unit's dimension.
- Functional: duplicate symbols/names, implicit overrides, overrides of absent base symbols, ambiguous names, and invalid base references are rejected deterministically.
- Functional: loading an effective catalog leaves the shipped versioned base catalog unchanged and produces stable symbol ordering.
- Functional: the immutable base is loaded by the existing `base_caliper_catalog_id()` identity; version 1 is the currently supported format. The loader rejects a missing record, wrong object type, malformed references, or an unsupported `catalog_version`; it does not select a catalog by name, symbol, or unordered type scan.
- Functional: base membership is derived only from the selected persisted catalog's unit references; no symbol/name heuristic may classify a record as base.
- Functional: the selected extension write path creates new extension objects and cannot update or reuse the stable catalog object ID or any Unit/Dimension object ID referenced by that base catalog.
- Functional: an extension's `base_unit_id` resolves to exactly one Unit in the effective catalog and its dimension matches that target. Missing or ambiguous targets and dimension mismatches are rejected when loading/composing. Cycle detection and numeric conversion-chain validation remain ER-0076 responsibilities.
- Functional (pending System Engineer ruling): if an extension's base reference names a base Unit whose symbol is overridden in the effective catalog, either resolution follows that stable logical symbol to the selected effective replacement or rejects the reference unless it points directly to the overriding Unit object. A catalog test covers the ruled behavior and verifies the effective conversion metadata.
- Non-functional: register the changed `Caliper::Unit` schema as version 2, superseding its current version 1 definition, with a new definition identity and the optional override field. A pre-change database migration fixture proves bootstrap adds v2 once even when a non-empty schema registry already contains v1, `get_latest_definition_by_type` resolves v2, and repeated bootstrap does not add another definition. Existing version 1 object refs and payloads are not rewritten; v1 records remain readable. New extension objects and newly bootstrapped records reference v2, while persisted base references keep their original definition identity. Tests verify the bootstrap/catalog write path selects v2 and decodes v1 records by their referenced definition identity. The migration compatibility path is deterministic and covered by fixtures using both definition versions.

## Proposed Approach

- Summary: load the supported versioned base catalog and derive base membership only from its persisted unit references. Treat other `Caliper::Unit` records as extensions; add optional persisted override intent, defaulting to false when absent for backward compatibility. Convert both layers through the existing catalog model and `compose_caliper_catalog` in `load_caliper_catalog` before exposing the effective catalog. Use the selected write boundary to create extension objects without permitting writes to base identities. Preserve the base catalog and every referenced Unit/Dimension record during bootstrap and extension writes.
- Alternatives considered: mutating the shipped catalog object or inferring override intent from unit names/systems was rejected because neither preserves explicit precedence or base immutability.

## Acceptance Criteria

- Reopen tests persist a uniquely-symbolled user unit, reload the effective catalog, and demonstrate that the unit is discoverable and convertible.
- Catalog loading resolves the canonical base by its stable object identity, accepts the supported version deterministically, and rejects missing, wrong-type, unsupported-version, or internally inconsistent catalog references with stable errors.
- Reopen tests persist an explicit same-dimension override and demonstrate that the effective catalog uses it while the base `Caliper::Catalog`, every referenced Unit and Dimension object ID/version/definition reference/payload remain unchanged.
- Negative tests reject implicit base collisions, dimension-changing overrides, overrides of unknown base symbols, duplicate symbols within a layer, duplicate names within an extension layer, ambiguous cross-layer names, multiple overrides for one base symbol, and unresolved or dimension-mismatched base-unit references. A single explicit same-symbol, same-dimension override remains valid and replaces the prior name in the effective catalog.
- Existing stores containing only versioned base records load identically. After extension creation and repeated bootstrap, the base catalog object and all referenced Unit/Dimension records retain their original IDs, versions, definition references, and payload bytes.
- A pre-change store fixture containing the registered version 1 `Caliper::Unit` definition and payloads without extension metadata loads referenced records as base and unreferenced records as non-overriding extensions; ambiguous collisions fail rather than being silently reclassified. Bootstrap registers Unit v2 exactly once as superseding v1; the latest definition resolves to v2, existing v1 object refs/payloads remain unchanged/readable, and new extension records reference v2.
- Persistence tests create extension records only through the write boundary selected by the System Engineer and prove that it does not mutate or replace base object identities. They attempt to create an extension using each stable base object ID and prove the selected boundary rejects the collision without changing any base payload or reference.
- Conch `caliper list`, `inspect`, and `convert` observe the same effective catalog returned by the API.
- Deterministic ordering and existing conversion/command tests pass.

## Risks / Open Questions

- Risk: the Refract version chain and old object definition references must remain readable. Preserve version 1 objects without rewriting their payloads; test both version 1 and version 2 schema references. The base catalog's unit references are encoded in its versioned `units` field; tests must cover malformed references and the stable-ID/version selection rule.
- Question: When an extension references a base Unit by object ID and a same-symbol override replaces it in the effective catalog, should that reference follow the logical symbol to the effective override, or should the loader require an ID for the override object? The acceptance criterion and chain test follow the System Engineer ruling.
- Question: Should extensions be persisted through the existing generic Referee/`SqliteStore` object-creation API, without a Caliper-specific writer, or should this ER add a public Caliper extension-write API? Acceptance tests and immutability enforcement follow the System Engineer ruling.

## Dependencies

- Dependency 1: ER-0075 Full Caliper Catalog Expansion.
- Dependency 2: ER-0076 Runtime Conversion and Compatibility Engine.
- Dependency 3: ER-0077 Conch Conversion and Inspection Commands.

## Implementation Notes

- Keep the extension layer in Referee objects; no external catalog file or dependency is required.
- Do not alter shipped base catalog serialization or version to persist user extensions.
- If adding an optional field to `Caliper::Unit`, document its schema compatibility and bootstrap behavior.

## Verification Plan

- Tests to run:
  - `./bootstrap.sh`
  - `./configure`
  - `make -j`
  - `make -C tests check TESTS='test_refract_bootstrap test_conch_authoring'`
  - `make check`
  - `git diff --check`
- Manual checks: Not required; persistence, layer precedence, and Conch visibility are covered by file-backed store-reopen and subprocess integration tests.

### Live Pass (Human)

- Required: No
- If no, reason: Persisted API and Conch command behavior is covered by file-backed automated tests; no target-specific environment or interaction is required.
- Build / environment / target: Not applicable.
- Steps: Not applicable.
- Expected observations: Not applicable.
- Observed results: Not applicable.
- Result: Not Required
- Performed by: Not applicable
- Date: Not applicable
