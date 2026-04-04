#ifndef PICKUP_H
#define PICKUP_H

#include "raylib.h"
#include "raymath.h"
#include "config.h"
#include "enemy/enemy_components.h"

// Pickup weapon types — faction + rank determines which gun drops
typedef enum {
    PICKUP_KOSMOS7,         // Soviet trooper — KOSMOS-7 SMG
    PICKUP_LIBERTY,         // American trooper — LIBERTY BLASTER
    PICKUP_KS23_MOLOT,      // Soviet NCO — KS-23 Molot (shotgun)
    PICKUP_M8A1_STARHAWK,   // American NCO — M8A1 Starhawk (burst rifle)
    PICKUP_ZARYA_TK4,       // Soviet Officer — Zarya TK-4 (charged pistol)
    PICKUP_ARC9_LONGBOW,    // American Officer — ARC-9 Longbow (piercing beam)
    PICKUP_TYPE_COUNT
} PickupWeaponType;

typedef struct {
    Vector3 position;
    PickupWeaponType weaponType;
    float life;          // despawn timer (counts down from 5)
    bool active;
    float bobTimer;
} DroppedGun;

typedef struct {
    DroppedGun guns[MAX_PICKUPS];
    // Player's picked-up enemy weapon
    bool hasPickup;
    PickupWeaponType pickupType;
    int pickupAmmo;
    float pickupFireRate;
    float pickupTimer;
    float pickupDamage;
    float pickupMuzzleFlash;
    float pickupRecoil;
    // Starhawk burst state
    int burstRemaining;       // rounds left in current burst
    float burstTimer;         // time until next burst round
    // Zarya charge state
    float chargeTime;         // how long fire held (0 = not charging)
    bool charging;
    // Sounds
    Sound sndSovietFire;
    Sound sndAmericanFire;
    Sound sndMolotFire;
    Sound sndStarhawkFire;
    Sound sndZaryaFire;
    Sound sndLongbowFire;
    bool soundLoaded;
} PickupManager;

// Convert faction + rank to pickup weapon type
PickupWeaponType PickupTypeFromRank(EnemyType faction, EnemyRank rank);

void PickupManagerInit(PickupManager *pm);
void PickupDrop(PickupManager *pm, Vector3 pos, PickupWeaponType type);
void PickupManagerUpdate(PickupManager *pm, Vector3 playerPos, float dt);
void PickupManagerDraw(PickupManager *pm);
bool PickupTryGrab(PickupManager *pm, Vector3 playerPos);
bool PickupFire(PickupManager *pm, Vector3 origin, Vector3 dir);
bool PickupFireZaryaRelease(PickupManager *pm);
void PickupDrawFirstPerson(PickupManager *pm, Camera3D camera, float weaponBobTimer);
void PickupManagerSetSFXVolume(PickupManager *pm, float vol);

// Get display name for current pickup weapon
const char *PickupGetName(PickupWeaponType type);

#endif
