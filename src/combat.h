#ifndef COMBAT_H
#define COMBAT_H

#include "player.h"
#include "weapon.h"
#include "enemy.h"
#include "pickup.h"
#include "game.h"

typedef struct {
    Player *player;
    Weapon *weapon;
    EnemyManager *enemies;
    PickupManager *pickups;
    Game *game;
} CombatContext;

// Process pickup weapon fire (left-click while holding pickup)
void CombatProcessPickupFire(CombatContext *ctx);

// Process normal weapon fire (left-click / V key)
void CombatProcessWeaponFire(CombatContext *ctx);

// Process in-flight projectile hits (Raketenfaust rockets)
void CombatProcessProjectiles(CombatContext *ctx);

// Process active Raketenfaust beam (continuous kill beam)
void CombatProcessBeam(CombatContext *ctx, float dt);

// Process enemy damage to player
void CombatProcessEnemyDamage(CombatContext *ctx, float dt);

#endif
