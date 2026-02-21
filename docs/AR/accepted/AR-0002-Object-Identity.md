---
GitHub-Issue: N/A
---

# AR-0002 — Object Identity and Versioning Semantics

- Status: Accepted
- Date: 2026-02-14
- Owners: Mike

## Context

Referee stores serialized class instances (“Objects”) that are retrieved by ObjectID and
by relationships. The system will evolve to support cross-file and cross-machine references
(remote references with deferred synchronization).

We need an ObjectID scheme that is:

- globally unique without central coordination
- stable across process restarts
- independent of storage location and file layout
- independent of serialization format and content details
- suitable as an index key (binary-friendly)

We also must decide whether versioning is embedded in ObjectID or handled separately.

## Decision

1. **ObjectID format**: UUID v4 (random-based, 128-bit).

2. **Generation time**: ObjectIDs are generated at object creation time.

3. **Immutability**: ObjectID never changes for the lifetime of the logical object.

4. **Versioning**: Versioning is **not embedded** in ObjectID.
   - If versioning is needed, it is represented separately in object metadata (e.g. schema version,
     object revision, last-modified, etc.).

5. **Representation**:
   - Internal/index form: 16-byte binary UUID
   - External/text form: canonical UUID string (for logs, CLI, export/import)

6. **Collision policy**:
   - UUID v4 collisions are treated as negligible.
   - Storage/index enforces uniqueness; collision is treated as a hard error.

## Alternatives Considered

### Content-addressed IDs (hash of serialized bytes)

Rejected because:
- identity changes on any mutation
- identity becomes coupled to serialization format and canonicalization rules
- complicates partial updates and migrations
- makes “same logical object, new revision” awkward

### Monotonic sequence IDs

Rejected because:
- requires coordination for global uniqueness
- poor fit for cross-machine replication
- introduces contention in distributed scenarios

### UUID v7 (time-ordered)

Deferred because:
- no current requirement for time-order locality
- UUID v4 is simpler and ubiquitous
- can be introduced later if ordering becomes valuable (with clear migration rules)

### Composite IDs (UUID + version in the ID)

Rejected because:
- identity should remain stable across revisions
- conflates identity and state evolution
- complicates relationships (edges would need updating across versions)

## Consequences

Positive:
- stable, location-independent identity
- easy cross-system exchange and remote references
- simple fixed-size key for indexing
- decoupled from serialization and schema evolution

Negative:
- no natural ordering by time
- slightly larger key than 64-bit integers

## Implementation Notes

- Define `ObjectID` as a 16-byte value type with:
  - generation (CSPRNG-backed)
  - parse/format to canonical string
  - equality/hash for unordered maps
- Store ObjectID in indexes as binary (BLOB(16) in SQLite or equivalent).
- Add uniqueness constraints at the storage/index layer.
- Keep versioning fields in metadata (schema version, revision, timestamps), not in ObjectID.
