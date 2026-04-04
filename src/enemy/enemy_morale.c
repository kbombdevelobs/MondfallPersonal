#include "enemy_morale.h"
#include "enemy_components.h"
#include "config.h"
#include <math.h>
#include <stdlib.h>

// ============================================================================
// SysMoraleUpdate — find nearest leader, update morale recovery/decay
// ============================================================================

static void SysMoraleUpdate(ecs_iter_t *it) {
    EcTransform *tr = ecs_field(it, EcTransform, 0);
    EcMorale *mor = ecs_field(it, EcMorale, 1);
    EcRank *rk = ecs_field(it, EcRank, 2);
    float dt = it->delta_time;

    // Build a small list of alive leaders for proximity checks
    ecs_world_t *world = it->world;
    Vector3 leaderPos[16];
    ecs_entity_t leaderEnt[16];
    EnemyType leaderFac[16];
    int leaderCount = 0;

    ecs_query_t *lq = ecs_query(world, {
        .terms = {
            { .id = ecs_id(EcTransform) },
            { .id = ecs_id(EcRank) },
            { .id = ecs_id(EcFaction) },
            { .id = ecs_id(EcAlive) }
        }
    });
    ecs_iter_t lit = ecs_query_iter(world, lq);
    while (ecs_query_next(&lit) && leaderCount < 16) {
        EcTransform *lt = ecs_field(&lit, EcTransform, 0);
        EcRank *lr = ecs_field(&lit, EcRank, 1);
        EcFaction *lf = ecs_field(&lit, EcFaction, 2);
        for (int j = 0; j < lit.count && leaderCount < 16; j++) {
            if (lr[j].rank >= RANK_NCO) {
                leaderPos[leaderCount] = lt[j].position;
                leaderEnt[leaderCount] = lit.entities[j];
                leaderFac[leaderCount] = lf[j].type;
                leaderCount++;
            }
        }
    }
    ecs_query_fini(lq);

    // Now update each enemy's morale
    for (int i = 0; i < it->count; i++) {
        // Officers and NCOs always have full morale
        if (rk[i].rank >= RANK_NCO) {
            mor[i].morale = 1.0f;
            mor[i].leader = 0;
            mor[i].leaderDist = 0;
            continue;
        }

        // Find nearest same-faction leader
        const EcFaction *fac = ecs_get(world, it->entities[i], EcFaction);
        EnemyType myFac = fac ? fac->type : ENEMY_SOVIET;
        float bestDist = 999.0f;
        ecs_entity_t bestLeader = 0;

        for (int j = 0; j < leaderCount; j++) {
            if (leaderFac[j] != myFac) continue;
            float dx = tr[i].position.x - leaderPos[j].x;
            float dz = tr[i].position.z - leaderPos[j].z;
            float dist = sqrtf(dx * dx + dz * dz);
            if (dist < bestDist) {
                bestDist = dist;
                bestLeader = leaderEnt[j];
            }
        }

        mor[i].leader = bestLeader;
        mor[i].leaderDist = bestDist;

        // Morale recovery/decay
        if (bestDist <= LEADERSHIP_RADIUS) {
            // Near leader: faster recovery
            mor[i].morale += MORALE_NCO_RALLY_RATE * dt;
        } else {
            // No leader nearby: slow natural recovery
            mor[i].morale += MORALE_NATURAL_RECOVERY * dt;
            // Gentle decay if above 0.5 (leadership drift)
            if (mor[i].morale > 0.5f) {
                mor[i].morale -= MORALE_DECAY_RATE * dt;
                if (mor[i].morale < 0.5f) mor[i].morale = 0.5f;
            }
        }
        if (mor[i].morale > 1.0f) mor[i].morale = 1.0f;
        if (mor[i].morale < 0.0f) mor[i].morale = 0.0f;
    }
}

// ============================================================================
// SysMoraleCheck — trigger flee / rally based on morale thresholds
// ============================================================================

static void SysMoraleCheck(ecs_iter_t *it) {
    EcMorale *mor = ecs_field(it, EcMorale, 0);
    EcAIState *ai = ecs_field(it, EcAIState, 1);
    EcRank *rk = ecs_field(it, EcRank, 2);
    float dt = it->delta_time;

    for (int i = 0; i < it->count; i++) {
        // Officers and NCOs don't flee (NCOs only at half threshold)
        if (rk[i].rank == RANK_OFFICER) continue;
        float threshold = MORALE_FLEE_THRESHOLD;
        if (rk[i].rank == RANK_NCO) threshold *= 0.5f;

        if (ai[i].behavior == AI_FLEE) {
            // Currently fleeing — check for recovery
            mor[i].fleeTimer += dt;

            if (mor[i].fleeTimer >= MORALE_FLEE_DURATION_MIN) {
                bool rally = false;
                // Rally if near leader and morale recovered
                if (mor[i].leaderDist <= LEADERSHIP_RADIUS &&
                    mor[i].morale >= MORALE_RALLY_THRESHOLD) {
                    rally = true;
                }
                // Forced recovery after max flee time
                if (mor[i].fleeTimer >= MORALE_FLEE_DURATION_MAX) {
                    mor[i].morale = MORALE_RALLY_THRESHOLD;
                    rally = true;
                }
                if (rally) {
                    ai[i].behavior = mor[i].prevBehavior;
                    mor[i].fleeTimer = 0;
                }
            }
        } else {
            // Not fleeing — check if should start
            if (mor[i].morale < threshold) {
                if (rand() % MORALE_FLEE_CHANCE == 0) {
                    mor[i].prevBehavior = ai[i].behavior;
                    ai[i].behavior = AI_FLEE;
                    mor[i].fleeTimer = 0;
                }
            }
        }
    }
}

// ============================================================================
// Registration
// ============================================================================

void EcsEnemyMoraleSystemsCleanup(void) {
    // No cached queries to clean up — morale uses per-call queries
}

void EcsEnemyMoraleSystemsRegister(ecs_world_t *world) {
    ecs_system(world, {
        .entity = ecs_entity(world, { .name = "SysMoraleUpdate", .add = ecs_ids(ecs_dependson(EcsOnUpdate)) }),
        .query.terms = {
            { .id = ecs_id(EcTransform) },
            { .id = ecs_id(EcMorale) },
            { .id = ecs_id(EcRank) },
            { .id = ecs_id(EcAlive) }
        },
        .callback = SysMoraleUpdate
    });

    ecs_system(world, {
        .entity = ecs_entity(world, { .name = "SysMoraleCheck", .add = ecs_ids(ecs_dependson(EcsPostUpdate)) }),
        .query.terms = {
            { .id = ecs_id(EcMorale) },
            { .id = ecs_id(EcAIState) },
            { .id = ecs_id(EcRank) },
            { .id = ecs_id(EcAlive) }
        },
        .callback = SysMoraleCheck
    });
}

// ============================================================================
// Morale hit on leader death — apply to nearby same-faction troops
// ============================================================================

void EcsApplyMoraleHit(ecs_world_t *world, ecs_entity_t deadEntity, float amount) {
    const EcTransform *deadTr = ecs_get(world, deadEntity, EcTransform);
    const EcFaction *deadFac = ecs_get(world, deadEntity, EcFaction);
    if (!deadTr || !deadFac) return;

    Vector3 deathPos = deadTr->position;
    EnemyType faction = deadFac->type;

    ecs_query_t *q = ecs_query(world, {
        .terms = {
            { .id = ecs_id(EcTransform) },
            { .id = ecs_id(EcMorale) },
            { .id = ecs_id(EcFaction) },
            { .id = ecs_id(EcAlive) }
        }
    });
    ecs_iter_t it = ecs_query_iter(world, q);
    while (ecs_query_next(&it)) {
        EcTransform *tr = ecs_field(&it, EcTransform, 0);
        EcMorale *mor = ecs_field(&it, EcMorale, 1);
        EcFaction *fac = ecs_field(&it, EcFaction, 2);
        for (int i = 0; i < it.count; i++) {
            if (fac[i].type != faction) continue;
            float dx = tr[i].position.x - deathPos.x;
            float dz = tr[i].position.z - deathPos.z;
            float dist = sqrtf(dx * dx + dz * dz);
            if (dist <= LEADERSHIP_RADIUS) {
                mor[i].morale -= amount;
                if (mor[i].morale < 0.0f) mor[i].morale = 0.0f;
            }
        }
    }
    ecs_query_fini(q);
}
