#include "combat.h"
#include "config.h"
#include "world.h"
#include <stdlib.h>
#include <math.h>

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
        Ray pickRay = {shootOrigin, shootDir};
        float hd = 0;
        int hit = EnemyCheckHit(enemies, pickRay, PICKUP_SOVIET_RANGE, &hd);
        if (hit >= 0) {
            EnemyDamage(enemies, hit, pickups->pickupDamage);
            if (enemies->enemies[hit].state == ENEMY_DYING) {
                game->killCount++;
                PickupDrop(pickups, enemies->enemies[hit].position, enemies->enemies[hit].type);
            }
        }
        if (weapon->beamCount < MAX_BEAM_TRAILS) {
            Color bc = (pickups->pickupType == ENEMY_SOVIET) ?
                (Color)COLOR_BEAM_SOVIET : (Color)COLOR_BEAM_AMERICAN;
            weapon->beams[weapon->beamCount] = (BeamTrail){barrelPos,
                Vector3Add(shootOrigin, Vector3Scale(shootDir, PICKUP_SOVIET_RANGE)), bc, BEAM_TRAIL_LIFE};
            weapon->beamCount++;
        }
        PlayerApplyRecoil(player, shootDir, PICKUP_RECOIL_FORCE);
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
            Vector3 meleePos = Vector3Add(shootOrigin, Vector3Scale(shootDir, weapon->jackhammerRange * 0.5f));
            int hit = EnemyCheckSphereHit(enemies, meleePos, weapon->jackhammerRange);
            if (hit >= 0) {
                EnemyDamage(enemies, hit, weapon->jackhammerDamage);
                if (enemies->enemies[hit].state == ENEMY_DYING) {
                    game->killCount++;
                    PickupDrop(pickups, enemies->enemies[hit].position, enemies->enemies[hit].type);
                }
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
        PlayerTakeDamage(ctx->player, dmg);
    }
}
