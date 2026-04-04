#include "ecs_world.h"

static ecs_world_t *g_world = NULL;

ecs_world_t *GameEcsGetWorld(void) {
    return g_world;
}

void GameEcsInit(void) {
    if (g_world) ecs_fini(g_world);
    g_world = ecs_init();
}

void GameEcsFini(void) {
    if (g_world) {
        ecs_fini(g_world);
        g_world = NULL;
    }
}
