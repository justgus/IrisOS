---
GitHub-Issue: #292
ER-Dependencies: ER-0015, ER-0016, ER-0017, ER-0045, ER-0045.1, ER-0045.2, ER-0045.3, ER-0045.4, ER-0070, ER-0071, ER-0072, ER-0073, ER-0074
---

# AR-0024 — Erector Machine And Comms Delivery Tracks

- Status: In Progress
- Date: 2026-03-14
- Owners: Mike

## Context

AR-0008 and AR-0010 define a broad foundational subsystem structure that includes
`Erector::Machine` and a richer `Erector::Comms` model. In practice, the repository currently
implements CEO waitables, task-backed IO plumbing, and loopback comms primitives, but it does not
implement a Machine subsystem or hardware-grounded comms types.

The missing `Erector::Machine` layer makes it difficult to state how future Comms work should be
grounded.

## Recommendation

Define explicit delivery tracks for Machine and Comms.

### Track 1: Machine Core

- Representation primitives such as bytes, words, addresses, blobs, slices, and packets
- Stable primitive object and schema identities

### Track 2: Machine Descriptors

- Processor, memory, bus, and device descriptor objects
- Resource facts rather than permission-bearing handles

### Track 3: Machine Handles And Leases

- Capability-bearing handles such as memory handles, device handles, and IO region handles

### Track 4: Comms Realization Above Machine

- Transport and session objects
- Protocol objects
- Hardware mapping from Machine descriptors and handles into Comms objects

## Goals

- Separate the absent Machine subsystem from the already-working local comms substrate.
- Provide a clean architecture path from loopback primitives to hardware-grounded comms.
- Prevent future ERs from bypassing the Machine layer implicitly.

## Non-Goals (v1)

- Full hardware driver architecture
- Complete network protocol stack definition
- Commitment to one specific device model

## Relationship To Existing ARs

- Refines [AR-0008-Erector-Subsystems.md](/home/justgus/Dev/irisOS/docs/AR/accepted/AR-0008-Erector-Subsystems.md)
- Refines [AR-0010-Comms-Subsystem.md](/home/justgus/Dev/irisOS/docs/AR/accepted/AR-0010-Comms-Subsystem.md)

## Next Steps

- Complete System Engineer verification of ER-0070 through ER-0074 before marking this AR Implemented.

## Implementation Progress

- The 2026-10-13 reconciliation found implementations for all four tracks: representation types
  and Refract registrations (`src/machine/primitives.cc`, `buffers.cc`, and `refract.cc`);
  architecture/topology/inventory descriptors (`definitions.cc`, `topology.cc`, `inventory.cc`);
  capability handles and leases (`authority.cc`); and transport/session/protocol compatibility and
  Machine mappings (`src/comms/transport_session.cc`).
- ER-0070's Proposed label was stale: its construction/equality/bounds criteria have matching
  source and passing tests. It is now Complete, matching ER-0071 through ER-0074. No ER is marked
  Verified by this reconciliation.
- Fresh `TMPDIR=/private/tmp make check` passed 31/31 outside the sandbox. The relevant suites
  passed 8 Machine primitive checks, 6 inventory checks, 6 authority checks, and 13 transport/session
  checks. These are local automated results, not Independent Live Pass evidence.
- Independent Live Pass and System Engineer verification remain Pending. Track 4 implements the
  accepted metadata/mapping foundations and a bounded loopback packet path; full drivers and a
  complete network stack remain outside v1.
