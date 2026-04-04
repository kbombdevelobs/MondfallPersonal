---
title: Enemy System
status: Active
owner_area: Enemy
created: 2026-04-04
last_updated: 2026-04-04
last_reviewed: 2026-04-04
parent_doc: CLAUDE.md
related_docs:
  - docs/combat-system.md
  - docs/wave-system.md
  - docs/ecs-integration.md
---

# Enemy System -- src/enemy/

Astronaut enemies (Soviet and American factions), AI behaviors, spawning, hit detection, death animations, and rendering. Recently migrated from a monolithic struct-based system to flecs ECS architecture.

## Files

| File | Status | Purpose |
|------|--------|---------|
| `enemy_components.h` | **Active** | All ECS component types: EcTransform, EcVelocity, EcFaction, EcAlive, EcCombatStats, EcAIState, EcAnimation, plus death-state components (EcDying, EcVaporizing, EcEviscerating, EcRagdollDeath, EcVaporizeDeath, EcEviscerateDeath), singletons (EcEnemyResources, EcGameContext), and prefab entity externs |
| `enemy_components.c` | **Active** | Component registration with flecs via ECS_COMPONENT_DEFINE macros; creates Soviet and American prefab entities with default stats from config.h |
| `enemy_systems.h` | **Active** | ECS system registration API and public functions: EcsEnemyDamage, EcsEnemyVaporize, EcsEnemyEviscerate, EcsEnemyCountAlive |
| `enemy_systems.c` | **Active** | Central API: EcsEnemyDamage, EcsEnemyVaporize, EcsEnemyEviscerate, EcsEnemyCountAlive, EcsEnemySystemsCleanup, EcsEnemySystemsRegister. Delegates to sub-system files. |
| `enemy_ai.h/c` | **Active** | AI systems: SysAITargeting, SysAIBehavior, SysCollisionAvoidance |
| `enemy_physics.h/c` | **Active** | Physics + combat systems: SysPhysics, SysAttack |
| `enemy_death_systems.h/c` | **Active** | Death systems: SysRagdollDeath, SysVaporizeDeath, SysEviscerateDeath, SysRadioTimer |
| `enemy_spawn.h` | **Active** | Spawn API: EcsEnemySpawnAt, EcsEnemySpawnAroundPlayer |
| `enemy_spawn.c` | **Active** | Entity creation with full component setup; places enemies at terrain height via WorldGetHeight; configures faction-specific stats and AI parameters |
| `enemy_draw_ecs.h` | **Active** | ECS draw bridge API: EcsEnemyManagerDraw, EcsEnemyResourcesInit/Unload |
| `enemy_draw_ecs.c` | **Active** | Queries ECS components, fills temporary legacy Enemy structs, and delegates to DrawAstronautModel. Handles resource loading (models, sounds, death audio). |
| `enemy_draw.h` | **Active** | Draw API: DrawAstronautModel (all states), DrawAstronautLOD1, DrawAstronautLOD2 |
| `enemy_draw.c` | **Active** | Alive astronaut rendering (torso/helmet/arms/legs/gun/backpack), LOD1/LOD2 draw functions. Delegates death states to enemy_draw_death.c. ~256 lines. |
| `enemy_draw_death.h` | **Active** | Death rendering API: DrawAstronautEviscerate, DrawAstronautVaporize, DrawAstronautRagdoll |
| `enemy_draw_death.c` | **Active** | Death state rendering: eviscerate (limbs fly apart with blood), vaporize (jerk/freeze/swell/pop/disintegrate), ragdoll (suit breach jets, blood pools). ~500 lines. |
| `enemy.h` | **Legacy** | Defines legacy Enemy and EnemyManager structs. Kept because DrawAstronautModel still takes these struct pointers. Not compiled independently -- included by enemy_draw_ecs.c and enemy_draw.c for struct definitions. |
| `../legacy/enemy.c` | **Legacy** | Moved to `src/legacy/`. Original monolithic enemy logic, superseded by ECS files. |

## Compiled Sources (from Makefile)

The Makefile compiles these files from src/enemy/:
- `enemy_components.c`
- `enemy_systems.c`, `enemy_ai.c`, `enemy_physics.c`, `enemy_death_systems.c`
- `enemy_spawn.c`
- `enemy_draw_ecs.c`
- `enemy_draw.c`, `enemy_draw_death.c`

Note: `enemy.c` has been moved to `src/legacy/`. The `enemy.h` header is still included transitively for struct definitions used by the draw code.

## Enemy States (ECS tags)

| State | ECS Tag/Component | Description |
|-------|-------------------|-------------|
| Alive | `EcAlive` tag | Active AI, shooting, moving |
| Dying | `EcDying` tag + `EcRagdollDeath` | Ragdoll blowout (60%) or crumple with blood pool (40%). Bodies persist 10-12s clamped to terrain. |
| Vaporizing | `EcVaporizing` tag + `EcVaporizeDeath` | Beam weapon death: jerk, freeze, optional swell/pop (15%), disintegrate |
| Eviscerating | `EcEviscerating` tag + `EcEviscerateDeath` | Jackhammer death: limbs separate with physics, blood spurts, bone fragments, weapon drops |
| Dead | Entity deleted | Cleaned up by SysCleanupDead when deathTimer expires |

## Factions

- **Soviet** (`ENEMY_SOVIET`): Red suits, gold helmets. Aggressive rush AI -- wide spread, circle-strafe close. PPSh-style weapons. Stats from `SOVIET_*` config constants.
- **American** (`ENEMY_AMERICAN`): Navy blue suits, silver helmets. Tactical AI -- seek cover behind rocks, maintain distance, retreat when hurt. Ray guns. Stats from `AMERICAN_*` config constants.

## AI Behaviors (EcAIState.behavior)

- `AI_ADVANCE` -- move toward player
- `AI_STRAFE` -- lateral movement while engaging
- `AI_SHOOT` -- stationary firing
- `AI_DODGE` -- evasive maneuver on cooldown
- `AI_RETREAT` -- fall back when hurt (American faction)

## Key Functions

| Function | File | Description |
|----------|------|-------------|
| `EcsEnemyComponentsRegister` | enemy_components.c | Register all component types and create faction prefabs |
| `EcsEnemySystemsRegister` | enemy_systems.c | Register all ECS systems (AI, movement, death, cleanup) |
| `EcsEnemySpawnAt` | enemy_spawn.c | Spawn enemy entity at world position (snapped to terrain) |
| `EcsEnemySpawnAroundPlayer` | enemy_spawn.c | Spawn enemy at random offset from player position |
| `EcsEnemyDamage` | enemy_systems.c | Apply damage; triggers ragdoll/crumple death if health <= 0 |
| `EcsEnemyVaporize` | enemy_systems.c | Instant beam death (no weapon drop) |
| `EcsEnemyEviscerate` | enemy_systems.c | Jackhammer death (drops weapon, limbs fly) |
| `EcsEnemyCountAlive` | enemy_systems.c | Count entities with EcAlive tag |
| `EcsEnemySystemsCleanup` | enemy_systems.c | Free stored alive query; safe to call multiple times |
| `EcsEnemyManagerDraw` | enemy_draw_ecs.c | Query ECS, fill legacy structs, delegate to DrawAstronautModel |
| `EcsEnemyResourcesInit` | enemy_draw_ecs.c | Load models, sounds, death audio into EcEnemyResources singleton |
| `DrawAstronautModel` | enemy_draw.c | Render a single astronaut in any state (alive/dying/vaporizing/eviscerating) |

## Dependencies

- **flecs ECS** (`vendor/flecs/`) -- entity-component-system framework; all enemy data lives in ECS components
- **ecs_world** (`src/ecs_world.c/h`) -- global ECS world lifecycle; enemy components and systems register through it
- **world system** (`src/world.c`, `src/world/world_noise.c`) -- `WorldGetHeight()` for terrain-clamping spawns, ragdoll settling, and blood pool placement
- **config.h** -- all faction stats (health, speed, damage, attack range/rate), AI timing constants, audio sample rate
- **sound_gen** (`src/sound_gen.c/h`) -- `SoundGenCreateWave()` for procedural enemy fire sounds
- **combat_ecs** (`src/combat_ecs.c/h`) -- hit detection dispatches to EcsEnemyDamage/Vaporize/Eviscerate
- **lander system** (`src/lander.c/h`) -- spawns enemies via EcsEnemySpawnAt when landers deploy

## Connections

- **combat-system.md** -- Combat resolution calls into enemy damage/vaporize/eviscerate functions
- **wave-system.md** -- Wave manager triggers lander deployment which spawns enemy entities
- **ecs-integration.md** -- Documents the ECS migration pattern; enemy system is the primary ECS consumer

## Maintenance Notes

- The legacy `enemy.c` is not compiled but remains in the repository. It can be deleted once the ECS migration is fully validated and no reference is needed.
- The `enemy.h` legacy structs (Enemy, EnemyManager) are still used as a rendering bridge -- `enemy_draw_ecs.c` fills temporary Enemy structs from ECS components and passes them to `DrawAstronautModel`. A future cleanup could make the draw code operate directly on ECS components.
- When adding new enemy types or behaviors, add ECS components in `enemy_components.h/c` and systems in `enemy_systems.c`. Do not modify `enemy.c`.
- Death sounds are loaded from mp3 files in `sounds/soviet_death_sounds/` and `sounds/american_death_sounds/`. Add new files there and update the path arrays in `EcsEnemyResourcesInit()` in `enemy_draw_ecs.c`.
- Enemy stats are defined as constants in `config.h` (SOVIET_HEALTH, AMERICAN_SPEED, etc.) and referenced during prefab creation and spawning.
