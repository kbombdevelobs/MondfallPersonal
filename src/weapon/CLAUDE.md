---
title: Weapon System
status: Active
owner_area: Weapon
created: 2026-04-04
last_updated: 2026-04-04
last_reviewed: 2026-04-04
parent_doc: CLAUDE.md
related_docs:
  - docs/combat-system.md
  - docs/procedural-audio.md
---

# Weapon System -- src/weapon/

Procedural sound generation and all weapon rendering (viewmodels, beam trails, explosions, muzzle flashes). The core weapon logic (init, fire, reload, switch, ammo) lives in the parent files `src/weapon.c` and `src/weapon.h`.

## Files

| File | Lines | Purpose |
|------|-------|---------|
| `weapon_sound.h` | ~12 | Exports 6 procedural sound generators |
| `weapon_sound.c` | ~100 | Procedural waveform synthesis for all weapon audio: gunshot, beam blast, jackhammer crunch, reload click, explosion boom, empty click. No audio files needed. |
| `weapon_draw.h` | ~12 | Exports WeaponDrawFirst, WeaponDrawWorld, WeaponGetBarrelWorldPos, WeaponSpawnExplosion |
| `weapon_draw.c` | ~470 | All weapon rendering: 3 first-person viewmodels (MP40, Raketenfaust, Jackhammer), beam trails with fade, projectile glows, explosion fireballs, muzzle flashes. Uses cached meshes (meshSphere/meshCube) for performance. |

## Parent Files

| File | Purpose |
|------|---------|
| `src/weapon.c` (~250 lines) | Core weapon logic: WeaponInit (stats, sounds, mesh caching), WeaponUnload, WeaponUpdate (timers, input, weapon switching), WeaponFire (dispatch per type, jackhammer lunge), WeaponReload, ammo getters, SFX volume control |
| `src/weapon.h` | Defines Weapon struct, WeaponType enum, BeamTrail, Explosion, Projectile types, and full public API |

## Player Weapons

| Weapon | Type | Key Stats |
|--------|------|-----------|
| **Mond-MP40** | Hitscan SMG | 32-round mag, fast fire rate, cyan energy beams, chrome space-age viewmodel |
| **Raketenfaust** | Death beam | 2s sustained fire, kills everything in path, massive knockback, violent shudder, orange energy |
| **Jackhammer** | Pneumatic war-pick | One-hit eviscerate kill, forward lunge on attack, negative recoil (thrust forward) |

## Pickup Weapons (defined in src/pickup.c/h)

| Weapon | Source | Key Stats |
|--------|--------|-----------|
| **KOSMOS-7 SMG (PPSh)** | Dropped by Soviet enemies | Fastest fire rate (0.05s), 35 damage, 3 spread tracers per shot |
| **LIBERTY BLASTER** | Dropped by American enemies | One-shot vaporize kill, wide hitbox, thick lingering rail beam |

Pickup weapon stats and rendering are handled through the same weapon system; pickup buffs only apply when held by the player.

## Key Functions

| Function | File | Description |
|----------|------|-------------|
| `WeaponInit` | weapon.c | Setup weapon stats, generate procedural sounds, cache GPU meshes (unit sphere + unit cube) |
| `WeaponUnload` | weapon.c | Free sounds and cached meshes |
| `WeaponUpdate` | weapon.c | Per-frame timer decay, input handling, weapon switching logic |
| `WeaponFire` | weapon.c | Fire dispatch per weapon type; triggers jackhammer lunge |
| `WeaponReload` | weapon.c | Start reload animation and timer |
| `WeaponDrawFirst` | weapon_draw.c | First-person viewmodel rendering with procedural cubes/spheres via rlPushMatrix/rlPopMatrix |
| `WeaponDrawWorld` | weapon_draw.c | Beam trails, projectile glows, explosions rendered in world space |
| `WeaponSpawnExplosion` | weapon_draw.c | Create explosion effect at world position |
| `WeaponGetBarrelWorldPos` | weapon_draw.c | Compute barrel tip position in world space for beam/projectile origin |
| `GenSoundGunshot` | weapon_sound.c | Procedural SMG fire sound |
| `GenSoundBeamBlast` | weapon_sound.c | Procedural beam weapon sound |
| `GenSoundJackhammerCrunch` | weapon_sound.c | Procedural melee impact sound |
| `GenSoundReloadClick` | weapon_sound.c | Procedural reload sound |
| `GenSoundExplosion` | weapon_sound.c | Procedural explosion sound |
| `GenSoundEmptyClick` | weapon_sound.c | Procedural empty magazine click |

## Dependencies

- **sound_gen** (`src/sound_gen.c/h`) -- `SoundGenCreateWave()` utility for waveform buffer allocation used by all GenSound* functions
- **combat_ecs** (`src/combat_ecs.c/h`) -- WeaponFire triggers hit resolution through the combat system, which dispatches damage/vaporize/eviscerate to enemy entities
- **config.h** -- weapon stats constants, audio sample rate (AUDIO_SAMPLE_RATE), render dimensions
- **raylib** -- DrawMesh with cached meshes for particle effects, rlPushMatrix/rlPopMatrix for viewmodel transforms

## Connections

- **combat-system.md** -- Weapon fire events flow through combat resolution to apply damage, vaporize, or eviscerate enemies
- **procedural-audio.md** -- All weapon sounds are procedurally synthesized; weapon_sound.c is the primary example of the procedural audio approach

## Maintenance Notes

- All weapon stats are configured in `WeaponInit()` in `src/weapon.c`. Viewmodel geometry is in `WeaponDrawFirst()` in `weapon_draw.c`.
- The cached mesh pattern (meshSphere/meshCube generated once in WeaponInit, reused via DrawMesh with transform matrices) eliminates ~200 per-frame mesh regenerations. Do not replace with per-frame GenMesh calls.
- When adding a new weapon: add its WeaponType enum value in weapon.h, configure stats in WeaponInit, add fire logic in WeaponFire, add viewmodel in WeaponDrawFirst, and generate a procedural sound in weapon_sound.c.
- Pickup weapons (KOSMOS-7, LIBERTY BLASTER) are defined in `src/pickup.c/h` but render through the same weapon draw pipeline.
