# Enemy System — src/enemy/

Astronaut enemies (Soviet & American), AI behaviors, death animations, and rendering.

## Files

| File | Lines | Purpose |
|------|-------|---------|
| `enemy.h` | ~80 | Types: `Enemy`, `EnemyManager`, `EnemyState`, `EnemyType`, `AIBehavior`. Includes evisceration limb data. |
| `enemy.c` | ~600 | Init, unload, spawn, AI update loop (advance/strafe/dodge/retreat/cover-seek), hit detection, damage, vaporize, eviscerate, death sounds. |
| `enemy_draw.h` | ~10 | Exports `DrawAstronautModel()` |
| `enemy_draw.c` | ~450 | All rendering: alive astronaut model (torso/helmet/arms/legs/gun/backpack), ragdoll death, crumple death, vaporize death (jerk/freeze/swell/pop/disintegrate), eviscerate death (limbs fly apart with blood). |

## Enemy States
- `ENEMY_ALIVE` — active AI, shooting, moving
- `ENEMY_DYING` — ragdoll blowout (60%) or crumple + blood pool (40%)
- `ENEMY_VAPORIZING` — beam weapon death: jerk -> freeze -> optional swell/pop -> disintegrate
- `ENEMY_EVISCERATING` — jackhammer death: limbs separate, blood spurts, bones scatter, weapon drops
- `ENEMY_DEAD` — removed from game

## Factions
- **Soviet**: red suits, gold helmets, aggressive rush AI, PPSh-style guns
- **American**: navy blue suits, silver helmets, tactical cover-seeking AI, ray guns

## Key Functions
- `EnemyManagerInit/Unload` — setup models, sounds, death audio
- `EnemyManagerUpdate` — per-frame AI + physics for all enemies
- `EnemyManagerDraw` — renders all active enemies
- `EnemyDamage` — apply damage, trigger death if health <= 0
- `EnemyVaporize` — instant beam death (no weapon drop)
- `EnemyEviscerate` — jackhammer death (drops weapon, limbs fly)
- `EnemyCheckHit/EnemyCheckSphereHit` — hit detection for ray/sphere
