#include "world.h"
#include "rlgl.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>

// ---- Noise functions for terrain height ----
static float Hash2D(float x, float z) {
    // Simple hash for 2D noise
    int ix = (int)floorf(x) * 73856093;
    int iz = (int)floorf(z) * 19349663;
    int h = (ix ^ iz) & 0x7FFFFFFF;
    return (float)(h % 10000) / 10000.0f;
}

static float LerpF(float a, float b, float t) { return a + (b - a) * t; }

static float SmoothStep(float t) { return t * t * (3.0f - 2.0f * t); }

static float ValueNoise(float x, float z) {
    float ix = floorf(x), iz = floorf(z);
    float fx = x - ix, fz = z - iz;
    fx = SmoothStep(fx);
    fz = SmoothStep(fz);
    float a = Hash2D(ix, iz);
    float b = Hash2D(ix + 1, iz);
    float c = Hash2D(ix, iz + 1);
    float d = Hash2D(ix + 1, iz + 1);
    return LerpF(LerpF(a, b, fx), LerpF(c, d, fx), fz);
}

// Multi-octave noise for terrain
float WorldGetHeight(float x, float z) {
    float h = 0;
    float scale = 0.012f;
    h += ValueNoise(x * scale, z * scale) * 8.0f;           // big steep hills
    h += ValueNoise(x * scale * 2.5f, z * scale * 2.5f) * 3.0f;  // ridges
    h += ValueNoise(x * scale * 6.0f, z * scale * 6.0f) * 1.0f;  // bumps
    h += ValueNoise(x * scale * 15.0f, z * scale * 15.0f) * 0.3f; // fine grit
    return h - 5.0f;
}

static unsigned int ChunkHash(int cx, int cz) {
    unsigned int h = (unsigned int)(cx * 73856093) ^ (unsigned int)(cz * 19349663);
    h ^= h >> 16; h *= 0x45d9f3b; h ^= h >> 16;
    return h;
}

static float SeededFloat(unsigned int *seed, float min, float max) {
    *seed = *seed * 1103515245 + 12345;
    float t = (float)((*seed >> 16) & 0x7FFF) / 32767.0f;
    return min + t * (max - min);
}

static Texture2D GenMoonTexture(void) {
    int w = 256, h = 256;
    Image base = GenImagePerlinNoise(w, h, 0, 0, 3.0f);
    ImageColorTint(&base, (Color){155, 150, 140, 255});
    // Crater marks
    for (int i = 0; i < 8; i++) {
        int cx = GetRandomValue(30, w - 30);
        int cy = GetRandomValue(30, h - 30);
        int r = GetRandomValue(10, 40);
        ImageDrawCircle(&base, cx, cy, r, (Color){125, 120, 112, 140});
        ImageDrawCircleLines(&base, cx, cy, r + 1, (Color){172, 168, 158, 180});
        ImageDrawCircle(&base, cx + 2, cy + 2, r - 3, (Color){115, 112, 105, 100});
    }
    // Fine regolith noise
    for (int i = 0; i < 200; i++) {
        int px = GetRandomValue(0, w - 1);
        int py = GetRandomValue(0, h - 1);
        int shade = GetRandomValue(130, 175);
        ImageDrawPixel(&base, px, py, (Color){(unsigned char)shade, (unsigned char)shade, (unsigned char)(shade - 8), 255});
    }
    // Boot print style marks
    for (int i = 0; i < 8; i++) {
        int px = GetRandomValue(50, w - 50);
        int py = GetRandomValue(50, h - 50);
        ImageDrawRectangle(&base, px, py, GetRandomValue(3, 8), GetRandomValue(12, 25), (Color){135, 132, 125, 100});
    }
    Texture2D tex = LoadTextureFromImage(base);
    SetTextureFilter(tex, TEXTURE_FILTER_BILINEAR);
    SetTextureWrap(tex, TEXTURE_WRAP_REPEAT);
    UnloadImage(base);
    return tex;
}

// Generate chunk mesh with heightmap + crater depressions
static Model GenTerrainMesh(int cx, int cz, Texture2D moonTex, Crater *craters, int craterCount) {
    float baseX = cx * CHUNK_SIZE;
    float baseZ = cz * CHUNK_SIZE;

    Mesh mesh = GenMeshPlane(CHUNK_SIZE, CHUNK_SIZE, MESH_RES, MESH_RES);

    // Displace vertices by noise height + crater holes
    int vertCount = mesh.vertexCount;
    for (int i = 0; i < vertCount; i++) {
        float vx = mesh.vertices[i * 3 + 0] + baseX + CHUNK_SIZE * 0.5f;
        float vz = mesh.vertices[i * 3 + 2] + baseZ + CHUNK_SIZE * 0.5f;
        float h = WorldGetHeight(vx, vz);

        // Crater depressions — smooth bowl shape
        for (int c = 0; c < craterCount; c++) {
            float dx = vx - craters[c].position.x;
            float dz = vz - craters[c].position.z;
            float dist = sqrtf(dx * dx + dz * dz);
            float r = craters[c].radius;
            if (dist < r) {
                // Smooth bowl: deepest at center, rim at edge
                float t = dist / r; // 0=center, 1=edge
                float bowl = (1.0f - t * t) * craters[c].depth * 2.5f;
                h -= bowl;
                // Slight rim bump at edge
                if (t > 0.8f) {
                    float rimT = (t - 0.8f) / 0.2f;
                    h += rimT * craters[c].depth * 0.4f;
                }
            }
        }

        mesh.vertices[i * 3 + 1] = h;
    }

    // Recalculate normals
    for (int i = 0; i < mesh.triangleCount; i++) {
        int i0, i1, i2;
        if (mesh.indices) {
            i0 = mesh.indices[i * 3 + 0];
            i1 = mesh.indices[i * 3 + 1];
            i2 = mesh.indices[i * 3 + 2];
        } else {
            i0 = i * 3 + 0; i1 = i * 3 + 1; i2 = i * 3 + 2;
        }
        Vector3 v0 = {mesh.vertices[i0*3], mesh.vertices[i0*3+1], mesh.vertices[i0*3+2]};
        Vector3 v1 = {mesh.vertices[i1*3], mesh.vertices[i1*3+1], mesh.vertices[i1*3+2]};
        Vector3 v2 = {mesh.vertices[i2*3], mesh.vertices[i2*3+1], mesh.vertices[i2*3+2]};
        Vector3 e1 = Vector3Subtract(v1, v0);
        Vector3 e2 = Vector3Subtract(v2, v0);
        Vector3 n = Vector3Normalize(Vector3CrossProduct(e1, e2));
        for (int j = 0; j < 3; j++) {
            int idx;
            if (mesh.indices) idx = mesh.indices[i*3+j]; else idx = i*3+j;
            mesh.normals[idx*3+0] = n.x;
            mesh.normals[idx*3+1] = n.y;
            mesh.normals[idx*3+2] = n.z;
        }
    }

    UpdateMeshBuffer(mesh, 0, mesh.vertices, mesh.vertexCount * 3 * sizeof(float), 0);
    UpdateMeshBuffer(mesh, 2, mesh.normals, mesh.vertexCount * 3 * sizeof(float), 0);

    Model model = LoadModelFromMesh(mesh);
    model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = moonTex;
    return model;
}

static void GenerateChunk(Chunk *chunk, int cx, int cz, Texture2D moonTex) {
    chunk->cx = cx;
    chunk->cz = cz;
    chunk->generated = true;

    float baseX = cx * CHUNK_SIZE;
    float baseZ = cz * CHUNK_SIZE;
    unsigned int seed = ChunkHash(cx, cz);

    // Craters (visual overlays)
    chunk->craterCount = 1 + (int)(seed % MAX_CHUNK_CRATERS);
    for (int i = 0; i < chunk->craterCount; i++) {
        float px = baseX + SeededFloat(&seed, 5.0f, CHUNK_SIZE - 5.0f);
        float pz = baseZ + SeededFloat(&seed, 5.0f, CHUNK_SIZE - 5.0f);
        chunk->craters[i].position = (Vector3){px, WorldGetHeight(px, pz) + 0.02f, pz};
        chunk->craters[i].radius = SeededFloat(&seed, 2.0f, 7.0f);
        chunk->craters[i].depth = SeededFloat(&seed, 0.15f, 0.6f);
    }

    // Rocks
    chunk->rockCount = 3 + (int)(seed % (MAX_CHUNK_ROCKS - 2));
    for (int i = 0; i < chunk->rockCount; i++) {
        float s = SeededFloat(&seed, 1.0f, 5.0f); // much bigger — cover sized
        float rx = baseX + SeededFloat(&seed, 3.0f, CHUNK_SIZE - 3.0f);
        float rz = baseZ + SeededFloat(&seed, 3.0f, CHUNK_SIZE - 3.0f);
        chunk->rocks[i].position = (Vector3){rx, WorldGetHeight(rx, rz) + s * 0.35f, rz};
        chunk->rocks[i].size = (Vector3){
            s * SeededFloat(&seed, 0.8f, 1.5f),
            s * SeededFloat(&seed, 0.6f, 1.8f),  // tall enough to hide behind
            s * SeededFloat(&seed, 0.8f, 1.5f)
        };
        chunk->rocks[i].rotation = SeededFloat(&seed, 0, 360.0f);
        int shade = 70 + (int)(SeededFloat(&seed, 0, 1) * 55);
        chunk->rocks[i].color = (Color){(unsigned char)shade, (unsigned char)shade, (unsigned char)(shade - 8), 255};
    }

    // Generate terrain mesh with crater depressions baked in
    chunk->ground = GenTerrainMesh(cx, cz, moonTex, chunk->craters, chunk->craterCount);
}

static Chunk *FindChunk(World *world, int cx, int cz) {
    for (int i = 0; i < world->chunkCount; i++) {
        if (world->chunks[i].cx == cx && world->chunks[i].cz == cz)
            return &world->chunks[i];
    }
    return NULL;
}

static Chunk *CreateChunk(World *world, int cx, int cz) {
    int slot;
    if (world->chunkCount < MAX_CACHED_CHUNKS) {
        slot = world->chunkCount++;
    } else {
        // Evict furthest chunk from requested position
        int best = 0;
        int bestDist = 0;
        for (int i = 0; i < world->chunkCount; i++) {
            int d = abs(world->chunks[i].cx - cx) + abs(world->chunks[i].cz - cz);
            if (d > bestDist) { bestDist = d; best = i; }
        }
        slot = best;
        if (world->chunks[slot].generated) UnloadModel(world->chunks[slot].ground);
    }
    GenerateChunk(&world->chunks[slot], cx, cz, world->moonTex);
    return &world->chunks[slot];
}

static World *g_activeWorld = NULL;
World *WorldGetActive(void) { return g_activeWorld; }

void WorldInit(World *world) {
    g_activeWorld = world;
    memset(world, 0, sizeof(World));
    world->moonTex = GenMoonTexture();

    Image rockImg = GenImagePerlinNoise(64, 64, 50, 50, 6.0f);
    ImageColorTint(&rockImg, (Color){125, 120, 112, 255});
    ImageColorContrast(&rockImg, 25.0f);
    world->rockTex = LoadTextureFromImage(rockImg);
    SetTextureFilter(world->rockTex, TEXTURE_FILTER_POINT);
    UnloadImage(rockImg);

    world->rockModelCount = 0;
    world->texturesLoaded = true;

    world->starCount = MAX_STARS;
    unsigned int starSeed = 42;
    for (int i = 0; i < world->starCount; i++) {
        float a1 = SeededFloat(&starSeed, 0, 2.0f * PI);
        float a2 = SeededFloat(&starSeed, 0.05f, PI * 0.5f);
        world->stars[i].position = (Vector3){
            cosf(a2) * sinf(a1) * 500.0f,
            sinf(a2) * 500.0f,
            cosf(a2) * cosf(a1) * 500.0f
        };
    }
}

void WorldUnload(World *world) {
    for (int i = 0; i < world->chunkCount; i++) {
        if (world->chunks[i].generated) UnloadModel(world->chunks[i].ground);
    }
    if (world->texturesLoaded) {
        UnloadTexture(world->moonTex);
        UnloadTexture(world->rockTex);
    }
}

void WorldUpdate(World *world, Vector3 playerPos) {
    int pcx = (int)floorf(playerPos.x / CHUNK_SIZE);
    int pcz = (int)floorf(playerPos.z / CHUNK_SIZE);
    int half = RENDER_CHUNKS / 2;
    int generated = 0;

    // Generate missing chunks, closest first (spiral out from center)
    for (int ring = 0; ring <= half && generated < MAX_CHUNKS_PER_FRAME; ring++) {
        for (int dz = -ring; dz <= ring && generated < MAX_CHUNKS_PER_FRAME; dz++) {
            for (int dx = -ring; dx <= ring && generated < MAX_CHUNKS_PER_FRAME; dx++) {
                // Only process the border of each ring (skip inner already done)
                if (abs(dx) != ring && abs(dz) != ring) continue;
                if (abs(dx) > half || abs(dz) > half) continue;

                int cx = pcx + dx, cz = pcz + dz;
                if (!FindChunk(world, cx, cz)) {
                    CreateChunk(world, cx, cz);
                    generated++;
                }
            }
        }
    }
}

void WorldDrawSky(World *world, Camera3D camera) {
    for (int i = 0; i < world->starCount; i++) {
        Vector3 sp = world->stars[i].position;
        sp.x += camera.position.x;
        sp.z += camera.position.z;
        DrawPoint3D(sp, WHITE);
        // Draw a tiny cross for brighter stars
        if (i % 5 == 0) {
            DrawLine3D(Vector3Add(sp, (Vector3){-0.3f,0,0}), Vector3Add(sp, (Vector3){0.3f,0,0}), WHITE);
            DrawLine3D(Vector3Add(sp, (Vector3){0,-0.3f,0}), Vector3Add(sp, (Vector3){0,0.3f,0}), WHITE);
        }
    }
    Vector3 ep = {camera.position.x - 100.0f, 150.0f, camera.position.z - 200.0f};
    DrawSphereEx(ep, 30.0f, 12, 12, (Color){35, 70, 150, 255});   // ocean
    DrawSphereEx((Vector3){ep.x+6, ep.y+4, ep.z+26}, 13.0f, 6, 6, (Color){45, 120, 45, 220}); // continent
    DrawSphereEx((Vector3){ep.x-10, ep.y-8, ep.z+22}, 9.0f, 6, 6, (Color){50, 130, 50, 200}); // continent 2
    DrawSphereEx((Vector3){ep.x+2, ep.y+12, ep.z+24}, 7.0f, 5, 5, (Color){220, 220, 220, 180}); // ice cap
    DrawSphereWires(ep, 30.5f, 12, 12, (Color){80, 140, 220, 30}); // atmosphere glow
}

static void DrawChunk(World *world, Chunk *chunk, Vector3 playerPos) {
    float baseX = chunk->cx * CHUNK_SIZE;
    float baseZ = chunk->cz * CHUNK_SIZE;
    Vector3 center = {baseX + CHUNK_SIZE * 0.5f, 0, baseZ + CHUNK_SIZE * 0.5f};

    // Distance fade for far chunks
    float dist = Vector3Distance(playerPos, center);
    float maxDist = CHUNK_SIZE * (RENDER_CHUNKS / 2);
    float fade = 1.0f;
    if (dist > maxDist * 0.7f) {
        fade = 1.0f - (dist - maxDist * 0.7f) / (maxDist * 0.3f);
        if (fade < 0) fade = 0;
    }
    unsigned char alpha = (unsigned char)(fade * 255);
    Color tint = {alpha, alpha, alpha, 255};

    DrawModel(chunk->ground, center, 1.0f, tint);

    // Crater rims — just a subtle wireframe ring since the depression is in the mesh
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

        // Skip far rocks
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

        // Use low rings/slices for chunky look + performance
        int detail = (rdist < CHUNK_SIZE) ? 6 : 4;

        // Main body
        DrawSphereEx(p, avg * 0.5f, detail, detail, rc);
        // Base
        DrawSphereEx((Vector3){p.x, p.y - avg * 0.15f, p.z}, avg * 0.45f, detail, detail, rcDark);
        // Top lump
        DrawSphereEx((Vector3){p.x + cosf(rot) * avg * 0.15f, p.y + avg * 0.25f,
                     p.z + sinf(rot) * avg * 0.1f}, avg * 0.35f, detail, detail, rcLight);

        // Only draw extra detail spheres when close
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

void WorldDraw(World *world, Vector3 playerPos) {
    int pcx = (int)floorf(playerPos.x / CHUNK_SIZE);
    int pcz = (int)floorf(playerPos.z / CHUNK_SIZE);
    int half = RENDER_CHUNKS / 2;
    for (int i = 0; i < world->chunkCount; i++) {
        Chunk *ch = &world->chunks[i];
        if (!ch->generated) continue;
        int dx = ch->cx - pcx, dz = ch->cz - pcz;
        if (dx >= -half && dx <= half && dz >= -half && dz <= half)
            DrawChunk(world, ch, playerPos);
    }
}

bool WorldCheckCollision(World *world, Vector3 pos, float radius) {
    for (int i = 0; i < world->chunkCount; i++) {
        Chunk *ch = &world->chunks[i];
        if (!ch->generated) continue;
        for (int j = 0; j < ch->rockCount; j++) {
            Rock *r = &ch->rocks[j];
            float dx = fmaxf(fabsf(pos.x - r->position.x) - r->size.x * 0.5f, 0.0f);
            float dz = fmaxf(fabsf(pos.z - r->position.z) - r->size.z * 0.5f, 0.0f);
            if (dx * dx + dz * dz < radius * radius) return true;
        }
    }
    return false;
}
