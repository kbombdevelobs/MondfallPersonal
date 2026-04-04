#include "enemy_death_systems.h"
#include "enemy_systems.h"
#include "enemy_components.h"
#include "ecs_world.h"
#include "world.h"
#include "config.h"
#include <math.h>
#include <stdlib.h>

// ============================================================================
// SysRagdollDeath -- ragdoll blowout + crumple
// ============================================================================
static void SysRagdollDeath(ecs_iter_t *it) {
    EcTransform *tr = ecs_field(it, EcTransform, 0);
    EcAnimation *anim = ecs_field(it, EcAnimation, 1);
    EcRagdollDeath *rd = ecs_field(it, EcRagdollDeath, 2);

    float dt = it->delta_time;

    for (int i = it->count - 1; i >= 0; i--) {
        anim[i].animState = ANIM_DEATH;
        rd[i].deathTimer -= dt;

        if (rd[i].deathStyle == 0) {
            // RAGDOLL -- pressurized suit blowout spin
            anim[i].deathAngle += rd[i].spinX * dt;
            tr[i].facingAngle += rd[i].spinY * dt * 0.02f;
            tr[i].position.x += rd[i].ragdollVelX * dt;
            tr[i].position.z += rd[i].ragdollVelZ * dt;
            tr[i].position.y += rd[i].ragdollVelY * dt;
            rd[i].ragdollVelY -= MOON_GRAVITY * dt;
            rd[i].spinX *= (1.0f - 0.3f * dt);
            rd[i].spinY *= (1.0f - 0.3f * dt);
            rd[i].ragdollVelX *= (1.0f - 0.5f * dt);
            rd[i].ragdollVelZ *= (1.0f - 0.5f * dt);

            float gH = WorldGetHeight(tr[i].position.x, tr[i].position.z) + 0.6f;
            if (tr[i].position.y < gH) {
                tr[i].position.y = gH;
                if (fabsf(rd[i].ragdollVelY) < 0.5f) {
                    rd[i].ragdollVelY = 0;
                    rd[i].ragdollVelX *= 0.7f;
                    rd[i].ragdollVelZ *= 0.7f;
                    rd[i].spinX *= 0.85f;
                } else {
                    rd[i].ragdollVelY = fabsf(rd[i].ragdollVelY) * 0.3f;
                    rd[i].spinX *= 0.6f;
                }
            }
        } else {
            // CRUMPLE -- fall forward
            anim[i].deathAngle += rd[i].spinX * dt;
            if (anim[i].deathAngle > 90.0f) {
                anim[i].deathAngle = 90.0f;
                rd[i].spinX = 0;
            }
            float gH = WorldGetHeight(tr[i].position.x, tr[i].position.z) + 0.5f;
            if (tr[i].position.y > gH + 0.1f) tr[i].position.y -= dt * 2.0f;
            else tr[i].position.y = gH;
        }

        if (rd[i].deathTimer <= 0) {
            ecs_delete(it->world, it->entities[i]);
        }
    }
}

// ============================================================================
// SysVaporizeDeath -- jerk, freeze, swell, disintegrate
// ============================================================================
static void SysVaporizeDeath(ecs_iter_t *it) {
    EcTransform *tr = ecs_field(it, EcTransform, 0);
    EcAnimation *anim = ecs_field(it, EcAnimation, 1);
    EcVaporizeDeath *vd = ecs_field(it, EcVaporizeDeath, 2);

    float dt = it->delta_time;

    for (int i = it->count - 1; i >= 0; i--) {
        vd[i].deathTimer -= dt;
        vd[i].vaporizeTimer += dt;
        float t = vd[i].vaporizeTimer;

        if (t < 0.8f) {
            // Phase 1: JERK
            float intensity = 1.0f - t / 0.8f;
            anim[i].walkCycle += dt * 60.0f * intensity;
            anim[i].shootAnim = sinf(t * 40.0f) * 0.5f * intensity + 0.5f;
            anim[i].deathAngle = sinf(t * 30.0f) * 20.0f * intensity;
            vd[i].vaporizeScale = 1.0f;
        } else if (t < 2.0f) {
            // Phase 2: FREEZE
            anim[i].shootAnim = 0.5f;
            anim[i].deathAngle = 3.0f;
            vd[i].vaporizeScale = 1.0f;
            tr[i].position.y += dt * 0.15f;
        } else if (vd[i].deathStyle == 1 && t < 2.5f) {
            // Phase 2b: SWELL (15% only)
            float swellT = (t - 2.0f) / 0.5f;
            vd[i].vaporizeScale = 1.0f + swellT * 1.0f;
            tr[i].position.y += dt * 0.1f;
        } else if (vd[i].deathStyle == 1 && t < 2.6f) {
            // Phase 2c: POP (15% only)
            vd[i].vaporizeScale = 1.0f;
        } else {
            // Phase 3: DISINTEGRATE
            float startDis = (vd[i].deathStyle == 1) ? 2.6f : 2.0f;
            float disDur = (vd[i].deathStyle == 1) ? 3.2f : 2.3f;
            float disT = (t - startDis) / disDur;
            if (disT > 1.0f) disT = 1.0f;
            vd[i].vaporizeScale = 1.0f - disT;
            tr[i].position.y += dt * 0.3f;
        }

        if (vd[i].deathTimer <= 0) {
            ecs_delete(it->world, it->entities[i]);
        }
    }
}

// ============================================================================
// SysEviscerateDeath -- limb physics
// ============================================================================
static void SysEviscerateDeath(ecs_iter_t *it) {
    EcTransform *tr = ecs_field(it, EcTransform, 0);
    EcEviscerateDeath *ed = ecs_field(it, EcEviscerateDeath, 1);

    float dt = it->delta_time;

    for (int i = it->count - 1; i >= 0; i--) {
        ed[i].deathTimer -= dt;
        ed[i].evisTimer += dt;
        float t = ed[i].evisTimer;

        for (int li = 0; li < 6; li++) {
            ed[i].evisLimbPos[li] = Vector3Add(ed[i].evisLimbPos[li],
                Vector3Scale(ed[i].evisLimbVel[li], dt));
            ed[i].evisLimbVel[li].y -= MOON_GRAVITY * dt;

            float gH = WorldGetHeight(
                tr[i].position.x + ed[i].evisLimbPos[li].x,
                tr[i].position.z + ed[i].evisLimbPos[li].z);
            float limbGround = gH - tr[i].position.y + 0.1f;
            if (ed[i].evisLimbPos[li].y < limbGround) {
                ed[i].evisLimbPos[li].y = limbGround;
                ed[i].evisLimbVel[li].y = fabsf(ed[i].evisLimbVel[li].y) * 0.2f;
                ed[i].evisLimbVel[li].x *= 0.6f;
                ed[i].evisLimbVel[li].z *= 0.6f;
            }
            if (t < 3.0f) ed[i].evisBloodTimer[li] += dt;
        }

        if (ed[i].deathTimer <= 0) {
            ecs_delete(it->world, it->entities[i]);
        }
    }
}

// ============================================================================
// Registration
// ============================================================================

// ============================================================================
// SysRadioTimer -- decay the radio transmission display timer
// ============================================================================
static void SysRadioTimer(ecs_iter_t *it) {
    EcEnemyResources *res = ecs_field(it, EcEnemyResources, 0);
    float dt = it->delta_time;
    if (res->radioTransmissionTimer > 0)
        res->radioTransmissionTimer -= dt;
    // Reset death sound play counters when wave is cleared
    if (EcsEnemyCountAlive(it->world) == 0) {
        res->sovietDeathPlays = 0;
        res->americanDeathPlays = 0;
    }
}

void EcsEnemyDeathSystemsRegister(ecs_world_t *world) {
    // SysRagdollDeath -- EcsOnUpdate
    ecs_system_init(world, &(ecs_system_desc_t){
        .entity = ecs_entity(world, {
            .name = "SysRagdollDeath",
            .add = ecs_ids(ecs_dependson(EcsOnUpdate))
        }),
        .query.terms = {
            { .id = ecs_id(EcTransform) },
            { .id = ecs_id(EcAnimation) },
            { .id = ecs_id(EcRagdollDeath) },
            { .id = ecs_id(EcDying) }
        },
        .callback = SysRagdollDeath
    });

    // SysVaporizeDeath -- EcsOnUpdate
    ecs_system_init(world, &(ecs_system_desc_t){
        .entity = ecs_entity(world, {
            .name = "SysVaporizeDeath",
            .add = ecs_ids(ecs_dependson(EcsOnUpdate))
        }),
        .query.terms = {
            { .id = ecs_id(EcTransform) },
            { .id = ecs_id(EcAnimation) },
            { .id = ecs_id(EcVaporizeDeath) },
            { .id = ecs_id(EcVaporizing) }
        },
        .callback = SysVaporizeDeath
    });

    // SysEviscerateDeath -- EcsOnUpdate
    ecs_system_init(world, &(ecs_system_desc_t){
        .entity = ecs_entity(world, {
            .name = "SysEviscerateDeath",
            .add = ecs_ids(ecs_dependson(EcsOnUpdate))
        }),
        .query.terms = {
            { .id = ecs_id(EcTransform) },
            { .id = ecs_id(EcEviscerateDeath) },
            { .id = ecs_id(EcEviscerating) }
        },
        .callback = SysEviscerateDeath
    });

    // SysRadioTimer -- EcsOnUpdate (singleton)
    ecs_system_init(world, &(ecs_system_desc_t){
        .entity = ecs_entity(world, {
            .name = "SysRadioTimer",
            .add = ecs_ids(ecs_dependson(EcsOnUpdate))
        }),
        .query.terms = {
            { .id = ecs_id(EcEnemyResources), .src.id = ecs_id(EcEnemyResources) }
        },
        .callback = SysRadioTimer
    });
}
