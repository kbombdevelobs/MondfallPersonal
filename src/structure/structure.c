#include "structure.h"
#include "world.h"
#include <math.h>
#include <string.h>

static StructureManager *g_activeStructures = NULL;
StructureManager *StructureGetActive(void) { return g_activeStructures; }

// Interior door positions — 3 doors evenly spaced around room perimeter
// Door 0: south wall center (Z = -halfD)
// Door 1: east wall, toward north (X = +halfW)
// Door 2: west wall, toward north (X = -halfW)
#define CLOSET_X         0.0f
#define CLOSET_Z_OFFSET  (MOONBASE_INTERIOR_D * 0.5f - 1.0f)
#define CLOSET_INTERACT_RANGE 3.5f
#define DOOR_HW          1.5f

// Deterministic hash for chunk coordinates (same as world.c)
static unsigned int StructChunkHash(int cx, int cz) {
    unsigned int h = (unsigned int)(cx * 73856093) ^ (unsigned int)(cz * 19349663);
    h ^= h >> 16; h *= 0x45d9f3b; h ^= h >> 16;
    return h;
}

static void GetInteriorDoorPos(int doorIdx, float *outX, float *outZ) {
    float halfW = MOONBASE_INTERIOR_W * 0.5f;
    float halfD = MOONBASE_INTERIOR_D * 0.5f;
    switch (doorIdx) {
        case 0: *outX = 0.0f;         *outZ = -halfD + 1.0f; break;
        case 1: *outX = halfW - 1.0f;  *outZ = 2.0f;          break;
        case 2: *outX = -halfW + 1.0f; *outZ = 2.0f;          break;
        default: *outX = 0; *outZ = 0; break;
    }
}

// Set up a single structure at a world XZ position
static void InitStructureAtPos(Structure *base, float wx, float wz, float interiorY) {
    base->type = STRUCTURE_MOON_BASE;
    base->worldPos = (Vector3){wx, 0.0f, wz};

    // Average terrain height under footprint
    float r = MOONBASE_EXTERIOR_RADIUS;
    float sumH = WorldGetHeight(wx, wz);
    int samples = 1;
    for (int i = 0; i < 8; i++) {
        float a = (float)i / 8.0f * 2.0f * PI;
        sumH += WorldGetHeight(wx + cosf(a) * r, wz + sinf(a) * r);
        samples++;
    }
    base->worldPos.y = sumH / (float)samples;

    base->interiorY = interiorY;
    base->resuppliesLeft = MOONBASE_RESUPPLIES;
    base->active = true;

    // 3 doors at 120-degree intervals
    float airlockLen = 2.8f;
    base->doorCount = MOONBASE_DOOR_COUNT;
    for (int i = 0; i < base->doorCount; i++) {
        float angle = (float)i * (2.0f * PI / (float)base->doorCount) - PI * 0.5f;
        base->doorAngles[i] = angle;
        float baseYOff = 1.2f;
        float doorDist = r * 1.1f + airlockLen;
        float doorX = wx + cosf(angle) * doorDist;
        float doorZ = wz + sinf(angle) * doorDist;
        base->doorWorldPos[i] = (Vector3){doorX, base->worldPos.y + baseYOff + 1.0f, doorZ};
    }
}

// Check if a structure already exists near a world position
static bool StructureNearby(StructureManager *sm, float wx, float wz, float minDist) {
    for (int i = 0; i < sm->count; i++) {
        if (!sm->structures[i].active) continue;
        float dx = sm->structures[i].worldPos.x - wx;
        float dz = sm->structures[i].worldPos.z - wz;
        if (dx * dx + dz * dz < minDist * minDist) return true;
    }
    return false;
}

void StructureManagerInit(StructureManager *sm) {
    memset(sm, 0, sizeof(StructureManager));
    g_activeStructures = sm;
    sm->insideIndex = -1;
    sm->enteredDoor = 0;

    // Guaranteed base near spawn
    InitStructureAtPos(&sm->structures[0], 30.0f, 30.0f, STRUCTURE_INTERIOR_Y);
    sm->count = 1;
}

void StructureManagerCheckSpawns(StructureManager *sm, Vector3 playerPos) {
    if (sm->count >= MAX_STRUCTURES) return;

    int pcx = (int)floorf(playerPos.x / CHUNK_SIZE);
    int pcz = (int)floorf(playerPos.z / CHUNK_SIZE);
    int half = RENDER_CHUNKS / 2;

    for (int dz = -half; dz <= half; dz++) {
        for (int dx = -half; dx <= half; dx++) {
            if (sm->count >= MAX_STRUCTURES) return;

            int cx = pcx + dx;
            int cz = pcz + dz;

            // Skip the spawn chunk (0,0) — already has guaranteed base
            if (cx == 0 && cz == 0) continue;

            // Deterministic: does this chunk have a base?
            unsigned int hash = StructChunkHash(cx, cz);
            if ((hash % STRUCTURE_SPAWN_CHANCE) != 0) continue;

            // Place at deterministic position within chunk
            unsigned int seed = hash;
            seed = seed * 1103515245 + 12345;
            float fx = (float)((seed >> 16) & 0x7FFF) / 32767.0f;
            seed = seed * 1103515245 + 12345;
            float fz = (float)((seed >> 16) & 0x7FFF) / 32767.0f;

            float wx = (float)cx * CHUNK_SIZE + 10.0f + fx * (CHUNK_SIZE - 20.0f);
            float wz = (float)cz * CHUNK_SIZE + 10.0f + fz * (CHUNK_SIZE - 20.0f);

            // Don't double-spawn
            if (StructureNearby(sm, wx, wz, CHUNK_SIZE * 0.5f)) continue;

            // Each structure gets a unique interior Y so they don't overlap
            float intY = STRUCTURE_INTERIOR_Y + (float)sm->count * 50.0f;

            InitStructureAtPos(&sm->structures[sm->count], wx, wz, intY);
            sm->count++;
        }
    }
}

void StructureManagerUnload(StructureManager *sm) {
    (void)sm;
}

bool StructureIsPlayerInside(StructureManager *sm) {
    return sm->playerInside;
}

StructurePrompt StructureGetPrompt(StructureManager *sm) {
    return sm->currentPrompt;
}

float StructureGetResupplyFlash(StructureManager *sm) {
    return sm->resupplyFlashTimer;
}

float StructureGetEmptyMessageTimer(StructureManager *sm) {
    return sm->emptyMessageTimer;
}

bool StructureCurrentBaseExpended(StructureManager *sm) {
    if (sm->insideIndex < 0) return false;
    return sm->structures[sm->insideIndex].resuppliesLeft <= 0;
}

float StructureInteriorFloorY(StructureManager *sm) {
    if (sm->insideIndex < 0) return 0;
    return sm->structures[sm->insideIndex].interiorY;
}

bool StructureInteriorCollision(StructureManager *sm, Vector3 pos, float radius) {
    if (sm->insideIndex < 0) return false;
    float halfW = MOONBASE_INTERIOR_W * 0.5f;
    float halfD = MOONBASE_INTERIOR_D * 0.5f;
    if (pos.x - radius < -halfW || pos.x + radius > halfW) return true;
    if (pos.z - radius < -halfD || pos.z + radius > halfD) return true;
    return false;
}

// Exterior collision — circular cylinder around each structure
bool StructureCheckCollision(StructureManager *sm, Vector3 pos, float radius) {
    // Collision radius = dome radius + airlock protrusion
    float collR = MOONBASE_EXTERIOR_RADIUS + 0.5f;
    for (int i = 0; i < sm->count; i++) {
        Structure *s = &sm->structures[i];
        if (!s->active) continue;
        float dx = pos.x - s->worldPos.x;
        float dz = pos.z - s->worldPos.z;
        float dist2 = dx * dx + dz * dz;
        float minDist = collR + radius;
        if (dist2 < minDist * minDist) return true;
    }
    return false;
}

static void TeleportPlayerInside(StructureManager *sm, Player *player, int structIndex, int doorIdx) {
    Structure *s = &sm->structures[structIndex];

    sm->savedWorldPos = player->position;
    sm->savedYaw = player->yaw;
    sm->enteredDoor = doorIdx;

    float dx, dz;
    GetInteriorDoorPos(doorIdx, &dx, &dz);
    float halfW = MOONBASE_INTERIOR_W * 0.5f;
    float halfD = MOONBASE_INTERIOR_D * 0.5f;
    float inwardX = (dx > halfW * 0.5f) ? dx - 2.0f : (dx < -halfW * 0.5f) ? dx + 2.0f : dx;
    float inwardZ = (dz < -halfD * 0.3f) ? dz + 2.0f : dz;

    player->position = (Vector3){inwardX, s->interiorY + PLAYER_HEIGHT, inwardZ};
    player->velocity = (Vector3){0, 0, 0};
    float faceAngle = atan2f(-dz, -dx);
    if (doorIdx == 0) faceAngle = 0;
    player->yaw = faceAngle;
    player->onGround = true;

    sm->playerInside = true;
    sm->insideIndex = structIndex;
}

static void TeleportPlayerOutside(StructureManager *sm, Player *player, int doorIdx) {
    Structure *s = &sm->structures[sm->insideIndex];
    Vector3 doorWorld = s->doorWorldPos[doorIdx];
    float angle = s->doorAngles[doorIdx];
    player->position = (Vector3){
        doorWorld.x + cosf(angle) * 2.0f,
        doorWorld.y,
        doorWorld.z + sinf(angle) * 2.0f
    };
    player->velocity = (Vector3){0, 0, 0};
    player->yaw = angle + PI;
    player->onGround = true;

    float groundH = WorldGetHeight(player->position.x, player->position.z) + PLAYER_HEIGHT;
    if (player->position.y < groundH) player->position.y = groundH;

    sm->playerInside = false;
    sm->insideIndex = -1;
}

static void ResupplyWeapons(Weapon *weapon, PickupManager *pickups) {
    weapon->mp40Mag = weapon->mp40MagSize;
    weapon->mp40Reserve = MP40_RESERVE;
    weapon->raketenMag = weapon->raketenMagSize;
    weapon->raketenReserve = RAKETEN_RESERVE;
    if (pickups->hasPickup) {
        if (pickups->pickupType == ENEMY_SOVIET)
            pickups->pickupAmmo = PICKUP_SOVIET_AMMO;
        else
            pickups->pickupAmmo = PICKUP_AMERICAN_AMMO;
    }
}

void StructureManagerUpdate(StructureManager *sm, Player *player, Weapon *weapon, PickupManager *pickups) {
    sm->currentPrompt = PROMPT_NONE;

    if (sm->resupplyFlashTimer > 0) {
        sm->resupplyFlashTimer -= GetFrameTime();
        if (sm->resupplyFlashTimer < 0) sm->resupplyFlashTimer = 0;
    }
    if (sm->emptyMessageTimer > 0) {
        sm->emptyMessageTimer -= GetFrameTime();
        if (sm->emptyMessageTimer < 0) sm->emptyMessageTimer = 0;
    }

    if (sm->playerInside) {
        Structure *s = &sm->structures[sm->insideIndex];
        float floorY = s->interiorY;

        if (player->position.y <= floorY + PLAYER_HEIGHT) {
            player->position.y = floorY + PLAYER_HEIGHT;
            player->velocity.y = 0;
            player->onGround = true;
        }

        float ceilY = floorY + MOONBASE_INTERIOR_H;
        if (player->position.y > ceilY) {
            player->position.y = ceilY;
            player->velocity.y = 0;
        }

        float halfW = MOONBASE_INTERIOR_W * 0.5f - COLLISION_RADIUS;
        float halfD = MOONBASE_INTERIOR_D * 0.5f - COLLISION_RADIUS;
        if (player->position.x < -halfW) { player->position.x = -halfW; player->velocity.x = 0; }
        if (player->position.x > halfW) { player->position.x = halfW; player->velocity.x = 0; }
        if (player->position.z < -halfD) { player->position.z = -halfD; player->velocity.z = 0; }
        if (player->position.z > halfD) { player->position.z = halfD; player->velocity.z = 0; }

        for (int d = 0; d < s->doorCount; d++) {
            float doorX, doorZ;
            GetInteriorDoorPos(d, &doorX, &doorZ);
            float ddx = player->position.x - doorX;
            float ddz = player->position.z - doorZ;
            float dist = sqrtf(ddx * ddx + ddz * ddz);
            if (dist < STRUCTURE_INTERACT_RANGE) {
                sm->currentPrompt = PROMPT_EXIT;
                if (IsKeyPressed(KEY_E)) {
                    TeleportPlayerOutside(sm, player, d);
                    return;
                }
                break;
            }
        }

        if (sm->currentPrompt == PROMPT_NONE) {
            float closetZ = CLOSET_Z_OFFSET;
            float ddx = player->position.x - CLOSET_X;
            float ddz = player->position.z - closetZ;
            float dist = sqrtf(ddx * ddx + ddz * ddz);
            if (dist < CLOSET_INTERACT_RANGE) {
                Structure *cs = &sm->structures[sm->insideIndex];
                if (cs->resuppliesLeft > 0) {
                    sm->currentPrompt = PROMPT_RESUPPLY;
                    if (IsKeyPressed(KEY_E)) {
                        ResupplyWeapons(weapon, pickups);
                        cs->resuppliesLeft--;
                        sm->resupplyFlashTimer = 0.5f;
                    }
                } else {
                    sm->currentPrompt = PROMPT_EMPTY;
                    if (IsKeyPressed(KEY_E)) {
                        sm->emptyMessageTimer = 3.0f;
                    }
                }
            }
        }
    } else {
        for (int i = 0; i < sm->count; i++) {
            Structure *s = &sm->structures[i];
            if (!s->active) continue;
            for (int d = 0; d < s->doorCount; d++) {
                float dx = player->position.x - s->doorWorldPos[d].x;
                float dz = player->position.z - s->doorWorldPos[d].z;
                float dist = sqrtf(dx * dx + dz * dz);
                if (dist < STRUCTURE_INTERACT_RANGE) {
                    sm->currentPrompt = PROMPT_ENTER;
                    if (IsKeyPressed(KEY_E)) {
                        TeleportPlayerInside(sm, player, i, d);
                        return;
                    }
                }
            }
        }
    }
}
