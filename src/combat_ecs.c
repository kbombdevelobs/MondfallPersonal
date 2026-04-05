#include "combat_ecs.h"
#include "enemy/enemy_components.h"
#include "enemy/enemy_systems.h"
#include "config.h"
#include "world.h"
#include <stdlib.h>
#include <math.h>

// Helper: get pickup weapon type for an entity (faction + rank)
static PickupWeaponType GetEntityPickupType(ecs_world_t *world, ecs_entity_t e) {
    if (!ecs_is_alive(world, e)) return PICKUP_KOSMOS7;
    const EcFaction *fac = ecs_get(world, e, EcFaction);
    const EcRank *rk = ecs_get(world, e, EcRank);
    EnemyType faction = fac ? fac->type : ENEMY_SOVIET;
    EnemyRank rank = rk ? rk->rank : RANK_TROOPER;
    return PickupTypeFromRank(faction, rank);
}

// Liberty Blaster: wide-area ray check with generous bounding boxes
static ecs_entity_t LibertyCheckHitEcs(ecs_world_t *world, Ray ray, float maxDist, float *hitDist) {
    ecs_entity_t closest = 0;
    float cd = maxDist;
    float pad = 2.0f;

    ecs_query_t *q = ecs_query(world, {
        .terms = {
            { .id = ecs_id(EcTransform) },
            { .id = ecs_id(EcAlive) }
        }
    });
    ecs_iter_t it = ecs_query_iter(world, q);
    while (ecs_query_next(&it)) {
        EcTransform *t = ecs_field(&it, EcTransform, 0);
        for (int i = 0; i < it.count; i++) {
            Vector3 pos = t[i].position;
            BoundingBox box = {
                {pos.x - pad, pos.y - 1.5f, pos.z - pad},
                {pos.x + pad, pos.y + 1.8f, pos.z + pad}
            };
            RayCollision col = GetRayCollisionBox(ray, box);
            if (col.hit && col.distance < cd) {
                cd = col.distance;
                closest = it.entities[i];
            }
        }
    }
    ecs_query_fini(q);

    if (hitDist) *hitDist = cd;
    return closest;
}

ecs_entity_t EcsEnemyCheckHit(ecs_world_t *world, Ray ray, float maxDist, float *hitDist) {
    ecs_entity_t closest = 0;
    float cd = maxDist;

    ecs_query_t *q = ecs_query(world, {
        .terms = {
            { .id = ecs_id(EcTransform) },
            { .id = ecs_id(EcAlive) }
        }
    });
    ecs_iter_t it = ecs_query_iter(world, q);
    while (ecs_query_next(&it)) {
        EcTransform *t = ecs_field(&it, EcTransform, 0);
        for (int i = 0; i < it.count; i++) {
            Vector3 pos = t[i].position;
            BoundingBox box = {
                {pos.x - 0.5f, pos.y - 1.2f, pos.z - 0.5f},
                {pos.x + 0.5f, pos.y + 1.3f, pos.z + 0.5f}
            };
            RayCollision col = GetRayCollisionBox(ray, box);
            if (col.hit && col.distance < cd) {
                cd = col.distance;
                closest = it.entities[i];
            }
        }
    }
    ecs_query_fini(q);

    if (hitDist) *hitDist = cd;
    return closest;
}

ecs_entity_t EcsEnemyCheckSphereHit(ecs_world_t *world, Vector3 center, float radius) {
    ecs_query_t *q = ecs_query(world, {
        .terms = {
            { .id = ecs_id(EcTransform) },
            { .id = ecs_id(EcAlive) }
        }
    });
    ecs_iter_t it = ecs_query_iter(world, q);
    while (ecs_query_next(&it)) {
        EcTransform *t = ecs_field(&it, EcTransform, 0);
        for (int i = 0; i < it.count; i++) {
            if (Vector3Distance(center, t[i].position) < radius + 0.8f) {
                ecs_entity_t e = it.entities[i];
                ecs_iter_fini(&it);
                ecs_query_fini(q);
                return e;
            }
        }
    }
    ecs_query_fini(q);
    return 0;
}

void EcsCombatProcessPickupFire(EcsCombatContext *ctx) {
    if (!ctx->pickups->hasPickup) return;

    Player *player = ctx->player;
    Weapon *weapon = ctx->weapon;
    PickupManager *pickups = ctx->pickups;
    Game *game = ctx->game;
    ecs_world_t *world = ctx->ecsWorld;

    Vector3 shootDir = PlayerGetForward(player);
    Vector3 shootOrigin = player->camera.position;
    Vector3 barrelPos = WeaponGetBarrelWorldPos(weapon, player->camera);

    // Zarya TK-4: charge on hold, fire on release
    if (pickups->pickupType == PICKUP_ZARYA_TK4) {
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            // Start or continue charging
            PickupFire(pickups, shootOrigin, shootDir); // sets charging state
            return;
        } else if (pickups->charging) {
            // Released — fire!
            if (PickupFireZaryaRelease(pickups)) {
                Ray pickRay = {shootOrigin, shootDir};
                float hd = 0;
                ecs_entity_t hit = EcsEnemyCheckHit(world, pickRay, PICKUP_ZARYA_RANGE, &hd);
                Vector3 beamEnd = (hit != 0) ?
                    Vector3Add(shootOrigin, Vector3Scale(shootDir, hd)) :
                    Vector3Add(shootOrigin, Vector3Scale(shootDir, PICKUP_ZARYA_RANGE));
                if (hit != 0) {
                    const EcTransform *tr = ecs_get(world, hit, EcTransform);
                    EcsEnemyDamage(world, hit, pickups->pickupDamage);
                    if (!ecs_has(world, hit, EcAlive)) {
                        game->killCount++;
                        if (tr) PickupDrop(pickups, tr->position, GetEntityPickupType(world, hit));
                    }
                }
                if (weapon->beamCount < MAX_BEAM_TRAILS) {
                    float chargeRatio = pickups->pickupRecoil / PICKUP_ZARYA_RECOIL;
                    float thickness = 0.1f + chargeRatio * 0.25f;
                    weapon->beams[weapon->beamCount] = (BeamTrail){barrelPos,
                        beamEnd, (Color){255,80,30,255}, thickness, thickness, 3.0f};
                    weapon->beamCount++;
                }
                PlayerApplyRecoil(player, shootDir, pickups->pickupRecoil);
            }
            return;
        }
        return;
    }

    // ARC-9 Longbow: piercing beam
    if (pickups->pickupType == PICKUP_ARC9_LONGBOW && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        if (PickupFire(pickups, shootOrigin, shootDir)) {
            Ray pickRay = {shootOrigin, shootDir};
            Vector3 rayOrigin = shootOrigin;
            int pierced = 0;
            float totalDist = 0;
            Vector3 lastHitPos = shootOrigin;

            while (pierced < PICKUP_LONGBOW_PIERCE && totalDist < PICKUP_LONGBOW_RANGE) {
                float hd = 0;
                pickRay.position = rayOrigin;
                ecs_entity_t hit = EcsEnemyCheckHit(world, pickRay, PICKUP_LONGBOW_RANGE - totalDist, &hd);
                if (hit == 0) break;

                const EcTransform *tr = ecs_get(world, hit, EcTransform);
                EcsEnemyDamage(world, hit, PICKUP_LONGBOW_DAMAGE);
                if (!ecs_has(world, hit, EcAlive)) {
                    game->killCount++;
                    if (tr) PickupDrop(pickups, tr->position, GetEntityPickupType(world, hit));
                }
                // Continue ray from just past hit point
                totalDist += hd + 1.0f;
                rayOrigin = Vector3Add(rayOrigin, Vector3Scale(shootDir, hd + 1.0f));
                if (tr) lastHitPos = tr->position;
                pierced++;
            }
            // Beam visual: from barrel to last hit or max range
            Vector3 beamEnd = (pierced > 0) ?
                Vector3Add(lastHitPos, Vector3Scale(shootDir, 2.0f)) :
                Vector3Add(shootOrigin, Vector3Scale(shootDir, PICKUP_LONGBOW_RANGE));
            if (weapon->beamCount < MAX_BEAM_TRAILS) {
                weapon->beams[weapon->beamCount] = (BeamTrail){barrelPos,
                    beamEnd, (Color){150,200,255,255}, 0.15f, 0.15f, 5.0f};
                weapon->beamCount++;
            }
            PlayerApplyRecoil(player, shootDir, PICKUP_LONGBOW_RECOIL);
        }
        return;
    }

    // KS-23 Molot: shotgun pellet spread
    if (pickups->pickupType == PICKUP_KS23_MOLOT && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        if (PickupFire(pickups, shootOrigin, shootDir)) {
            for (int p = 0; p < PICKUP_MOLOT_PELLETS; p++) {
                float sx = ((float)rand()/RAND_MAX - 0.5f) * PICKUP_MOLOT_SPREAD * 2.0f;
                float sy = ((float)rand()/RAND_MAX - 0.5f) * PICKUP_MOLOT_SPREAD * 2.0f;
                Vector3 pelletDir = Vector3Normalize((Vector3){shootDir.x + sx, shootDir.y + sy, shootDir.z});
                Ray pelletRay = {shootOrigin, pelletDir};
                float hd = 0;
                ecs_entity_t hit = EcsEnemyCheckHit(world, pelletRay, PICKUP_MOLOT_RANGE, &hd);
                if (hit != 0) {
                    const EcTransform *tr = ecs_get(world, hit, EcTransform);
                    EcsEnemyDamage(world, hit, PICKUP_MOLOT_DAMAGE);
                    if (!ecs_has(world, hit, EcAlive)) {
                        game->killCount++;
                        if (tr) PickupDrop(pickups, tr->position, GetEntityPickupType(world, hit));
                    }
                }
                // Pellet tracer
                if (weapon->beamCount < MAX_BEAM_TRAILS) {
                    Vector3 tEnd = (hit != 0) ?
                        Vector3Add(shootOrigin, Vector3Scale(pelletDir, hd)) :
                        Vector3Add(shootOrigin, Vector3Scale(pelletDir, PICKUP_MOLOT_RANGE));
                    weapon->beams[weapon->beamCount] = (BeamTrail){barrelPos,
                        tEnd, (Color){255,180,50,200}, 0.04f, 0.04f, 1.0f};
                    weapon->beamCount++;
                }
            }
            PlayerApplyRecoil(player, shootDir, PICKUP_MOLOT_RECOIL);
        }
        return;
    }

    // Standard pickup fire (KOSMOS-7, LIBERTY, M8A1 Starhawk)
    if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT)) return;

    if (PickupFire(pickups, shootOrigin, shootDir)) {
        float range;
        switch (pickups->pickupType) {
            case PICKUP_KOSMOS7:       range = PICKUP_SOVIET_RANGE; break;
            case PICKUP_LIBERTY:       range = PICKUP_AMERICAN_RANGE; break;
            case PICKUP_M8A1_STARHAWK: range = PICKUP_STARHAWK_RANGE; break;
            default:                   range = 60.0f; break;
        }
        Ray pickRay = {shootOrigin, shootDir};
        float hd = 0;
        ecs_entity_t hit;
        if (pickups->pickupType == PICKUP_LIBERTY)
            hit = LibertyCheckHitEcs(world, pickRay, range, &hd);
        else
            hit = EcsEnemyCheckHit(world, pickRay, range, &hd);

        Vector3 beamEnd = (hit != 0) ?
            Vector3Add(shootOrigin, Vector3Scale(shootDir, hd)) :
            Vector3Add(shootOrigin, Vector3Scale(shootDir, range));

        if (hit != 0) {
            if (pickups->pickupType == PICKUP_LIBERTY) {
                game->killCount++;
                EcsEnemyVaporize(world, hit);
            } else {
                const EcTransform *tr = ecs_get(world, hit, EcTransform);
                EcsEnemyDamage(world, hit, pickups->pickupDamage);
                if (!ecs_has(world, hit, EcAlive)) {
                    game->killCount++;
                    if (tr) PickupDrop(pickups, tr->position, GetEntityPickupType(world, hit));
                }
            }
        }

        if (pickups->pickupType == PICKUP_KOSMOS7) {
            for (int t = 0; t < 3 && weapon->beamCount < MAX_BEAM_TRAILS; t++) {
                float sx = ((float)rand()/RAND_MAX - 0.5f) * 0.03f;
                float sy = ((float)rand()/RAND_MAX - 0.5f) * 0.03f;
                Vector3 spreadDir = Vector3Normalize((Vector3){shootDir.x + sx, shootDir.y + sy, shootDir.z});
                Vector3 tEnd = (hit != 0) ?
                    Vector3Add(shootOrigin, Vector3Scale(spreadDir, hd + 2.0f)) :
                    Vector3Add(shootOrigin, Vector3Scale(spreadDir, range));
                weapon->beams[weapon->beamCount] = (BeamTrail){barrelPos,
                    tEnd, (Color)COLOR_BEAM_SOVIET, 0.08f, 0.08f, 1.5f};
                weapon->beamCount++;
            }
        } else if (pickups->pickupType == PICKUP_LIBERTY) {
            if (weapon->beamCount < MAX_BEAM_TRAILS) {
                weapon->beams[weapon->beamCount] = (BeamTrail){barrelPos,
                    beamEnd, (Color)COLOR_BEAM_AMERICAN, 0.35f, 0.35f, 4.0f};
                weapon->beamCount++;
            }
        } else if (pickups->pickupType == PICKUP_M8A1_STARHAWK) {
            // Starhawk: single precise tracer per burst round
            if (weapon->beamCount < MAX_BEAM_TRAILS) {
                float sx = ((float)rand()/RAND_MAX - 0.5f) * PICKUP_STARHAWK_SPREAD;
                float sy = ((float)rand()/RAND_MAX - 0.5f) * PICKUP_STARHAWK_SPREAD;
                Vector3 spreadDir = Vector3Normalize((Vector3){shootDir.x + sx, shootDir.y + sy, shootDir.z});
                Vector3 tEnd = (hit != 0) ?
                    Vector3Add(shootOrigin, Vector3Scale(spreadDir, hd + 1.0f)) :
                    Vector3Add(shootOrigin, Vector3Scale(spreadDir, range));
                weapon->beams[weapon->beamCount] = (BeamTrail){barrelPos,
                    tEnd, (Color)COLOR_BEAM_AMERICAN, 0.06f, 0.06f, 2.0f};
                weapon->beamCount++;
            }
        }

        float recoil;
        switch (pickups->pickupType) {
            case PICKUP_KOSMOS7:       recoil = PICKUP_SOVIET_RECOIL; break;
            case PICKUP_LIBERTY:       recoil = PICKUP_AMERICAN_RECOIL; break;
            case PICKUP_M8A1_STARHAWK: recoil = PICKUP_STARHAWK_RECOIL; break;
            default:                   recoil = 1.0f; break;
        }
        PlayerApplyRecoil(player, shootDir, recoil);
    }
}

void EcsCombatProcessWeaponFire(EcsCombatContext *ctx) {
    if (ctx->pickups->hasPickup && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) return;
    if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT) &&
        !(ctx->weapon->current == WEAPON_JACKHAMMER && IsKeyDown(KEY_V))) return;

    Player *player = ctx->player;
    Weapon *weapon = ctx->weapon;
    PickupManager *pickups = ctx->pickups;
    Game *game = ctx->game;
    ecs_world_t *world = ctx->ecsWorld;

    Vector3 shootDir = PlayerGetForward(player);
    Vector3 barrelPos = WeaponGetBarrelWorldPos(weapon, player->camera);
    Vector3 shootOrigin = player->camera.position;

    if (WeaponFire(weapon, barrelPos, shootDir)) {
        float recoilForce = (weapon->current == WEAPON_RAKETENFAUST) ? RAKETEN_RECOIL :
                           (weapon->current == WEAPON_MOND_MP40) ? MP40_RECOIL : 0;
        if (recoilForce > 0) PlayerApplyRecoil(player, shootDir, recoilForce);

        if (weapon->current == WEAPON_MOND_MP40) {
            Ray shootRay = {shootOrigin, shootDir};
            float hitDist = 0;
            ecs_entity_t hit = EcsEnemyCheckHit(world, shootRay, MP40_RANGE, &hitDist);
            if (hit != 0) {
                const EcTransform *tr = ecs_get(world, hit, EcTransform);
                EcsEnemyDamage(world, hit, weapon->mp40Damage);
                if (!ecs_has(world, hit, EcAlive)) {
                    game->killCount++;
                    if (tr) PickupDrop(pickups, tr->position, GetEntityPickupType(world, hit));
                }
            }
        } else if (weapon->current == WEAPON_JACKHAMMER) {
            Vector3 lungeDir = {shootDir.x, 0, shootDir.z};
            lungeDir = Vector3Normalize(lungeDir);
            player->velocity.x = lungeDir.x * JACKHAMMER_LUNGE_SPEED;
            player->velocity.z = lungeDir.z * JACKHAMMER_LUNGE_SPEED;
            player->velocity.y = 2.5f;
            player->lungeTimer = JACKHAMMER_LUNGE_DURATION;

            Vector3 meleePos = Vector3Add(shootOrigin, Vector3Scale(shootDir, weapon->jackhammerRange * 0.5f));
            ecs_entity_t hit = EcsEnemyCheckSphereHit(world, meleePos, weapon->jackhammerRange);
            if (hit != 0) {
                const EcTransform *tr = ecs_get(world, hit, EcTransform);
                game->killCount++;
                if (tr) PickupDrop(pickups, tr->position, GetEntityPickupType(world, hit));
                EcsEnemyEviscerate(world, hit, shootDir);
            }
        }
    }
}

void EcsCombatProcessProjectiles(EcsCombatContext *ctx) {
    Weapon *weapon = ctx->weapon;
    PickupManager *pickups = ctx->pickups;
    Game *game = ctx->game;
    ecs_world_t *world = ctx->ecsWorld;

    for (int i = 0; i < MAX_PROJECTILES; i++) {
        if (!weapon->projectiles[i].active) continue;

        ecs_entity_t hit = EcsEnemyCheckSphereHit(world, weapon->projectiles[i].position, weapon->projectiles[i].radius);
        if (hit != 0) {
            ecs_query_t *q = ecs_query(world, {
                .terms = {
                    { .id = ecs_id(EcTransform) },
                    { .id = ecs_id(EcAlive) }
                }
            });
            ecs_iter_t it = ecs_query_iter(world, q);
            while (ecs_query_next(&it)) {
                EcTransform *t = ecs_field(&it, EcTransform, 0);
                for (int j = 0; j < it.count; j++) {
                    float d = Vector3Distance(weapon->projectiles[i].position, t[j].position);
                    if (d < weapon->projectiles[i].radius) {
                        ecs_entity_t e = it.entities[j];
                        float dmg = weapon->projectiles[i].damage * (1.0f - d / weapon->projectiles[i].radius);
                        EcsEnemyDamage(world, e, dmg);
                        if (!ecs_has(world, e, EcAlive)) {
                            game->killCount++;
                            PickupDrop(pickups, t[j].position, GetEntityPickupType(world, e));
                        }
                    }
                }
            }
            ecs_query_fini(q);

            WeaponSpawnExplosion(weapon, weapon->projectiles[i].position, weapon->projectiles[i].radius);
            weapon->projectiles[i].active = false;
        }
        float groundH = WorldGetHeight(weapon->projectiles[i].position.x, weapon->projectiles[i].position.z);
        if (weapon->projectiles[i].position.y <= groundH) {
            WeaponSpawnExplosion(weapon, weapon->projectiles[i].position, weapon->projectiles[i].radius);
            weapon->projectiles[i].active = false;
        }
    }
}

void EcsCombatProcessBeam(EcsCombatContext *ctx, float dt) {
    if (!ctx->weapon->raketenFiring) return;

    Player *player = ctx->player;
    Weapon *weapon = ctx->weapon;
    Game *game = ctx->game;
    ecs_world_t *world = ctx->ecsWorld;

    Vector3 shootDir = PlayerGetForward(player);
    Vector3 barrel = WeaponGetBarrelWorldPos(weapon, player->camera);
    weapon->raketenBeamStart = barrel;
    weapon->raketenBeamEnd = Vector3Add(player->camera.position, Vector3Scale(shootDir, RAKETEN_BEAM_RANGE));

    player->velocity.x -= shootDir.x * RAKETEN_BEAM_PUSH * dt;
    player->velocity.z -= shootDir.z * RAKETEN_BEAM_PUSH * dt;
    player->velocity.y += RAKETEN_BEAM_LIFT * dt;
    player->onGround = false;

    Ray beamRay = {player->camera.position, shootDir};
    ecs_query_t *q = ecs_query(world, {
        .terms = {
            { .id = ecs_id(EcTransform) },
            { .id = ecs_id(EcAlive) }
        }
    });
    ecs_iter_t it = ecs_query_iter(world, q);
    while (ecs_query_next(&it)) {
        EcTransform *t = ecs_field(&it, EcTransform, 0);
        for (int i = 0; i < it.count; i++) {
            Vector3 pos = t[i].position;
            BoundingBox box = {
                {pos.x - 1.0f, pos.y - 2.0f, pos.z - 1.0f},
                {pos.x + 1.0f, pos.y + 2.0f, pos.z + 1.0f}
            };
            RayCollision col = GetRayCollisionBox(beamRay, box);
            if (col.hit && col.distance < RAKETEN_BEAM_RANGE) {
                game->killCount++;
                EcsEnemyVaporize(world, it.entities[i]);
            }
        }
    }
    ecs_query_fini(q);
}
