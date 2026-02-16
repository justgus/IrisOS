# AR-0011 — Vizier Interpretation Layer (Recommendation)

- Status: Accepted
- Date: 2026-02-16
- Owners: Mike

## Context

IrisOS presents a live object graph to users via Conch. The system needs a consistent, interpretable
layer that maps artifacts and object relationships into renderable views without forcing producers
to "do UI." This layer should be grounded in Refract (truth/structure) and provide mediation and
interpretation rather than control.

## Recommendation

Define **Refract::Vizier** as the interpretation layer that:

- observes the object graph and Refract metadata,
- selects appropriate view models (Viz objects), and
- guides Conch in spawning Concho views.

### Responsibilities

- Interpret artifacts and relationship patterns (produced, progress, diagnostic, stream, etc.).
- Apply routing rules:
  - If an object is `Viz::TextLog`, spawn a Log Concho.
  - If an object is `Viz::Table`, spawn a Table Concho.
  - If an object is `CEO::Task`, spawn a Task Concho.
- Use Refract metadata:
  - preferred renderer for a type
  - operations and emitted artifact types

## Notes

Vizier mediates interpretation; it does not own rendering or UI state. Conch remains the shell UI,
and Viz remains the display artifact model.
