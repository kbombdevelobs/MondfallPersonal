#include "enemy_model_loader.h"
#include "config.h"
#include "rlgl.h"
#include "raymath.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* ============================================================================
 * Bone name table -- must match skeleton.py bone names exactly
 * ============================================================================ */

static const char *BONE_SLOT_NAMES[BONE_SLOT_COUNT] = {
    [BONE_ROOT]       = "root",
    [BONE_SPINE]      = "spine",
    [BONE_HEAD]       = "head",
    [BONE_BACKPACK]   = "backpack",
    [BONE_ARM_R]      = "arm_R",
    [BONE_HAND_R]     = "hand_R",
    [BONE_ARM_L]      = "arm_L",
    [BONE_HAND_L]     = "hand_L",
    [BONE_LEG_R]      = "leg_R",
    [BONE_SHIN_R]     = "shin_R",
    [BONE_LEG_L]      = "leg_L",
    [BONE_SHIN_L]     = "shin_L",
    [BONE_NECK]       = "neck",
    [BONE_PELVIS]     = "pelvis",
    [BONE_FOREARM_R]  = "forearm_R",
    [BONE_FOREARM_L]  = "forearm_L",
    [BONE_FOOT_R]     = "foot_R",
    [BONE_FOOT_L]     = "foot_L",
    [BONE_CHEST]      = "chest",
    [BONE_SHOULDER_R] = "shoulder_R",
    [BONE_SHOULDER_L] = "shoulder_L",
};

/* Faction/rank filename fragments -- 2 factions only (no friendly) */
static const char *FACTION_NAMES[2] = { "soviet", "american" };
static const char *RANK_NAMES[3]    = { "trooper", "nco", "officer" };
static const char *GUN_TYPES[2]     = { "trooper", "officer" };

/* ============================================================================
 * Internal helpers
 * ============================================================================ */

/// Look up bone indices by name after model load.
static void CacheBoneIndices(AstroModel *am) {
    for (int s = 0; s < BONE_SLOT_COUNT; s++) {
        am->bones[s] = -1;
    }

    if (!am->loaded || am->model.boneCount <= 0) return;

    for (int b = 0; b < am->model.boneCount; b++) {
        const char *name = am->model.bones[b].name;
        for (int s = 0; s < BONE_SLOT_COUNT; s++) {
            if (strcmp(name, BONE_SLOT_NAMES[s]) == 0) {
                am->bones[s] = b;
                break;
            }
        }
    }

    /* Validate that all required bones were found */
    for (int s = 0; s < BONE_SLOT_COUNT; s++) {
        if (am->bones[s] < 0) {
            TraceLog(LOG_WARNING, "AstroModel: missing bone '%s'", BONE_SLOT_NAMES[s]);
        }
    }
}

/// Try to load a single .glb model. Returns true on success.
bool AstroModelLoadFromPath(AstroModel *am, const char *path) {
    memset(am, 0, sizeof(*am));
    am->loaded = false;
    for (int s = 0; s < BONE_SLOT_COUNT; s++) am->bones[s] = -1;

    if (!FileExists(path)) {
        return false;
    }

    am->model = LoadModel(path);
    if (am->model.meshCount <= 0) {
        TraceLog(LOG_WARNING, "AstroModel: failed to load '%s' (0 meshes)", path);
        return false;
    }

    am->loaded = true;

    /* Manually allocate boneMatrices for each mesh.
     * raylib only allocates these when LoadModelAnimations is called,
     * but we drive bones procedurally via AstroModelApplySpringState.
     * Without this, all bone-driven rendering silently does nothing. */
    if (am->model.boneCount > 0) {
        for (int mi = 0; mi < am->model.meshCount; mi++) {
            if (!am->model.meshes[mi].boneMatrices) {
                am->model.meshes[mi].boneMatrices =
                    (Matrix *)RL_CALLOC(am->model.boneCount, sizeof(Matrix));
                /* Initialize to identity so the model renders correctly
                 * even before the first ApplySpringState call */
                for (int b = 0; b < am->model.boneCount; b++) {
                    am->model.meshes[mi].boneMatrices[b] = MatrixIdentity();
                }
            }
        }
    }

    CacheBoneIndices(am);

    /* Diagnostic: verify skinning data is complete */
    TraceLog(LOG_INFO, "AstroModel loaded: %s (%d meshes, %d bones, bindPose=%s)",
         path, am->model.meshCount, am->model.boneCount,
         am->model.bindPose ? "OK" : "NULL");
    for (int mi = 0; mi < am->model.meshCount; mi++) {
        Mesh *mesh = &am->model.meshes[mi];
        TraceLog(LOG_INFO, "  mesh[%d]: verts=%d, boneIds=%s, boneWeights=%s, boneMatrices=%s",
             mi, mesh->vertexCount,
             mesh->boneIds ? "OK" : "NULL",
             mesh->boneWeights ? "OK" : "NULL",
             mesh->boneMatrices ? "OK" : "NULL");
        if (!mesh->boneIds || !mesh->boneWeights) {
            TraceLog(LOG_WARNING, "  mesh[%d]: MISSING joint/weight data -- GPU skinning won't work!", mi);
        }
    }

    return true;
}

/// Unload a single model if loaded.
static void UnloadSingleModel(AstroModel *am) {
    if (am->loaded) {
        UnloadModel(am->model);
        am->loaded = false;
    }
}

/* ============================================================================
 * Public API
 * ============================================================================ */

void AstroModelsLoad(AstroModelSet *set) {
    if (!set) { TraceLog(LOG_WARNING, "AstroModelsLoad: null set"); return; }
    memset(set, 0, sizeof(*set));
    set->available = false;

    char path[256];
    int loaded_count = 0;

    /* Load character models: LOD0 and LOD1 */
    for (int f = 0; f < 2; f++) {
        for (int r = 0; r < 3; r++) {
            /* LOD0 */
            snprintf(path, sizeof(path), "%scharacters/%s_%s.glb",
                     MODEL_PATH_PREFIX, FACTION_NAMES[f], RANK_NAMES[r]);
            if (AstroModelLoadFromPath(&set->characters[f][r], path)) {
                loaded_count++;
            }

            /* LOD1 */
            snprintf(path, sizeof(path), "%scharacters/%s_%s_lod1.glb",
                     MODEL_PATH_PREFIX, FACTION_NAMES[f], RANK_NAMES[r]);
            if (AstroModelLoadFromPath(&set->charactersLOD1[f][r], path)) {
                loaded_count++;
            }
        }
    }

    /* Load gun models */
    for (int f = 0; f < 2; f++) {
        for (int g = 0; g < 2; g++) {
            snprintf(path, sizeof(path), "%sguns/gun_%s_%s.glb",
                     MODEL_PATH_PREFIX, FACTION_NAMES[f], GUN_TYPES[g]);
            if (AstroModelLoadFromPath(&set->guns[f][g], path)) {
                loaded_count++;
            }
        }
    }

    /* Load dismemberment models */
    static const char *DISMEMBER_NAMES[DISMEMBER_COUNT] = {
        "head", "torso", "arm_r", "arm_l", "leg_r", "leg_l"
    };
    for (int f = 0; f < 2; f++) {
        for (int d = 0; d < DISMEMBER_COUNT; d++) {
            snprintf(path, sizeof(path), "%sdismemberment/%s_%s.glb",
                     MODEL_PATH_PREFIX, FACTION_NAMES[f], DISMEMBER_NAMES[d]);
            if (AstroModelLoadFromPath(&set->dismember[f][d], path)) {
                loaded_count++;
            }
        }
    }

    /* Load per-rank headshot models */
    for (int f = 0; f < 2; f++) {
        for (int r = 0; r < 3; r++) {
            snprintf(path, sizeof(path), "%sdismemberment/%s_%s_headshot.glb",
                     MODEL_PATH_PREFIX, FACTION_NAMES[f], RANK_NAMES[r]);
            if (AstroModelLoadFromPath(&set->headshot[f][r], path)) {
                loaded_count++;
            }
        }
    }

    /* Load GPU skinning shader and assign to all character models */
    set->skinShaderLoaded = false;
    if (FileExists("resources/skinning.vs") && FileExists("resources/skinning.fs")) {
        set->skinShader = LoadShader("resources/skinning.vs", "resources/skinning.fs");

        if (set->skinShader.id > 0) {
            /* Get the bone matrices uniform location */
            set->skinShader.locs[SHADER_LOC_BONE_MATRICES] =
                GetShaderLocation(set->skinShader, "boneMatrices");
            set->skinShader.locs[SHADER_LOC_MATRIX_MVP] =
                GetShaderLocation(set->skinShader, "mvp");
            set->skinShader.locs[SHADER_LOC_COLOR_DIFFUSE] =
                GetShaderLocation(set->skinShader, "colDiffuse");
            set->skinShader.locs[SHADER_LOC_MAP_DIFFUSE] =
                GetShaderLocation(set->skinShader, "texture0");

            set->skinShaderLoaded = true;
            TraceLog(LOG_INFO, "Skinning shader loaded: boneLoc=%d",
                     set->skinShader.locs[SHADER_LOC_BONE_MATRICES]);

            /* Assign skinning shader to all character models (LOD0 + LOD1) */
            for (int f = 0; f < 2; f++) {
                for (int r = 0; r < 3; r++) {
                    if (set->characters[f][r].loaded) {
                        for (int mi = 0; mi < set->characters[f][r].model.materialCount; mi++) {
                            set->characters[f][r].model.materials[mi].shader = set->skinShader;
                        }
                    }
                    if (set->charactersLOD1[f][r].loaded) {
                        for (int mi = 0; mi < set->charactersLOD1[f][r].model.materialCount; mi++) {
                            set->charactersLOD1[f][r].model.materials[mi].shader = set->skinShader;
                        }
                    }
                }
            }

            /* No skinning shader needed for damaged heads (static meshes) */
        } else {
            TraceLog(LOG_WARNING, "Skinning shader failed to compile");
        }
    } else {
        TraceLog(LOG_WARNING, "Skinning shader files not found -- bone animation disabled");
    }

    set->available = (loaded_count > 0);
    TraceLog(LOG_INFO, "AstroModels: loaded %d model files (available=%s)",
             loaded_count, set->available ? "true" : "false");
}

void AstroModelsUnload(AstroModelSet *set) {
    if (!set) { TraceLog(LOG_WARNING, "AstroModelsUnload: null set"); return; }

    for (int f = 0; f < 2; f++) {
        for (int r = 0; r < 3; r++) {
            UnloadSingleModel(&set->characters[f][r]);
            UnloadSingleModel(&set->charactersLOD1[f][r]);
        }
    }
    for (int f = 0; f < 2; f++) {
        for (int g = 0; g < 2; g++) {
            UnloadSingleModel(&set->guns[f][g]);
        }
    }
    for (int f = 0; f < 2; f++) {
        for (int d = 0; d < DISMEMBER_COUNT; d++) {
            UnloadSingleModel(&set->dismember[f][d]);
        }
    }
    for (int f = 0; f < 2; f++) {
        for (int r = 0; r < 3; r++) {
            UnloadSingleModel(&set->headshot[f][r]);
        }
    }
    if (set->skinShaderLoaded) {
        UnloadShader(set->skinShader);
        set->skinShaderLoaded = false;
    }

    set->available = false;
    TraceLog(LOG_INFO, "AstroModels: unloaded all models");
}

AstroModel *AstroModelGetGun(AstroModelSet *set, int faction, int rank) {
    if (!set) return NULL;
    if (faction < 0 || faction >= 2) return NULL;

    /* NCO (rank=1) uses trooper gun (index 0), officer (rank=2) uses officer gun (index 1) */
    int gun_idx = (rank >= 2) ? 1 : 0;

    AstroModel *gun = &set->guns[faction][gun_idx];
    return gun->loaded ? gun : NULL;
}
