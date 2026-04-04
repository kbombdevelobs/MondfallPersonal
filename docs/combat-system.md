---
title: Combat System
status: Active
owner_area: Core
created: 2026-04-04
last_updated: 2026-04-04
last_reviewed: 2026-04-04
parent_doc: CLAUDE.md
related_docs:
  - src/enemy/CLAUDE.md
  - src/weapon/CLAUDE.md
  - docs/ecs-integration.md
---

# Combat System

Combat is orchestrated by the ECS-based combat module (`combat_ecs.c/h`), which replaced the legacy `combat.c`. It connects the weapon system, pickup weapons, and ECS enemy entities through four processing functions called each frame from `main.c`.

## EcsCombatContext

The central struct that wires all systems together:

```c
typedef struct {
    Player *player;
    Weapon *weapon;
    PickupManager *pickups;
    Game *game;
    ecs_world_t *ecsWorld;
} EcsCombatContext;
```

Initialized once at startup and updated when the ECS world is reset (new game, restart).

## Combat Processing Functions

### EcsCombatProcessPickupFire

Handles firing of picked-up enemy weapons (KOSMOS-7 PPSh, LIBERTY BLASTER). Uses `PickupFire()` for rate limiting and ammo, then performs hit detection via ECS queries.

- **PPSh (Soviet)**: Normal-width ray hit detection (`EcsEnemyCheckHit`), applies damage, spawns 3 spread tracer beams per shot. On kill, drops a pickup weapon.
- **Liberty Blaster (American)**: Wide-area ray hit detection (`LibertyCheckHitEcs` with 2.0 unit padding), instant vaporize kill on any hit, spawns a thick rail beam.

### EcsCombatProcessWeaponFire

Handles the three player weapons:

- **Mond-MP40**: Hitscan ray from camera position. Applies `mp40Damage` on hit. Kills drop pickup weapons.
- **Raketenfaust**: Not handled here (beam is continuous, see `EcsCombatProcessBeam`). Fire button launches projectiles tracked separately.
- **Jackhammer**: Sphere hit detection at melee range. Applies lunge physics (velocity + upward boost). On hit, calls `EcsEnemyEviscerate()` for dismemberment death.

### EcsCombatProcessProjectiles

Iterates active projectiles (Raketenfaust rockets). On enemy sphere collision or ground hit:
- Damages all enemies within blast radius (damage falloff with distance)
- Spawns explosion visual via `WeaponSpawnExplosion()`
- Kills drop pickup weapons

### EcsCombatProcessBeam

Active while the Raketenfaust beam is firing (`weapon.raketenFiring`). Each frame:
- Updates beam start/end positions from barrel to max range
- Applies massive knockback to the player (push backward + upward lift)
- Kills everything in the beam path via ray-box intersection, triggering `EcsEnemyVaporize()`

## Hit Detection Functions

| Function | Method | Used By |
|----------|--------|---------|
| `EcsEnemyCheckHit()` | Ray-AABB (0.5 unit half-width) | MP40, PPSh |
| `LibertyCheckHitEcs()` | Ray-AABB (2.0 unit padding) | Liberty Blaster |
| `EcsEnemyCheckSphereHit()` | Sphere overlap (0.8 unit padding) | Jackhammer, projectiles |

All hit detection queries filter on entities that have both `EcTransform` and `EcAlive` components.

## Weapon-to-Death-Type Mapping

| Weapon | Death Type | Function Called |
|--------|-----------|----------------|
| Mond-MP40 | Ragdoll blowout (60%) or crumple (40%) | `EcsEnemyDamage()` (health reaches 0) |
| Raketenfaust beam | Vaporize | `EcsEnemyVaporize()` |
| Raketenfaust projectile | Ragdoll/crumple (via damage) | `EcsEnemyDamage()` |
| Jackhammer | Eviscerate (dismemberment) | `EcsEnemyEviscerate()` |
| KOSMOS-7 PPSh | Ragdoll/crumple (via damage) | `EcsEnemyDamage()` |
| LIBERTY BLASTER | Vaporize | `EcsEnemyVaporize()` |

## Kill Tracking and Pickup Drops

Every kill increments `game->killCount`. When an enemy dies from damage (health reaching zero) or melee, `PickupDrop()` is called with the enemy's position and faction type, spawning a collectible weapon on the ground.

## Key Files

| File | Role |
|------|------|
| `src/combat_ecs.c` | All combat processing logic |
| `src/combat_ecs.h` | `EcsCombatContext` struct and function declarations |
| `src/enemy/enemy_systems.c` | `EcsEnemyDamage()`, `EcsEnemyVaporize()`, `EcsEnemyEviscerate()` |
| `src/weapon.c` | Weapon state, fire logic, projectile tracking |
| `src/pickup.c` | Pickup weapon fire, ammo, grab mechanics |
