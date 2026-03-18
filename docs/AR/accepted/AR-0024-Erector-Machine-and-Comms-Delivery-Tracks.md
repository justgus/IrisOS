---
GitHub-Issue: N/A
---

# AR-0024 — Erector Machine And Comms Delivery Tracks

- Status: Accepted
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

- Draft Machine-focused ERs after the Service Plane and parser foundation work is underway.
