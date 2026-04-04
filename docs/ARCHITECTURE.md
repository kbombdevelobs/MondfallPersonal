---
title: Mondfall Architecture
status: Active
owner_area: Core
created: 2026-03-15
last_updated: 2026-04-04
last_reviewed: 2026-04-04
parent_doc: CLAUDE.md
related_docs:
  - docs/rendering-pipeline.md
  - docs/ecs-integration.md
  - docs/combat-system.md
  - docs/wave-system.md
---

# Mondfall Architecture

## System Overview

```
main.c (game loop)
  |
  |-- State Machine: MENU -> PLAYING <-> PAUSED -> GAME_OVER
  |
  |-- Update Phase (STATE_PLAYING)
  |     |-- PlayerUpdate        (player.c)    movement, physics, camera
  |     |-- GameUpdate          (game.c)      wave timer, progression
  |     |-- WeaponUpdate        (weapon.c)    fire rates, reload, projectiles
  |     |-- EnemyManagerUpdate  (enemy.c)     AI, animation, attacks
  |     |-- WorldUpdate         (world.c)     chunk loading around player
  |     |-- LanderManagerUpdate (lander.c)    descent, deployment, explosions
  |     |-- PickupManagerUpdate (pickup.c)    despawn timers, held weapon
  |     |-- Combat*             (combat.c)    hit detection, damage, kills
  |
  |-- Render Phase (3-layer pipeline)
        |-- Layer 1: 3D scene -> 640x360 RenderTexture
        |     |-- WorldDrawSky / WorldDraw
        |     |-- EnemyManagerDraw
        |     |-- LanderManagerDraw
        |     |-- PickupManagerDraw / WeaponDrawWorld
        |     |-- WeaponDrawFirst / PickupDrawFirstPerson
        |
        |-- Layer 2: CRT shader post-process -> window
        |
        |-- Layer 3: HUD -> separate RenderTexture -> visor curve shader
```

## Module Dependency Graph

```
config.h  <--  (included by all headers)
    |
    v
player.h  weapon.h  enemy.h  world.h  game.h  lander.h  pickup.h
    |         |         |       |        |        |          |
    v         v         v       v        v        v          v
player.c  weapon.c  enemy.c  world.c  game.c  lander.c  pickup.c
                                                  |          |
              sound_gen.h  <----+----+----+-------+----------+
                               |    |    |
                            weapon enemy lander pickup
                              .c    .c    .c    .c

combat.h  <-- player.h, weapon.h, enemy.h, pickup.h, game.h
    |
    v
combat.c  (orchestrates hit detection across all systems)

hud.h  <-- player.h, weapon.h, game.h, pickup.h, lander.h
    |
    v
hud.c  (draws all UI elements)
```

## Key Design Patterns

### Centralized Configuration (`config.h`)
All gameplay constants live in one header. Weapon stats, enemy stats, physics tuning,
pool sizes, audio parameters, and color definitions. Change game balance without
hunting through .c files.

### Manager Pattern
Each system uses a manager struct that owns its pool of entities:
- `EnemyManager` owns `Enemy enemies[MAX_ENEMIES]`
- `LanderManager` owns `Lander landers[MAX_LANDERS]`
- `PickupManager` owns `DroppedGun guns[MAX_PICKUPS]`
- `Weapon` owns `BeamTrail beams[]`, `Projectile projectiles[]`, `Explosion explosions[]`

All pools are fixed-size arrays. No heap allocation at runtime.

### Combat Context (`combat.c`)
A `CombatContext` struct holds pointers to all managers, passed to five focused
functions that handle different combat scenarios. This keeps main.c's game loop
clean while combat logic stays centralized and testable.

### Procedural Sound (`sound_gen.c`)
Shared utilities for procedural audio generation. `SoundGenCreateWave` allocates
a 16-bit mono Wave, `SoundGenNoiseTone` generates the most common sound pattern,
and `SoundGenDegradeRadio` applies the radio distortion effect used for death sounds.

### World Singleton
`WorldGetActive()` returns a pointer to the single active World, allowing any
system to query terrain height via `WorldGetHeight(x, z)` without passing the
world pointer through every function.

## Rendering Pipeline

1. **3D Scene** renders to a 640x360 `RenderTexture2D` (low-res for pixel art look)
2. **CRT Shader** (`resources/crt.fs`) post-processes with scanlines, phosphor mask, bloom, chromatic aberration, fish-eye distortion, film grain, and breath fog
3. Scaled to window with **nearest-neighbor filtering** (crispy pixels)
4. **HUD** renders to a separate full-resolution `RenderTexture2D`
5. **Visor Shader** (`resources/hud.fs`) curves the HUD overlay
6. **Menu/Pause/Game Over** screens drawn at full window resolution on top

## Wave System Flow

```
GameUpdate detects wave cleared
  -> waveTimer counts up to WAVE_DELAY (6s)
  -> wave++ , enemiesPerWave = BASE + wave * PER_WAVE
  -> main.c calls LanderSpawnWave()
     -> 1 + (wave/3) landers spawn around player
     -> klaxon plays, ACHTUNG alert shows
     -> landers descend from ~100m altitude
     -> land, deploy enemies at intervals
     -> self-destruct after deployment
  -> wave clears when all landers done + all enemies dead
```

## Adding New Systems

1. Create `src/newsystem.h` and `src/newsystem.c`
2. Add constants to `src/config.h` under a new section
3. Add the .c file to `SRC` in `Makefile`
4. Include the header in `main.c`, init in the asset loading block, update/draw in the game loop
5. If it needs combat integration, add a function to `combat.h/c`
