---
title: ECS Integration
status: Active
owner_area: Core
created: 2026-04-04
last_updated: 2026-04-04
last_reviewed: 2026-04-04
parent_doc: CLAUDE.md
related_docs:
  - src/enemy/CLAUDE.md
  - docs/combat-system.md
---

# ECS Integration

The game uses [flecs](https://github.com/SanderMertens/flecs), a high-performance Entity Component System library, for the enemy system and combat resolution. The flecs source is vendored at `vendor/flecs/`.

## Migration Status

The enemy system has been fully migrated to ECS. The rest of the game (player, weapons, world, landers, pickups) still uses the traditional manager-struct pattern.

### Active ECS Files (compiled in Makefile)

| File | Role |
|------|------|
| `src/ecs_world.c/h` | Global ECS world lifecycle (`GameEcsInit`, `GameEcsGetWorld`, `GameEcsFini`) |
| `src/enemy/enemy_components.c/h` | All ECS component type definitions and registration |
| `src/enemy/enemy_systems.c/h` | AI, physics, attack, death systems + damage/vaporize/eviscerate functions |
| `src/enemy/enemy_spawn.c/h` | `EcsEnemySpawnAt()`, `EcsEnemySpawnAroundPlayer()` |
| `src/enemy/enemy_draw_ecs.c/h` | ECS-aware enemy rendering, shared resource init/unload |
| `src/combat_ecs.c/h` | ECS-based combat processing (hit detection, damage, weapon fire) |
| `vendor/flecs/flecs.c` | Vendored flecs library |

### Legacy Files (NOT compiled, kept for reference)

| File | Status |
|------|--------|
| `src/enemy/enemy.c` | Replaced by enemy_systems.c + enemy_spawn.c + enemy_components.c |
| `src/combat.c` | Replaced by combat_ecs.c |

These legacy files are not listed in the Makefile `SRC` variable and are not part of the build. The old `enemy.h` and `enemy_draw.c/h` are still compiled because the ECS draw adapter (`enemy_draw_ecs.c`) fills temporary `Enemy` structs and delegates to the original draw functions (`DrawAstronautModel`, `DrawAstronautLOD1`, `DrawAstronautLOD2`).

## ECS World Lifecycle

The global ECS world is managed by `ecs_world.c`:

- `GameEcsInit()` -- Creates (or recreates) the flecs world. Called at startup and on every game restart/new game to ensure a clean enemy slate.
- `GameEcsGetWorld()` -- Returns the global `ecs_world_t*`.
- `GameEcsFini()` -- Destroys the world at shutdown.

After world creation, `main.c` calls the registration sequence:
1. `EcsEnemyComponentsRegister(world)` -- registers all component types and creates prefab entities
2. `EcsEnemySystemsRegister(world)` -- registers AI, physics, attack, and death systems
3. `EcsEnemyResourcesInit(world)` -- loads models and sounds into the `EcEnemyResources` singleton

## Components

Defined in `enemy_components.h`:

### Core Components (on every enemy)

| Component | Fields | Purpose |
|-----------|--------|---------|
| `EcTransform` | position, facingAngle | World position and orientation |
| `EcVelocity` | velocity, vertVel, jumpTimer | Movement physics |
| `EcFaction` | type (ENEMY_SOVIET / ENEMY_AMERICAN) | Faction identity |
| `EcAlive` | (tag, no data) | Presence indicates entity participates in AI/combat |
| `EcCombatStats` | health, maxHealth, speed, damage, attackRange, attackRate, preferredDist | Combat parameters |
| `EcAIState` | behavior, timers, strafe/dodge/burst state | AI decision state |
| `EcAnimation` | animState, walkCycle, shootAnim, hitFlash, muzzleFlash, deathAngle | Visual animation state |

### Death Components (sparse, added on death)

| Component | Added When | Data |
|-----------|-----------|------|
| `EcDying` | Ragdoll/crumple death | Tag only |
| `EcRagdollDeath` | Ragdoll/crumple death | Spin, velocity, timer, style (0=ragdoll, 1=crumple) |
| `EcVaporizing` | Beam weapon kill | Tag only |
| `EcVaporizeDeath` | Beam weapon kill | Timer, scale, style (0=normal, 1=swell+pop at 15%) |
| `EcEviscerating` | Jackhammer kill | Tag only |
| `EcEviscerateDeath` | Jackhammer kill | Limb positions/velocities, blood timers, direction |

### Singleton Components

| Component | Purpose |
|-----------|---------|
| `EcEnemyResources` | Shared models (visor, arm, boot), firing sounds, death sounds, radio timer |
| `EcGameContext` | Player position, camera, test mode flag, per-frame damage/kill accumulators |

## Systems

Registered in `EcsEnemySystemsRegister()`, executed via `ecs_progress(world, dt)`:

| System | Components Required | Purpose |
|--------|-------------------|---------|
| `SysAITargeting` | EcTransform, EcAIState, EcAlive | Face enemies toward player |
| `SysAIBehavior` | EcTransform, EcAIState, EcCombatStats, EcFaction, EcVelocity, EcAlive | Soviet rush / American tactical AI |
| `SysCollisionAvoidance` | EcTransform, EcVelocity, EcAlive | Push apart overlapping enemies |
| `SysPhysics` | EcTransform, EcVelocity, EcAnimation, EcAlive | Apply velocity, gravity, jumping, animation |
| `SysAttack` | EcTransform, EcAIState, EcCombatStats, EcFaction, EcAnimation, EcAlive | Enemy shooting at player |
| `SysRagdollDeath` | EcTransform, EcRagdollDeath, EcDying | Ragdoll/crumple physics and cleanup |
| `SysVaporizeDeath` | EcTransform, EcVaporizeDeath, EcVaporizing | Vaporize animation and cleanup |
| `SysEviscerateDeath` | EcTransform, EcEviscerateDeath, EcEviscerating | Dismemberment physics and cleanup |
| `SysRadioTimer` | (singleton) | Decay radio transmission display timer |

## Prefab Entities

Two prefab entities (`g_sovietPrefab`, `g_americanPrefab`) are created during component registration with default combat stats and AI state for each faction. These serve as templates but spawning currently sets components directly rather than instantiating from prefabs.

## Drawing Adapter

`enemy_draw_ecs.c` bridges ECS data to the legacy draw functions. `EcsEnemyManagerDraw()` queries all entities with `EcTransform + EcFaction + EcAnimation`, fills a temporary `Enemy` struct via `FillTempEnemy()`, and delegates to `DrawAstronautModel()` / `DrawAstronautLOD1()` / `DrawAstronautLOD2()`. In test mode, frustum culling and LOD selection are applied.

## Key Files

| File | Role |
|------|------|
| `src/ecs_world.c/h` | Global world management |
| `src/enemy/enemy_components.c/h` | Component definitions and registration |
| `src/enemy/enemy_systems.c/h` | All ECS systems |
| `src/enemy/enemy_spawn.c/h` | Entity creation |
| `src/enemy/enemy_draw_ecs.c/h` | ECS-to-draw adapter |
| `src/combat_ecs.c/h` | Combat processing using ECS queries |
| `vendor/flecs/` | Vendored flecs library |
