#include "world_rocks.h"
#include "rlgl.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

/* ============================================================================
 * Rock model loader and renderer
 *
 * Loads 3 rock .glb variants from resources/models/rocks/.
 * Maps rock size to model variant:
 *   small  (avg size < 2.0) -> rock_small.glb
 *   medium (avg size < 4.0) -> rock_medium.glb
 *   large  (avg size >= 4.0) -> rock_large.glb
 * Falls back to procedural sphere-cluster rendering if models not loaded.
 * ============================================================================ */

#define ROCK_MODEL_PATH "resources/models/rocks/"

/// File names indexed to match rockModels[0..2] = small, medium, large
static const char *ROCK_FILES[3] = {
    "rock_small.glb",
    "rock_medium.glb",
    "rock_large.glb",
};

/// Reference sizes the models were built at (diameter)
static const float ROCK_REF_SIZES[3] = { 0.5f, 1.5f, 3.0f };

/* -------------------------------------------------------------------------- */

void WorldRocksInit(World *world) {
    if (!world) return;

    world->rockModelCount = 0;

    for (int i = 0; i < 3; i++) {
        char path[256];
        snprintf(path, sizeof(path), "%s%s", ROCK_MODEL_PATH, ROCK_FILES[i]);

        if (!FileExists(path)) {
            TraceLog(LOG_INFO, "WorldRocks: file not found: %s", path);
            continue;
        }

        world->rockModels[i] = LoadModel(path);
        if (world->rockModels[i].meshCount <= 0) {
            TraceLog(LOG_WARNING, "WorldRocks: failed to load: %s", path);
            continue;
        }

        world->rockModelCount++;
        TraceLog(LOG_INFO, "WorldRocks: loaded %s (%d meshes)",
                 path, world->rockModels[i].meshCount);
    }

    if (world->rockModelCount > 0) {
        TraceLog(LOG_INFO, "WorldRocks: %d/3 rock models loaded", world->rockModelCount);
    } else {
        TraceLog(LOG_INFO, "WorldRocks: no rock models found, using procedural fallback");
    }
}

void WorldRocksUnload(World *world) {
    if (!world) return;

    for (int i = 0; i < 3; i++) {
        if (world->rockModels[i].meshCount > 0) {
            UnloadModel(world->rockModels[i]);
            world->rockModels[i].meshCount = 0;
        }
    }
    world->rockModelCount = 0;
    TraceLog(LOG_INFO, "WorldRocks: all rock models unloaded");
}

/// Pick the model index (0=small, 1=medium, 2=large) based on average size.
static int RockSizeIndex(Rock *r) {
    float avg = (r->size.x + r->size.y + r->size.z) / 3.0f;
    if (avg < 2.0f) return 0;
    if (avg < 4.0f) return 1;
    return 2;
}

/// Procedural sphere-cluster fallback (original DrawChunk rock rendering).
static void DrawRockProcedural(Rock *r, Vector3 playerPos, float chunkSize) {
    float rdist = Vector3Distance(playerPos, r->position);
    if (rdist > chunkSize * 3) return;

    Color rc = r->color;
    Color rcDark = {(unsigned char)(rc.r * 0.7f), (unsigned char)(rc.g * 0.7f),
                    (unsigned char)(rc.b * 0.7f), 255};
    Color rcLight = {(unsigned char)fminf(rc.r * 1.15f, 255),
                     (unsigned char)fminf(rc.g * 1.15f, 255),
                     (unsigned char)fminf(rc.b * 1.1f, 255), 255};

    float avg = (r->size.x + r->size.y + r->size.z) / 3.0f;
    Vector3 p = r->position;
    float rot = r->rotation;
    int detail = (rdist < chunkSize) ? 6 : 4;

    /* Main body */
    DrawSphereEx(p, avg * 0.5f, detail, detail, rc);
    /* Base */
    DrawSphereEx((Vector3){p.x, p.y - avg * 0.15f, p.z},
                 avg * 0.45f, detail, detail, rcDark);
    /* Top lump */
    DrawSphereEx((Vector3){p.x + cosf(rot) * avg * 0.15f,
                           p.y + avg * 0.25f,
                           p.z + sinf(rot) * avg * 0.1f},
                 avg * 0.35f, detail, detail, rcLight);

    /* Extra detail when close */
    if (rdist < chunkSize * 1.5f) {
        DrawSphereEx((Vector3){p.x + cosf(rot + 1.5f) * avg * 0.3f,
                               p.y - avg * 0.05f,
                               p.z + sinf(rot + 1.5f) * avg * 0.3f},
                     avg * 0.3f, detail, detail, rcDark);
        DrawSphereEx((Vector3){p.x + cosf(rot + 3.5f) * avg * 0.25f,
                               p.y + avg * 0.1f,
                               p.z + sinf(rot + 3.5f) * avg * 0.25f},
                     avg * 0.28f, detail, detail, rc);
    }

    /* Shadow disc */
    DrawCylinder((Vector3){p.x, p.y - avg * 0.48f, p.z},
                 avg * 0.55f, avg * 0.5f, 0.02f, 6,
                 (Color){30, 28, 25, 100});
}

void WorldRockDraw(World *world, Rock *rock, Vector3 playerPos, float chunkSize) {
    if (!world || !rock) return;

    float rdist = Vector3Distance(playerPos, rock->position);
    if (rdist > chunkSize * 3) return;

    /* If no models loaded, use procedural fallback */
    if (world->rockModelCount == 0) {
        DrawRockProcedural(rock, playerPos, chunkSize);
        return;
    }

    int idx = RockSizeIndex(rock);

    /* If this specific variant didn't load, try fallback to procedural */
    if (world->rockModels[idx].meshCount <= 0) {
        DrawRockProcedural(rock, playerPos, chunkSize);
        return;
    }

    /* Scale model to match the rock's actual size */
    float avg = (rock->size.x + rock->size.y + rock->size.z) / 3.0f;
    float modelScale = avg / ROCK_REF_SIZES[idx];

    /* Draw the .glb model at the rock's position with rotation */
    rlPushMatrix();
    rlTranslatef(rock->position.x, rock->position.y, rock->position.z);
    rlRotatef(rock->rotation, 0, 1, 0);
    rlScalef(modelScale, modelScale, modelScale);

    DrawModel(world->rockModels[idx], (Vector3){0, 0, 0}, 1.0f, WHITE);

    rlPopMatrix();

    /* Shadow disc */
    DrawCylinder((Vector3){rock->position.x,
                           rock->position.y - avg * 0.48f,
                           rock->position.z},
                 avg * 0.55f, avg * 0.5f, 0.02f, 6,
                 (Color){30, 28, 25, 100});
}
