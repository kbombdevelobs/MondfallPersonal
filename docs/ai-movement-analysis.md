---
title: AI Movement & Command Deep-Dive Analysis
status: Active
owner_area: Enemy AI
created: 2026-04-04
last_updated: 2026-04-04
last_reviewed: 2026-04-04
parent_doc: CLAUDE.md
related_docs:
  - docs/combat-system.md
  - docs/wave-system.md
  - docs/ecs-integration.md
---

# AI Movement & Command Deep-Dive Analysis

Systematic analysis of the current enemy AI movement, bunching behavior, pathfinding, and command hierarchy — with concrete improvement proposals and new rank-specific pickup weapons.

---

## Table of Contents

1. [Current Architecture Summary](#1-current-architecture-summary)
2. [Movement Analysis](#2-movement-analysis)
3. [Bunching Problems](#3-bunching-problems)
4. [Pathfinding Gaps](#4-pathfinding-gaps)
5. [Command Hierarchy Gaps](#5-command-hierarchy-gaps)
6. [Improvement Proposals](#6-improvement-proposals)
7. [Rank-Specific Pickup Weapons](#7-rank-specific-pickup-weapons)
8. [Implementation Priority](#8-implementation-priority)

---

## 1. Current Architecture Summary

### System Pipeline (per frame)

```
EcsOnUpdate:
  SysAITargeting     → face toward player (angular interpolation)
  SysAIBehavior      → decide move direction based on faction + rank + morale
  SysMoraleUpdate    → find nearest leader, recover/decay morale

EcsPostUpdate:
  SysCollisionAvoidance → push overlapping enemies apart
  SysPhysics            → apply velocity, gravity, structure collision, jumping
  SysAttack             → fire at player if in range
  SysMoraleCheck        → trigger flee/rally
```

### Key Files
| File | Lines | Responsibility |
|------|-------|---------------|
| `enemy_ai.c` | 368 | Targeting, behavior decisions, collision avoidance |
| `enemy_physics.c` | 189 | Movement application, gravity, structure slide, attack |
| `enemy_morale.c` | 213 | Leader tracking, morale recovery/decay, flee/rally |
| `enemy_spawn.c` | 121 | Entity creation with rank multipliers |
| `enemy_components.h` | 183 | All component structs and enums |

---

## 2. Movement Analysis

### 2.1 Current Movement Model

Each enemy computes a `moveDir` vector in `SysAIBehavior`, which is written into `EcVelocity`. `SysPhysics` then applies it to position.

**Soviet movement** (enemy_ai.c:103-149):
- `dist > 4`: forward + 0.6x strafe → diagonal rush
- `dist <= 4`: 1.5x strafe + 0.2x forward → circle-strafe
- Random dodge: 2/80 chance per frame when `dist < 12`
- NCO: overrides to charge closer (0.6x preferredDist)
- Officer: overrides to hold at 0.8x preferredDist with strafe

**American movement** (enemy_ai.c:150-243):
- Cover-seeking: iterates ALL chunks + ALL rocks looking for cover
- Structure flanking: detects blocking bases, swings wide
- Distance management: retreat if too close, advance if too far, strafe in sweet spot
- NCO: tighter strafe at mid-range
- Officer: always retreats if no cover found

### 2.2 Movement Issues Identified

**Issue M1: No velocity smoothing / momentum**
- `vel.velocity` is set to a brand-new direction every frame
- Causes instant 180-degree movement direction changes
- Enemies feel weightless and jittery, not like suited astronauts on the Moon
- **Fix**: Add acceleration-based movement with momentum decay

**Issue M2: Speed is binary**
- Enemy is either moving at full `cs.speed` or stopped
- No gradual speed changes, no slow-down when approaching preferred distance
- **Fix**: Speed should scale with distance-to-target relative to preferred distance

**Issue M3: Moon jumping is random noise, not purposeful**
- `SysPhysics` line 67-69: enemies jump randomly with `rand()` timers
- No tactical reason to jump — just visual chaos
- **Fix**: Jump should be contextual (dodge incoming fire, traverse craters, cross rilles)

**Issue M4: Strafe direction changes are timer-based, not reactive**
- `strafeTimer` counts down and flips `strafeDir` — no connection to gameplay
- **Fix**: Strafe should change in response to incoming fire or terrain obstacles

---

## 3. Bunching Problems

### 3.1 Current Anti-Bunching

`SysCollisionAvoidance` (enemy_ai.c:264-301):
- O(N^2) nested iteration: for each enemy, iterates ALL alive enemies
- `separationRadius = 3.5f` — fixed, doesn't scale with group size
- Push force: `(3.5 - dist) / 3.5 * 4.0` — linear falloff
- `COLLISION_CAP = 8` limits checks in test mode only
- Push is added directly to velocity (no clamping)

### 3.2 Bunching Issues Identified

**Issue B1: O(N^2) is the primary performance bottleneck**
- 50 enemies = 2,500 distance checks per frame
- No spatial partitioning — every enemy checks every other enemy
- `g_aiAliveQuery` iterates the full alive set for each outer enemy
- **Fix**: Spatial hash grid (cell size = separationRadius). O(N) average.

**Issue B2: Separation force doesn't account for movement direction**
- Two enemies moving the same direction still push apart even if not converging
- Creates oscillation: enemies jitter back and forth
- **Fix**: Only apply separation when closing velocity is positive (approaching)

**Issue B3: Enemies from the same lander all spawn at the same point**
- `LanderSpawnWave` deploys enemies from the lander's landing position
- All 4-8 enemies per lander start at nearly identical positions
- Initial bunching causes a chaotic push-apart explosion
- **Fix**: Spawn enemies in a spread formation around the lander (radius 3-5 units)

**Issue B4: No formation awareness**
- Soviets should charge in a loose line/wedge, not a clump
- Americans should space out in fire-and-maneuver pairs
- **Fix**: Add simple formation offsets based on spawn index + faction

**Issue B5: Collision avoidance ignores rank**
- Officers get pushed around by troopers — breaks command-from-rear fantasy
- **Fix**: Officers should have stronger push-out force (others give way to them)

---

## 4. Pathfinding Gaps

### 4.1 Current "Pathfinding"

There is no actual pathfinding. Movement is purely reactive:
- Soviet: move toward player + strafe offset
- American: seek nearest rock for cover, or distance-manage
- Both: detect structures blocking direct path → swing wide

### 4.2 Pathfinding Issues Identified

**Issue P1: Cover search is O(chunks * rocks) — expensive and naive**
- `enemy_ai.c:184-208`: iterates every chunk's every rock
- No caching of cover positions between frames
- Same rock may be "claimed" by multiple enemies simultaneously
- **Fix**: Pre-compute cover positions per chunk. Add cover reservation system.

**Issue P2: No terrain awareness**
- Enemies walk straight up steep crater walls at full speed
- No rille-following or rille-crossing logic
- No advantage of high ground
- **Fix**: Sample `WorldGetHeight` at candidate positions; penalize steep slopes in movement scoring

**Issue P3: Structure avoidance is binary**
- Structure detection checks one direction only — "is base between me and player?"
- If yes: add strafe. If no: ignore all structures.
- No avoidance of structures that are to the side of the path
- **Fix**: Continuous obstacle steering — sample positions ahead and to sides, steer away from obstructions

**Issue P4: No crater navigation**
- Large craters are natural arena features but enemies don't use them
- Could use crater rims for cover (Americans) or funnel through crater bowls (Soviets)
- **Fix**: Detect nearby craters via `WorldGetHeight` sampling; incorporate into cover/path decisions

**Issue P5: Enemies can't coordinate flanking**
- Soviet "split around base" is individual: each enemy independently picks a strafe direction
- No guarantee they actually split — could all go the same way
- **Fix**: Use entity index parity or faction-shared state to force alternating flank sides

---

## 5. Command Hierarchy Gaps

### 5.1 Current Command Model

- Officers and NCOs have stat multipliers but minimal behavioral differentiation
- Morale system: leader death → morale hit → flee → rally. Functional but shallow.
- No actual "commands" — leaders don't direct troop movement

### 5.2 Command Issues Identified

**Issue C1: Officers don't command — they're just tough enemies at the back**
- Officer behavior: hold at preferred distance, dodge more
- No influence on trooper behavior beyond morale proximity
- **Fix**: Officers should set attack vectors — nearby troopers bias toward officer's facing direction

**Issue C2: NCOs don't lead charges — they just have more HP**
- Soviet NCO: charges closer. American NCO: tighter strafe. That's it.
- **Fix**: NCOs should be "squad anchors" — troopers within LEADERSHIP_RADIUS form a loose group around the NCO

**Issue C3: No squad cohesion**
- Enemies are fully independent agents with no group awareness
- Results in a scattered mob, not organized squads
- **Fix**: Assign enemies to implicit squads (by lander). Squad members bias movement toward squad centroid.

**Issue C4: Morale system creates per-frame temporary queries**
- `SysMoraleUpdate` creates and destroys a query every frame to find leaders
- This is unnecessary overhead — should be a stored query
- **Fix**: Cache the leader query at registration time, like `g_aiAliveQuery`

---

## 6. Improvement Proposals

### 6.1 Momentum-Based Movement (addresses M1, M2)

Add `EcSteering` component:

```c
typedef struct {
    Vector3 desiredVelocity;  // what AI wants
    Vector3 currentVelocity;  // what physics uses (smoothed)
    float acceleration;        // how fast to reach desired (rank-dependent)
    float maxSpeed;
} EcSteering;
```

In `SysPhysics`, blend toward desired velocity:
```c
float accel = steer.acceleration * dt;
steer.currentVelocity = Vector3Lerp(steer.currentVelocity, steer.desiredVelocity, accel);
```

- Troopers: accel = 4.0 (responsive)
- NCOs: accel = 3.5 (steady)
- Officers: accel = 2.5 (measured, deliberate)

### 6.2 Spatial Hash for Collision (addresses B1)

```c
#define SPATIAL_CELL_SIZE 4.0f
#define SPATIAL_GRID_DIM 64

typedef struct {
    ecs_entity_t entities[8];
    int count;
} SpatialCell;

SpatialCell g_spatialGrid[SPATIAL_GRID_DIM][SPATIAL_GRID_DIM];
```

- Rebuild grid each frame in a pre-pass system (O(N))
- Collision avoidance checks only adjacent cells (O(N) average, O(1) per entity)
- Grid centered on player position, wraps for out-of-bounds

### 6.3 Formation Spawning (addresses B3, B4)

Modify `enemy_spawn.c` or lander deployment:
- Soviet: spawn in wedge formation (NCO at point, officer at rear)
- American: spawn in staggered pairs (fire teams), officer flanked by NCO

```c
Vector3 FormationOffset(int index, int total, EnemyType faction) {
    if (faction == ENEMY_SOVIET) {
        // Wedge: angle out from center
        float angle = ((float)index / total - 0.5f) * 1.2f; // ~70 degree spread
        float depth = (float)index * 1.5f;
        return (Vector3){ sinf(angle) * depth * 2.0f, 0, -depth };
    } else {
        // Staggered pairs
        int pair = index / 2;
        int side = (index % 2) ? 1 : -1;
        return (Vector3){ side * 3.0f, 0, -pair * 4.0f };
    }
}
```

### 6.4 Squad Cohesion System (addresses C2, C3)

New `EcSquad` component:
```c
typedef struct {
    int squadId;            // assigned at spawn (lander index)
    ecs_entity_t anchor;    // NCO entity this trooper follows
    Vector3 squadCentroid;  // updated each frame
} EcSquad;
```

New `SysSquadCohesion` system (EcsOnUpdate, before SysAIBehavior):
- Compute centroid of each squad's alive members
- Troopers: add bias toward centroid (strength 0.3) to prevent scattering
- If anchor NCO is dead: squad dissolves, members become independent

### 6.5 Slope-Aware Movement (addresses P2)

In `SysPhysics`, before applying position:
```c
float currentH = WorldGetHeight(tr.position.x, tr.position.z);
float targetH = WorldGetHeight(newX, newZ);
float slope = fabsf(targetH - currentH) / (speed * dt);
if (slope > 0.8f) {
    // Too steep — reduce speed proportionally
    float slopePenalty = 1.0f - (slope - 0.8f) * 2.0f;
    if (slopePenalty < 0.2f) slopePenalty = 0.2f;
    newX = tr.position.x + vel.velocity.x * dt * slopePenalty;
    newZ = tr.position.z + vel.velocity.z * dt * slopePenalty;
}
```

### 6.6 Cover Reservation (addresses P1)

```c
#define MAX_COVER_SPOTS 64

typedef struct {
    Vector3 position;
    ecs_entity_t claimedBy;  // 0 = available
    float quality;            // how good the cover is (angle to player)
} CoverSpot;

// Singleton component
typedef struct {
    CoverSpot spots[MAX_COVER_SPOTS];
    int count;
    float rebuildTimer;  // rebuild every 2 seconds, not every frame
} EcCoverCache;
```

Americans request cover spots instead of searching rocks each frame. Once claimed, other enemies skip that spot.

### 6.7 Officer Command Influence (addresses C1)

Officers project a "command cone" in their facing direction:
- Troopers within LEADERSHIP_RADIUS whose position is within officer's forward 90-degree cone get a movement bias toward that direction
- Creates emergent "officer points, troops attack that way" behavior
- No explicit orders — just a subtle directional pull

---

## 7. Rank-Specific Pickup Weapons

Currently all enemies drop the same weapon regardless of rank:
- Soviet → KOSMOS-7 SMG (PPSh)
- American → LIBERTY BLASTER

### 7.1 NCO Weapons (New)

**Soviet NCO: "KS-23 Molot" (Space Assault Shotgun)**
- Faction: Soviet NCO
- Behavior: Close-range devastation, wide spread
- Ammo: 12 shells
- Fire rate: 0.35s (pump action)
- Damage: 60 per pellet, 5 pellets per shot (300 potential)
- Spread: 0.08 radians (wide cone)
- Range: 25 units (short but deadly)
- Recoil: 1.8 (heavy pump kick)
- Visual: Chunky double-barrel, brass accents, red muzzle flash
- Sound: Deep boom + brass tinkle
- Pickup name: "KS-23 MOLOT"

**American NCO: "M8A1 Starhawk" (Burst Rifle)**
- Faction: American NCO
- Behavior: Precision 3-round burst, good at mid-range
- Ammo: 36 rounds (12 bursts)
- Fire rate: 0.08s between burst rounds, 0.5s between bursts
- Damage: 45 per round (135 per burst)
- Spread: 0.015 radians (tight grouping)
- Range: 60 units (mid-long range)
- Recoil: 1.2 (controlled burst kick)
- Visual: Sleek angular rifle, blue energy rails, chrome barrel
- Sound: Sharp triple-crack
- Pickup name: "M8A1 STARHAWK"

### 7.2 Officer Weapons (New)

**Soviet Officer: "Zarya TK-4" (Heavy Caliber Charged Pistol)**
- Faction: Soviet Officer
- Behavior: Hold fire to charge, release for massive single shot
- Ammo: 6 charges
- Fire rate: 0.8s (charge time — hold to power up, release to fire)
- Damage: 150 uncharged, up to 500 fully charged (1.5s hold)
- Spread: 0.005 (pinpoint)
- Range: 50 units
- Recoil: 2.2 (massive kick on full charge)
- Visual: Ornate pistol with glowing red energy core, charge indicator glow, gold trim
- Sound: Rising whine during charge, thunderclap on release
- Pickup name: "ZARYA TK-4"

**American Officer: "ARC-9 Longbow" (Advanced Rail Cannon)**
- Faction: American Officer
- Behavior: Fires a beam that pierces through multiple enemies
- Ammo: 4 shots
- Fire rate: 1.0s
- Damage: 200 per enemy hit (pierces up to 5)
- Spread: 0 (perfect line)
- Range: 70 units
- Recoil: 1.5 (sustained push)
- Visual: Long thin rod, blue-white energy coils, lens at tip, piercing beam with afterglow trail
- Sound: Electric lance crack + sustained hum
- Pickup name: "ARC-9 LONGBOW"

### 7.3 Drop System Changes

Modify `PickupDrop` signature to include rank:

```c
typedef enum {
    PICKUP_KOSMOS7,         // KOSMOS-7 SMG (existing Soviet trooper)
    PICKUP_LIBERTY,         // LIBERTY BLASTER (existing American trooper)
    PICKUP_KS23_MOLOT,      // KS-23 Molot (Soviet NCO shotgun)
    PICKUP_M8A1_STARHAWK,   // M8A1 Starhawk (American NCO burst rifle)
    PICKUP_ZARYA_TK4,       // Zarya TK-4 (Soviet Officer charged pistol)
    PICKUP_ARC9_LONGBOW,    // ARC-9 Longbow (American Officer piercing beam)
} PickupWeaponType;
```

Each `DroppedGun` stores a `PickupWeaponType` instead of bare `EnemyType`. The grab function reads the type and sets appropriate stats. All existing code paths that call `PickupDrop` need to pass rank info from the dying entity's `EcRank` component.

### 7.4 Config Constants (New)

```c
// KS-23 Molot — Soviet NCO Shotgun
#define PICKUP_MOLOT_AMMO       12
#define PICKUP_MOLOT_FIRE_RATE  0.35f
#define PICKUP_MOLOT_DAMAGE     60.0f   // per pellet
#define PICKUP_MOLOT_PELLETS    5
#define PICKUP_MOLOT_SPREAD     0.08f
#define PICKUP_MOLOT_RANGE      25.0f
#define PICKUP_MOLOT_RECOIL     1.8f

// M8A1 Starhawk — American NCO Burst Rifle
#define PICKUP_STARHAWK_AMMO      36
#define PICKUP_STARHAWK_FIRE_RATE 0.08f   // between burst rounds
#define PICKUP_STARHAWK_BURST_CD  0.5f    // between bursts
#define PICKUP_STARHAWK_DAMAGE    45.0f
#define PICKUP_STARHAWK_RANGE     60.0f
#define PICKUP_STARHAWK_RECOIL    1.2f

// Zarya TK-4 — Soviet Officer Charged Pistol
#define PICKUP_ZARYA_AMMO        6
#define PICKUP_ZARYA_CHARGE_TIME 1.5f
#define PICKUP_ZARYA_DMG_MIN     150.0f
#define PICKUP_ZARYA_DMG_MAX     500.0f
#define PICKUP_ZARYA_RANGE       50.0f
#define PICKUP_ZARYA_RECOIL      2.2f

// ARC-9 Longbow — American Officer Piercing Beam
#define PICKUP_LONGBOW_AMMO      4
#define PICKUP_LONGBOW_FIRE_RATE 1.0f
#define PICKUP_LONGBOW_DAMAGE    200.0f
#define PICKUP_LONGBOW_PIERCE    5       // max enemies pierced
#define PICKUP_LONGBOW_RANGE     70.0f
#define PICKUP_LONGBOW_RECOIL    1.5f
```

---

## 8. Implementation Priority

Ordered by gameplay impact / effort ratio:

| Priority | Proposal | Impact | Effort | Files Touched |
|----------|----------|--------|--------|---------------|
| **P0** | Spatial hash collision (6.2) | High perf + less jitter | Medium | enemy_ai.c, new spatial_hash.h/c |
| **P0** | Formation spawning (6.3) | Eliminates spawn bunching | Low | enemy_spawn.c, lander.c |
| **P1** | Momentum movement (6.1) | Better feel, less jitter | Medium | enemy_components.h, enemy_ai.c, enemy_physics.c |
| **P1** | Squad cohesion (6.4) | Organized combat feel | Medium | enemy_components.h, enemy_ai.c, enemy_morale.c |
| **P1** | NCO pickup weapons (7.1) | Reward for hard kills | Medium | pickup.h/c, config.h, combat_ecs.c |
| **P2** | Officer pickup weapons (7.2) | Unique power fantasy | High | pickup.h/c, config.h, combat_ecs.c |
| **P2** | Cover reservation (6.6) | Smarter Americans | Medium | enemy_ai.c, enemy_components.h |
| **P2** | Slope-aware movement (6.5) | Terrain integration | Low | enemy_physics.c |
| **P3** | Officer command influence (6.7) | Emergent tactics | Low | enemy_ai.c |
| **P3** | Cached morale query (C4) | Minor perf win | Low | enemy_morale.c |

### Dependency Graph

```
Formation Spawning (6.3)
  └─> Squad Cohesion (6.4) [needs squad IDs from spawn]
       └─> Officer Command (6.7) [builds on squad structure]

Spatial Hash (6.2) [independent, do first]

Momentum Movement (6.1) [independent, do anytime]

Cover Reservation (6.6)
  └─> Slope-Aware Movement (6.5) [cover quality includes slope]

NCO Weapons (7.1) [independent]
  └─> Officer Weapons (7.2) [same pattern, more complex]
```

---

## Appendix: Current Constants Reference

| Constant | Value | Used In |
|----------|-------|---------|
| `MAX_ENEMIES` | 50 | Max alive entities |
| `COLLISION_CAP` | 8 | Test mode collision check limit |
| `AI_TURN_SPEED` | 6.0 | Facing angle interpolation speed |
| `AI_STRAFE_TIMER_BASE` | 1.5s | Min time before strafe flip |
| `AI_STRAFE_TIMER_RAND` | 2.0s | Random addition to strafe timer |
| `AI_DODGE_COOLDOWN` | 3.0s | Min time between dodges |
| `LEADERSHIP_RADIUS` | 15.0 | Morale influence range |
| `MORALE_FLEE_THRESHOLD` | 0.25 | Morale level triggering flee |
| `SOVIET_SPEED` | 5.5 | Base Soviet move speed |
| `AMERICAN_SPEED` | 5.0 | Base American move speed |
| `SOVIET_PREFERRED_DIST` | 8.0 | Soviet ideal engagement range |
| `AMERICAN_PREFERRED_DIST` | 18.0 | American ideal engagement range |
