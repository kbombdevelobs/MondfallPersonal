#ifndef STRUCTURE_MODEL_H
#define STRUCTURE_MODEL_H

#include "raylib.h"
#include <stdbool.h>

/* ============================================================================
 * Structure Model Loader
 *
 * Loads .glb models for moonbase exterior and interior rendering.
 * Falls back gracefully to procedural rendering if model files are missing.
 * ============================================================================ */

typedef struct {
    Model exteriorModel;
    Model interiorModel;
    bool exteriorLoaded;
    bool interiorLoaded;
    bool available;          // true if at least one model loaded
} StructureModelSet;

/// Load structure models from resources/models/structures/*.glb.
/// Safe to call even if files don't exist — sets loaded flags accordingly.
void StructureModelsLoad(StructureModelSet *set);

/// Unload any loaded structure models.
void StructureModelsUnload(StructureModelSet *set);

/// Get the global singleton model set (initialized by StructureModelsLoad).
StructureModelSet *StructureModelsGet(void);

/// Draw the exterior model at the given position. Returns true if drawn.
/// If the model is not loaded, returns false so the caller can fall back
/// to procedural rendering.
bool StructureModelDrawExterior(StructureModelSet *set, Vector3 position);

/// Draw the interior model at the given Y offset. Returns true if drawn.
/// If the model is not loaded, returns false for procedural fallback.
bool StructureModelDrawInterior(StructureModelSet *set, float interiorY);

#endif
