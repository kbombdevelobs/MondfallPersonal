#include "world.h"
#include "world/world_noise.h"
#include "world/world_draw.h"
#include "rlgl.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>

static World *g_activeWorld = NULL;

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
    Image img = GenImageColor(w, h, BLACK);

    for (int py = 0; py < h; py++) {
        for (int px = 0; px < w; px++) {
            float nx = (float)px / (float)w * 10.0f;
            float ny = (float)py / (float)h * 10.0f;

            float n = ValueNoise(nx, ny) * 0.5f;
            n += ValueNoise(nx * 2.5f, ny * 2.5f) * 0.25f;
            n += ValueNoise(nx * 6.0f, ny * 6.0f) * 0.12f;
            n += ValueNoise(nx * 13.0f, ny * 13.0f) * 0.06f;

            float crater = 0;
            for (int ci = 0; ci < 15; ci++) {
                float ccx = ValueNoise((float)ci * 7.1f, 0.5f) * 10.0f;
                float ccy = ValueNoise(0.5f, (float)ci * 7.1f) * 10.0f;
                float cr = 0.3f + ValueNoise((float)ci * 3.3f, (float)ci * 5.7f) * 0.6f;
                float ddx = nx - ccx;
                float ddy = ny - ccy;
                if (ddx > 5.0f) ddx -= 10.0f;
                if (ddx < -5.0f) ddx += 10.0f;
                if (ddy > 5.0f) ddy -= 10.0f;
                if (ddy < -5.0f) ddy += 10.0f;
                float dd = sqrtf(ddx * ddx + ddy * ddy);
                if (dd < cr) {
                    float t = dd / cr;
                    crater -= (1.0f - t * t) * 0.08f;
                    if (t > 0.75f) crater += 0.04f;
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

// Crater profile with terracing and central peaks for large craters
static float CraterProfile(float t, float r, float depth) {
    // t = 0 at center, 1 at rim edge
    // Raised rim outside
    if (t > 0.85f) {
        float rimT = (t - 0.85f) / 0.15f;
        return depth * 0.35f * (1.0f - rimT * rimT);
    }

    // Base bowl shape
    float bowl = -(1.0f - t * t) * depth * 2.5f;

    // Central peak for large craters (radius > 5)
    if (r > 5.0f && t < 0.15f) {
        float peakT = t / 0.15f;
        bowl += depth * 1.5f * (1.0f - peakT * peakT);
    }

    // Terraced walls for large craters
    if (r > 5.0f && t > 0.3f && t < 0.85f) {
        float wallT = (t - 0.3f) / 0.55f; // 0..1 across wall zone
        float stepF = floorf(wallT * 3.0f) / 3.0f;
        float frac = wallT * 3.0f - floorf(wallT * 3.0f);
        // Flatten each terrace, smooth transition between steps
        float smooth = frac < 0.2f ? frac / 0.2f : 1.0f;
        float terraceT = stepF + smooth / 3.0f;
        float flatBowl = -(1.0f - (0.3f + terraceT * 0.55f) * (0.3f + terraceT * 0.55f)) * depth * 2.5f;
        bowl = LerpF(flatBowl, bowl, 0.5f);
    }

    return bowl;
}

// Global crater height modifier — checks ALL nearby chunks so craters span borders
// Uses min-depth approach to fix overlapping crater artifacts
static float CraterHeight(World *world, float wx, float wz) {
    if (!world) return 0;
    float minDepression = 0.0f;
    float maxRim = 0.0f;
    for (int i = 0; i < world->chunkCount; i++) {
        Chunk *ch = &world->chunks[i];
        if (!ch->generated) continue;
        for (int c = 0; c < ch->craterCount; c++) {
            float dx = wx - ch->craters[c].position.x;
            float dz = wz - ch->craters[c].position.z;
            float dist2 = dx * dx + dz * dz;
            float r = ch->craters[c].radius;
            // Check slightly beyond rim for the raised rim zone
            if (dist2 >= r * r * 1.44f) continue; // 1.2x radius squared
            float dist = sqrtf(dist2);
            float t = dist / r;
            if (t > 1.0f) continue;
            float profile = CraterProfile(t, r, ch->craters[c].depth);
            if (profile < 0 && profile < minDepression) minDepression = profile;
            if (profile > 0 && profile > maxRim) maxRim = profile;
        }
    }
    return (minDepression < -0.1f) ? minDepression : minDepression + maxRim;
}

// Generate chunk mesh — vertices in WORLD SPACE, drawn at origin
static Model GenTerrainMesh(int cx, int cz, Texture2D moonTex, World *world) {
    float baseX = cx * CHUNK_SIZE;
    float baseZ = cz * CHUNK_SIZE;

    Mesh mesh = GenMeshPlane(CHUNK_SIZE, CHUNK_SIZE, MESH_RES, MESH_RES);

    int vertCount = mesh.vertexCount;
    for (int i = 0; i < vertCount; i++) {
        float vx = mesh.vertices[i * 3 + 0] + baseX + CHUNK_SIZE * 0.5f;
        float vz = mesh.vertices[i * 3 + 2] + baseZ + CHUNK_SIZE * 0.5f;

        mesh.vertices[i * 3 + 0] = vx;
        mesh.vertices[i * 3 + 2] = vz;
        float craterH = CraterHeight(world, vx, vz);
        mesh.vertices[i * 3 + 1] = WorldGetHeight(vx, vz) + craterH;

        if (mesh.texcoords) {
            float texScale = 1.0f / 30.0f;
            mesh.texcoords[i * 2 + 0] = vx * texScale;
            mesh.texcoords[i * 2 + 1] = vz * texScale;
        }

        // Vertex colors — slope-based + noise for seamless chunks
        if (!mesh.colors) {
            mesh.colors = RL_CALLOC(vertCount * 4, sizeof(unsigned char));
        }
        float hL = WorldGetHeight(vx - 1.0f, vz);
        float hR = WorldGetHeight(vx + 1.0f, vz);
        float hU = WorldGetHeight(vx, vz - 1.0f);
        float hD = WorldGetHeight(vx, vz + 1.0f);
        float slope = fabsf(hR - hL) + fabsf(hD - hU);

        float cn = GradientNoise(vx * 0.08f, vz * 0.08f) * 0.3f
                 + GradientNoise(vx * 0.2f, vz * 0.2f) * 0.15f;
        float craterDarken = (craterH < -0.1f) ? craterH * 0.15f : 0.0f;
        float slopeDarken = slope * 8.0f;
        if (slopeDarken > 35.0f) slopeDarken = 35.0f;

        // Maria darkening — dark basalt seas
        float mare = WorldGetMareFactor(vx, vz);
        float mareDarken = mare * 25.0f;

        // Ejecta rays — bright radial streaks from large craters
        float ejectaBright = 0.0f;
        for (int ci = 0; ci < world->chunkCount; ci++) {
            Chunk *ech = &world->chunks[ci];
            if (!ech->generated) continue;
            for (int cc = 0; cc < ech->craterCount; cc++) {
                Crater *cr = &ech->craters[cc];
                if (cr->radius < 4.5f) continue; // only big craters
                float cdx = vx - cr->position.x;
                float cdz = vz - cr->position.z;
                float cdist = sqrtf(cdx * cdx + cdz * cdz);
                if (cdist < cr->radius || cdist > cr->radius * 4.0f) continue;
                float angle = atan2f(cdz, cdx);
                float rayPattern = cosf(angle * 8.0f);
                rayPattern = rayPattern * rayPattern * rayPattern * rayPattern; // sharp rays
                float falloff = 1.0f - (cdist / (cr->radius * 4.0f));
                float bright = rayPattern * falloff * 0.3f;
                if (bright > ejectaBright) ejectaBright = bright;
            }
        }

        int shade = (int)(145.0f + cn * 60.0f + craterDarken * 80.0f - slopeDarken - mareDarken + ejectaBright * 55.0f);
        if (shade < 60) shade = 60;
        if (shade > 200) shade = 200;
        // Analytical normal from height differences — continuous across chunk borders
        float anx = hL - hR;
        float any = 2.0f;
        float anz = hU - hD;
        float anLen = sqrtf(anx*anx + any*any + anz*anz);
        anx /= anLen; any /= anLen; anz /= anLen;

        // Store analytical normal in mesh
        mesh.normals[i * 3 + 0] = anx;
        mesh.normals[i * 3 + 1] = any;
        mesh.normals[i * 3 + 2] = anz;

        // Bake directional sunlight using analytical normal
        // Pre-normalized sun direction {0.3, 0.6, -0.4} / len
        float ndotl = anx * 0.3922f + any * 0.7845f + anz * -0.5222f;
        if (ndotl < 0.15f) ndotl = 0.15f;
        if (ndotl > 1.0f) ndotl = 1.0f;

        int sr = (int)(shade * ndotl);
        int sg = (int)(shade * ndotl);
        int sb = (int)((shade > 5 ? shade - 5 : 0) * ndotl);
        mesh.colors[i * 4 + 0] = (unsigned char)sr;
        mesh.colors[i * 4 + 1] = (unsigned char)sg;
        mesh.colors[i * 4 + 2] = (unsigned char)sb;
        mesh.colors[i * 4 + 3] = 255;
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

    chunk->craterCount = 1 + (int)(seed % MAX_CHUNK_CRATERS);
    for (int i = 0; i < chunk->craterCount; i++) {
        float rad = SeededFloat(&seed, 2.0f, 7.0f);
        float margin = rad + 2.0f;
        float px = baseX + SeededFloat(&seed, margin, CHUNK_SIZE - margin);
        float pz = baseZ + SeededFloat(&seed, margin, CHUNK_SIZE - margin);
        chunk->craters[i].position = (Vector3){px, WorldGetHeight(px, pz), pz};
        chunk->craters[i].radius = rad;
        chunk->craters[i].depth = SeededFloat(&seed, 0.2f, 0.8f);
    }

    chunk->rockCount = 3 + (int)(seed % (MAX_CHUNK_ROCKS - 2));
    for (int i = 0; i < chunk->rockCount; i++) {
        float s = SeededFloat(&seed, 1.0f, 5.0f);
        float rx = baseX + SeededFloat(&seed, 3.0f, CHUNK_SIZE - 3.0f);
        float rz = baseZ + SeededFloat(&seed, 3.0f, CHUNK_SIZE - 3.0f);
        chunk->rocks[i].position = (Vector3){rx, WorldGetHeight(rx, rz) + s * 0.35f, rz};
        chunk->rocks[i].size = (Vector3){
            s * SeededFloat(&seed, 0.8f, 1.5f),
            s * SeededFloat(&seed, 0.6f, 1.8f),
            s * SeededFloat(&seed, 0.8f, 1.5f)
        };
        chunk->rocks[i].rotation = SeededFloat(&seed, 0, 360.0f);
        int shade = 70 + (int)(SeededFloat(&seed, 0, 1) * 55);
        chunk->rocks[i].color = (Color){(unsigned char)shade, (unsigned char)shade, (unsigned char)(shade - 8), 255};
    }

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

    for (int ring = 0; ring <= half && generated < MAX_CHUNKS_PER_FRAME; ring++) {
        for (int dz = -ring; dz <= ring && generated < MAX_CHUNKS_PER_FRAME; dz++) {
            for (int dx = -ring; dx <= ring && generated < MAX_CHUNKS_PER_FRAME; dx++) {
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

bool WorldCheckCollision(World *world, Vector3 pos, float radius) {
    if (!world) return false;
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
