#include "structure_model.h"
#include "config.h"
#include "raymath.h"
#include <string.h>
#include <stdio.h>

/* ============================================================================
 * Structure Model Loader
 *
 * Loads vertex-colored .glb models for moonbase exterior and interior.
 * Mirrors the pattern used by weapon_model.c and enemy_model_loader.c.
 * ============================================================================ */

#define STRUCTURE_MODEL_PATH "resources/models/structures/"

static StructureModelSet g_structureModels = {0};

/* --------------------------------------------------------------------------
 * Internal helpers
 * -------------------------------------------------------------------------- */

/// Try to load a single .glb model. Returns true on success.
static bool LoadStructureModel(Model *out, const char *filename) {
    if (!out || !filename) return false;

    char path[256];
    snprintf(path, sizeof(path), "%s%s", STRUCTURE_MODEL_PATH, filename);

    if (!FileExists(path)) {
        TraceLog(LOG_INFO, "StructureModel: file not found: %s (will use procedural fallback)", path);
        return false;
    }

    *out = LoadModel(path);
    if (out->meshCount <= 0) {
        TraceLog(LOG_WARNING, "StructureModel: failed to load: %s", path);
        return false;
    }

    TraceLog(LOG_INFO, "StructureModel: loaded %s (%d meshes)", path, out->meshCount);
    return true;
}

/* --------------------------------------------------------------------------
 * Public API
 * -------------------------------------------------------------------------- */

void StructureModelsLoad(StructureModelSet *set) {
    if (!set) return;

    memset(set, 0, sizeof(*set));
    set->available = false;
    int loaded = 0;

    if (LoadStructureModel(&set->exteriorModel, "moonbase_exterior.glb")) {
        set->exteriorLoaded = true;
        loaded++;
    }

    if (LoadStructureModel(&set->interiorModel, "moonbase_interior.glb")) {
        set->interiorLoaded = true;
        loaded++;
    }

    if (loaded > 0) {
        set->available = true;
        TraceLog(LOG_INFO, "StructureModel: %d/2 structure models loaded", loaded);
    } else {
        TraceLog(LOG_INFO, "StructureModel: no structure models found, using procedural rendering");
    }
}

void StructureModelsUnload(StructureModelSet *set) {
    if (!set) return;

    if (set->exteriorLoaded) {
        UnloadModel(set->exteriorModel);
        set->exteriorLoaded = false;
    }
    if (set->interiorLoaded) {
        UnloadModel(set->interiorModel);
        set->interiorLoaded = false;
    }
    set->available = false;
}

StructureModelSet *StructureModelsGet(void) {
    return &g_structureModels;
}

bool StructureModelDrawExterior(StructureModelSet *set, Vector3 position) {
    if (!set || !set->exteriorLoaded) return false;

    DrawModel(set->exteriorModel, position, 1.0f, WHITE);
    return true;
}

bool StructureModelDrawInterior(StructureModelSet *set, float interiorY) {
    if (!set || !set->interiorLoaded) return false;

    Vector3 pos = {0.0f, interiorY, 0.0f};
    DrawModel(set->interiorModel, pos, 1.0f, WHITE);
    return true;
}
