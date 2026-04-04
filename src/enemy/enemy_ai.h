#ifndef ENEMY_AI_H
#define ENEMY_AI_H

#include "flecs.h"

void EcsEnemyAISystemsRegister(ecs_world_t *world);

// Cleanup stored queries used by AI systems
void EcsEnemyAISystemsCleanup(void);

#endif
