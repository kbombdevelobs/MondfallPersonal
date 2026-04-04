#include "enemy_spawn.h"
#include "enemy_components.h"
#include "world.h"
#include "config.h"
#include <stdlib.h>
#include <math.h>

ecs_entity_t EcsEnemySpawnAt(ecs_world_t *world, EnemyType type, Vector3 pos) {
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

    float strafeDir = (rand() % 2) ? 1.0f : -1.0f;
    float strafeTimer = AI_STRAFE_TIMER_BASE + (float)rand() / RAND_MAX * AI_STRAFE_TIMER_RAND;

    if (type == ENEMY_SOVIET) {
        ecs_set(world, e, EcCombatStats, {
            .health = SOVIET_HEALTH, .maxHealth = SOVIET_HEALTH,
            .speed = SOVIET_SPEED, .damage = SOVIET_DAMAGE,
            .attackRange = SOVIET_ATTACK_RANGE, .attackRate = SOVIET_ATTACK_RATE,
            .preferredDist = SOVIET_PREFERRED_DIST
        });
        ecs_set(world, e, EcAIState, {
            .behavior = AI_ADVANCE,
            .behaviorTimer = 0,
            .strafeDir = strafeDir,
            .strafeTimer = strafeTimer,
            .dodgeTimer = 0,
            .dodgeCooldown = AI_DODGE_COOLDOWN,
            .burstShots = 0,
            .burstCooldown = 0,
            .attackTimer = 0
        });
    } else {
        ecs_set(world, e, EcCombatStats, {
            .health = AMERICAN_HEALTH, .maxHealth = AMERICAN_HEALTH,
            .speed = AMERICAN_SPEED, .damage = AMERICAN_DAMAGE,
            .attackRange = AMERICAN_ATTACK_RANGE, .attackRate = AMERICAN_ATTACK_RATE,
            .preferredDist = AMERICAN_PREFERRED_DIST
        });
        ecs_set(world, e, EcAIState, {
            .behavior = AI_STRAFE,
            .behaviorTimer = 0,
            .strafeDir = strafeDir,
            .strafeTimer = strafeTimer,
            .dodgeTimer = 0,
            .dodgeCooldown = AI_DODGE_COOLDOWN,
            .burstShots = 0,
            .burstCooldown = 0,
            .attackTimer = 0
        });
    }

    ecs_set(world, e, EcAnimation, {
        .animState = ANIM_IDLE,
        .walkCycle = 0,
        .shootAnim = 0,
        .hitFlash = 0,
        .muzzleFlash = 0,
        .deathAngle = 0
    });

    return e;
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
