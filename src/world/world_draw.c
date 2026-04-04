#include "world_draw.h"
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

static void DrawChunk(Chunk *chunk, Vector3 playerPos) {
    float fade = 1.0f;

    DrawModel(chunk->ground, (Vector3){0, 0, 0}, 1.0f, WHITE);

    // Crater rims — subtle wireframe ring since the depression is in the mesh
    for (int i = 0; i < chunk->craterCount; i++) {
        Crater *c = &chunk->craters[i];
        float rimH = WorldGetHeight(c->position.x, c->position.z) + 0.15f;
        DrawCylinderWires((Vector3){c->position.x, rimH, c->position.z},
            c->radius * 1.05f, c->radius * 0.95f, 0.08f, 16,
            (Color){(unsigned char)(165 * fade), (unsigned char)(162 * fade), (unsigned char)(155 * fade), 180});
    }

    // Boulders — organic sphere clusters (low-poly for performance)
    for (int i = 0; i < chunk->rockCount; i++) {
        Rock *r = &chunk->rocks[i];

        float rdist = Vector3Distance(playerPos, r->position);
        if (rdist > CHUNK_SIZE * 3) continue;

        Color rc = {(unsigned char)(r->color.r * fade), (unsigned char)(r->color.g * fade),
                    (unsigned char)(r->color.b * fade), 255};
        Color rcDark = {(unsigned char)(rc.r * 0.7f), (unsigned char)(rc.g * 0.7f),
                        (unsigned char)(rc.b * 0.7f), 255};
        Color rcLight = {(unsigned char)fminf(rc.r * 1.15f, 255), (unsigned char)fminf(rc.g * 1.15f, 255),
                         (unsigned char)fminf(rc.b * 1.1f, 255), 255};

        float avg = (r->size.x + r->size.y + r->size.z) / 3.0f;
        Vector3 p = r->position;
        float rot = r->rotation;

        int detail = (rdist < CHUNK_SIZE) ? 6 : 4;

        // Main body
        DrawSphereEx(p, avg * 0.5f, detail, detail, rc);
        // Base
        DrawSphereEx((Vector3){p.x, p.y - avg * 0.15f, p.z}, avg * 0.45f, detail, detail, rcDark);
        // Top lump
        DrawSphereEx((Vector3){p.x + cosf(rot) * avg * 0.15f, p.y + avg * 0.25f,
                     p.z + sinf(rot) * avg * 0.1f}, avg * 0.35f, detail, detail, rcLight);

        // Extra detail when close
        if (rdist < CHUNK_SIZE * 1.5f) {
            DrawSphereEx((Vector3){p.x + cosf(rot+1.5f)*avg*0.3f, p.y - avg*0.05f,
                         p.z + sinf(rot+1.5f)*avg*0.3f}, avg*0.3f, detail, detail, rcDark);
            DrawSphereEx((Vector3){p.x + cosf(rot+3.5f)*avg*0.25f, p.y + avg*0.1f,
                         p.z + sinf(rot+3.5f)*avg*0.25f}, avg*0.28f, detail, detail, rc);
        }

        // Shadow
        DrawCylinder((Vector3){p.x, p.y - avg * 0.48f, p.z},
            avg * 0.55f, avg * 0.5f, 0.02f, 6,
            (Color){(unsigned char)(30 * fade), (unsigned char)(28 * fade), (unsigned char)(25 * fade), 100});
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

        // Frustum cull: skip chunks behind camera (except the player's own chunk)
        if (abs(dx) + abs(dz) > 0) {
            Vector3 chunkCenter = {
                (ch->cx + 0.5f) * CHUNK_SIZE, 0.0f,
                (ch->cz + 0.5f) * CHUNK_SIZE
            };
            Vector3 toChunk = Vector3Subtract(chunkCenter, camera.position);
            float dot = toChunk.x * camFwd.x + toChunk.z * camFwd.z;
            float dist = sqrtf(toChunk.x * toChunk.x + toChunk.z * toChunk.z);
            if (dot < -CHUNK_SIZE) continue;
            if (dist > CHUNK_SIZE * 2.0f && dist > 0.01f && dot / dist < 0.15f) continue;
        }

        DrawChunk(ch, playerPos);
    }
}
