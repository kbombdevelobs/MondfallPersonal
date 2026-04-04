#include "enemy_systems.h"
#include "enemy_ai.h"
#include "enemy_physics.h"
#include "enemy_death_systems.h"
#include "enemy_morale.h"
#include "enemy_components.h"
#include "ecs_world.h"
#include "config.h"
#include <stdlib.h>

// ============================================================================
// Stored query for EcsEnemyCountAlive
// ============================================================================
static ecs_query_t *g_aliveQuery = NULL;

// ============================================================================
// Helper: play death radio sound (shared by all death triggers)
// ============================================================================
static void PlayDeathRadioSound(ecs_world_t *world, EnemyType type, int chance) {
    const EcEnemyResources *res = ecs_singleton_get(world, EcEnemyResources);
    if (!res) return;

    // Check if any death sound is currently playing
    bool anyPlaying = false;
    for (int si = 0; si < res->sovietDeathCount; si++)
        if (IsSoundPlaying(res->sndSovietDeath[si])) anyPlaying = true;
    for (int ai = 0; ai < res->americanDeathCount; ai++)
        if (IsSoundPlaying(res->sndAmericanDeath[ai])) anyPlaying = true;

    if (anyPlaying || (rand() % 100 >= chance)) return;

    // Need mutable access for play counts and timer
    EcEnemyResources *resMut = ecs_singleton_ensure(world, EcEnemyResources);
    if (!resMut) return;

    if (type == ENEMY_SOVIET && resMut->sovietDeathCount > 0 && resMut->sovietDeathPlays < 3) {
        int pick = GetRandomValue(0, resMut->sovietDeathCount - 1);
        PlaySound(resMut->sndSovietDeath[pick]);
        resMut->sovietDeathPlays++;
        resMut->radioTransmissionTimer = 3.0f;
    } else if (type == ENEMY_AMERICAN && resMut->americanDeathCount > 0 && resMut->americanDeathPlays < 3) {
        int pick = GetRandomValue(0, resMut->americanDeathCount - 1);
        PlaySound(resMut->sndAmericanDeath[pick]);
        resMut->americanDeathPlays++;
        resMut->radioTransmissionTimer = 3.0f;
    }
}

// ============================================================================
// Damage / Death trigger functions
// ============================================================================

void EcsEnemyDamage(ecs_world_t *world, ecs_entity_t entity, float damage) {
    if (!ecs_is_alive(world, entity)) return;
    if (!ecs_has(world, entity, EcAlive)) return;

    EcCombatStats *cs = ecs_ensure(world, entity, EcCombatStats);
    EcAnimation *anim = ecs_ensure(world, entity, EcAnimation);
    EcAIState *ai = ecs_ensure(world, entity, EcAIState);
    const EcFaction *fac = ecs_get(world, entity, EcFaction);

    cs->health -= damage;
    anim->hitFlash = 1;
    if (ai->dodgeTimer <= 0) { ai->strafeDir *= -1; ai->dodgeTimer = 1; }

    if (cs->health <= 0) {
        // Remove alive, add dying
        ecs_remove(world, entity, EcAlive);
        ecs_add(world, entity, EcDying);

        anim->deathAngle = 0;
        int deathStyle = (rand() % 100 < 40) ? 1 : 0;

        if (deathStyle == 0) {
            float launchAngle = ((float)rand() / RAND_MAX) * 2.0f * PI;
            float launchForce = 3.0f + ((float)rand() / RAND_MAX) * 5.0f;
            ecs_set(world, entity, EcRagdollDeath, {
                .spinX = 120.0f + (float)(rand() % 300),
                .spinY = (float)(rand() % 200) - 100.0f,
                .spinZ = 0,
                .ragdollVelX = cosf(launchAngle) * launchForce,
                .ragdollVelZ = sinf(launchAngle) * launchForce,
                .ragdollVelY = 2.0f + ((float)rand() / RAND_MAX) * 4.0f,
                .deathTimer = 10.0f,
                .deathStyle = 0
            });
        } else {
            ecs_set(world, entity, EcRagdollDeath, {
                .spinX = 80.0f + (float)(rand() % 60),
                .spinY = 0,
                .spinZ = 0,
                .ragdollVelX = 0, .ragdollVelZ = 0, .ragdollVelY = 0,
                .deathTimer = 12.0f,
                .deathStyle = 1
            });
        }

        if (fac) PlayDeathRadioSound(world, fac->type, 50);

        EcGameContext *ctx = ecs_singleton_ensure(world, EcGameContext);
        if (ctx) ctx->killCount++;

        // Morale hit and HUD flash when officer/NCO dies
        const EcRank *rank = ecs_get(world, entity, EcRank);
        if (rank && rank->rank >= RANK_NCO) {
            float hit = (rank->rank == RANK_OFFICER) ? MORALE_OFFICER_DEATH_HIT : MORALE_NCO_DEATH_HIT;
            EcsApplyMoraleHit(world, entity, hit);
            if (ctx) ctx->rankKillType = (rank->rank == RANK_OFFICER) ? 2 : 1;
        }
    }
}

void EcsEnemyVaporize(ecs_world_t *world, ecs_entity_t entity) {
    if (!ecs_is_alive(world, entity)) return;
    if (!ecs_has(world, entity, EcAlive)) return;

    const EcFaction *fac = ecs_get(world, entity, EcFaction);
    EcCombatStats *cs = ecs_ensure(world, entity, EcCombatStats);
    cs->health = 0;

    ecs_remove(world, entity, EcAlive);
    ecs_add(world, entity, EcVaporizing);

    int deathStyle = (rand() % 100 < 15) ? 1 : 0;
    ecs_set(world, entity, EcVaporizeDeath, {
        .vaporizeTimer = 0,
        .vaporizeScale = 1.0f,
        .deathTimer = (deathStyle == 1) ? 6.0f : 4.5f,
        .deathStyle = deathStyle
    });

    if (fac) PlayDeathRadioSound(world, fac->type, 50);

    EcGameContext *ctx = ecs_singleton_ensure(world, EcGameContext);
    if (ctx) ctx->killCount++;

    // Morale hit and HUD flash when officer/NCO dies
    const EcRank *rank = ecs_get(world, entity, EcRank);
    if (rank && rank->rank >= RANK_NCO) {
        float hit = (rank->rank == RANK_OFFICER) ? MORALE_OFFICER_DEATH_HIT : MORALE_NCO_DEATH_HIT;
        EcsApplyMoraleHit(world, entity, hit);
        if (ctx) ctx->rankKillType = (rank->rank == RANK_OFFICER) ? 2 : 1;
    }
}

void EcsEnemyEviscerate(ecs_world_t *world, ecs_entity_t entity, Vector3 hitDir) {
    if (!ecs_is_alive(world, entity)) return;
    if (!ecs_has(world, entity, EcAlive)) return;

    const EcFaction *fac = ecs_get(world, entity, EcFaction);
    EcCombatStats *cs = ecs_ensure(world, entity, EcCombatStats);
    cs->health = 0;

    ecs_remove(world, entity, EcAlive);
    ecs_add(world, entity, EcEviscerating);

    hitDir = Vector3Normalize(hitDir);

    // 0=head, 1=torso, 2=right arm, 3=left arm, 4=right leg, 5=left leg
    Vector3 origins[6] = {
        {0, 1.1f, 0}, {0, 0, 0}, {0.52f, 0.3f, 0},
        {-0.52f, 0.3f, 0}, {0.22f, -0.85f, 0}, {-0.22f, -0.85f, 0}
    };

    EcEviscerateDeath ed = {
        .evisTimer = 0,
        .evisDir = hitDir,
        .deathTimer = 10.0f
    };

    for (int i = 0; i < 6; i++) {
        ed.evisLimbPos[i] = origins[i];
        ed.evisBloodTimer[i] = 0;
        float rx = ((float)rand() / RAND_MAX - 0.5f) * 6.0f;
        float ry = 2.0f + ((float)rand() / RAND_MAX) * 5.0f;
        float rz = ((float)rand() / RAND_MAX - 0.5f) * 6.0f;
        ed.evisLimbVel[i] = (Vector3){
            hitDir.x * 4.0f + rx + origins[i].x * 2.0f, ry,
            hitDir.z * 4.0f + rz + origins[i].z * 2.0f
        };
    }
    ed.evisLimbVel[1].x += hitDir.x * 3.0f;
    ed.evisLimbVel[1].z += hitDir.z * 3.0f;
    ed.evisLimbVel[0].y += 4.0f;

    ecs_set_id(world, entity, ecs_id(EcEviscerateDeath), sizeof(EcEviscerateDeath), &ed);

    if (fac) PlayDeathRadioSound(world, fac->type, 65);

    EcGameContext *ctx = ecs_singleton_ensure(world, EcGameContext);
    if (ctx) ctx->killCount++;

    // Morale hit and HUD flash when officer/NCO dies
    const EcRank *rank = ecs_get(world, entity, EcRank);
    if (rank && rank->rank >= RANK_NCO) {
        float hit = (rank->rank == RANK_OFFICER) ? MORALE_OFFICER_DEATH_HIT : MORALE_NCO_DEATH_HIT;
        EcsApplyMoraleHit(world, entity, hit);
        if (ctx) ctx->rankKillType = (rank->rank == RANK_OFFICER) ? 2 : 1;
    }
}

int EcsEnemyCountAlive(ecs_world_t *world) {
    if (!g_aliveQuery) return 0;
    int count = 0;
    ecs_iter_t it = ecs_query_iter(world, g_aliveQuery);
    while (ecs_query_next(&it)) {
        count += it.count;
    }
    return count;
}

void EcsEnemySystemsCleanup(void) {
    if (g_aliveQuery) {
        ecs_query_fini(g_aliveQuery);
        g_aliveQuery = NULL;
    }
    EcsEnemyAISystemsCleanup();
}

// ============================================================================
// System Registration -- delegates to sub-modules
// ============================================================================

void EcsEnemySystemsRegister(ecs_world_t *world) {
    // Create the alive query for EcsEnemyCountAlive
    g_aliveQuery = ecs_query(world, {
        .terms = {
            { .id = ecs_id(EcTransform) },
            { .id = ecs_id(EcAlive) }
        }
    });

    EcsEnemyAISystemsRegister(world);
    EcsEnemyPhysicsSystemsRegister(world);
    EcsEnemyDeathSystemsRegister(world);
    EcsEnemyMoraleSystemsRegister(world);
}
