# AR-0003 — Storage Layout Strategy

- Status: Accepted
- Date: 2026-02-16
- Owners: Mike

## Context

Referee needs a durable, deterministic on-disk layout for object payloads,
indexes, and metadata. The layout must support:

- append-friendly writes
- fast lookup by ObjectID
- relationship graph reconstruction
- future compaction and recovery
- cross-file portability

## Decision

Referee uses a simple, versioned, directory-based storage layout with
append-only data segments and separate indexes.

### Layout

- Root directory contains:
  - `data/` for object payload segments
  - `index/` for ObjectID and relationship indexes
  - `meta/` for manifests, format version, and schema metadata

### Data Segments

- Object payloads are stored in append-only segment files under `data/`.
- Each segment contains a sequence of records:
  - header (layout version, segment id)
  - record entries: `{ObjectID, payload_len, payload_bytes, crc32}`
- Segment filenames are monotonic and stable, e.g. `segment-000001.dat`.

### Indexes

- Index files live under `index/` and map:
  - `ObjectID -> (segment id, offset, length)`
  - relationship edges as needed for graph reconstruction
- Index formats are versioned and independent of segment encoding.

### Metadata

- `meta/manifest.json` (or equivalent) records:
  - layout version
  - active segment
  - index versions
  - optional compaction state

### Durability and Updates

- Writes append to the active segment first, then update indexes.
- A write is considered durable only after both segment and index updates are persisted.
- Compaction (when introduced) rewrites segments and indexes, then swaps using atomic rename.

## Alternatives Considered

### Single Monolithic Data File

Rejected due to:
- poor compaction characteristics
- growing rewrite costs
- higher corruption risk

### Mixed Index + Payload Interleaving

Rejected due to:
- harder portability and recovery
- tight coupling between data and index layout

## Consequences

Positive:
- clear separation of concerns
- easy recovery by replaying segments
- append-friendly write path
- room for future compaction

Negative:
- more files on disk
- needs explicit index maintenance

## Implementation Notes

- Define layout version constants for `data/` and `index/` formats.
- Keep record headers fixed-size for easy scanning.
- Add integrity checks on read (crc32).
