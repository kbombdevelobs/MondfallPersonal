#ifndef ENEMY_SPAWN_H
#define ENEMY_SPAWN_H

#include "flecs.h"
#include "enemy_components.h"
#include "raylib.h"

ecs_entity_t EcsEnemySpawnAt(ecs_world_t *world, EnemyType type, Vector3 pos);
ecs_entity_t EcsEnemySpawnAroundPlayer(ecs_world_t *world, EnemyType type, Vector3 playerPos, float spawnRadius);

#endif
