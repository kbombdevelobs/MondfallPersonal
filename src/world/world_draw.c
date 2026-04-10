#include "world_draw.h"
#include "world_rocks.h"
#include "world_noise.h"
#include <math.h>
#include <stdlib.h>

void WorldDrawSky(World *world, Camera3D camera) {
    // Stars — bigger cubes so they survive pixelation
    for (int i = 0; i < world->starCount; i++) {
        Vector3 sp = world->stars[i].position;
        sp.x += camera.position.x;
        sp.z += camera.position.z;

        if (i % 8 == 0) {
            DrawCube(sp, 1.5f, 1.5f, 1.5f, (Color){255, 255, 230, 255});
        } else if (i % 3 == 0) {
            DrawCube(sp, 1.0f, 1.0f, 1.0f, WHITE);
        } else {
            DrawCube(sp, 0.6f, 0.6f, 0.6f, (Color){200, 200, 210, 255});
        }
    }

    // Earth — flat billboard, obviously Earth
    Vector3 ep = {camera.position.x - 80.0f, 120.0f, camera.position.z - 180.0f};
    Vector3 toCamera = Vector3Normalize(Vector3Subtract(camera.position, ep));
    Vector3 earthRight = Vector3Normalize(Vector3CrossProduct(toCamera, (Vector3){0,1,0}));
    Vector3 earthUp = Vector3CrossProduct(earthRight, toCamera);
    float R = 25.0f;

    // Draw Earth as a filled disc of pixels
    int res = 20;
    float step = R * 2.0f / (float)res;
    for (int gy = 0; gy < res; gy++) {
        for (int gx = 0; gx < res; gx++) {
            float lx = -R + ((float)gx + 0.5f) * step;
            float ly = -R + ((float)gy + 0.5f) * step;
            float d = sqrtf(lx * lx + ly * ly);
            if (d > R) continue;

            float nx = lx / R;
            float ny = ly / R;

            Color ec = {30, 65, 150, 255}; // ocean blue

            // === CONTINENTS — recognizable shapes ===
            if (nx > -0.7f && nx < -0.05f && ny > 0.1f && ny < 0.7f) ec = (Color){50, 130, 55, 255};
            if (nx > -0.35f && nx < -0.15f && ny > -0.05f && ny < 0.15f) ec = (Color){55, 125, 50, 255};
            if (nx > -0.4f && nx < -0.05f && ny > -0.7f && ny < -0.05f &&
                fabsf(nx + 0.2f) < 0.2f - ny * 0.1f) ec = (Color){48, 135, 52, 255};
            if (nx > 0.05f && nx < 0.35f && ny > 0.35f && ny < 0.6f) ec = (Color){55, 128, 48, 255};
            if (nx > 0.1f && nx < 0.5f && ny > -0.45f && ny < 0.35f &&
                fabsf(nx - 0.3f) < 0.2f - fabsf(ny) * 0.1f) ec = (Color){60, 135, 45, 255};
            if (nx > 0.3f && nx < 0.85f && ny > 0.15f && ny < 0.7f) ec = (Color){52, 125, 50, 255};
            if (nx > 0.45f && nx < 0.7f && ny > -0.55f && ny < -0.3f) ec = (Color){65, 130, 48, 255};

            // Desert areas
            if (nx > 0.1f && nx < 0.45f && ny > 0.1f && ny < 0.3f) ec = (Color){170, 155, 100, 255};
            if (nx > 0.35f && nx < 0.55f && ny > 0.05f && ny < 0.2f) ec = (Color){175, 160, 105, 255};

            // Ice caps
            if (ny > 0.8f) ec = (Color){230, 235, 240, 255};
            if (ny < -0.82f) ec = (Color){225, 230, 238, 255};

            // Cloud wisps
            float cloudNoise = sinf(nx * 8.0f + 2.0f) * sinf(ny * 6.0f + 1.0f);
            if (cloudNoise > 0.5f) {
                ec.r = (unsigned char)fminf(ec.r + 80, 255);
                ec.g = (unsigned char)fminf(ec.g + 80, 255);
                ec.b = (unsigned char)fminf(ec.b + 80, 255);
            }

            // Edge darkening (atmospheric limb)
            float limb = 1.0f - (d / R);
            if (limb < 0.15f) {
                float f = limb / 0.15f;
                ec.r = (unsigned char)(ec.r * f + 40 * (1.0f - f));
                ec.g = (unsigned char)(ec.g * f + 80 * (1.0f - f));
                ec.b = (unsigned char)(ec.b * f + 180 * (1.0f - f));
            }

            Vector3 p = Vector3Add(ep,
                Vector3Add(Vector3Scale(earthRight, lx),
                           Vector3Scale(earthUp, ly)));
            DrawCube(p, step * 0.95f, step * 0.95f, step * 0.95f, ec);
        }
    }

    // Atmosphere halo — blue glow ring just outside the disc
    for (int s = 0; s < 40; s++) {
        float a = (float)s / 40.0f * 2.0f * PI;
        for (int layer = 0; layer < 3; layer++) {
            float lr = R + 1.5f + (float)layer * 1.5f;
            Vector3 p = Vector3Add(ep,
                Vector3Add(Vector3Scale(earthRight, cosf(a) * lr),
                           Vector3Scale(earthUp, sinf(a) * lr)));
            unsigned char ha = (unsigned char)(50 - layer * 15);
            DrawCube(p, 2.0f, 2.0f, 2.0f, (Color){80, 150, 255, ha});
        }
    }
}

static void DrawChunk(Chunk *chunk, Vector3 playerPos, World *world) {
    DrawModel(chunk->ground, (Vector3){0, 0, 0}, 1.0f, WHITE);

    // Boulders — use .glb models if loaded, otherwise procedural fallback
    for (int i = 0; i < chunk->rockCount; i++) {
        WorldRockDraw(world, &chunk->rocks[i], playerPos, CHUNK_SIZE);
    }
}

void WorldDraw(World *world, Vector3 playerPos, Camera3D camera) {
    int pcx = (int)floorf(playerPos.x / CHUNK_SIZE);
    int pcz = (int)floorf(playerPos.z / CHUNK_SIZE);
    int half = RENDER_CHUNKS / 2;

    // Camera forward vector for frustum culling
    Vector3 camFwd = Vector3Normalize(Vector3Subtract(camera.target, camera.position));

    for (int i = 0; i < world->chunkCount; i++) {
        Chunk *ch = &world->chunks[i];
        if (!ch->generated) continue;
        int dx = ch->cx - pcx, dz = ch->cz - pcz;
        if (dx < -half || dx > half || dz < -half || dz > half) continue;

        // Frustum cull: only skip chunks fully behind camera
        // Test all 4 corners — if ANY corner is in front of camera, draw the chunk
        if (abs(dx) + abs(dz) > 1) {
            float cx0 = ch->cx * CHUNK_SIZE;
            float cz0 = ch->cz * CHUNK_SIZE;
            float cx1 = cx0 + CHUNK_SIZE;
            float cz1 = cz0 + CHUNK_SIZE;
            float d0 = (cx0 - camera.position.x) * camFwd.x + (cz0 - camera.position.z) * camFwd.z;
            float d1 = (cx1 - camera.position.x) * camFwd.x + (cz0 - camera.position.z) * camFwd.z;
            float d2 = (cx0 - camera.position.x) * camFwd.x + (cz1 - camera.position.z) * camFwd.z;
            float d3 = (cx1 - camera.position.x) * camFwd.x + (cz1 - camera.position.z) * camFwd.z;
            float maxDot = d0;
            if (d1 > maxDot) maxDot = d1;
            if (d2 > maxDot) maxDot = d2;
            if (d3 > maxDot) maxDot = d3;
            // Only cull if ALL corners are behind camera
            if (maxDot < 0.0f) continue;
        }

        DrawChunk(ch, playerPos, world);
    }
}
