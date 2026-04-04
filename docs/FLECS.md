# Flecs ECS — Enemy AI Architecture

## Overview

All enemy AI in Mondfall runs through **Flecs 4.1** (Entity Component System). Flecs is the central AI brain — every enemy is a Flecs entity, all behavior is driven by Flecs systems, and all future AI work must go through Flecs.

## How It Works

### Entities
Each enemy is a Flecs entity (a 64-bit ID). Entities have no inherent behavior — they're just IDs with components attached.

### Components (data only, no logic)
Components are plain C structs attached to entities. Defined in `src/enemy/enemy_components.h`:

| Component | Purpose | Fields |
|-----------|---------|--------|
| `EcTransform` | Position + facing | position (Vector3), facingAngle |
| `EcVelocity` | Movement + jump | velocity (Vector3), vertVel, jumpTimer |
| `EcFaction` | Soviet or American | type (EnemyType enum) |
| `EcAlive` | Tag: entity is alive | (no data) |
| `EcCombatStats` | Health, damage, range | health, maxHealth, speed, damage, attackRange, attackRate, preferredDist |
| `EcAIState` | Behavior state machine | behavior, strafeDir, dodgeTimer, burstShots, etc. |
| `EcAnimation` | Visual state | animState, walkCycle, shootAnim, hitFlash, muzzleFlash, deathAngle |
| `EcDying` | Tag: ragdoll death | (no data) |
| `EcVaporizing` | Tag: beam death | (no data) |
| `EcEviscerating` | Tag: jackhammer death | (no data) |
| `EcRagdollDeath` | Ragdoll physics data | spinX/Y/Z, ragdollVel, deathTimer, deathStyle |
| `EcVaporizeDeath` | Vaporize animation data | vaporizeTimer, vaporizeScale, deathTimer |
| `EcEviscerateDeath` | Eviscerate limb data | evisTimer, evisDir, evisLimbVel[6], evisLimbPos[6], evisBloodTimer[6] |
| `EcEnemyResources` | Singleton: shared models/sounds | mdlVisor, mdlArm, mdlBoot, fire/death sounds |
| `EcGameContext` | Singleton: per-frame game state | playerPos, camera, testMode, playerDamageAccum, killCount |

### Systems (logic, no data)
Systems are functions that run each frame on matching entities. Defined in `src/enemy/enemy_systems.c`:

| System | Phase | Query | Purpose |
|--------|-------|-------|---------|
| `SysAITargeting` | OnUpdate | Transform, AIState, Alive | Turn to face player |
| `SysAIBehavior` | OnUpdate | Transform, AIState, CombatStats, Faction, Alive, Velocity | Soviet rush / American tactical |
| `SysCollisionAvoidance` | PostUpdate | Transform, Velocity, Alive | Push apart overlapping enemies |
| `SysPhysics` | PostUpdate | Transform, Velocity, Animation, Alive | Gravity, ground collision, jumping |
| `SysAttack` | PostUpdate | Transform, CombatStats, AIState, Faction, Animation, Alive | Fire at player, burst timing |
| `SysRagdollDeath` | OnUpdate | Transform, Animation, RagdollDeath, Dying | Ragdoll/crumple physics |
| `SysVaporizeDeath` | OnUpdate | Transform, Animation, VaporizeDeath, Vaporizing | Disintegration phases |
| `SysEviscerateDeath` | OnUpdate | Transform, EviscerateDeath, Eviscerating | Limb physics |

### Execution Flow

```
main.c STATE_PLAYING:
  1. Set EcGameContext singleton (playerPos, camera, testMode)
  2. ecs_progress(ecsWorld, dt)  ← runs ALL systems in phase order
     ├── OnUpdate: SysAITargeting → SysAIBehavior → death systems
     └── PostUpdate: SysCollisionAvoidance → SysPhysics → SysAttack
  3. Read EcGameContext.playerDamageAccum → apply to player
  4. EcsCombatProcess*() → weapon hit detection + damage
  5. EcsEnemyManagerDraw() → render from ECS data
```

## Key Files

| File | Role |
|------|------|
| `vendor/flecs/flecs.h/c` | Flecs 4.1.5 library |
| `src/ecs_world.h/c` | Global world wrapper |
| `src/enemy/enemy_components.h/c` | Component definitions + registration |
| `src/enemy/enemy_systems.h/c` | All AI/physics/death systems |
| `src/enemy/enemy_spawn.h/c` | Entity creation functions |
| `src/enemy/enemy_draw_ecs.h/c` | ECS-based rendering |
| `src/combat_ecs.h/c` | Combat with Flecs queries |

## How to Add New AI Behavior

### Adding a new component
1. Define the struct in `enemy_components.h`
2. Add `ECS_COMPONENT_DECLARE` in `enemy_components.c`
3. Add `ECS_COMPONENT_DEFINE` in `EcsEnemyComponentsRegister()`
4. Add `extern ecs_entity_t ecs_id(YourComponent)` to the header

### Adding a new system
1. Write the system callback in `enemy_systems.c`:
   ```c
   static void SysYourSystem(ecs_iter_t *it) {
       YourComp *yc = ecs_field(it, YourComp, 0);
       for (int i = 0; i < it->count; i++) {
           // process yc[i]
       }
   }
   ```
2. Register it in `EcsEnemySystemsRegister()`:
   ```c
   ecs_system_init(world, &(ecs_system_desc_t){
       .entity = ecs_entity(world, {
           .name = "SysYourSystem",
           .add = ecs_ids(ecs_dependson(EcsOnUpdate))
       }),
       .query.terms = { { .id = ecs_id(YourComp) } },
       .callback = SysYourSystem
   });
   ```

### Adding a new enemy type
1. Create a new prefab in `EcsEnemyComponentsRegister()` with preset stats
2. Update `EcsEnemySpawnAt()` to handle the new type

## Sparse Components Pattern

Death data is **only attached when needed**:
- When alive: entity has `EcAlive` tag, no death components
- When killed: `EcAlive` removed, `EcDying` + `EcRagdollDeath` added
- Death systems only iterate entities WITH death tags → zero cost for alive enemies
- When death timer expires: `ecs_delete()` removes entity entirely

## Performance Features

- **Cache-friendly**: components stored in contiguous arrays per type
- **Sparse death data**: only dying enemies carry ragdoll/vaporize/eviscerate data
- **Staggered AI**: in test mode, only 1/4 enemies run full AI per frame
- **Capped collision**: test mode limits neighbor checks to 8
- **Frustum culling + LOD**: rendering skips off-screen enemies, uses simplified models at distance
