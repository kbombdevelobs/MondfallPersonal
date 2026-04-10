#ifndef WORLD_ROCKS_H
#define WORLD_ROCKS_H

#include "world.h"

/* ============================================================================
 * Rock model loader for Blender-exported .glb rock models.
 *
 * Loads vertex-colored rock models from resources/models/rocks/.
 * Falls back gracefully to the existing procedural sphere-cluster rendering
 * if model files are missing.
 * ============================================================================ */

/// Load rock .glb models (small, medium, large) into world->rockModels[].
/// Sets world->rockModelCount to the number successfully loaded.
/// Safe to call when model files don't exist -- rockModelCount stays 0.
void WorldRocksInit(World *world);

/// Unload all loaded rock models and reset rockModelCount.
void WorldRocksUnload(World *world);

/// Draw a rock using a loaded .glb model if available, otherwise fall back
/// to the procedural sphere-cluster rendering.
/// @param world       World with rock models and texture
/// @param rock        Rock data (position, size, rotation, color)
/// @param playerPos   Player position for LOD/distance culling
/// @param chunkSize   Chunk size for distance culling threshold
void WorldRockDraw(World *world, Rock *rock, Vector3 playerPos, float chunkSize);

#endif /* WORLD_ROCKS_H */
