# Weapon System — src/weapon/

Procedural sound generation and all weapon rendering (viewmodels, beams, explosions).

## Files

| File | Lines | Purpose |
|------|-------|---------|
| `weapon_sound.h` | ~12 | Exports 6 procedural sound generators |
| `weapon_sound.c` | ~100 | Procedural waveform synthesis: gunshot, beam blast, jackhammer crunch, reload click, explosion boom, empty click |
| `weapon_draw.h` | ~12 | Exports `WeaponDrawFirst`, `WeaponDrawWorld`, `WeaponGetBarrelWorldPos`, `WeaponSpawnExplosion` |
| `weapon_draw.c` | ~470 | All weapon rendering: 3 first-person viewmodels (MP40, Raketenfaust, Jackhammer), beam trails, projectile glows, explosion fireballs, muzzle flashes. Uses cached meshes for performance. |

## Parent file: src/weapon.c (~250 lines)
Contains weapon logic: init, unload, reload, update (timers/input), fire dispatch, switch, ammo getters, SFX volume.

## Parent file: src/weapon.h
Defines `Weapon` struct, `WeaponType` enum, `BeamTrail`, `Explosion`, `Projectile` types, and full public API.

## Weapons
- **Mond-MP40**: Hitscan SMG, 32-round mag, cyan energy beams, chrome space-age viewmodel
- **Raketenfaust**: Death beam, 2s sustained fire, massive knockback, violent shudder, orange energy
- **Jackhammer**: Pneumatic war-pick, one-hit eviscerate kill, forward lunge, negative recoil (thrust forward)

## Key Functions
- `WeaponInit/Unload` — setup sounds, cached meshes
- `WeaponUpdate` — timer decay, input handling, weapon switching
- `WeaponFire` — fire dispatch per weapon type, triggers lunge for jackhammer
- `WeaponDrawFirst` — first-person viewmodel rendering (procedural cubes/spheres)
- `WeaponDrawWorld` — beam trails, projectiles, explosions in world space
- `WeaponSpawnExplosion` — create explosion at position
- `GenSound*` — procedural sound generation (no audio files needed)
