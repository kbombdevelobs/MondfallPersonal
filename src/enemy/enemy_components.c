#include "enemy_components.h"

// Component ID storage (populated by ECS_COMPONENT_DEFINE)
ECS_COMPONENT_DECLARE(EcTransform);
ECS_COMPONENT_DECLARE(EcVelocity);
ECS_COMPONENT_DECLARE(EcFaction);
ECS_COMPONENT_DECLARE(EcAlive);
ECS_COMPONENT_DECLARE(EcCombatStats);
ECS_COMPONENT_DECLARE(EcAIState);
ECS_COMPONENT_DECLARE(EcAnimation);
ECS_COMPONENT_DECLARE(EcLimbState);
ECS_COMPONENT_DECLARE(EcDying);
ECS_COMPONENT_DECLARE(EcVaporizing);
ECS_COMPONENT_DECLARE(EcEviscerating);
ECS_COMPONENT_DECLARE(EcDecapitating);
ECS_COMPONENT_DECLARE(EcRagdollDeath);
ECS_COMPONENT_DECLARE(EcVaporizeDeath);
ECS_COMPONENT_DECLARE(EcEviscerateDeath);
ECS_COMPONENT_DECLARE(EcDecapitateDeath);
ECS_COMPONENT_DECLARE(EcRank);
ECS_COMPONENT_DECLARE(EcMorale);
ECS_COMPONENT_DECLARE(EcSteering);
ECS_COMPONENT_DECLARE(EcSquad);
ECS_COMPONENT_DECLARE(EcEnemyResources);
ECS_COMPONENT_DECLARE(EcGameContext);

ecs_entity_t g_sovietPrefab = 0;
ecs_entity_t g_americanPrefab = 0;

void EcsEnemyComponentsRegister(ecs_world_t *world) {
    // Register all component types
    ECS_COMPONENT_DEFINE(world, EcTransform);
    ECS_COMPONENT_DEFINE(world, EcVelocity);
    ECS_COMPONENT_DEFINE(world, EcFaction);
    ECS_COMPONENT_DEFINE(world, EcAlive);
    ECS_COMPONENT_DEFINE(world, EcCombatStats);
    ECS_COMPONENT_DEFINE(world, EcAIState);
    ECS_COMPONENT_DEFINE(world, EcAnimation);
    ECS_COMPONENT_DEFINE(world, EcLimbState);
    ECS_COMPONENT_DEFINE(world, EcDying);
    ECS_COMPONENT_DEFINE(world, EcVaporizing);
    ECS_COMPONENT_DEFINE(world, EcEviscerating);
    ECS_COMPONENT_DEFINE(world, EcDecapitating);
    ECS_COMPONENT_DEFINE(world, EcRagdollDeath);
    ECS_COMPONENT_DEFINE(world, EcVaporizeDeath);
    ECS_COMPONENT_DEFINE(world, EcEviscerateDeath);
    ECS_COMPONENT_DEFINE(world, EcDecapitateDeath);
    ECS_COMPONENT_DEFINE(world, EcRank);
    ECS_COMPONENT_DEFINE(world, EcMorale);
    ECS_COMPONENT_DEFINE(world, EcSteering);
    ECS_COMPONENT_DEFINE(world, EcSquad);
    ECS_COMPONENT_DEFINE(world, EcEnemyResources);
    ECS_COMPONENT_DEFINE(world, EcGameContext);

    // Create Soviet prefab — template for Soviet enemies
    g_sovietPrefab = ecs_new(world);
    ecs_set(world, g_sovietPrefab, EcFaction, { .type = ENEMY_SOVIET });
    ecs_set(world, g_sovietPrefab, EcCombatStats, {
        .health = SOVIET_HEALTH, .maxHealth = SOVIET_HEALTH,
        .speed = SOVIET_SPEED, .damage = SOVIET_DAMAGE,
        .attackRange = SOVIET_ATTACK_RANGE, .attackRate = SOVIET_ATTACK_RATE,
        .preferredDist = SOVIET_PREFERRED_DIST
    });
    ecs_set(world, g_sovietPrefab, EcAIState, {
        .behavior = AI_ADVANCE,
        .strafeDir = 1.0f,
        .strafeTimer = AI_STRAFE_TIMER_BASE,
        .dodgeCooldown = AI_DODGE_COOLDOWN
    });
    ecs_add_id(world, g_sovietPrefab, EcsPrefab);

    // Create American prefab — template for American enemies
    g_americanPrefab = ecs_new(world);
    ecs_set(world, g_americanPrefab, EcFaction, { .type = ENEMY_AMERICAN });
    ecs_set(world, g_americanPrefab, EcCombatStats, {
        .health = AMERICAN_HEALTH, .maxHealth = AMERICAN_HEALTH,
        .speed = AMERICAN_SPEED, .damage = AMERICAN_DAMAGE,
        .attackRange = AMERICAN_ATTACK_RANGE, .attackRate = AMERICAN_ATTACK_RATE,
        .preferredDist = AMERICAN_PREFERRED_DIST
    });
    ecs_set(world, g_americanPrefab, EcAIState, {
        .behavior = AI_STRAFE,
        .strafeDir = 1.0f,
        .strafeTimer = AI_STRAFE_TIMER_BASE,
        .dodgeCooldown = AI_DODGE_COOLDOWN
    });
    ecs_add_id(world, g_americanPrefab, EcsPrefab);
}
