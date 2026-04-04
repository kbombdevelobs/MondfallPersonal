---
title: Wave System
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

# Wave System

The wave system controls enemy spawning through a cycle of wave detection, lander deployment, and enemy release. Waves escalate in size as the game progresses.

## Wave Flow

1. **Wave cleared detection** (`main.c`): When `game.waveActive` is true and all landers are done (`LanderWaveActive()` returns false) and all ECS enemies are dead (`EcsEnemyCountAlive()` returns 0), the wave is marked as cleared.

2. **Inter-wave timer** (`GameUpdate()` in `game.c`): Once a wave is cleared (`waveActive == false`, `enemiesRemaining <= 0`), a timer counts up. After `WAVE_DELAY` seconds, the next wave triggers.

3. **Wave scaling formula**: Each new wave sets `enemiesPerWave = WAVE_ENEMIES_BASE + wave * WAVE_ENEMIES_PER_WAVE`. With defaults of `WAVE_ENEMIES_BASE = 3` and `WAVE_ENEMIES_PER_WAVE = 2`, wave 1 has 5 enemies, wave 2 has 7, etc.

4. **Lander spawning** (`main.c`): When `game.waveActive` is true and `game.enemiesSpawned == 0`, `LanderSpawnWave()` is called, which distributes enemies across 1-4 landers.

5. **Klaxon alert**: `LanderSpawnWave()` immediately plays the klaxon siren sound. The HUD displays an ACHTUNG warning during the klaxon period.

## Lander Lifecycle

Each lander goes through a state machine defined by `LanderState`:

| State | Duration | Behavior |
|-------|----------|----------|
| `LANDER_WAITING` | 3.5 seconds | Klaxon playing, lander not yet visible |
| `LANDER_DESCENDING` | Variable | Falls from 80-120 units above ground, decelerating near surface. Retro rockets fire, screen shakes at 0.3 intensity |
| `LANDER_LANDED` | Variable | Deploys enemies one at a time at 0.4-second intervals from the hatch. Enemies spawn on the side facing away from the player |
| `LANDER_EXPLODING` | 1.5 seconds | Self-destructs with fireball and debris. Screen shake at 3.0 intensity |
| `LANDER_DONE` | -- | Inactive, removed from processing |

## Lander Count Scaling

The number of landers per wave increases with wave number:
- Waves 1-2: 1 lander
- Waves 3-4: 1-2 landers (formula: `1 + wave/3`)
- Wave 5+: 2+ landers (formula: `2 + wave/2`, capped at `MAX_LANDERS` = 4)

Enemies are distributed evenly across landers, with remainder going to the last lander.

## Lander Placement

Landers target positions 30-55 units from the player at random angles. Each lander starts 80-120 units above its target. The faction (Soviet or American) is randomly chosen per lander.

## Enemy Deployment

Enemies spawn at the lander's hatch position, offset 3-5 units in the "away from player" direction (calculated at landing time). A small random offset prevents stacking. Enemies are spawned via `EcsEnemySpawnAt()` into the ECS world.

## Screen Shake

Landers contribute screen shake that is applied to the player camera in `main.c`. `LanderGetScreenShake()` returns the maximum shake value across all active landers. Shake decays multiplicatively (0.95x per frame when landed, 0.9x when exploding).

## Sounds

All lander sounds are procedurally generated in `LanderManagerInit()`:
- **Klaxon**: 3-second WW2-style air raid siren (sawtooth wave, 100-300Hz sweep, 3 wail cycles)
- **Impact**: Deep thud (40Hz sine + noise, exponential decay)
- **Explosion**: Low rumble + noise burst (55Hz sine + noise)

## Key Files

| File | Role |
|------|------|
| `src/lander.c` | Lander state machine, spawning, physics, drawing, sounds |
| `src/lander.h` | `Lander`, `LanderManager`, `LanderState` types and API |
| `src/game.c` | `GameUpdate()` wave timer and scaling formula |
| `src/game.h` | Wave-related fields in `Game` struct |
| `src/config.h` | `WAVE_DELAY`, `WAVE_ENEMIES_BASE`, `WAVE_ENEMIES_PER_WAVE`, `MAX_LANDERS` |
