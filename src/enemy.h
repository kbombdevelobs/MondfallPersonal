#ifndef ENEMY_H
#define ENEMY_H

#include "raylib.h"
#include "raymath.h"

#define MAX_ENEMIES 50

typedef enum { ENEMY_SOVIET, ENEMY_AMERICAN } EnemyType;
typedef enum { ENEMY_ALIVE, ENEMY_DYING, ENEMY_DEAD } EnemyState;
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
    float vertVel;        // vertical velocity for jumping
    float jumpTimer;      // cooldown between jumps
} Enemy;

typedef struct {
    Enemy enemies[MAX_ENEMIES];
    int count;
    float spawnTimer, spawnRate;
    Model mdlVisor, mdlArm, mdlBoot;
    Sound sndSovietFire, sndAmericanFire;
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
int EnemyCountAlive(EnemyManager *em);
float EnemyCheckPlayerDamage(EnemyManager *em, Vector3 playerPos, float dt);

#endif
