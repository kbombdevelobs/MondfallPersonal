#ifndef ENEMY_SYSTEMS_H
#define ENEMY_SYSTEMS_H

#include "flecs.h"
#include "raylib.h"

void EcsEnemySystemsRegister(ecs_world_t *world);

void EcsEnemyDamage(ecs_world_t *world, ecs_entity_t entity, float damage);
void EcsEnemyVaporize(ecs_world_t *world, ecs_entity_t entity);
void EcsEnemyEviscerate(ecs_world_t *world, ecs_entity_t entity, Vector3 hitDir);
int EcsEnemyCountAlive(ecs_world_t *world);

#endif
