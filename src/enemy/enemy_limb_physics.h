#ifndef ENEMY_LIMB_PHYSICS_H
#define ENEMY_LIMB_PHYSICS_H

#include "flecs.h"
#include "enemy_components.h"   // LimbSpring, EcLimbState, ecs_id(EcLimbState)

// ============================================================================
// Spring tuning parameters (subset of Mondfall's AnimProfile/GunDef)
// ============================================================================

typedef struct {
    float stiffness;
    float damping;
} LimbSpringTune;

// Animation profile — faction-specific spring tuning and motion amplitudes.
// Standalone definition for the flat-world build (no enemy_bodydef dependency).
typedef struct {
    float armSwingDeg;
    float legSwingDeg;
    float armBasePitch;
    float armBaseSpread;
    float kneeBendFactor;

    LimbSpringTune armSwingSpr;
    LimbSpringTune armSpreadSpr;
    LimbSpringTune legSwingSpr;
    LimbSpringTune legSpreadSpr;
    LimbSpringTune kneeSpr;
    LimbSpringTune hipSwaySpr;
    LimbSpringTune torsoLeanSpr;
    LimbSpringTune torsoTwistSpr;
    LimbSpringTune headBobSpr;

    float hipSwayAmount;
    float torsoLeanDeg;
    float torsoTwistDeg;
    float headBobDeg;
    float breathRate;
    float breathScale;
    float kneeFlexBase;

    float gunRestPitchOffset;   // merged from GunDef.restPitchOffset
} LimbAnimProfile;

/// Return the faction-specific limb animation profile.
const LimbAnimProfile *LimbAnimProfileGet(int factionType);

/// Register the spring-damper limb animation system (SysLimbPhysics).
/// Call from EcsEnemySystemsRegister() or after component registration.
void EcsEnemyLimbPhysicsRegister(ecs_world_t *world);

#endif /* ENEMY_LIMB_PHYSICS_H */
