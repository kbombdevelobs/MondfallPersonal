---
title: "ADR-002: Fixed-Size Pools with Zero Heap Allocation"
status: Accepted
owner_area: Core
created: 2026-04-04
last_updated: 2026-04-04
last_reviewed: 2026-04-04
parent_doc: docs/decisions/README.md
related_docs: []
---

# ADR-002: Fixed-Size Pools with Zero Heap Allocation

## Context

The game manages multiple entity types at runtime: up to 50 enemies, 4 landers, 30 pickups, and various projectile/particle effects. These entities are created and destroyed frequently during gameplay (enemy spawns, deaths, weapon pickups, lander deployments). The memory management strategy needed to be simple, predictable, and free of runtime allocation failures.

## Decision

Use static arrays with compile-time `MAX_*` constants for all entity pools. No `malloc` or `free` calls occur during gameplay. Entity slots are reused by marking them active/inactive.

## Alternatives Considered

- **Dynamic heap allocation (`malloc`/`free`):** Flexible sizing but introduces fragmentation risk over long play sessions, potential allocation failures, and cache-unfriendly scattered memory.
- **Object pools with free lists:** More sophisticated reuse strategy with linked free lists for O(1) allocation. Adds unnecessary complexity when simple linear scans over small fixed arrays (50 enemies, 30 pickups) are fast enough.

## Consequences

- Predictable, bounded memory usage known at compile time.
- Zero heap fragmentation regardless of play duration.
- Simple linear iteration over contiguous arrays is cache-friendly.
- No allocation failure paths to handle during gameplay.
- Trade-off: hard caps on entity counts (`MAX_ENEMIES=50`, `MAX_LANDERS=4`, `MAX_PICKUPS=30`). Exceeding these limits silently drops new entities rather than growing storage.
- Adding a new entity type requires defining a new static array and max constant in `config.h`.
