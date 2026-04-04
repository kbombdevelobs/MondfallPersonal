#ifndef ENEMY_H
#define ENEMY_H

#include "raylib.h"
#include "raymath.h"
#include "config.h"

typedef enum { ENEMY_SOVIET, ENEMY_AMERICAN } EnemyType;
typedef enum { ENEMY_ALIVE, ENEMY_DYING, ENEMY_VAPORIZING, ENEMY_DEAD } EnemyState;
typedef enum { ANIM_IDLE, ANIM_WALK, ANIM_SHOOT, ANIM_HIT, ANIM_DEATH } EnemyAnimState;
typedef enum { AI_ADVANCE, AI_STRAFE, AI_SHOOT, AI_DODGE, AI_RETREAT } AIBehavior;

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
    // Ragdoll death
    float spinX, spinY, spinZ;   // angular velocity (degrees/sec)
    float ragdollVelX, ragdollVelZ; // lateral launch velocity
    float ragdollVelY;           // vertical launch
    // Vaporize
    float vaporizeTimer;
    float vaporizeScale;
    // Death style: 0 = ragdoll blowout, 1 = crumple + blood pool
    int deathStyle;
} Enemy;

typedef struct {
    Enemy enemies[MAX_ENEMIES];
    int count;
    float spawnTimer, spawnRate;
    Model mdlVisor, mdlArm, mdlBoot;
    Sound sndSovietFire, sndAmericanFire;
    Sound sndSovietDeath[2];
    int sovietDeathCount;
    int sovietDeathPlays;
    Sound sndAmericanDeath[2];
    int americanDeathCount;
    int americanDeathPlays;
    float radioTransmissionTimer; // > 0 means show "TRANSMISSION" on HUD
    bool modelsLoaded;
} EnemyManager;

void EnemyManagerInit(EnemyManager *em);
void EnemyManagerUnload(EnemyManager *em);
void EnemyManagerUpdate(EnemyManager *em, Vector3 playerPos, float dt);
void EnemyManagerDraw(EnemyManager *em);
void EnemySpawnAroundPlayer(EnemyManager *em, EnemyType type, Vector3 playerPos, float spawnRadius);
void EnemySpawnAt(EnemyManager *em, EnemyType type, Vector3 pos);
int EnemyCheckHit(EnemyManager *em, Ray ray, float maxDist, float *hitDist);
int EnemyCheckSphereHit(EnemyManager *em, Vector3 center, float radius);
void EnemyDamage(EnemyManager *em, int index, float damage);
void EnemyVaporize(EnemyManager *em, int index);
int EnemyCountAlive(EnemyManager *em);
float EnemyCheckPlayerDamage(EnemyManager *em, Vector3 playerPos, float dt);
void EnemyManagerSetSFXVolume(EnemyManager *em, float vol);

#endif
