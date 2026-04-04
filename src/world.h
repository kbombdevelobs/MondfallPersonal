#ifndef WORLD_H
#define WORLD_H

#include "raylib.h"
#include "raymath.h"

#define CHUNK_SIZE 60.0f
#define RENDER_CHUNKS 5
#define MESH_RES 12
#define MAX_CHUNK_CRATERS 5
#define MAX_CHUNK_ROCKS 10
#define MAX_CHUNKS_PER_FRAME 3
#define MAX_STARS 300

typedef struct {
    Vector3 position;
    float radius;
    float depth;
} Crater;

typedef struct {
    Vector3 position;
    Vector3 size;
    Color color;
    float rotation;
} Rock;

typedef struct {
    int cx, cz;
    Crater craters[MAX_CHUNK_CRATERS];
    int craterCount;
    Rock rocks[MAX_CHUNK_ROCKS];
    int rockCount;
    Model ground;
    bool generated;
} Chunk;

typedef struct {
    Vector3 position;
} Star;

#define MAX_CACHED_CHUNKS (RENDER_CHUNKS * RENDER_CHUNKS)

typedef struct {
    Chunk chunks[MAX_CACHED_CHUNKS];
    int chunkCount;
    Star stars[MAX_STARS];
    int starCount;
    Texture2D moonTex;
    Texture2D rockTex;
    Model rockModels[3]; // large, medium, small
    int rockModelCount;
    bool texturesLoaded;
} World;

void WorldInit(World *world);
World *WorldGetActive(void);
void WorldUpdate(World *world, Vector3 playerPos);
void WorldDraw(World *world, Vector3 playerPos);
void WorldDrawSky(World *world, Camera3D camera);
float WorldGetHeight(float x, float z);
bool WorldCheckCollision(World *world, Vector3 pos, float radius);
void WorldUnload(World *world);

#endif
