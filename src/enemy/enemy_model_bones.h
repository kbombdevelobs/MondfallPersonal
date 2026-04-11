#ifndef ENEMY_MODEL_BONES_H
#define ENEMY_MODEL_BONES_H

#include "enemy_model_loader.h"
#include "enemy_components.h"
#include "enemy_bodydef.h"

/* ============================================================================
 * Procedural bone driving for skeletal astronaut models.
 *
 * Maps the 15 spring-damper axes in EcLimbState to bone rotations each frame.
 * Called before DrawModel() to pose the skeleton.
 *
 * The approach: start from bind pose, apply spring angles as rotations on
 * each bone, then write the final transforms to the model's bone matrices.
 * ============================================================================ */

/// Apply spring-damper limb state to the model's skeleton.
/// Must be called each frame before rendering with DrawModel().
void AstroModelApplySpringState(
    AstroModel *am,
    const EcLimbState *ls,
    const AnimProfile *ap,
    float hitFlash,
    float knockdownTimer,
    float knockdownAngle,
    bool  isCowering,
    bool  isDying,
    float deathTime
);

/// Get the world-space position of a bone after spring state has been applied.
/// Useful for attaching gun models to hand_R.
Vector3 AstroModelGetBonePosition(const AstroModel *am, BoneSlot slot);

#endif /* ENEMY_MODEL_BONES_H */
