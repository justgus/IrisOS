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
- Status: Verified
- Date: 2026-10-10
- Owners: Mike
- Type: Enhancement

## Engineering Guidelines

- Implementation language baseline: C++20 or C++24.
- Avoid line compaction or formatting changes that risk obscuring or losing content.
- Keep source files reasonably small; do not add dependencies.

## Context

- Problem statement: AR-0019 requires user-defined unit extensions and explicit overrides over an immutable shipped base catalog. `compose_caliper_catalog` currently validates this precedence only for in-memory vectors and is called only by its unit test. Persisted `Caliper::Unit` records do not store explicit override intent, and `load_caliper_catalog` reads all unit records without composing an extension layer.
- Background / constraints: Shipped `Caliper::Catalog` objects are versioned and immutable. The System Engineer approved an expanded immutable catalog v2 under a new stable object ID; the existing v1 object and all of its referenced Unit/Dimension records remain unchanged and readable. `CaliperCatalogUnit` already has an `override_base` flag, and the accepted AR requires base immutability, extension layering, and explicit overrides. Existing Conch list/inspect/convert operations use `load_caliper_catalog`.

## Goals

- Persist user unit extensions separately from the immutable shipped base catalog while using the existing Referee object model.
- Compose base and extension entries deterministically when loading the effective runtime catalog.
- Persist and enforce explicit override intent using the existing `override_base` semantics.

## Non-Goals

- Mutate an existing shipped catalog object or any Unit/Dimension record referenced by a historical shipped catalog.
- Add a new Conch authoring UI or change unit conversion mathematics.

## Scope

- In scope: the System Engineer-selected extension write boundary, persisted extension identification and override intent, effective catalog loading/composition, backwards-compatible treatment of existing base records, and API/Conch integration tests.
- Out of scope: expansion of the shipped unit set (ER-0075), conversion algorithm behavior (ER-0076), and command syntax/output redesign (ER-0077).

## Requirements

- Functional: a user-added unit with a unique symbol is persisted, loaded, and usable by catalog consumers after store reopen.
- Functional: an extension may replace a base unit only when explicit override intent is persisted; overrides cannot change the base unit's dimension.
- Functional: duplicate symbols/names, implicit overrides, overrides of absent base symbols, ambiguous names, and invalid base references are rejected deterministically.
- Functional: loading an effective catalog leaves the shipped versioned base catalog unchanged and produces stable symbol ordering.
- Functional: the active immutable base is loaded by the stable v2 catalog identity `CALIPER-BASE-002`; it has `catalog_version: 2`. The legacy `CALIPER-BASE-001` object remains readable and unchanged. The loader rejects a missing v2 record, wrong object type, malformed references, or an unsupported `catalog_version`; it does not select a catalog by name, symbol, or unordered type scan.
- Functional: base membership is derived only from the selected persisted catalog's Unit object references; no symbol/name heuristic may classify a record as base. Catalog v2 records direct Unit and Dimension IDs.
- Compatibility exception: only Unit v1 records matching the retired historical aliases `C`/`celsius` and `F`/`fahrenheit` are excluded from the extension layer, preserving the pre-v2 starter catalog's symbol transition to coulomb/farad. Unit v2 records with those identities remain ordinary extensions and must pass normal collision validation.
- Functional: the public extension writer creates new extension objects and cannot update or reuse the stable catalog ID or any Unit/Dimension object ID referenced by that base catalog.
- Functional: extension `base_unit_id` references must be supplied as direct object IDs, resolve to exactly one persisted Unit, and match its dimension. Missing and dimension-mismatched targets are rejected at write/load time. Conversion follows that exact ID across symbol overrides; displaced base records remain internally available to conversions and hidden from listing/inspection. Cycle detection and numeric conversion-chain validation remain ER-0076 responsibilities.
- Non-functional: register `Caliper::Unit` schema version 2, superseding version 1, with a new definition identity and optional `override_base`. A pre-change database fixture proves bootstrap adds v2 once when a non-empty registry contains v1, `get_latest_definition_by_type` resolves v2, and repeated bootstrap adds no duplicate. Existing v1 refs and payloads remain unchanged and readable; unreferenced v1 records default to non-overriding extensions. New extension objects and newly bootstrapped Unit records use v2, while existing base records preserve their original definition references. Tests cover both versions.

## Proposed Approach

- Summary: bootstrap immutable catalog v2 as `CALIPER-BASE-002`, retaining historical v1 and existing Unit/Dimension records unchanged. Catalog v2 carries direct IDs for its Unit and Dimension records. The loader derives base membership only from those Unit IDs; other Unit records become extensions with `override_base` defaulting to false when absent. A public writer persists new Unit v2 objects, requires direct IDs for base links, and always allocates a fresh object ID. Composition applies deterministic precedence. Conversion links remain pinned to the referenced object across symbol overrides; displaced base Units remain internal to conversion and are hidden from list/inspect. Preserve versioned catalogs and referenced records during extension writes.
- Alternatives considered: mutating the v1 shipped catalog object was rejected because immutable catalog v2 is approved and preserves historical identity/payloads. Inferring override intent from unit names/systems was rejected because it does not preserve explicit precedence.

## Acceptance Criteria

- Reopen tests persist a uniquely-symbolled user unit, reload the effective catalog, and demonstrate that the unit is discoverable and convertible.
- Catalog loading resolves the canonical base by its stable object identity, accepts the supported version deterministically, and rejects missing, wrong-type, unsupported-version, or internally inconsistent catalog references with stable errors.
- Reopen tests persist an explicit same-dimension override and demonstrate that the effective catalog uses it while the base `Caliper::Catalog`, every referenced Unit and Dimension object ID/version/definition reference/payload remain unchanged.
- Negative tests reject implicit base collisions, dimension-changing overrides, overrides of unknown base symbols, duplicate symbols within a layer, duplicate names within an extension layer, ambiguous cross-layer names, multiple overrides for one base symbol, and unresolved or dimension-mismatched base-unit references. A single explicit same-symbol, same-dimension override remains valid and replaces the prior name in the effective catalog.
- Existing stores containing only the v1 base load with v2 added as the active base. The v1 catalog and every Unit/Dimension record it references retain their original IDs, versions, definition references, and payload bytes. After extension creation and repeated bootstrap, both versioned base catalogs and all referenced records retain their original IDs, versions, definition references, and payload bytes.
- A pre-change store fixture containing the registered version 1 `Caliper::Unit` definition and payloads without extension metadata loads referenced records as base and unreferenced records as non-overriding extensions; ambiguous collisions fail rather than being silently reclassified, except for the exact retired v1 aliases documented above. Bootstrap registers Unit v2 exactly once as superseding v1; the latest definition resolves to v2, existing v1 object refs/payloads remain unchanged/readable, and new extension records reference v2.
- Persistence tests create extensions only through the public Caliper API and prove it does not mutate or replace base identities. Caller-selected extension object IDs are rejected. Tests snapshot catalog v2 and referenced Unit payloads and confirm they remain unchanged after extension and override writes.
- Conch `caliper list`, `inspect`, and `convert` observe the same effective catalog returned by the API.
- Deterministic ordering and existing conversion/command tests pass.

## Risks / Open Questions

- Risk: the Refract version chain and old object definition references must remain readable. Preserve version 1 objects without rewriting their payloads; test both version 1 and version 2 schema references. The base catalog's unit references are encoded in its versioned `units` field; tests must cover malformed references and the stable-ID/version selection rule.
- Decision (System Engineer): persisted base links require direct object IDs. A link to a displaced base Unit continues to target that object and never follows a symbol override.
- Decision (System Engineer): extension records use a public Caliper write API, which allocates fresh identities and validates extension-layer intent.

## Dependencies

- Dependency 1: ER-0075 Full Caliper Catalog Expansion.
- Dependency 2: ER-0076 Runtime Conversion and Compatibility Engine.
- Dependency 3: ER-0077 Conch Conversion and Inspection Commands.

## Implementation Notes

- Keep the extension layer in Referee objects; no external catalog file or dependency is required.
- Do not alter a shipped base catalog object or its referenced Unit/Dimension records after creation. Catalog v2 is authored with direct Unit/Dimension IDs; v1 remains historical.
- If adding an optional field to `Caliper::Unit`, document its schema compatibility and bootstrap behavior.

## Verification Plan

- Tests to run:
  - `./bootstrap.sh`
  - `./configure`
  - `make -j2 CXXFLAGS='-g -O2 -Wno-unused-const-variable -Wno-unused-private-field'`
  - `DYLD_LIBRARY_PATH="$PWD/src/.libs" TMPDIR=/private/tmp make -C tests check TESTS='test_refract_bootstrap test_conch_authoring' CXXFLAGS='-g -O2 -Wno-unused-const-variable -Wno-unused-private-field -Wno-gnu-zero-variadic-macro-arguments'`
  - `DYLD_LIBRARY_PATH="$PWD/src/.libs" TMPDIR=/private/tmp make check CXXFLAGS='-g -O2 -Wno-unused-const-variable -Wno-unused-private-field -Wno-gnu-zero-variadic-macro-arguments'`
  - `git diff --check`
- Manual checks: Not required; persistence, layer precedence, and Conch visibility are covered by file-backed store-reopen and subprocess integration tests.

### Independent Live Pass

- Required: No
- If no, reason: Persisted API and Conch command behavior is covered by file-backed automated tests; no target-specific environment or interaction is required.
- Build / environment / target: Not applicable.
- Steps: Not applicable.
- Expected observations: Not applicable.
- Observed results: Not applicable.
- Result: Not Required
- Performed by: Not applicable
- Date: Not applicable


## Final Batch Verification

- Verified after local validation, independent review where required, PR merge, and PR #347; CI run 38166633486 passing its required CI checks. The detailed evidence is recorded in `docs/Plans/AR-Batch-Progress.md`.
