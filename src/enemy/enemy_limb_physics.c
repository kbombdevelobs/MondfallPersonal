#include "enemy_limb_physics.h"
#include "enemy_components.h"
#include "../world.h"
#include "config.h"
#include <math.h>

// EcLimbState component ID is declared in enemy_components.h and registered
// in enemy_components.c — this file only provides the system and profiles.

// ============================================================================
// Faction-specific animation profiles (ported from Mondfall enemy_bodydef.c)
// ============================================================================

/* Soviet: floppy, aggressive springs — big overshoot, heavy sway */
static const LimbAnimProfile SOVIET_LIMB_PROFILE = {
    .armSwingDeg    = 42.0f,
    .legSwingDeg    = 45.0f,
    .armBasePitch   = -30.0f,
    .armBaseSpread  = -16.0f,
    .kneeBendFactor = 1.2f,

    .armSwingSpr   = {3.0f,  1.0f},
    .armSpreadSpr  = {2.5f,  0.8f},
    .legSwingSpr   = {5.0f,  1.5f},
    .legSpreadSpr  = {3.5f,  1.2f},
    .kneeSpr       = {14.0f, 2.5f},
    .hipSwaySpr    = {2.5f,  0.8f},
    .torsoLeanSpr  = {2.0f,  1.2f},
    .torsoTwistSpr = {3.5f,  1.0f},
    .headBobSpr    = {4.0f,  1.2f},

    .hipSwayAmount = 0.22f,
    .torsoLeanDeg  = 16.0f,
    .torsoTwistDeg = 14.0f,
    .headBobDeg    = 10.0f,
    .breathRate    = 0.25f,
    .breathScale   = 0.015f,
    .kneeFlexBase  = -15.0f,

    .gunRestPitchOffset = -2.0f,
};

/* American: stiffer, disciplined springs — tighter control, smaller amplitudes */
static const LimbAnimProfile AMERICAN_LIMB_PROFILE = {
    .armSwingDeg    = 30.0f,
    .legSwingDeg    = 32.0f,
    .armBasePitch   = -28.0f,
    .armBaseSpread  = -12.0f,
    .kneeBendFactor = 0.9f,

    .armSwingSpr   = {5.0f,  1.6f},
    .armSpreadSpr  = {4.0f,  1.4f},
    .legSwingSpr   = {7.0f,  2.0f},
    .legSpreadSpr  = {5.0f,  1.8f},
    .kneeSpr       = {16.0f, 3.0f},
    .hipSwaySpr    = {4.5f,  1.4f},
    .torsoLeanSpr  = {4.0f,  1.8f},
    .torsoTwistSpr = {5.0f,  1.6f},
    .headBobSpr    = {5.5f,  1.8f},

    .hipSwayAmount = 0.14f,
    .torsoLeanDeg  = 10.0f,
    .torsoTwistDeg = 8.0f,
    .headBobDeg    = 6.0f,
    .breathRate    = 0.20f,
    .breathScale   = 0.010f,
    .kneeFlexBase  = -10.0f,

    .gunRestPitchOffset = 1.0f,
};

const LimbAnimProfile *LimbAnimProfileGet(int factionType) {
    return (factionType == ENEMY_SOVIET) ? &SOVIET_LIMB_PROFILE : &AMERICAN_LIMB_PROFILE;
}

// ============================================================================
// LimbSpringUpdate — single spring-damper tick
// ============================================================================
static void LimbSpringUpdate(LimbSpring *s, float target, float stiffness, float damping, float dt) {
    float force = (target - s->angle) * stiffness;
    s->vel += force * dt;
    s->vel *= (1.0f - damping * dt);
    if (s->vel > 500.0f) s->vel = 500.0f;
    if (s->vel < -500.0f) s->vel = -500.0f;
    s->angle += s->vel * dt;
    /* Clamp angle to prevent runaway from accumulated impulses */
    if (s->angle > 360.0f) s->angle = 360.0f;
    if (s->angle < -360.0f) s->angle = -360.0f;
}

// ============================================================================
// SysLimbPhysics — spring-damper animation for 15 limb axes per enemy
// ============================================================================
static void SysLimbPhysics(ecs_iter_t *it) {
    EcTransform *tr   = ecs_field(it, EcTransform, 0);
    EcVelocity  *vel  = ecs_field(it, EcVelocity,  1);
    EcAnimation *anim = ecs_field(it, EcAnimation,  2);
    EcLimbState *ls   = ecs_field(it, EcLimbState,  3);
    EcFaction   *fac  = ecs_field(it, EcFaction,    4);
    if (!tr || !vel || !anim || !ls || !fac) return;
    float dt = it->delta_time;
    if (dt <= 0 || dt > 0.1f) return;

    for (int i = 0; i < it->count; i++) {
        /* Faction-specific animation profile */
        const LimbAnimProfile *ap = LimbAnimProfileGet(fac[i].type);
        float basePitch = ap->armBasePitch + ap->gunRestPitchOffset;

        float spd   = Vector3Length(vel[i].velocity);
        float walk  = anim[i].walkCycle;
        float shoot = anim[i].shootAnim;
        /* knockdownAngle/staggerTimer/lastHitDir not present in flat-world
           EcAnimation — use safe zero defaults */
        float kd    = 0.0f;
        float t  = (float)GetTime();
        float ks = tr[i].facingAngle * 2.31f;
        float jerk = 0.0f;
        float seed = tr[i].facingAngle * 5.17f;

        /* --- Per-limb hit reactions --- */
        float hitArmR = 0, hitArmL = 0, hitLegR = 0, hitLegL = 0;
        if (anim[i].hitFlash > 0.9f) {
            /* No lastHitDir in flat-world build — symmetric flinch */
            float imp = 45.0f;
            hitArmR = -imp * 0.5f;
            hitArmL = -imp * 0.5f;
        }

        /* --- ARM targets --- */
        float walkDamp = 1.0f - shoot * 0.7f;
        float aimRaise = shoot * 22.0f;
        float aR = (basePitch - sinf(walk)*ap->armSwingDeg*walkDamp + aimRaise)*(1.0f-kd)
                 + (sinf(t*0.7f+ks)*40.0f-60.0f)*kd + sinf(seed)*60.0f*jerk + hitArmR;
        float aL = (basePitch + sinf(walk)*ap->armSwingDeg*walkDamp + shoot*18.0f)*(1.0f-kd)
                 + (sinf(t*0.9f+ks*1.3f)*35.0f-55.0f)*kd + cosf(seed*1.3f)*55.0f*jerk + hitArmL;
        float sR = ap->armBaseSpread*(1.0f-kd)
                 + (cosf(t*0.5f+ks)*20.0f-25.0f)*kd + sinf(seed*0.7f)*30.0f*jerk;
        float sL = (-ap->armBaseSpread*(1.0f - shoot*0.5f) + shoot*(-5.0f))*(1.0f-kd)
                 + (cosf(t*0.6f+ks*1.7f)*20.0f+25.0f)*kd - cosf(seed*0.9f)*30.0f*jerk;

        LimbSpringUpdate(&ls[i].armSwingR,  aR, ap->armSwingSpr.stiffness,  ap->armSwingSpr.damping,  dt);
        LimbSpringUpdate(&ls[i].armSwingL,  aL, ap->armSwingSpr.stiffness,  ap->armSwingSpr.damping,  dt);
        LimbSpringUpdate(&ls[i].armSpreadR, sR, ap->armSpreadSpr.stiffness, ap->armSpreadSpr.damping, dt);
        LimbSpringUpdate(&ls[i].armSpreadL, sL, ap->armSpreadSpr.stiffness, ap->armSpreadSpr.damping, dt);

        /* --- LEG targets with foot planting + airborne detection --- */
        bool airborne = fabsf(vel[i].vertVel) > 0.5f;
        float slopeR = 0, slopeL = 0;
        if (!airborne) {
            /* Flat world slope approximation using WorldGetHeight */
            float facRad2 = tr[i].facingAngle * DEG2RAD;
            float hC = WorldGetHeight(tr[i].position.x, tr[i].position.z);
            float hF = WorldGetHeight(tr[i].position.x + cosf(facRad2)*0.5f,
                                      tr[i].position.z + sinf(facRad2)*0.5f);
            float slopeAdj = (hF - hC) * 12.0f;
            float rPhase = sinf(walk);
            slopeR = (rPhase > 0) ? slopeAdj : -slopeAdj * 0.5f;
            slopeL = (rPhase < 0) ? slopeAdj : -slopeAdj * 0.5f;
        }

        float lR, lL, lsR, lsL, kR, kL;
        if (airborne && kd < 0.1f) {
            /* Airborne: legs trail behind movement direction, knees bent and floppy.
             * The faster they're moving, the more the legs swing back.
             * Opposite legs splay apart for a dramatic flying pose. */
            float airT = t * 0.8f + ks;
            /* Movement-relative trailing: legs swing opposite to velocity */
            float facRad3 = tr[i].facingAngle * DEG2RAD;
            float fwdDot = vel[i].velocity.x * sinf(facRad3) + vel[i].velocity.z * cosf(facRad3);
            float trailAngle = -fwdDot * 8.0f; /* strong backward swing from forward motion */
            if (trailAngle < -55.0f) trailAngle = -55.0f;
            if (trailAngle > 30.0f) trailAngle = 30.0f;
            /* Vertical velocity adds upward kick (going up = legs trail down more) */
            float vertKick = -vel[i].vertVel * 4.0f;
            if (vertKick < -30.0f) vertKick = -30.0f;
            if (vertKick > 20.0f) vertKick = 20.0f;
            lR  = trailAngle + vertKick + sinf(airT) * 12.0f;
            lL  = trailAngle + vertKick + sinf(airT + 2.0f) * 12.0f;
            lsR =  sinf(airT * 0.7f) * 10.0f + 5.0f;  /* splay outward */
            lsL = -sinf(airT * 0.7f + 1.0f) * 10.0f - 5.0f;
            kR  = -ap->kneeFlexBase + 30.0f + sinf(airT * 0.9f) * 10.0f;  /* deeply bent, floppy */
            kL  = -ap->kneeFlexBase + 28.0f + sinf(airT * 0.9f + 2.0f) * 10.0f; /* deeply bent, floppy */
        } else {
            lR = sinf(walk)*ap->legSwingDeg*(1.0f-kd)
                     + (sinf(t*0.6f+ks*0.7f)*25.0f+15.0f)*kd + hitLegR + slopeR;
            lL = -sinf(walk)*ap->legSwingDeg*(1.0f-kd)
                     + (sinf(t*0.8f+ks*1.1f)*25.0f-15.0f)*kd + hitLegL + slopeL;
            lsR = (cosf(t*0.4f+ks)*10.0f+8.0f)*kd;
            lsL = (cosf(t*0.5f+ks*1.4f)*10.0f-8.0f)*kd;
            /* Knee: ALWAYS flex (negative), never hyperextend.
               Back leg bends deep (clearing foot), front leg mostly straight. */
            float flexR = (lR < 0) ? fabsf(lR) * ap->kneeBendFactor
                                   : fabsf(lR) * ap->kneeBendFactor * 0.3f;
            float flexL = (lL < 0) ? fabsf(lL) * ap->kneeBendFactor
                                   : fabsf(lL) * ap->kneeBendFactor * 0.3f;
            kR = (flexR - ap->kneeFlexBase) * (1.0f-kd);
            kL = (flexL - ap->kneeFlexBase) * (1.0f-kd);
            /* Slope knee correction */
            kR += fabsf(slopeR) * ap->kneeBendFactor * 0.4f * (1.0f - kd);
            kL += fabsf(slopeL) * ap->kneeBendFactor * 0.4f * (1.0f - kd);
        }

        LimbSpringUpdate(&ls[i].legSwingR,  lR,  ap->legSwingSpr.stiffness,  ap->legSwingSpr.damping,  dt);
        LimbSpringUpdate(&ls[i].legSwingL,  lL,  ap->legSwingSpr.stiffness,  ap->legSwingSpr.damping,  dt);
        LimbSpringUpdate(&ls[i].legSpreadR, lsR, ap->legSpreadSpr.stiffness, ap->legSpreadSpr.damping, dt);
        LimbSpringUpdate(&ls[i].legSpreadL, lsL, ap->legSpreadSpr.stiffness, ap->legSpreadSpr.damping, dt);
        LimbSpringUpdate(&ls[i].kneeR,      kR,  ap->kneeSpr.stiffness,      ap->kneeSpr.damping,      dt);
        LimbSpringUpdate(&ls[i].kneeL,      kL,  ap->kneeSpr.stiffness,      ap->kneeSpr.damping,      dt);

        /* --- Secondary motion --- */
        float hipT = (spd > 0.1f) ? sinf(walk)*ap->hipSwayAmount : 0;
        LimbSpringUpdate(&ls[i].hipSway, hipT, ap->hipSwaySpr.stiffness, ap->hipSwaySpr.damping, dt);

        const EcCombatStats *cs = ecs_get(it->world, it->entities[i], EcCombatStats);
        float maxSpd = cs ? cs->speed : 3.0f;
        float sf = (maxSpd > 0.01f) ? fminf(spd/maxSpd, 1.0f) : 0;
        LimbSpringUpdate(&ls[i].torsoLean, sf*ap->torsoLeanDeg,
                         ap->torsoLeanSpr.stiffness, ap->torsoLeanSpr.damping, dt);

        /* Turn-in-place: facing delta drives torsoTwist impulse */
        float facD = tr[i].facingAngle - ls[i].prevFacing;
        while (facD > 180) facD -= 360;
        while (facD < -180) facD += 360;

        float twistT = (spd > 0.1f) ? sinf(walk*0.5f)*ap->torsoTwistDeg : 0;
        ls[i].torsoTwist.angle -= facD * 0.4f;
        if (ls[i].torsoTwist.angle > 25) ls[i].torsoTwist.angle = 25;
        if (ls[i].torsoTwist.angle < -25) ls[i].torsoTwist.angle = -25;
        LimbSpringUpdate(&ls[i].torsoTwist, twistT,
                         ap->torsoTwistSpr.stiffness, ap->torsoTwistSpr.damping, dt);

        /* Head bob */
        float headT = (spd > 0.1f) ? sinf(walk*2.0f)*ap->headBobDeg : 0;
        headT += 25.0f * jerk;
        LimbSpringUpdate(&ls[i].headPitch, headT,
                         ap->headBobSpr.stiffness, ap->headBobSpr.damping, dt);

        /* Head yaw lag — resists facing changes */
        ls[i].headYaw.angle -= facD * 0.5f;
        if (ls[i].headYaw.angle > 20) ls[i].headYaw.angle = 20;
        if (ls[i].headYaw.angle < -20) ls[i].headYaw.angle = -20;
        float headYawT = cosf(seed)*18.0f*jerk;
        LimbSpringUpdate(&ls[i].headYaw, headYawT,
                         ap->headBobSpr.stiffness, ap->headBobSpr.damping, dt);

        /* Breathing */
        ls[i].breathPhase += dt * ap->breathRate * 6.28318f;
        if (ls[i].breathPhase > 6.28318f) ls[i].breathPhase -= 6.28318f;
        ls[i].prevSpeed  = spd;
        ls[i].prevFacing = tr[i].facingAngle;
    }
}

// ============================================================================
// EcsEnemyLimbPhysicsRegister — register EcLimbState component + system
// ============================================================================
void EcsEnemyLimbPhysicsRegister(ecs_world_t *world) {
    // EcLimbState component already registered by EcsEnemyComponentsRegister()
    ecs_system_init(world, &(ecs_system_desc_t){
        .entity = ecs_entity(world, {
            .name = "SysLimbPhysics",
            .add = ecs_ids(ecs_dependson(EcsPostUpdate))
        }),
        .query.terms = {
            { .id = ecs_id(EcTransform) },
            { .id = ecs_id(EcVelocity) },
            { .id = ecs_id(EcAnimation) },
            { .id = ecs_id(EcLimbState) },
            { .id = ecs_id(EcFaction) },
            { .id = ecs_id(EcAlive) }
        },
        .callback = SysLimbPhysics
    });
}
