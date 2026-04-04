#include "combat_ecs.h"
#include "enemy/enemy_components.h"
#include "enemy/enemy_systems.h"
#include "config.h"
#include "world.h"
#include <stdlib.h>
#include <math.h>

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
    if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT) || !ctx->pickups->hasPickup) return;

    Player *player = ctx->player;
    Weapon *weapon = ctx->weapon;
    PickupManager *pickups = ctx->pickups;
    Game *game = ctx->game;
    ecs_world_t *world = ctx->ecsWorld;

    Vector3 shootDir = PlayerGetForward(player);
    Vector3 shootOrigin = player->camera.position;
    Vector3 barrelPos = WeaponGetBarrelWorldPos(weapon, player->camera);

    if (PickupFire(pickups, shootOrigin, shootDir)) {
        float range = (pickups->pickupType == ENEMY_SOVIET) ? PICKUP_SOVIET_RANGE : PICKUP_AMERICAN_RANGE;
        Ray pickRay = {shootOrigin, shootDir};
        float hd = 0;
        ecs_entity_t hit;
        // Liberty Blaster uses wide hitbox, PPSh uses normal
        if (pickups->pickupType == ENEMY_AMERICAN)
            hit = LibertyCheckHitEcs(world, pickRay, range, &hd);
        else
            hit = EcsEnemyCheckHit(world, pickRay, range, &hd);

        // Beam endpoint: shorten to hit point, or full range if miss
        Vector3 beamEnd = (hit != 0) ?
            Vector3Add(shootOrigin, Vector3Scale(shootDir, hd)) :
            Vector3Add(shootOrigin, Vector3Scale(shootDir, range));

        if (hit != 0) {
            if (pickups->pickupType == ENEMY_AMERICAN) {
                // Liberty Blaster: instant kill, always vaporize
                game->killCount++;
                EcsEnemyVaporize(world, hit);
            } else {
                const EcTransform *tr = ecs_get(world, hit, EcTransform);
                const EcFaction *fac = ecs_get(world, hit, EcFaction);
                EcsEnemyDamage(world, hit, pickups->pickupDamage);
                // Check if enemy died (EcAlive removed)
                if (!ecs_has(world, hit, EcAlive)) {
                    game->killCount++;
                    if (tr && fac) PickupDrop(pickups, tr->position, fac->type);
                }
            }
        }
        if (pickups->pickupType == ENEMY_SOVIET) {
            // PPSh: 3 rapid spread tracers per shot for bullet-hose feel
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
        } else {
            // Liberty Blaster: single thick rail beam, ends at target
            if (weapon->beamCount < MAX_BEAM_TRAILS) {
                weapon->beams[weapon->beamCount] = (BeamTrail){barrelPos,
                    beamEnd, (Color)COLOR_BEAM_AMERICAN, 0.35f, 0.35f, 4.0f};
                weapon->beamCount++;
            }
        }
        float recoil = (pickups->pickupType == ENEMY_SOVIET) ? PICKUP_SOVIET_RECOIL : PICKUP_AMERICAN_RECOIL;
        PlayerApplyRecoil(player, shootDir, recoil);
    }
}

void EcsCombatProcessWeaponFire(EcsCombatContext *ctx) {
    // Skip if pickup weapon is active (handled by EcsCombatProcessPickupFire)
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
                const EcFaction *fac = ecs_get(world, hit, EcFaction);
                EcsEnemyDamage(world, hit, weapon->mp40Damage);
                if (!ecs_has(world, hit, EcAlive)) {
                    game->killCount++;
                    if (tr && fac) PickupDrop(pickups, tr->position, fac->type);
                }
            }
        } else if (weapon->current == WEAPON_JACKHAMMER) {
            // Lunge forward
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
                const EcFaction *fac = ecs_get(world, hit, EcFaction);
                game->killCount++;
                if (tr && fac) PickupDrop(pickups, tr->position, fac->type);
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
            // Damage all enemies in blast radius
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
                        const EcFaction *fac = ecs_get(world, e, EcFaction);
                        float dmg = weapon->projectiles[i].damage * (1.0f - d / weapon->projectiles[i].radius);
                        EcsEnemyDamage(world, e, dmg);
                        if (!ecs_has(world, e, EcAlive)) {
                            game->killCount++;
                            if (fac) PickupDrop(pickups, t[j].position, fac->type);
                        }
                    }
                }
            }
            ecs_query_fini(q);

            WeaponSpawnExplosion(weapon, weapon->projectiles[i].position, weapon->projectiles[i].radius);
            weapon->projectiles[i].active = false;
        }
        // Ground hit
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

    // Massive knockback
    player->velocity.x -= shootDir.x * RAKETEN_BEAM_PUSH * dt;
    player->velocity.z -= shootDir.z * RAKETEN_BEAM_PUSH * dt;
    player->velocity.y += RAKETEN_BEAM_LIFT * dt;
    player->onGround = false;

    // Kill everything in the beam path
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
