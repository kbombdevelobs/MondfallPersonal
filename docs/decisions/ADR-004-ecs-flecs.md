---
title: "ADR-004: ECS Integration with flecs"
status: Accepted
owner_area: Core
created: 2026-04-04
last_updated: 2026-04-04
last_reviewed: 2026-04-04
parent_doc: docs/decisions/README.md
related_docs:
  - docs/ecs-integration.md
  - src/enemy/CLAUDE.md
---

# ADR-004: ECS Integration with flecs

## Context

The enemy system originally used a monolithic manager pattern in `enemy.c`, where AI behaviors, component state, hit detection, spawning, and rendering were all interleaved in a single file. As faction-specific behaviors (Soviet vs. American), multiple death types, and pickup weapon interactions were added, the code became increasingly difficult to extend. Adding a new AI behavior or death animation required touching multiple interleaved sections.

## Decision

Adopt the flecs ECS library to restructure the enemy system into discrete components and systems. Enemy state is decomposed into components (position, health, AI behavior, faction, death state), and logic is organized into systems that operate on component queries. The migration moves from the monolithic `enemy.c` to files like `enemy_components.c`, `enemy_systems.c`, `enemy_spawn.c`, and `enemy_draw_ecs.c`.

## Alternatives Considered

- **Keep the manager pattern:** Simpler and already working, but scaling poorly. Every new feature required careful surgery across tightly coupled code paths.
- **Custom ECS implementation:** Would avoid the vendor dependency but is not worth the engineering effort when flecs is mature, C-native, and well-documented.
- **EnTT:** A popular ECS library, but C++ only. Incompatible with the project's C99 codebase.

## Consequences

- Cleaner separation of concerns: AI logic, rendering, spawning, and death handling are independent systems.
- Composable behaviors: new AI states or faction-specific logic can be added by creating new systems that query relevant components.
- Adds a vendor dependency (flecs source in `vendor/`), increasing the build footprint.
- Learning curve for ECS patterns, particularly flecs query syntax and system ordering.
- Legacy files (`enemy.c`, `combat.c`) remain on disk but are excluded from compilation; the active code lives in the ECS-based files.
