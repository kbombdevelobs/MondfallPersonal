#include "structure.h"
#include "world.h"
#include <math.h>
#include <string.h>

// Interior door positions — 3 doors evenly spaced around room perimeter
// Door 0: south wall center (Z = -halfD)
// Door 1: east wall, toward north (X = +halfW)
// Door 2: west wall, toward north (X = -halfW)
#define CLOSET_X         0.0f
#define CLOSET_Z_OFFSET  (MOONBASE_INTERIOR_D * 0.5f - 1.0f)
#define CLOSET_INTERACT_RANGE 3.5f
#define DOOR_HW          1.5f  // door half-width for interaction zone

// Interior door zone positions (XZ relative to interior origin)
static void GetInteriorDoorPos(int doorIdx, float *outX, float *outZ) {
    float halfW = MOONBASE_INTERIOR_W * 0.5f;
    float halfD = MOONBASE_INTERIOR_D * 0.5f;
    switch (doorIdx) {
        case 0: *outX = 0.0f;         *outZ = -halfD + 1.0f; break; // south
        case 1: *outX = halfW - 1.0f;  *outZ = 2.0f;          break; // east
        case 2: *outX = -halfW + 1.0f; *outZ = 2.0f;          break; // west
        default: *outX = 0; *outZ = 0; break;
    }
}

void StructureManagerInit(StructureManager *sm) {
    memset(sm, 0, sizeof(StructureManager));
    sm->insideIndex = -1;
    sm->enteredDoor = 0;

    // Place one moon base near spawn
    Structure *base = &sm->structures[0];
    base->type = STRUCTURE_MOON_BASE;
    base->worldPos = (Vector3){30.0f, 0.0f, 30.0f};

    // Sample terrain around the footprint and use the AVERAGE height
    // This sits the dome naturally on the terrain without sinking into dips
    float r = MOONBASE_EXTERIOR_RADIUS;
    float sumH = WorldGetHeight(base->worldPos.x, base->worldPos.z);
    int samples = 1;
    for (int i = 0; i < 8; i++) {
        float a = (float)i / 8.0f * 2.0f * PI;
        float sx = base->worldPos.x + cosf(a) * r;
        float sz = base->worldPos.z + sinf(a) * r;
        sumH += WorldGetHeight(sx, sz);
        samples++;
    }
    base->worldPos.y = sumH / (float)samples;

    base->interiorY = STRUCTURE_INTERIOR_Y;
    base->active = true;

    // 3 doors at 120-degree intervals around the dome
    // Airlock corridors extend ~2.8 units past dome edge; interaction at corridor end
    float airlockLen = 2.8f;
    base->doorCount = MOONBASE_DOOR_COUNT;
    for (int i = 0; i < base->doorCount; i++) {
        float angle = (float)i * (2.0f * PI / (float)base->doorCount) - PI * 0.5f; // start south
        base->doorAngles[i] = angle;
        // Interaction point at end of airlock corridor
        // baseY = y + 1.2 (cylinder 1.0 above terrain + 0.2 cap) — must match structure_draw.c
        float baseYOff = 1.2f;
        float doorDist = r * 0.95f + airlockLen;
        float doorX = base->worldPos.x + cosf(angle) * doorDist;
        float doorZ = base->worldPos.z + sinf(angle) * doorDist;
        base->doorWorldPos[i] = (Vector3){doorX, base->worldPos.y + baseYOff + 1.0f, doorZ};
    }

    sm->count = 1;
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

static void TeleportPlayerInside(StructureManager *sm, Player *player, int structIndex, int doorIdx) {
    Structure *s = &sm->structures[structIndex];

    sm->savedWorldPos = player->position;
    sm->savedYaw = player->yaw;
    sm->enteredDoor = doorIdx;

    // Spawn just inside the door they entered
    float dx, dz;
    GetInteriorDoorPos(doorIdx, &dx, &dz);
    // Step inward from the door
    float halfW = MOONBASE_INTERIOR_W * 0.5f;
    float halfD = MOONBASE_INTERIOR_D * 0.5f;
    float inwardX = (dx > halfW * 0.5f) ? dx - 2.0f : (dx < -halfW * 0.5f) ? dx + 2.0f : dx;
    float inwardZ = (dz < -halfD * 0.3f) ? dz + 2.0f : dz;

    player->position = (Vector3){inwardX, s->interiorY + PLAYER_HEIGHT, inwardZ};
    player->velocity = (Vector3){0, 0, 0};
    // Face inward
    float faceAngle = atan2f(-dz, -dx); // face toward center
    if (doorIdx == 0) faceAngle = 0; // face north
    player->yaw = faceAngle;
    player->onGround = true;

    sm->playerInside = true;
    sm->insideIndex = structIndex;
}

static void TeleportPlayerOutside(StructureManager *sm, Player *player, int doorIdx) {
    Structure *s = &sm->structures[sm->insideIndex];
    // Place player just outside the door they're exiting through
    Vector3 doorWorld = s->doorWorldPos[doorIdx];
    float angle = s->doorAngles[doorIdx];
    player->position = (Vector3){
        doorWorld.x + cosf(angle) * 2.0f,
        doorWorld.y,
        doorWorld.z + sinf(angle) * 2.0f
    };
    player->velocity = (Vector3){0, 0, 0};
    player->yaw = angle + PI; // face back toward the base
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

    if (sm->playerInside) {
        Structure *s = &sm->structures[sm->insideIndex];
        float floorY = s->interiorY;

        // Floor collision
        if (player->position.y <= floorY + PLAYER_HEIGHT) {
            player->position.y = floorY + PLAYER_HEIGHT;
            player->velocity.y = 0;
            player->onGround = true;
        }

        // Ceiling collision
        float ceilY = floorY + MOONBASE_INTERIOR_H;
        if (player->position.y > ceilY) {
            player->position.y = ceilY;
            player->velocity.y = 0;
        }

        // Wall clamp
        float halfW = MOONBASE_INTERIOR_W * 0.5f - COLLISION_RADIUS;
        float halfD = MOONBASE_INTERIOR_D * 0.5f - COLLISION_RADIUS;
        if (player->position.x < -halfW) { player->position.x = -halfW; player->velocity.x = 0; }
        if (player->position.x > halfW) { player->position.x = halfW; player->velocity.x = 0; }
        if (player->position.z < -halfD) { player->position.z = -halfD; player->velocity.z = 0; }
        if (player->position.z > halfD) { player->position.z = halfD; player->velocity.z = 0; }

        // Check proximity to each interior door
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
                break; // only one prompt at a time
            }
        }

        // Check resupply closet (north wall center)
        if (sm->currentPrompt == PROMPT_NONE) {
            float closetZ = CLOSET_Z_OFFSET;
            float ddx = player->position.x - CLOSET_X;
            float ddz = player->position.z - closetZ;
            float dist = sqrtf(ddx * ddx + ddz * ddz);
            if (dist < CLOSET_INTERACT_RANGE) {
                sm->currentPrompt = PROMPT_RESUPPLY;
                if (IsKeyPressed(KEY_E)) {
                    ResupplyWeapons(weapon, pickups);
                    sm->resupplyFlashTimer = 0.5f;
                }
            }
        }
    } else {
        // Check proximity to any structure door (any of the 3 doors)
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
