---
GitHub-Issue: #73
ER-Dependencies: ER-0002, ER-0003, ER-0004, ER-0018, ER-0019, ER-0020, ER-0034, ER-0035, ER-0036, ER-0048, ER-0049, ER-0050
---

# AR-0009 — Referee Object Store (Recommendation)

- Status: Implemented
- Date: 2026-02-16
- Owners: Mike

## Context

IrisOS persists objects in an Object System rather than a filesystem. Referee is the object store
that provides durable storage, versioning, and retrieval by ObjectID, with support for
relationships and graph traversal.

## Recommendation

Define **Referee** as the canonical object store and graph manager:

- Objects are stored and retrieved by ObjectID and version.
- Referee maintains object relationships as edges.
- The store is queryable by type, relationships, and metadata.
- Object persistence is deterministic and recoverable.

For the current phase, SQLite is an acceptable backend to validate the model and APIs, with the
intent to revisit segment/index storage later if needed.

### Core Capabilities

- Create object
- Retrieve object by ObjectID + Version
- Retrieve latest version by ObjectID
- Add and query edges (relationships)
- Schema/versioning for object records

## Notes

Referee underpins Refract and other system services. It must remain stable, deterministic, and
portable across platforms.
