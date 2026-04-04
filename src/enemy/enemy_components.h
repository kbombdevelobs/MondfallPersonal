#ifndef ENEMY_COMPONENTS_H
#define ENEMY_COMPONENTS_H

#include "raylib.h"
#include "raymath.h"
#include "config.h"
#include "flecs.h"

// ============================================================================
// Enums — canonical definitions, shared by old and new systems
// ============================================================================

typedef enum { ENEMY_SOVIET, ENEMY_AMERICAN } EnemyType;
typedef enum { ANIM_IDLE, ANIM_WALK, ANIM_SHOOT, ANIM_HIT, ANIM_DEATH } EnemyAnimState;
typedef enum { AI_ADVANCE, AI_STRAFE, AI_SHOOT, AI_DODGE, AI_RETREAT, AI_FLEE } AIBehavior;
typedef enum { RANK_TROOPER, RANK_NCO, RANK_OFFICER } EnemyRank;

// ============================================================================
// Core Components — present on every enemy entity
// ============================================================================

typedef struct {
    Vector3 position;
    float facingAngle;
} EcTransform;

typedef struct {
    Vector3 velocity;
    float vertVel;
    float jumpTimer;
} EcVelocity;

typedef struct {
    EnemyType type;
} EcFaction;

// Tag: entity is alive and participating in AI/combat
// (no data — presence on entity indicates alive status)
typedef struct { char dummy; } EcAlive;

typedef struct {
    float health;
    float maxHealth;
    float speed;
    float damage;
    float attackRange;
    float attackRate;
    float preferredDist;
} EcCombatStats;

typedef struct {
    AIBehavior behavior;
    float behaviorTimer;
    float strafeDir;
    float strafeTimer;
    float dodgeTimer;
    float dodgeCooldown;
    float burstShots;
    float burstCooldown;
    float attackTimer;
} EcAIState;

typedef struct {
    EnemyAnimState animState;
    float walkCycle;
    float shootAnim;
    float hitFlash;
    float muzzleFlash;
    float deathAngle;
} EcAnimation;

// Rank component — determines enhanced stats and visual distinction
typedef struct {
    EnemyRank rank;
} EcRank;

// Morale component — tracks cohesion state
typedef struct {
    float morale;            // 0.0 (broken) to 1.0 (full)
    ecs_entity_t leader;     // nearest officer/NCO entity (0 if none)
    float leaderDist;        // distance to leader
    float fleeTimer;         // time spent fleeing (for recovery)
    AIBehavior prevBehavior; // behavior before fleeing (to restore on rally)
} EcMorale;

// ============================================================================
// Sparse Components — only added when an enemy enters a death state
// ============================================================================

// Tag: entity is in ragdoll/crumple dying state
typedef struct { char dummy; } EcDying;

// Tag: entity is being vaporized
typedef struct { char dummy; } EcVaporizing;

// Tag: entity is being eviscerated
typedef struct { char dummy; } EcEviscerating;

typedef struct {
    float spinX, spinY, spinZ;
    float ragdollVelX, ragdollVelZ;
    float ragdollVelY;
    float deathTimer;
    int deathStyle;  // 0 = ragdoll blowout, 1 = crumple
} EcRagdollDeath;

typedef struct {
    float vaporizeTimer;
    float vaporizeScale;
    float deathTimer;
    int deathStyle;  // 0 = normal, 1 = swell+pop (15%)
} EcVaporizeDeath;

typedef struct {
    float evisTimer;
    Vector3 evisDir;
    Vector3 evisLimbVel[6];
    Vector3 evisLimbPos[6];
    float evisBloodTimer[6];
    float deathTimer;
} EcEviscerateDeath;

// ============================================================================
// Singleton Components — one instance in the world
// ============================================================================

// Shared enemy resources (models, sounds) — singleton on a resource entity
typedef struct {
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
} EcEnemyResources;

// Game context passed to systems
typedef struct {
    Vector3 playerPos;
    Camera3D camera;
    bool testMode;
    int aiFrameCounter;
    float playerDamageAccum;  // accumulated damage to player this frame
    int killCount;            // kills this frame (added to game.killCount)
    int rankKillType;         // 0=none, 1=NCO killed, 2=officer killed (for HUD flash)
} EcGameContext;

// ============================================================================
// Component Registration
// ============================================================================

// Call this after EcsWorldInit() to register all components
void EcsEnemyComponentsRegister(ecs_world_t *world);

// Access component IDs (set during registration)
extern ecs_entity_t ecs_id(EcTransform);
extern ecs_entity_t ecs_id(EcVelocity);
extern ecs_entity_t ecs_id(EcFaction);
extern ecs_entity_t ecs_id(EcAlive);
extern ecs_entity_t ecs_id(EcCombatStats);
extern ecs_entity_t ecs_id(EcAIState);
extern ecs_entity_t ecs_id(EcAnimation);
extern ecs_entity_t ecs_id(EcDying);
extern ecs_entity_t ecs_id(EcVaporizing);
extern ecs_entity_t ecs_id(EcEviscerating);
extern ecs_entity_t ecs_id(EcRagdollDeath);
extern ecs_entity_t ecs_id(EcVaporizeDeath);
extern ecs_entity_t ecs_id(EcEviscerateDeath);
extern ecs_entity_t ecs_id(EcRank);
extern ecs_entity_t ecs_id(EcMorale);
extern ecs_entity_t ecs_id(EcEnemyResources);
extern ecs_entity_t ecs_id(EcGameContext);

// Prefab entities
extern ecs_entity_t g_sovietPrefab;
extern ecs_entity_t g_americanPrefab;

#endif
