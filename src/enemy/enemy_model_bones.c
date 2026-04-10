#include "enemy_model_bones.h"
#include "raymath.h"
#include "rlgl.h"
#include <math.h>
#include <string.h>

/* ============================================================================
 * Procedural bone driving for skeletal astronaut models.
 *
 * NEW approach: proper parent-chain propagation.
 *   1. Convert bind pose from world-space to local-space
 *   2. Apply spring rotations to local poses
 *   3. Walk hierarchy root->leaf to compute world poses
 *   4. Compute boneMatrix = inverse(bindWorld) * currentWorld
 *
 * This ensures child bones follow parent rotations (arms follow spine, etc.)
 * ============================================================================ */

/// Create a rotation quaternion from axis and angle in degrees.
static Quaternion RotFromAxisDeg(Vector3 axis, float degrees) {
    float rad = degrees * DEG2RAD;
    return QuaternionFromAxisAngle(axis, rad);
}

/* ============================================================================
 * Transform math helpers
 * ============================================================================ */

/// Combine parent and child transforms: result = parent * child
static Transform CombineTransforms(Transform parent, Transform child) {
    Transform r;
    r.scale = Vector3Multiply(parent.scale, child.scale);
    r.rotation = QuaternionMultiply(parent.rotation, child.rotation);
    r.translation = Vector3Add(
        Vector3RotateByQuaternion(
            Vector3Multiply(child.translation, parent.scale),
            parent.rotation),
        parent.translation);
    return r;
}

/// Invert a transform: result * original = identity
static Transform InvertTransform(Transform t) {
    Transform r;
    r.rotation = QuaternionInvert(t.rotation);
    r.scale = (Vector3){
        (fabsf(t.scale.x) > 0.0001f) ? 1.0f / t.scale.x : 1.0f,
        (fabsf(t.scale.y) > 0.0001f) ? 1.0f / t.scale.y : 1.0f,
        (fabsf(t.scale.z) > 0.0001f) ? 1.0f / t.scale.z : 1.0f,
    };
    r.translation = Vector3Negate(
        Vector3RotateByQuaternion(
            Vector3Multiply(t.translation, r.scale),
            r.rotation));
    return r;
}

/// Convert a Transform to a Matrix
static Matrix TransformToMatrix(Transform t) {
    Matrix matS = MatrixScale(t.scale.x, t.scale.y, t.scale.z);
    Matrix matR = QuaternionToMatrix(t.rotation);
    Matrix matT = MatrixTranslate(t.translation.x, t.translation.y, t.translation.z);
    return MatrixMultiply(MatrixMultiply(matS, matR), matT);
}

/* ============================================================================
 * Main bone driving function
 * ============================================================================ */

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
) {
    if (!am) return;
    if (!am->loaded) return;
    if (!ls || !ap) return;

    Model *m = &am->model;
    if (!m->bindPose || m->boneCount <= 0 || !m->bones) return;

    int bc = m->boneCount;
    if (bc > 32) bc = 32;

    /* ================================================================
     * Step 1: Convert world-space bind poses to local-space
     * localBind[b] = inverse(worldBind[parent]) * worldBind[b]
     * ================================================================ */
    Transform localBind[32];
    for (int b = 0; b < bc; b++) {
        int parent = m->bones[b].parent;
        if (parent >= 0 && parent < bc) {
            localBind[b] = CombineTransforms(InvertTransform(m->bindPose[parent]), m->bindPose[b]);
        } else {
            localBind[b] = m->bindPose[b];
        }
    }

    /* ================================================================
     * Step 2: Copy local bind poses, then apply spring rotations
     * ================================================================ */
    Transform localPose[32];
    memcpy(localPose, localBind, bc * sizeof(Transform));

    #define BONE_IDX(slot) (am->bones[slot])
    #define HAS_BONE(slot) (am->bones[slot] >= 0 && am->bones[slot] < bc)

    /* Motion amplification -- skeletal models need bigger angles to look right */
    const float AMP = 2.0f;    /* general motion amplifier */
    const float ARM_AMP = 2.5f; /* arms need even more to look lively */
    const float LEG_AMP = 2.2f; /* legs too */

    /* === SPINE: torso lean, twist, hip sway, breathing === */
    if (HAS_BONE(BONE_SPINE)) {
        int bi = BONE_IDX(BONE_SPINE);
        if (!isDying) {
            Quaternion rot = QuaternionIdentity();
            rot = QuaternionMultiply(rot, RotFromAxisDeg((Vector3){1, 0, 0}, -ls->torsoLean.angle * AMP));
            rot = QuaternionMultiply(rot, RotFromAxisDeg((Vector3){0, 1, 0}, -ls->torsoTwist.angle * AMP));
            if (hitFlash > 0) {
                float f = hitFlash;
                rot = QuaternionMultiply(rot, RotFromAxisDeg((Vector3){1, 0, 0}, -f * 20.0f));
                rot = QuaternionMultiply(rot, RotFromAxisDeg((Vector3){0, 0, 1}, sinf(f * 30.0f) * f * 15.0f));
            }
            if (knockdownTimer > 0 && !isCowering)
                rot = QuaternionMultiply(rot, RotFromAxisDeg((Vector3){1, 0, 0}, -knockdownAngle));
            if (isCowering)
                rot = QuaternionMultiply(rot, RotFromAxisDeg((Vector3){1, 0, 0}, -knockdownAngle));
            localPose[bi].rotation = QuaternionMultiply(localPose[bi].rotation, rot);
            localPose[bi].translation.x += ls->hipSway.angle * 0.01f;
            float breath = sinf(ls->breathPhase) * ap->breathScale;
            localPose[bi].scale.x = 1.0f + breath;
            localPose[bi].scale.y = 1.0f + breath * 0.5f;
            localPose[bi].scale.z = 1.0f + breath;
        }
        /* When dying: spine stays in bind pose -- death rotation is applied
         * via rlRotatef in the draw code, not here. No spring oscillation. */
    }

    /* === HEAD (clamped -- no 360 degree spinning! Frozen when dying.) === */
    if (HAS_BONE(BONE_HEAD)) {
        int bi = BONE_IDX(BONE_HEAD);
        if (!isDying) {
            Quaternion rot = QuaternionIdentity();
            float hPitch = -ls->headPitch.angle * AMP;
            float hYaw   = -ls->headYaw.angle * AMP;
            if (hPitch >  45.0f) hPitch =  45.0f;
            if (hPitch < -45.0f) hPitch = -45.0f;
            if (hYaw   >  80.0f) hYaw   =  80.0f;
            if (hYaw   < -80.0f) hYaw   = -80.0f;
            rot = QuaternionMultiply(rot, RotFromAxisDeg((Vector3){1, 0, 0}, hPitch));
            rot = QuaternionMultiply(rot, RotFromAxisDeg((Vector3){0, 1, 0}, hYaw));
            localPose[bi].rotation = QuaternionMultiply(localPose[bi].rotation, rot);
        }
        /* When dying: head stays in bind pose -- no spring motion */
    }

    /* On death: arms ALWAYS ragdoll (gun disappears, arms go free).
     * Legs have a per-enemy random chance. */
    bool ragdollArmR = isDying;
    bool ragdollArmL = isDying;
    bool ragdollLegR = isDying && (((int)(ls->breathPhase * 100)) % 3 != 0);
    bool ragdollLegL = isDying && (((int)(ls->breathPhase * 100)) % 3 != 1);

    /* === RIGHT ARM === */
    if (HAS_BONE(BONE_ARM_R)) {
        int bi = BONE_IDX(BONE_ARM_R);
        Quaternion rot = QuaternionIdentity();
        if (ragdollArmR) {
            float seed = ls->breathPhase * 3.17f;
            float jd = (deathTime < 2.0f) ? 1.0f : (deathTime < 5.0f) ? (5.0f-deathTime)/3.0f : 0.0f;
            float drift = sinf(deathTime * 0.4f + seed) * 15.0f;
            float jerk = sinf(deathTime * 25.0f + seed) * 40.0f * jd;
            rot = QuaternionMultiply(rot, RotFromAxisDeg((Vector3){1,0,0}, -(jerk + drift - 60.0f)));
            rot = QuaternionMultiply(rot, RotFromAxisDeg((Vector3){0,0,1}, sinf(deathTime*0.5f+seed*1.3f)*30.0f));
            rot = QuaternionMultiply(rot, RotFromAxisDeg((Vector3){0,1,0}, sinf(deathTime*18.0f+seed)*25.0f*jd));
        } else {
            rot = QuaternionMultiply(rot, RotFromAxisDeg((Vector3){1, 0, 0},
                  -(ap->armBasePitch + ls->armSwingR.angle * ARM_AMP)));
            rot = QuaternionMultiply(rot, RotFromAxisDeg((Vector3){0, 0, 1},
                  -(ap->armBaseSpread + ls->armSpreadR.angle * ARM_AMP)));
        }
        localPose[bi].rotation = QuaternionMultiply(localPose[bi].rotation, rot);
    }

    /* === LEFT ARM === */
    if (HAS_BONE(BONE_ARM_L)) {
        int bi = BONE_IDX(BONE_ARM_L);
        Quaternion rot = QuaternionIdentity();
        if (ragdollArmL) {
            float seed = ls->breathPhase * 2.31f + 1.5f;
            float jd = (deathTime < 2.0f) ? 1.0f : (deathTime < 5.0f) ? (5.0f-deathTime)/3.0f : 0.0f;
            float drift = sinf(deathTime * 0.5f + seed) * 15.0f;
            float jerk = sinf(deathTime * 22.0f + seed) * 35.0f * jd;
            rot = QuaternionMultiply(rot, RotFromAxisDeg((Vector3){1,0,0}, -(jerk + drift - 55.0f)));
            rot = QuaternionMultiply(rot, RotFromAxisDeg((Vector3){0,0,1}, -sinf(deathTime*0.6f+seed*1.7f)*30.0f));
            rot = QuaternionMultiply(rot, RotFromAxisDeg((Vector3){0,1,0}, sinf(deathTime*20.0f+seed)*20.0f*jd));
        } else {
            rot = QuaternionMultiply(rot, RotFromAxisDeg((Vector3){1, 0, 0},
                  -(ap->armBasePitch + ls->armSwingL.angle * ARM_AMP)));
            rot = QuaternionMultiply(rot, RotFromAxisDeg((Vector3){0, 0, 1},
                  (ap->armBaseSpread + ls->armSpreadL.angle * ARM_AMP)));
        }
        localPose[bi].rotation = QuaternionMultiply(localPose[bi].rotation, rot);
    }

    /* === RIGHT LEG === */
    if (HAS_BONE(BONE_LEG_R)) {
        int bi = BONE_IDX(BONE_LEG_R);
        Quaternion rot = QuaternionIdentity();
        if (ragdollLegR) {
            float seed = ls->breathPhase * 1.73f;
            float jd = (deathTime < 2.0f) ? 1.0f : (deathTime < 5.0f) ? (5.0f-deathTime)/3.0f : 0.0f;
            float drift = sinf(deathTime * 0.3f + seed) * 8.0f;
            float jerk = sinf(deathTime * 15.0f + seed) * 25.0f * jd;
            rot = QuaternionMultiply(rot, RotFromAxisDeg((Vector3){1,0,0}, -(jerk + drift)));
            rot = QuaternionMultiply(rot, RotFromAxisDeg((Vector3){0,0,1}, sinf(deathTime*0.4f+seed)*10.0f));
        } else {
            rot = QuaternionMultiply(rot, RotFromAxisDeg((Vector3){1, 0, 0}, -ls->legSwingR.angle * LEG_AMP));
            rot = QuaternionMultiply(rot, RotFromAxisDeg((Vector3){0, 0, 1}, -ls->legSpreadR.angle * LEG_AMP));
        }
        localPose[bi].rotation = QuaternionMultiply(localPose[bi].rotation, rot);
    }

    /* === RIGHT SHIN === */
    if (HAS_BONE(BONE_SHIN_R)) {
        int bi = BONE_IDX(BONE_SHIN_R);
        float knee = ragdollLegR ? sinf(deathTime*0.5f+ls->breathPhase)*20.0f
                                 : ap->kneeFlexBase + ls->kneeR.angle * LEG_AMP;
        localPose[bi].rotation = QuaternionMultiply(
            localPose[bi].rotation, RotFromAxisDeg((Vector3){1, 0, 0}, -knee));
    }

    /* === LEFT LEG === */
    if (HAS_BONE(BONE_LEG_L)) {
        int bi = BONE_IDX(BONE_LEG_L);
        Quaternion rot = QuaternionIdentity();
        if (ragdollLegL) {
            float seed = ls->breathPhase * 2.91f + 0.8f;
            float jd = (deathTime < 2.0f) ? 1.0f : (deathTime < 5.0f) ? (5.0f-deathTime)/3.0f : 0.0f;
            float drift = sinf(deathTime * 0.35f + seed) * 8.0f;
            float jerk = sinf(deathTime * 13.0f + seed) * 22.0f * jd;
            rot = QuaternionMultiply(rot, RotFromAxisDeg((Vector3){1,0,0}, -(jerk + drift)));
            rot = QuaternionMultiply(rot, RotFromAxisDeg((Vector3){0,0,1}, -sinf(deathTime*0.45f+seed)*10.0f));
        } else {
            rot = QuaternionMultiply(rot, RotFromAxisDeg((Vector3){1, 0, 0}, -ls->legSwingL.angle * LEG_AMP));
            rot = QuaternionMultiply(rot, RotFromAxisDeg((Vector3){0, 0, 1}, ls->legSpreadL.angle * LEG_AMP));
        }
        localPose[bi].rotation = QuaternionMultiply(localPose[bi].rotation, rot);
    }

    /* === LEFT SHIN === */
    if (HAS_BONE(BONE_SHIN_L)) {
        int bi = BONE_IDX(BONE_SHIN_L);
        float knee = ragdollLegL ? sinf(deathTime*0.6f+ls->breathPhase+1.0f)*20.0f
                                 : ap->kneeFlexBase + ls->kneeL.angle * LEG_AMP;
        localPose[bi].rotation = QuaternionMultiply(
            localPose[bi].rotation, RotFromAxisDeg((Vector3){1, 0, 0}, -knee));
    }

    /* === Extra deformation bones (signs flipped for Blender coord system) === */
    if (HAS_BONE(BONE_NECK) && !isDying) {
        int bi = BONE_IDX(BONE_NECK);
        Quaternion rot = QuaternionIdentity();
        rot = QuaternionMultiply(rot, RotFromAxisDeg((Vector3){1, 0, 0},
              -(ls->torsoLean.angle * 0.3f + ls->headPitch.angle * 0.4f)));
        rot = QuaternionMultiply(rot, RotFromAxisDeg((Vector3){0, 1, 0},
              -(ls->torsoTwist.angle * 0.3f + ls->headYaw.angle * 0.3f)));
        localPose[bi].rotation = QuaternionMultiply(localPose[bi].rotation, rot);
    }
    if (HAS_BONE(BONE_CHEST) && !isDying) {
        int bi = BONE_IDX(BONE_CHEST);
        Quaternion rot = QuaternionIdentity();
        rot = QuaternionMultiply(rot, RotFromAxisDeg((Vector3){1, 0, 0}, -ls->torsoLean.angle * 0.5f));
        rot = QuaternionMultiply(rot, RotFromAxisDeg((Vector3){0, 1, 0}, -ls->torsoTwist.angle * 0.5f));
        localPose[bi].rotation = QuaternionMultiply(localPose[bi].rotation, rot);
    }
    if (HAS_BONE(BONE_PELVIS) && !isDying) {
        int bi = BONE_IDX(BONE_PELVIS);
        localPose[bi].translation.x += ls->hipSway.angle * 0.015f;
        localPose[bi].rotation = QuaternionMultiply(localPose[bi].rotation,
            RotFromAxisDeg((Vector3){0, 1, 0}, ls->torsoTwist.angle * 0.2f));
    }
    /* Shoulders, forearms, feet: skip when dying -- they just follow
     * their parent bone (arm/leg) which is already ragdolling. */
    if (!isDying) {
        if (HAS_BONE(BONE_SHOULDER_R)) {
            int bi = BONE_IDX(BONE_SHOULDER_R);
            localPose[bi].rotation = QuaternionMultiply(localPose[bi].rotation,
                RotFromAxisDeg((Vector3){1, 0, 0}, -(ap->armBasePitch + ls->armSwingR.angle) * 0.3f));
        }
        if (HAS_BONE(BONE_SHOULDER_L)) {
            int bi = BONE_IDX(BONE_SHOULDER_L);
            localPose[bi].rotation = QuaternionMultiply(localPose[bi].rotation,
                RotFromAxisDeg((Vector3){1, 0, 0}, -(ap->armBasePitch + ls->armSwingL.angle) * 0.3f));
        }
        if (HAS_BONE(BONE_FOREARM_R)) {
            int bi = BONE_IDX(BONE_FOREARM_R);
            localPose[bi].rotation = QuaternionMultiply(localPose[bi].rotation,
                RotFromAxisDeg((Vector3){1, 0, 0}, -(ap->armBasePitch + ls->armSwingR.angle) * 0.4f));
        }
        if (HAS_BONE(BONE_FOREARM_L)) {
            int bi = BONE_IDX(BONE_FOREARM_L);
            localPose[bi].rotation = QuaternionMultiply(localPose[bi].rotation,
                RotFromAxisDeg((Vector3){1, 0, 0}, -(ap->armBasePitch + ls->armSwingL.angle) * 0.4f));
        }
        if (HAS_BONE(BONE_FOOT_R)) {
            int bi = BONE_IDX(BONE_FOOT_R);
            float toeUp = ls->legSwingR.angle > 5.0f ? 15.0f : 0.0f;
            localPose[bi].rotation = QuaternionMultiply(localPose[bi].rotation,
                RotFromAxisDeg((Vector3){1, 0, 0}, toeUp));
        }
        if (HAS_BONE(BONE_FOOT_L)) {
            int bi = BONE_IDX(BONE_FOOT_L);
            float toeUp = ls->legSwingL.angle > 5.0f ? 15.0f : 0.0f;
            localPose[bi].rotation = QuaternionMultiply(localPose[bi].rotation,
                RotFromAxisDeg((Vector3){1, 0, 0}, toeUp));
        }
    }

    #undef BONE_IDX
    #undef HAS_BONE

    /* ================================================================
     * Step 3: Walk hierarchy root->leaf to compute world-space poses
     * worldPose[b] = worldPose[parent] * localPose[b]
     * ================================================================ */
    Transform worldPose[32];
    for (int b = 0; b < bc; b++) {
        int parent = m->bones[b].parent;
        if (parent >= 0 && parent < bc) {
            worldPose[b] = CombineTransforms(worldPose[parent], localPose[b]);
        } else {
            worldPose[b] = localPose[b];
        }
    }

    /* ================================================================
     * Step 4: Compute bone matrices
     * boneMatrix[b] = inverse(bindWorldPose[b]) * currentWorldPose[b]
     *
     * We use matrix form for this final step since it's what the GPU needs.
     * ================================================================ */
    for (int mi = 0; mi < m->meshCount; mi++) {
        if (!m->meshes[mi].boneMatrices) continue;

        for (int b = 0; b < bc; b++) {
            Matrix bindWorld = TransformToMatrix(m->bindPose[b]);
            Matrix curWorld  = TransformToMatrix(worldPose[b]);
            m->meshes[mi].boneMatrices[b] = MatrixMultiply(MatrixInvert(bindWorld), curWorld);
        }
    }
}

Vector3 AstroModelGetBonePosition(const AstroModel *am, BoneSlot slot) {
    if (!am || !am->loaded || slot < 0 || slot >= BONE_SLOT_COUNT) {
        return (Vector3){0, 0, 0};
    }

    int bi = am->bones[slot];
    if (bi < 0 || bi >= am->model.boneCount || !am->model.meshes[0].boneMatrices) {
        return (Vector3){0, 0, 0};
    }

    /* The bone matrix transforms from bind-space to current-space.
     * To get the bone's current model-space position, transform
     * the bind-pose position through the bone matrix. */
    Vector3 bindPos = am->model.bindPose[bi].translation;
    Matrix boneMat = am->model.meshes[0].boneMatrices[bi];
    return Vector3Transform(bindPos, boneMat);
}
