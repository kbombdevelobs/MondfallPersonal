---
title: Enemy System
status: Active
owner_area: Enemy
created: 2026-04-04
last_updated: 2026-04-10
last_reviewed: 2026-04-10
parent_doc: CLAUDE.md
related_docs:
  - docs/combat-system.md
  - docs/wave-system.md
  - docs/ecs-integration.md
---

# Enemy System -- src/enemy/

Astronaut enemies (Soviet and American factions), AI behaviors, spawning, hit detection, death animations, rendering with skeletal .glb models + procedural fallback, rank hierarchy (Trooper/NCO/Officer), morale system with cover-seeking, and 15-axis spring-damper limb physics. Built on Flecs ECS.

## Files

| File | Purpose |
|------|---------|
| `enemy_components.h/c` | All ECS components: Transform, Velocity, Faction, Alive, CombatStats, AIState, Animation (with staggerTimer, knockdownTimer, isCowering, lastHitDir), LimbState (15 spring axes), Rank, Morale (with fleeCover fields), Steering, Squad, death-state components (Ragdoll, Vaporize, Eviscerate, Decapitate), singletons (EcEnemyResources with ground pound screams, EcGameContext with bolts + rankKillType) |
| `enemy_systems.h/c` | Central API: EcsEnemyDamage (with hit push + stagger), EcsEnemyVaporize, EcsEnemyEviscerate, EcsEnemyDecapitate (headshot kill type 3), EcsEnemyCountAlive |
| `enemy_ai.c` | AI systems: targeting, behavior (advance/strafe/shoot/dodge/retreat/flee with cover-seeking), spatial hash collision avoidance, squad cohesion, structure flanking (cross-product side detection), American tactical AI (cover-on-hit, health retreat, fire-team advance) |
| `enemy_physics.c` | Physics: momentum steering with stagger/knockdown guards, terrain slope, structure collision slide. Attack: rank-based accuracy (30%/40%/55%), no-fire-while-stunned, visible bolt spawning |
| `enemy_death_systems.c` | Death systems: SysRagdollDeath, SysVaporizeDeath, SysEviscerateDeath, SysDecapitateDeath, SysRadioTimer |
| `enemy_morale.c` | Morale: leader proximity, recovery/decay (0.03/s natural), flee trigger (45% threshold, 17%/frame chance), cover-seeking with terrain scanning, cowering recovery (0.12/s), morale hit cascade on leader death (0.75 officer, 0.45 NCO) |
| `enemy_spawn.c` | Entity creation with rank multipliers, formation offsets, EcLimbState initialization |
| `enemy_bodydef.h/c` | Data-driven body geometry (BodyDef, PartGroup, BodyPart), faction color palettes (BodyColors), rank overlays (RankOverlayDef with coat tails, Sam Browne, epaulettes), animation profiles (AnimProfile with spring tuning), gun definitions (GunDef), DrawPartGroup/DrawPartGroupLOD |
| `enemy_limb_physics.h/c` | 15-axis spring-damper system: arm swing/spread, leg swing/spread/knee, hip sway, torso lean/twist, head pitch/yaw, breathing. Airborne legs trail opposite to movement with deep knee bend. Faction-specific stiffness/damping. |
| `enemy_model_loader.h/c` | Skeletal .glb model loader: 21-bone armature (BoneSlot enum), AstroModel with bone index cache, AstroModelSet with characters[2][3], guns[2][2], dismember[2][6], headshot[2][3] at LOD0/LOD1 |
| `enemy_model_bones.h/c` | Bone animation driver: maps LimbState springs to skeleton rotations via parent-chain propagation (local-space decomposition, hierarchy walk, bone matrix computation). AMP factors: 2.0x general, 2.5x arms, 2.2x legs |
| `enemy_draw_ecs.h/c` | ECS→rendering bridge: queries components, fills legacy Enemy structs (including limbState, knockdown, decap fields), LOD dispatch (LOD0/LOD1/LOD2 by distance), frustum culling, skeletal model loading, bolt update/draw |
| `enemy_draw.h/c` | Rendering: tries skeletal model path first (eviscerate → dismember models, decapitate → headshot models, alive/dying → character models with bone animation). Falls back to procedural BodyDef-based drawing with spring-driven limbs, guns from GunDef, rank overlays, muzzle flash |
| `enemy_draw_death.h/c` | Death rendering: eviscerate (procedural + skeletal dismemberment), vaporize, ragdoll, decapitate (procedural + skeletal headshot with blood spray/drips/air hissing) |
| `enemy.h` | Legacy Enemy struct (rendering bridge) with limbState, hasLimbState, knockdownAngle, isCowering, decap fields. EnemyManager with AstroModelSet pointer |

## Enemy States

| State | ECS Tag + Component | Rendering |
|-------|-------------------|-----------|
| Alive | `EcAlive` | Skeletal model with bone animation, or procedural with spring-driven limbs + gun |
| Dying | `EcDying` + `EcRagdollDeath` | Ragdoll with slower spins (60-180° pitch, ±150° yaw, ±80° lateral), arm flail/settle |
| Vaporizing | `EcVaporizing` + `EcVaporizeDeath` | Jerk, freeze, optional swell/pop (15%), disintegrate |
| Eviscerating | `EcEviscerating` + `EcEviscerateDeath` | Skeletal dismemberment .glb models (6 parts) or procedural limbs with blood |
| Decapitating | `EcDecapitating` + `EcDecapitateDeath` | Skeletal headshot model (damaged helmet) or procedural, with blood spray/drips/air jet |

## AI Behaviors

| Behavior | Description |
|----------|-------------|
| `AI_ADVANCE` | Soviet default: rush with 0.6 strafe bias, structure flanking with cross-product side detection |
| `AI_STRAFE` | American default: lateral movement, fire-team staggered advance |
| `AI_SHOOT` | Engage at preferred distance with rank-based accuracy |
| `AI_DODGE` | Evasive maneuver on cooldown |
| `AI_RETREAT` | American health-based retreat to cover (below 50% HP) |
| `AI_FLEE` | Morale break: terrain/structure cover seeking, cowering with forward lean, panicked sprint (1.4x speed) |

## Model Assets

| Directory | Contents |
|-----------|----------|
| `resources/models/characters/` | 12 .glb files: {soviet,american}_{trooper,nco,officer}{,_lod1}.glb |
| `resources/models/dismemberment/` | 18 .glb files: {soviet,american}_{head,torso,arm_r,arm_l,leg_r,leg_l}.glb + headshot variants |
| `resources/models/guns/` | 6 .glb files: gun_{soviet,american}_{trooper,officer}.glb |

## Key Functions

| Function | File | Description |
|----------|------|-------------|
| `EcsEnemyDamage` | enemy_systems.c | Apply damage with hit push, stagger (0.3s), vertical impulse. Triggers ragdoll/crumple on kill |
| `EcsEnemyDecapitate` | enemy_systems.c | Headshot kill — rankKillType 3, decap death physics, morale hit |
| `EcsEnemyLimbPhysicsRegister` | enemy_limb_physics.c | Register 15-axis spring-damper ECS system |
| `AstroModelsLoad` | enemy_model_loader.c | Load all .glb character/gun/dismember/headshot models |
| `AstroModelApplySpringState` | enemy_model_bones.c | Drive skeleton bones from spring-damper state |
| `DrawPartGroup` | enemy_bodydef.c | Render procedural body parts from BodyDef data |
| `EcsEnemyUpdateBolts` | enemy_draw_ecs.c | Advance enemy bolt projectiles each frame |
| `EcsEnemyDrawBolts` | enemy_draw_ecs.c | Render bolt spheres + trail lines |
