#ifndef ECS_WORLD_H
#define ECS_WORLD_H

#include "flecs.h"

// Global ECS world — initialized once, used by all systems
ecs_world_t *GameEcsGetWorld(void);
void GameEcsInit(void);
void GameEcsFini(void);

#endif
