#ifndef ENEMY_MORALE_H
#define ENEMY_MORALE_H

#include "flecs.h"

// Register morale ECS systems (SysMoraleUpdate, SysMoraleCheck)
void EcsEnemyMoraleSystemsRegister(ecs_world_t *world);

// Cleanup stored queries
void EcsEnemyMoraleSystemsCleanup(void);

// Apply morale hit to all nearby same-faction alive enemies when a leader dies
void EcsApplyMoraleHit(ecs_world_t *world, ecs_entity_t deadEntity, float amount);

#endif
