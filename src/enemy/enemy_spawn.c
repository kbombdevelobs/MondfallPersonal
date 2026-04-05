#include "enemy_spawn.h"
#include "enemy_components.h"
#include "world.h"
#include "config.h"
#include <stdlib.h>
#include <math.h>

// ============================================================================
// Formation offset — spread enemies at spawn instead of bunching
// ============================================================================
Vector3 FormationOffset(int index, int total, EnemyType faction, EnemyRank rank) {
    if (total <= 1) return (Vector3){0, 0, 0};

    // Officers always at rear center
    if (rank == RANK_OFFICER) {
        return (Vector3){0, 0, -4.0f};
    }
    // NCOs at front/middle
    if (rank == RANK_NCO) {
        return (faction == ENEMY_SOVIET)
            ? (Vector3){0, 0, 2.0f}   // Soviet NCO leads charge
            : (Vector3){2.0f, 0, 0};  // American NCO on flank
    }

    if (faction == ENEMY_SOVIET) {
        // Soviet wedge: fan out from center, deeper ranks further back
        float t = (float)index / (float)(total > 1 ? total - 1 : 1);
        float angle = (t - 0.5f) * 1.2f; // ~70 degree spread
        float depth = (float)index * 1.5f;
        return (Vector3){sinf(angle) * depth * 2.0f, 0, -depth * 0.5f};
    } else {
        // American staggered pairs: fire team formation
        int pair = index / 2;
        int side = (index % 2) ? 1 : -1;
        return (Vector3){(float)side * 3.0f, 0, -(float)pair * 4.0f};
    }
}

// ============================================================================
// Core spawn — all parameters
// ============================================================================
static ecs_entity_t SpawnEnemyInternal(ecs_world_t *world, EnemyType type, EnemyRank rank, Vector3 pos, int squadId) {
    pos.y = WorldGetHeight(pos.x, pos.z) + ENEMY_GROUND_OFFSET;

    ecs_entity_t e = ecs_new(world);

    ecs_set(world, e, EcTransform, {
        .position = pos,
        .facingAngle = 0
    });

    ecs_set(world, e, EcVelocity, {
        .velocity = (Vector3){0, 0, 0},
        .vertVel = 0,
        .jumpTimer = 0
    });

    ecs_set(world, e, EcFaction, { .type = type });
    ecs_set(world, e, EcAlive, { .dummy = 0 });
    ecs_set(world, e, EcRank, { .rank = rank });

    // Rank multipliers
    float hMult = 1.0f, dMult = 1.0f, sMult = 1.0f, rngMult = 1.0f, rateMult = 1.0f, distMult = 1.0f;
    if (rank == RANK_NCO) {
        hMult = NCO_HEALTH_MULT; dMult = NCO_DAMAGE_MULT; sMult = NCO_SPEED_MULT;
    } else if (rank == RANK_OFFICER) {
        hMult = OFFICER_HEALTH_MULT; dMult = OFFICER_DAMAGE_MULT;
        sMult = OFFICER_SPEED_MULT; rngMult = OFFICER_RANGE_MULT;
        rateMult = OFFICER_RATE_MULT; distMult = OFFICER_DIST_MULT;
    }

    float strafeDir = (rand() % 2) ? 1.0f : -1.0f;
    float strafeTimer = AI_STRAFE_TIMER_BASE + (float)rand() / RAND_MAX * AI_STRAFE_TIMER_RAND;
    float dodgeCd = AI_DODGE_COOLDOWN;
    if (rank == RANK_OFFICER) dodgeCd *= 0.5f; // officers dodge more frequently

    if (type == ENEMY_SOVIET) {
        float hp = SOVIET_HEALTH * hMult;
        ecs_set(world, e, EcCombatStats, {
            .health = hp, .maxHealth = hp,
            .speed = SOVIET_SPEED * sMult, .damage = SOVIET_DAMAGE * dMult,
            .attackRange = SOVIET_ATTACK_RANGE * rngMult,
            .attackRate = SOVIET_ATTACK_RATE * rateMult,
            .preferredDist = SOVIET_PREFERRED_DIST * distMult
        });
        ecs_set(world, e, EcAIState, {
            .behavior = AI_ADVANCE,
            .behaviorTimer = 0,
            .strafeDir = strafeDir,
            .strafeTimer = strafeTimer,
            .dodgeTimer = 0,
            .dodgeCooldown = dodgeCd,
            .burstShots = 0,
            .burstCooldown = 0,
            .attackTimer = 0
        });
    } else {
        float hp = AMERICAN_HEALTH * hMult;
        ecs_set(world, e, EcCombatStats, {
            .health = hp, .maxHealth = hp,
            .speed = AMERICAN_SPEED * sMult, .damage = AMERICAN_DAMAGE * dMult,
            .attackRange = AMERICAN_ATTACK_RANGE * rngMult,
            .attackRate = AMERICAN_ATTACK_RATE * rateMult,
            .preferredDist = AMERICAN_PREFERRED_DIST * distMult
        });
        ecs_set(world, e, EcAIState, {
            .behavior = AI_STRAFE,
            .behaviorTimer = 0,
            .strafeDir = strafeDir,
            .strafeTimer = strafeTimer,
            .dodgeTimer = 0,
            .dodgeCooldown = dodgeCd,
            .burstShots = 0,
            .burstCooldown = 0,
            .attackTimer = 0
        });
    }

    // Morale: officers/NCOs start full and never flee; troopers start full
    AIBehavior defaultBeh = (type == ENEMY_SOVIET) ? AI_ADVANCE : AI_STRAFE;
    ecs_set(world, e, EcMorale, {
        .morale = 1.0f,
        .leader = 0,
        .leaderDist = 999.0f,
        .fleeTimer = 0,
        .prevBehavior = defaultBeh
    });

    ecs_set(world, e, EcAnimation, {
        .animState = ANIM_IDLE,
        .walkCycle = 0,
        .shootAnim = 0,
        .hitFlash = 0,
        .muzzleFlash = 0,
        .deathAngle = 0
    });

    // Steering: momentum-based movement (rank-dependent acceleration)
    float accel = AI_ACCEL_TROOPER;
    if (rank == RANK_NCO) accel = AI_ACCEL_NCO;
    else if (rank == RANK_OFFICER) accel = AI_ACCEL_OFFICER;
    ecs_set(world, e, EcSteering, {
        .desiredVelocity = (Vector3){0, 0, 0},
        .acceleration = accel
    });

    // Squad assignment
    ecs_set(world, e, EcSquad, {
        .squadId = squadId,
        .anchor = 0
    });

    return e;
}

ecs_entity_t EcsEnemySpawnRanked(ecs_world_t *world, EnemyType type, EnemyRank rank, Vector3 pos) {
    return SpawnEnemyInternal(world, type, rank, pos, -1);
}

ecs_entity_t EcsEnemySpawnSquad(ecs_world_t *world, EnemyType type, EnemyRank rank, Vector3 pos, int squadId) {
    return SpawnEnemyInternal(world, type, rank, pos, squadId);
}

ecs_entity_t EcsEnemySpawnAt(ecs_world_t *world, EnemyType type, Vector3 pos) {
    return SpawnEnemyInternal(world, type, RANK_TROOPER, pos, -1);
}

ecs_entity_t EcsEnemySpawnAroundPlayer(ecs_world_t *world, EnemyType type, Vector3 playerPos, float spawnRadius) {
    float angle = ((float)rand() / RAND_MAX) * 2.0f * PI;
    float dist = spawnRadius + ((float)rand() / RAND_MAX) * 20.0f;
    Vector3 pos = {
        playerPos.x + cosf(angle) * dist,
        0,
        playerPos.z + sinf(angle) * dist
    };
    return EcsEnemySpawnAt(world, type, pos);
}
