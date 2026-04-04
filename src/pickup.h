#ifndef PICKUP_H
#define PICKUP_H

#include "raylib.h"
#include "raymath.h"
#include "config.h"
#include "enemy.h"

typedef struct {
    Vector3 position;
    EnemyType gunType;   // ENEMY_SOVIET = PPSh, ENEMY_AMERICAN = ray gun
    float life;          // despawn timer (counts down from 5)
    bool active;
    float bobTimer;
} DroppedGun;

typedef struct {
    DroppedGun guns[MAX_PICKUPS];
    // Player's picked-up enemy weapon
    bool hasPickup;
    EnemyType pickupType;
    int pickupAmmo;
    float pickupFireRate;
    float pickupTimer;
    float pickupDamage;
    float pickupMuzzleFlash;
    float pickupRecoil;
    Sound sndSovietFire;
    Sound sndAmericanFire;
    bool soundLoaded;
} PickupManager;

void PickupManagerInit(PickupManager *pm);
void PickupDrop(PickupManager *pm, Vector3 pos, EnemyType type);
void PickupManagerUpdate(PickupManager *pm, Vector3 playerPos, float dt);
void PickupManagerDraw(PickupManager *pm);
bool PickupTryGrab(PickupManager *pm, Vector3 playerPos);
bool PickupFire(PickupManager *pm, Vector3 origin, Vector3 dir);
void PickupDrawFirstPerson(PickupManager *pm, Camera3D camera, float weaponBobTimer);
void PickupManagerSetSFXVolume(PickupManager *pm, float vol);

#endif
