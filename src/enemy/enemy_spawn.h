#ifndef ENEMY_SPAWN_H
#define ENEMY_SPAWN_H

#include "flecs.h"
#include "enemy_components.h"
#include "raylib.h"

ecs_entity_t EcsEnemySpawnAt(ecs_world_t *world, EnemyType type, Vector3 pos);
ecs_entity_t EcsEnemySpawnRanked(ecs_world_t *world, EnemyType type, EnemyRank rank, Vector3 pos);
ecs_entity_t EcsEnemySpawnSquad(ecs_world_t *world, EnemyType type, EnemyRank rank, Vector3 pos, int squadId);
ecs_entity_t EcsEnemySpawnAroundPlayer(ecs_world_t *world, EnemyType type, Vector3 playerPos, float spawnRadius);

// Formation offset for spawn position (index in squad, total squad size, faction)
Vector3 FormationOffset(int index, int total, EnemyType faction, EnemyRank rank);

#endif
