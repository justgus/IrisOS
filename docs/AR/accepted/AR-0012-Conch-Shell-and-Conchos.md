---
GitHub-Issue: N/A
---

# AR-0012 — Conch Shell and Concho Views (Recommendation)

- Status: Accepted
- Date: 2026-02-16
- Owners: Mike

## Context

IrisOS aims to boot into an object shell where execution emits objects and the UI grows organically
as the object graph grows. A shell must expose objects, types, capabilities, and task execution
without requiring producers to explicitly "do UI."

## Recommendation

Define **Conch** as the object shell and **Conchos** as its view instances. Conch subscribes to
object graph changes and uses Vizier to map artifacts into views.

### Core Pattern

**Execution emits objects, UI subscribes to the graph**:

1) An Object runs under CEO.
2) During execution it creates Result/Status/Artifact objects in Referee.
3) It links them to itself via typed relationships (produced, progress, diagnostic, stream).
4) Conch is subscribed to those relationship patterns.
5) Conch spawns a Concho view when it sees a new relevant object.

### UI as Objects

- Task/Object --produced--> ArtifactObject
- ArtifactObject --renderedAs--> Viz::Panel (optional indirection)
- ConchSession --shows--> Viz::Panel

Even "what the user is seeing" is part of the object graph, making the UI introspectable.

### Layout Model

Conch uses a tiling workspace that expands as new Conchos appear. Conchos may own sub-Conchos
(nested tiles). This yields the "Quix boxes" feeling as the display grows organically.

## Notes

Conch depends on Referee + Refract + CEO. It is a userland face of IrisOS, not part of Refract
itself.
