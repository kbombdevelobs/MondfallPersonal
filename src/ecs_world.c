#include "ecs_world.h"
#include "enemy/enemy_systems.h"
#include "enemy/enemy_draw_ecs.h"

static ecs_world_t *g_world = NULL;

ecs_world_t *GameEcsGetWorld(void) {
    return g_world;
}

void GameEcsInit(void) {
    if (g_world) {
        EcsEnemySystemsCleanup();
        EcsEnemyResourcesUnload(g_world);
        ecs_fini(g_world);
    }
    g_world = ecs_init();
}

void GameEcsFini(void) {
    if (g_world) {
        ecs_fini(g_world);
        g_world = NULL;
    }
}
