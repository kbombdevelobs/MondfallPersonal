#ifndef ENEMY_H
#define ENEMY_H

#include "raylib.h"
#include "raymath.h"
#include "config.h"
#include "enemy_components.h"

// Legacy EnemyState — used by Enemy struct for draw code compatibility
typedef enum { ENEMY_ALIVE, ENEMY_DYING, ENEMY_VAPORIZING, ENEMY_EVISCERATING, ENEMY_DEAD } EnemyState;

// Legacy Enemy struct — kept for DrawAstronautModel compatibility
// The ECS components are the canonical data source; this struct is used
// only as a temporary fill target for rendering.
typedef struct {
    Vector3 position;
    Vector3 velocity;
    EnemyType type;
    EnemyState state;
    EnemyAnimState animState;
    AIBehavior behavior;
    float health, maxHealth, speed, damage;
    float attackRange, attackTimer, attackRate;
    float deathTimer;
    bool active;
    float facingAngle, walkCycle, shootAnim, hitFlash, deathAngle, muzzleFlash;
    float strafeDir, strafeTimer, dodgeTimer, dodgeCooldown;
    float behaviorTimer, burstShots, burstCooldown, preferredDist;
    float vertVel;
    float jumpTimer;
    float spinX, spinY, spinZ;
    float ragdollVelX, ragdollVelZ;
    float ragdollVelY;
    float vaporizeTimer;
    float vaporizeScale;
    float evisTimer;
    Vector3 evisDir;
    float evisLimbSpread;
    Vector3 evisLimbVel[6];
    Vector3 evisLimbPos[6];
    float evisBloodTimer[6];
    int deathStyle;
} Enemy;

// Legacy EnemyManager struct — kept for DrawAstronautModel compatibility
// Only the model fields are used (mdlVisor, mdlArm, mdlBoot).
typedef struct {
    Enemy *enemies;
    int capacity;
    int count;
    bool testMode;
    int aiFrameCounter;
    float spawnTimer, spawnRate;
    Model mdlVisor, mdlArm, mdlBoot;
    Sound sndSovietFire, sndAmericanFire;
    Sound sndSovietDeath[2];
    int sovietDeathCount;
    int sovietDeathPlays;
    Sound sndAmericanDeath[2];
    int americanDeathCount;
    int americanDeathPlays;
    float radioTransmissionTimer;
    bool modelsLoaded;
} EnemyManager;

// Draw functions (still take legacy struct pointers)
#include "enemy_draw.h"

#endif
