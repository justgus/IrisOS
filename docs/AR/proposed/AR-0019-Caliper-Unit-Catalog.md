---
GitHub-Issue: #TBD
---

# AR-0019 — Caliper Unit Catalog

- Status: Proposed
- Date: 2026-02-27
- Owners: Mike

## Context

ER-0033 introduces Caliper schemas and a starter unit catalog. IrisOS needs a complete and
authoritative catalog of metric and imperial units, including conversions and derived units, to
support scientific and engineering workloads across Refract, Astra, and Crate types.

## Recommendation

Define and ship a full Caliper unit catalog that:

- Covers SI base + derived units and common metric prefixes.
- Covers common imperial/US customary units and their relations to SI.
- Provides canonical symbols, names, and conversion metadata.
- Defines dimension metadata and conversion chains that are deterministic.
- Stores the canonical catalog as a versioned instance in Referee.
- Allows user-defined extensions and overrides with clear precedence rules.

## Goals

- Provide a complete, curated catalog for general engineering use.
- Ensure conversions are explicit, deterministic, and testable.
- Enable Conch and system services to list, search, and introspect units.

## Non-Goals (v1)

- Automatic symbolic simplification beyond explicit conversion metadata.
- Domain-specific catalogs (chemistry, astronomy, finance) beyond core SI/imperial.

## Proposed Model (Broad Strokes)

- Canonical **Dimension** definitions for all base and derived dimensions.
- Canonical **Unit** definitions with:
  - name
  - symbol
  - dimension reference
  - system tags (SI, imperial, etc.)
  - conversion metadata (scale/offset/base unit)
- Catalog is versioned and shipped with IrisOS.
- Catalog is persisted as a versioned Referee object for durable access and migration.
- User extensions are layered over the base catalog (base is immutable; overrides are explicit).

## Sources

- Use authoritative references (e.g., NIST, BIPM) for unit definitions and conversion factors.

## Constraints and Compatibility

- Compatibility is evaluated by dimension, not by unit symbol.
- Units that differ by exponent are distinct types. Example: `m` (length), `m^2`
  (area), `m^3` (volume).
- Base constraints should be defined at the dimension level. Example: temperature
  has an absolute zero bound that applies to Kelvin, Celsius, and Fahrenheit.
- Higher-level constraints may be context-dependent (e.g., area bounded by a surface).

## Open Questions

- Precedence rules for user overrides vs base catalog entries.
- Constraint representation model and how context-dependent constraints are expressed.

## Next Steps

- Draft a Phased approach and ERs per Phase to define full catalog contents and 
  data format.
- Decide on storage and update mechanism (bootstrap vs external catalog file).
- Add comprehensive tests for catalog coverage and conversion correctness.
