#include "combat.h"
#include "config.h"
#include "world.h"
#include <stdlib.h>
#include <math.h>

// Liberty Blaster: wide-area ray check with generous bounding boxes
static int LibertyCheckHit(EnemyManager *em, Ray ray, float maxDist, float *hitDist) {
    int closest = -1; float cd = maxDist;
    float pad = 2.0f; // much wider hitbox than normal (normal is 0.5)
    for (int i = 0; i < MAX_ENEMIES; i++) {
        Enemy *e = &em->enemies[i];
        if (!e->active || e->state != ENEMY_ALIVE) continue;
        BoundingBox box = {{e->position.x - pad, e->position.y - 1.5f, e->position.z - pad},
                           {e->position.x + pad, e->position.y + 1.8f, e->position.z + pad}};
        RayCollision col = GetRayCollisionBox(ray, box);
        if (col.hit && col.distance < cd) { cd = col.distance; closest = i; }
    }
    if (hitDist) *hitDist = cd;
    return closest;
}

void CombatProcessPickupFire(CombatContext *ctx) {
    if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT) || !ctx->pickups->hasPickup) return;

    Player *player = ctx->player;
    Weapon *weapon = ctx->weapon;
    EnemyManager *enemies = ctx->enemies;
    PickupManager *pickups = ctx->pickups;
    Game *game = ctx->game;

    Vector3 shootDir = PlayerGetForward(player);
    Vector3 shootOrigin = player->camera.position;
    Vector3 barrelPos = WeaponGetBarrelWorldPos(weapon, player->camera);

    if (PickupFire(pickups, shootOrigin, shootDir)) {
        float range = (pickups->pickupType == ENEMY_SOVIET) ? PICKUP_SOVIET_RANGE : PICKUP_AMERICAN_RANGE;
        Ray pickRay = {shootOrigin, shootDir};
        float hd = 0;
        int hit;
        // Liberty Blaster uses wide hitbox, PPSh uses normal
        if (pickups->pickupType == ENEMY_AMERICAN)
            hit = LibertyCheckHit(enemies, pickRay, range, &hd);
        else
            hit = EnemyCheckHit(enemies, pickRay, range, &hd);

        // Beam endpoint: shorten to hit point, or full range if miss
        Vector3 beamEnd = (hit >= 0) ?
            Vector3Add(shootOrigin, Vector3Scale(shootDir, hd)) :
            Vector3Add(shootOrigin, Vector3Scale(shootDir, range));

        if (hit >= 0) {
            if (pickups->pickupType == ENEMY_AMERICAN) {
                // Liberty Blaster: instant kill, always vaporize
                game->killCount++;
                EnemyVaporize(enemies, hit);
            } else {
                EnemyDamage(enemies, hit, pickups->pickupDamage);
                if (enemies->enemies[hit].state == ENEMY_DYING) {
                    game->killCount++;
                    PickupDrop(pickups, enemies->enemies[hit].position, enemies->enemies[hit].type);
                }
            }
        }
        if (pickups->pickupType == ENEMY_SOVIET) {
            // PPSh: 3 rapid spread tracers per shot for bullet-hose feel
            for (int t = 0; t < 3 && weapon->beamCount < MAX_BEAM_TRAILS; t++) {
                float sx = ((float)rand()/RAND_MAX - 0.5f) * 0.03f;
                float sy = ((float)rand()/RAND_MAX - 0.5f) * 0.03f;
                Vector3 spreadDir = Vector3Normalize((Vector3){shootDir.x + sx, shootDir.y + sy, shootDir.z});
                Vector3 tEnd = (hit >= 0) ?
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

void CombatProcessWeaponFire(CombatContext *ctx) {
    // Skip if pickup weapon is active (handled by CombatProcessPickupFire)
    if (ctx->pickups->hasPickup && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) return;
    if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT) &&
        !(ctx->weapon->current == WEAPON_JACKHAMMER && IsKeyDown(KEY_V))) return;

    Player *player = ctx->player;
    Weapon *weapon = ctx->weapon;
    EnemyManager *enemies = ctx->enemies;
    PickupManager *pickups = ctx->pickups;
    Game *game = ctx->game;

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
            int hit = EnemyCheckHit(enemies, shootRay, MP40_RANGE, &hitDist);
            if (hit >= 0) {
                EnemyDamage(enemies, hit, weapon->mp40Damage);
                if (enemies->enemies[hit].state == ENEMY_DYING) {
                    game->killCount++;
                    PickupDrop(pickups, enemies->enemies[hit].position, enemies->enemies[hit].type);
                }
            }
        } else if (weapon->current == WEAPON_JACKHAMMER) {
            // Lunge forward — hard impulse + lock out WASD for duration
            Vector3 lungeDir = {shootDir.x, 0, shootDir.z};
            lungeDir = Vector3Normalize(lungeDir);
            player->velocity.x = lungeDir.x * JACKHAMMER_LUNGE_SPEED;
            player->velocity.z = lungeDir.z * JACKHAMMER_LUNGE_SPEED;
            player->velocity.y = 2.5f; // lift for punch feel
            player->lungeTimer = JACKHAMMER_LUNGE_DURATION;

            Vector3 meleePos = Vector3Add(shootOrigin, Vector3Scale(shootDir, weapon->jackhammerRange * 0.5f));
            int hit = EnemyCheckSphereHit(enemies, meleePos, weapon->jackhammerRange);
            if (hit >= 0) {
                // One-hit eviscerate kill — enemy torn apart, drops weapon
                game->killCount++;
                PickupDrop(pickups, enemies->enemies[hit].position, enemies->enemies[hit].type);
                EnemyEviscerate(enemies, hit, shootDir);
            }
        }
    }
}

void CombatProcessProjectiles(CombatContext *ctx) {
    Weapon *weapon = ctx->weapon;
    EnemyManager *enemies = ctx->enemies;
    PickupManager *pickups = ctx->pickups;
    Game *game = ctx->game;

    for (int i = 0; i < MAX_PROJECTILES; i++) {
        if (!weapon->projectiles[i].active) continue;
        int hit = EnemyCheckSphereHit(enemies, weapon->projectiles[i].position, weapon->projectiles[i].radius);
        if (hit >= 0) {
            for (int j = 0; j < MAX_ENEMIES; j++) {
                if (!enemies->enemies[j].active || enemies->enemies[j].state != ENEMY_ALIVE) continue;
                float d = Vector3Distance(weapon->projectiles[i].position, enemies->enemies[j].position);
                if (d < weapon->projectiles[i].radius) {
                    float dmg = weapon->projectiles[i].damage * (1.0f - d / weapon->projectiles[i].radius);
                    EnemyDamage(enemies, j, dmg);
                    if (enemies->enemies[j].state == ENEMY_DYING) {
                        game->killCount++;
                        PickupDrop(pickups, enemies->enemies[j].position, enemies->enemies[j].type);
                    }
                }
            }
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

void CombatProcessBeam(CombatContext *ctx, float dt) {
    if (!ctx->weapon->raketenFiring) return;

    Player *player = ctx->player;
    Weapon *weapon = ctx->weapon;
    EnemyManager *enemies = ctx->enemies;
    Game *game = ctx->game;

    Vector3 shootDir = PlayerGetForward(player);
    Vector3 barrel = WeaponGetBarrelWorldPos(weapon, player->camera);
    weapon->raketenBeamStart = barrel;
    weapon->raketenBeamEnd = Vector3Add(player->camera.position, Vector3Scale(shootDir, RAKETEN_BEAM_RANGE));

    // Massive knockback — beam pushes you hard opposite
    player->velocity.x -= shootDir.x * RAKETEN_BEAM_PUSH * dt;
    player->velocity.z -= shootDir.z * RAKETEN_BEAM_PUSH * dt;
    player->velocity.y += RAKETEN_BEAM_LIFT * dt;
    player->onGround = false;

    // Kill everything in the beam path
    Ray beamRay = {player->camera.position, shootDir};
    for (int ei = 0; ei < MAX_ENEMIES; ei++) {
        Enemy *en = &enemies->enemies[ei];
        if (!en->active || en->state != ENEMY_ALIVE) continue;
        BoundingBox box = {
            {en->position.x - 1.0f, en->position.y - 2.0f, en->position.z - 1.0f},
            {en->position.x + 1.0f, en->position.y + 2.0f, en->position.z + 1.0f}};
        RayCollision col = GetRayCollisionBox(beamRay, box);
        if (col.hit && col.distance < RAKETEN_BEAM_RANGE) {
            game->killCount++;
            EnemyVaporize(enemies, ei);
        }
    }
}

void CombatProcessEnemyDamage(CombatContext *ctx, float dt) {
    float dmg = EnemyCheckPlayerDamage(ctx->enemies, ctx->player->position, dt);
    if (dmg > 0) {
        PlayerTakeDamage(ctx->player, dmg * ctx->game->damageScaler);
    }
}
