#include "world.h"
#include "rlgl.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>

static World *g_activeWorld = NULL;

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
    // Generate texture using our own ValueNoise — guaranteed tileable
    // because ValueNoise is continuous and deterministic at any coordinate.
    // We sample a 256x256 region of noise space mapped to one texture tile.
    int w = 256, h = 256;
    Image img = GenImageColor(w, h, BLACK);

    for (int py = 0; py < h; py++) {
        for (int px = 0; px < w; px++) {
            // Map pixel to noise space — one tile = 10 noise units
            float nx = (float)px / (float)w * 10.0f;
            float ny = (float)py / (float)h * 10.0f;

            // Multi-octave noise for regolith texture
            float n = ValueNoise(nx, ny) * 0.5f;
            n += ValueNoise(nx * 2.5f, ny * 2.5f) * 0.25f;
            n += ValueNoise(nx * 6.0f, ny * 6.0f) * 0.12f;
            n += ValueNoise(nx * 13.0f, ny * 13.0f) * 0.06f;

            // Crater marks — lots of them, deterministic via noise
            float crater = 0;
            for (int ci = 0; ci < 15; ci++) {
                float ccx = ValueNoise((float)ci * 7.1f, 0.5f) * 10.0f;
                float ccy = ValueNoise(0.5f, (float)ci * 7.1f) * 10.0f;
                float cr = 0.3f + ValueNoise((float)ci * 3.3f, (float)ci * 5.7f) * 0.6f;
                // Wrap distance for tileability
                float ddx = nx - ccx;
                float ddy = ny - ccy;
                // Wrap around the 10-unit tile
                if (ddx > 5.0f) ddx -= 10.0f;
                if (ddx < -5.0f) ddx += 10.0f;
                if (ddy > 5.0f) ddy -= 10.0f;
                if (ddy < -5.0f) ddy += 10.0f;
                float dd = sqrtf(ddx * ddx + ddy * ddy);
                if (dd < cr) {
                    float t = dd / cr;
                    crater -= (1.0f - t * t) * 0.08f; // darken inside
                    if (t > 0.75f) crater += 0.04f;    // bright rim
                }
            }

            int shade = (int)(130.0f + n * 50.0f + crater * 200.0f);
            if (shade < 70) shade = 70;
            if (shade > 190) shade = 190;
            ImageDrawPixel(&img, px, py,
                (Color){(unsigned char)shade, (unsigned char)shade, (unsigned char)(shade - 6), 255});
        }
    }

    Texture2D tex = LoadTextureFromImage(img);
    SetTextureFilter(tex, TEXTURE_FILTER_BILINEAR);
    SetTextureWrap(tex, TEXTURE_WRAP_REPEAT);
    UnloadImage(img);
    return tex;
}

// Global crater height modifier — checks ALL nearby chunks so craters span borders
static float CraterHeight(World *world, float wx, float wz) {
    if (!world) return 0;
    float h = 0;
    for (int i = 0; i < world->chunkCount; i++) {
        Chunk *ch = &world->chunks[i];
        if (!ch->generated) continue;
        for (int c = 0; c < ch->craterCount; c++) {
            float dx = wx - ch->craters[c].position.x;
            float dz = wz - ch->craters[c].position.z;
            float dist = sqrtf(dx * dx + dz * dz);
            float r = ch->craters[c].radius;
            if (dist < r) {
                float t = dist / r;
                h -= (1.0f - t * t) * ch->craters[c].depth * 2.5f;
                if (t > 0.8f) h += ((t - 0.8f) / 0.2f) * ch->craters[c].depth * 0.4f;
            }
        }
    }
    return h;
}

// Generate chunk mesh — vertices in WORLD SPACE, drawn at origin
static Model GenTerrainMesh(int cx, int cz, Texture2D moonTex, World *world) {
    float baseX = cx * CHUNK_SIZE;
    float baseZ = cz * CHUNK_SIZE;

    Mesh mesh = GenMeshPlane(CHUNK_SIZE, CHUNK_SIZE, MESH_RES, MESH_RES);

    int vertCount = mesh.vertexCount;
    for (int i = 0; i < vertCount; i++) {
        // GenMeshPlane verts are centered at origin: [-size/2, +size/2]
        // Offset to world position
        float vx = mesh.vertices[i * 3 + 0] + baseX + CHUNK_SIZE * 0.5f;
        float vz = mesh.vertices[i * 3 + 2] + baseZ + CHUNK_SIZE * 0.5f;

        // Store world-space X/Z in the mesh
        mesh.vertices[i * 3 + 0] = vx;
        mesh.vertices[i * 3 + 2] = vz;
        float craterH = CraterHeight(world, vx, vz);
        mesh.vertices[i * 3 + 1] = WorldGetHeight(vx, vz) + craterH;

        // World-space UVs
        if (mesh.texcoords) {
            float texScale = 1.0f / 30.0f;
            mesh.texcoords[i * 2 + 0] = vx * texScale;
            mesh.texcoords[i * 2 + 1] = vz * texScale;
        }

        // Vertex colors — continuous noise-based, guarantees no chunk seams
        if (!mesh.colors) {
            mesh.colors = RL_CALLOC(vertCount * 4, sizeof(unsigned char));
        }
        // Base shade from noise
        float cn = ValueNoise(vx * 0.08f, vz * 0.08f) * 0.3f
                 + ValueNoise(vx * 0.2f, vz * 0.2f) * 0.15f;
        // Darken in craters
        float craterDarken = (craterH < -0.1f) ? craterH * 0.15f : 0;
        int shade = (int)(145.0f + cn * 60.0f + craterDarken * 80.0f);
        if (shade < 80) shade = 80;
        if (shade > 200) shade = 200;
        mesh.colors[i * 4 + 0] = (unsigned char)shade;
        mesh.colors[i * 4 + 1] = (unsigned char)shade;
        mesh.colors[i * 4 + 2] = (unsigned char)(shade - 5);
        mesh.colors[i * 4 + 3] = 255;
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
    if (mesh.texcoords)
        UpdateMeshBuffer(mesh, 1, mesh.texcoords, mesh.vertexCount * 2 * sizeof(float), 0);
    UpdateMeshBuffer(mesh, 2, mesh.normals, mesh.vertexCount * 3 * sizeof(float), 0);
    if (mesh.colors)
        UpdateMeshBuffer(mesh, 3, mesh.colors, mesh.vertexCount * 4 * sizeof(unsigned char), 0);

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
        float rad = SeededFloat(&seed, 2.0f, 7.0f);
        float margin = rad + 2.0f; // keep crater fully inside chunk
        float px = baseX + SeededFloat(&seed, margin, CHUNK_SIZE - margin);
        float pz = baseZ + SeededFloat(&seed, margin, CHUNK_SIZE - margin);
        chunk->craters[i].position = (Vector3){px, WorldGetHeight(px, pz), pz};
        chunk->craters[i].radius = rad;
        chunk->craters[i].depth = SeededFloat(&seed, 0.2f, 0.8f);
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

    // Generate terrain mesh — world-space verts, craters from all chunks
    chunk->ground = GenTerrainMesh(cx, cz, moonTex, g_activeWorld);
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
    // Stars — bigger cubes so they survive pixelation
    for (int i = 0; i < world->starCount; i++) {
        Vector3 sp = world->stars[i].position;
        sp.x += camera.position.x;
        sp.z += camera.position.z;

        if (i % 8 == 0) {
            // Bright star — large, slight color
            DrawCube(sp, 1.5f, 1.5f, 1.5f, (Color){255, 255, 230, 255});
        } else if (i % 3 == 0) {
            // Medium star
            DrawCube(sp, 1.0f, 1.0f, 1.0f, WHITE);
        } else {
            // Dim star — still a cube so it renders
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
    // Use a grid and check distance from center
    int res = 20; // grid resolution across diameter
    float step = R * 2.0f / (float)res;
    for (int gy = 0; gy < res; gy++) {
        for (int gx = 0; gx < res; gx++) {
            float lx = -R + ((float)gx + 0.5f) * step; // local X (-R to R)
            float ly = -R + ((float)gy + 0.5f) * step; // local Y (-R to R)
            float d = sqrtf(lx * lx + ly * ly);
            if (d > R) continue; // outside circle

            // Normalize to -1..1
            float nx = lx / R;
            float ny = ly / R;

            Color ec = {30, 65, 150, 255}; // ocean blue

            // === CONTINENTS — recognizable shapes ===
            // North America (upper-left quadrant)
            if (nx > -0.7f && nx < -0.05f && ny > 0.1f && ny < 0.7f) ec = (Color){50, 130, 55, 255};
            // Central America (thin strip)
            if (nx > -0.35f && nx < -0.15f && ny > -0.05f && ny < 0.15f) ec = (Color){55, 125, 50, 255};
            // South America (lower-left)
            if (nx > -0.4f && nx < -0.05f && ny > -0.7f && ny < -0.05f &&
                fabsf(nx + 0.2f) < 0.2f - ny * 0.1f) ec = (Color){48, 135, 52, 255};
            // Europe (upper-center-right, small)
            if (nx > 0.05f && nx < 0.35f && ny > 0.35f && ny < 0.6f) ec = (Color){55, 128, 48, 255};
            // Africa (center-right, big)
            if (nx > 0.1f && nx < 0.5f && ny > -0.45f && ny < 0.35f &&
                fabsf(nx - 0.3f) < 0.2f - fabsf(ny) * 0.1f) ec = (Color){60, 135, 45, 255};
            // Asia (upper-right, large mass)
            if (nx > 0.3f && nx < 0.85f && ny > 0.15f && ny < 0.7f) ec = (Color){52, 125, 50, 255};
            // Australia (lower-right, small)
            if (nx > 0.45f && nx < 0.7f && ny > -0.55f && ny < -0.3f) ec = (Color){65, 130, 48, 255};

            // Desert areas (Sahara, Arabia)
            if (nx > 0.1f && nx < 0.45f && ny > 0.1f && ny < 0.3f) ec = (Color){170, 155, 100, 255};
            if (nx > 0.35f && nx < 0.55f && ny > 0.05f && ny < 0.2f) ec = (Color){175, 160, 105, 255};

            // Ice caps
            if (ny > 0.8f) ec = (Color){230, 235, 240, 255}; // north pole
            if (ny < -0.82f) ec = (Color){225, 230, 238, 255}; // south pole

            // Cloud wisps — white patches scattered
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
            DrawChunk(ch, playerPos);
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
