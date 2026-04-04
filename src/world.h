#ifndef WORLD_H
#define WORLD_H

#include "raylib.h"
#include "raymath.h"
#include "config.h"

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
float WorldGetHeight(float x, float z);
bool WorldCheckCollision(World *world, Vector3 pos, float radius);
void WorldUnload(World *world);

#endif
