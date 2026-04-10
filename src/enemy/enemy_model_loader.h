#ifndef ENEMY_MODEL_LOADER_H
#define ENEMY_MODEL_LOADER_H

#include "raylib.h"
#include <stdbool.h>

/* ============================================================================
 * Skeletal model loader for Blender-exported astronaut models.
 *
 * Loads .glb files from resources/models/ with 12-bone armatures.
 * Caches bone indices by name for fast runtime access.
 * Falls back gracefully if model files are missing.
 *
 * Adapted for MondfallPersonal: 2 factions (Soviet + American), no Friendly.
 * ============================================================================ */

/// Body part bone slots matching the Blender armature.
/// Core 12 bones are directly driven by spring-damper system.
/// Extra 9 bones improve deformation and receive inherited motion.
typedef enum {
    /* --- Core 12 (directly driven) --- */
    BONE_ROOT = 0,
    BONE_SPINE,
    BONE_HEAD,
    BONE_BACKPACK,
    BONE_ARM_R,
    BONE_HAND_R,
    BONE_ARM_L,
    BONE_HAND_L,
    BONE_LEG_R,
    BONE_SHIN_R,
    BONE_LEG_L,
    BONE_SHIN_L,
    /* --- Extra deformation bones --- */
    BONE_NECK,
    BONE_PELVIS,
    BONE_FOREARM_R,
    BONE_FOREARM_L,
    BONE_FOOT_R,
    BONE_FOOT_L,
    BONE_CHEST,
    BONE_SHOULDER_R,
    BONE_SHOULDER_L,
    BONE_SLOT_COUNT   /* 21 */
} BoneSlot;

/// A loaded skeletal character model with cached bone indices.
typedef struct {
    Model model;
    bool  loaded;
    int   bones[BONE_SLOT_COUNT];  /* index into model.bones[], -1 if not found */
} AstroModel;

/// Dismemberment body part indices.
typedef enum {
    DISMEMBER_HEAD = 0,
    DISMEMBER_TORSO,
    DISMEMBER_ARM_R,
    DISMEMBER_ARM_L,
    DISMEMBER_LEG_R,
    DISMEMBER_LEG_L,
    DISMEMBER_COUNT
} DismemberPart;

/// Complete set of all astronaut, gun, and dismemberment models.
/// Array sizes: [2] for factions (Soviet=0, American=1), [3] for ranks.
typedef struct AstroModelSet {
    AstroModel characters[2][3];      /* [faction][rank] LOD0 */
    AstroModel charactersLOD1[2][3];  /* [faction][rank] LOD1 */
    AstroModel guns[2][2];            /* [faction][0=trooper/nco, 1=officer] */
    AstroModel dismember[2][DISMEMBER_COUNT]; /* [faction][part] */
    AstroModel headshot[2][3];        /* [faction][rank] per-rank headshot models */
    Shader     skinShader;            /* GPU skinning shader for bone animation */
    bool       skinShaderLoaded;      /* true if skinning shader loaded OK */
    bool       available;             /* true if any models loaded */
} AstroModelSet;

/// Load a single .glb model from the given path. Returns true on success.
bool AstroModelLoadFromPath(AstroModel *am, const char *path);

/// Load all astronaut models from MODEL_PATH_PREFIX. Safe to call even if
/// directory is missing -- sets available=false and all loaded=false.
void AstroModelsLoad(AstroModelSet *set);

/// Unload all loaded models and reset the set.
void AstroModelsUnload(AstroModelSet *set);

/// Get the gun model for a given faction and rank.
/// NCO uses the trooper gun. Returns NULL if not loaded.
AstroModel *AstroModelGetGun(AstroModelSet *set, int faction, int rank);

#endif /* ENEMY_MODEL_LOADER_H */
