# AR-0013 — Viz Display Subsystem (Recommendation)

- Status: Accepted
- Date: 2026-02-16
- Owners: Mike

## Context

To avoid ad-hoc UI artifacts, IrisOS needs a small, standardized set of display-oriented objects.
Objects should publish "UI-worthy" artifacts without performing rendering themselves.

## Recommendation

Define **Erector::Viz** as a small display subsystem with data-model-first objects. Conch renders
Viz objects into views; producers only emit Viz objects and relationships.

### Core Viz Object Types

- `Viz::Panel` (renderable unit)
- `Viz::TextLog` (append-only log stream)
- `Viz::Table` (rows/cols, live-updating)
- `Viz::Tree` (graph-ish display)
- `Viz::Metric` / `Viz::Gauge` (numbers over time)
- `Viz::Image` (optional later)

### Relationships

- Producer --produced--> Viz::*
- Viz::* --renderedAs--> Viz::Panel (optional indirection)
- ConchSession --shows--> Viz::Panel

## Notes

Viz defines display artifacts, not rendering. Rendering is performed by Conch with guidance from
Vizier and Refract.
