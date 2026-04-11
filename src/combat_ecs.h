#ifndef COMBAT_ECS_H
#define COMBAT_ECS_H

#include "flecs.h"
#include "raylib.h"
#include "player.h"
#include "weapon.h"
#include "pickup.h"
#include "game.h"

typedef struct {
    Player *player;
    Weapon *weapon;
    PickupManager *pickups;
    Game *game;
    ecs_world_t *ecsWorld;
} EcsCombatContext;

// Hit detection using ECS queries — returns entity ID (0 = miss)
ecs_entity_t EcsEnemyCheckHit(ecs_world_t *world, Ray ray, float maxDist, float *hitDist, bool *outHeadshot);
ecs_entity_t EcsEnemyCheckSphereHit(ecs_world_t *world, Vector3 center, float radius);

// Combat processing (same role as old CombatProcess* functions)
void EcsCombatProcessPickupFire(EcsCombatContext *ctx);
void EcsCombatProcessWeaponFire(EcsCombatContext *ctx);
void EcsCombatProcessProjectiles(EcsCombatContext *ctx);
void EcsCombatProcessBeam(EcsCombatContext *ctx, float dt);

// Ground pound area-of-effect damage/knockback — returns screen shake amount (0 if no impact)
float EcsCombatProcessGroundPound(EcsCombatContext *ctx);

#endif
