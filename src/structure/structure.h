#ifndef STRUCTURE_H
#define STRUCTURE_H

#include "raylib.h"
#include "raymath.h"
#include "config.h"
#include "player.h"
#include "weapon.h"
#include "pickup.h"

typedef enum {
    STRUCTURE_MOON_BASE
} StructureType;

typedef enum {
    PROMPT_NONE,
    PROMPT_ENTER,
    PROMPT_EXIT,
    PROMPT_RESUPPLY,
    PROMPT_EMPTY,
    PROMPT_REFUEL,
    PROMPT_REFUEL_EMPTY
} StructurePrompt;

typedef struct {
    Vector3 worldPos;       // exterior position (center of structure on terrain)
    StructureType type;
    float interiorY;        // Y offset for this structure's interior space
    int doorCount;
    Vector3 doorWorldPos[MAX_STRUCTURE_DOORS];   // exterior door positions
    float doorAngles[MAX_STRUCTURE_DOORS];        // door facing angles (radians)
    int resuppliesLeft;     // number of resupply uses remaining
    int he3RefillsLeft;     // number of He-3 refill uses remaining (interior tank)
    bool active;
} Structure;

typedef struct {
    Structure structures[MAX_STRUCTURES];
    int count;

    // Player-inside state
    bool playerInside;
    int insideIndex;            // which structure player is inside (-1 if none)
    int enteredDoor;            // which door was used to enter
    Vector3 savedWorldPos;      // player position before entering
    float savedYaw;             // player yaw before entering

    // Interior interaction
    StructurePrompt currentPrompt;
    float resupplyFlashTimer;   // visual feedback on resupply
    float emptyMessageTimer;    // "cupboard is bare" message duration
} StructureManager;

void StructureManagerInit(StructureManager *sm);
StructureManager *StructureGetActive(void);
void StructureManagerUpdate(StructureManager *sm, Player *player, Weapon *weapon, PickupManager *pickups);
void StructureManagerUnload(StructureManager *sm);
bool StructureIsPlayerInside(StructureManager *sm);
StructurePrompt StructureGetPrompt(StructureManager *sm);
float StructureGetResupplyFlash(StructureManager *sm);
float StructureGetEmptyMessageTimer(StructureManager *sm);
bool StructureCurrentBaseExpended(StructureManager *sm);

// Procedural spawning — call each frame with player position to discover new bases
void StructureManagerCheckSpawns(StructureManager *sm, Vector3 playerPos);

// Exterior collision — blocks enemies and player from walking through structures
bool StructureCheckCollision(StructureManager *sm, Vector3 pos, float radius);

// Interior collision — keeps player inside the room
bool StructureInteriorCollision(StructureManager *sm, Vector3 pos, float radius);
float StructureInteriorFloorY(StructureManager *sm);

#endif
