#ifndef ENEMY_DRAW_ECS_H
#define ENEMY_DRAW_ECS_H

#include "flecs.h"
#include "raylib.h"

// Render all enemies from ECS data (with LOD + frustum culling when testMode)
void EcsEnemyManagerDraw(ecs_world_t *world, Camera3D camera, bool testMode);

// Load/unload shared enemy render resources into EcEnemyResources singleton
void EcsEnemyResourcesInit(ecs_world_t *world);
void EcsEnemyResourcesUnload(ecs_world_t *world);

#endif
