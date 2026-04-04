---
title: World System
status: Active
owner_area: World
created: 2026-04-04
last_updated: 2026-04-04
last_reviewed: 2026-04-04
parent_doc: CLAUDE.md
related_docs:
  - docs/rendering-pipeline.md
---

# World System -- src/world/

Terrain noise generation and world rendering, split from `world.c` (was 660 lines) per the 500-line rule. Core chunk management, crater profiles, mesh generation, and collision remain in the parent files.

## Files

| File | Lines | Purpose |
|------|-------|---------|
| `world_noise.h` | ~20 | Exports WorldGetHeight, WorldGetMareFactor, GradientNoise, RilleDepth |
| `world_noise.c` | ~145 | Noise functions: Perlin gradient noise with permutation table, domain warping (4 octaves), sinuous rille channels, cell-noise maria biomes. WorldGetHeight is the single source of truth for terrain elevation. |
| `world_draw.h` | ~15 | Exports WorldDraw, WorldDrawSky, DrawChunk |
| `world_draw.c` | ~190 | All world rendering: WorldDrawSky (stars, Earth billboard), DrawChunk (boulders, crater rims), WorldDraw (frustum culling with 4-corner test + chunk iteration) |

## Parent Files

| File | Lines | Purpose |
|------|-------|---------|
| `src/world.c` | ~375 | Core chunk management: WorldInit/Update/Unload, texture generation, CraterProfile (terraced walls + central peaks for radius > 5), CraterHeight (min-depth overlap), GenTerrainMesh (vertices, analytical normals, slope-based vertex colors, baked sun lighting, maria darkening, ejecta rays, wrinkle ridges), chunk create/find/evict, WorldCheckCollision (boulder AABB, X/Z only) |
| `src/world.h` | ~60 | Defines World, Chunk, Crater, Rock structs; full public API |

## Terrain Features

| Feature | Implementation | Location |
|---------|---------------|----------|
| **Base terrain** | 4-octave domain-warped Perlin gradient noise | world_noise.c: WorldGetHeight |
| **Domain warping** | Large-scale octaves fed through secondary noise to break repetition | world_noise.c: WorldGetHeight |
| **Rilles** | Two sinuous lunar channels at different angles carved into heightmap | world_noise.c: RilleDepth |
| **Maria** | Cell-noise biome regions where terrain flattens and darkens (dark basalt seas) | world_noise.c: WorldGetMareFactor |
| **Craters** | Terraced walls + central peaks (radius > 5), simple bowls (small); min-depth overlap | world.c: CraterProfile, CraterHeight |
| **Wrinkle ridges** | Narrow raised linear features in mare regions, modulated along length | world.c: GenTerrainMesh |
| **Ejecta rays** | Bright radial vertex color streaks around large craters (radius > 4.5) | world.c: GenTerrainMesh |
| **Boulders** | Sphere-cluster rocks with LOD (detail reduces with distance); collision is X/Z only | world.c + world_draw.c |
| **Baked lighting** | Analytical normals from WorldGetHeight with low-angle sun dot product (0.15 ambient floor) | world.c: GenTerrainMesh |
| **Slope coloring** | Steep terrain darkened by sampling 4 neighboring heights | world.c: GenTerrainMesh |
| **Horizon fog** | Subtle radial fade to black at screen edges | resources/crt.fs (CRT shader) |

## Key Functions

| Function | File | Description |
|----------|------|-------------|
| `WorldGetHeight(x, z)` | world_noise.c | Domain-warped 4-octave Perlin noise + rilles + maria flattening. Deterministic -- same input always returns same height. Used everywhere (enemy spawning, player physics, blood pools, boulder placement). |
| `WorldGetMareFactor(x, z)` | world_noise.c | Cell-noise biome factor: 0 = highlands, 1 = mare center. Controls terrain flattening and vertex color darkening. |
| `GradientNoise(x, z)` | world_noise.c | Core Perlin noise with permutation table |
| `RilleDepth(x, z, seed)` | world_noise.c | Sinuous lunar channel depth (static inline) |
| `WorldInit` | world.c | Allocate chunk array, generate textures, star positions |
| `WorldUpdate` | world.c | Lazy chunk generation (MAX_CHUNKS_PER_FRAME = 3), eviction of distant chunks |
| `WorldUnload` | world.c | Free all chunks, textures, meshes |
| `WorldDraw(world, pos, cam)` | world_draw.c | Render visible chunks with 4-corner frustum culling (~80 degree cone) |
| `WorldDrawSky(world, cam)` | world_draw.c | Stars + Earth billboard |
| `WorldCheckCollision` | world.c | Boulder AABB collision (X/Z only, NULL-safe). Players can jump over rocks. |
| `CraterProfile(t, r, depth)` | world.c | Terraced walls + central peaks for large craters (radius > 5), simple bowls for small |
| `CraterHeight` | world.c | Min-depth crater overlay across chunk borders |
| `GenTerrainMesh` | world.c | Full mesh generation: vertices, normals, slope colors, maria darkening, baked sun lighting, ejecta rays, wrinkle ridges |

## Chunk System

- **Chunk size:** 60x60 units (CHUNK_SIZE constant)
- **Render grid:** 5x5 chunks (RENDER_CHUNKS), 25 chunks visible at once
- **Lazy generation:** max 3 chunks created per frame (MAX_CHUNKS_PER_FRAME) to avoid frame spikes
- **Infinite terrain:** chunks created/evicted as player moves; deterministic noise means revisited areas are identical

## Dependencies

- **config.h** -- CHUNK_SIZE, RENDER_CHUNKS, MAX_CHUNKS_PER_FRAME, and other world constants
- **raylib** -- Mesh generation (GenMeshSphere for boulders), DrawMesh, DrawModel, rlPushMatrix/rlPopMatrix for transforms

## Connections

- **rendering-pipeline.md** -- World renders to the 640x360 low-res render target; CRT shader post-processes including horizon fog
- The **enemy system** depends on WorldGetHeight for terrain-clamped spawning and death animations
- The **player system** depends on WorldGetHeight for ground collision and WorldCheckCollision for boulder collision
- The **lander system** depends on WorldGetHeight for landing altitude

## Maintenance Notes

- `WorldGetHeight()` is the most-called function in the codebase (every entity every frame). Keep it fast. Avoid adding branches or expensive operations.
- Domain warp strength is controlled by a 0.15 multiplier in WorldGetHeight -- adjust for different terrain character. Octave scales/amplitudes are tuned for lunar realism.
- Mare cell noise scale (0.002) controls biome region size. Smaller values = larger maria.
- Rille angles and widths are hardcoded in RilleDepth. Add new rilles by duplicating the pattern with different seed/angle parameters.
- Sun direction vector is set in GenTerrainMesh -- modify for different shadow angles.
- Boulder sizes are configured in GenerateChunk in world.c.
- The CRT shader horizon fog is in `resources/crt.fs` (search for "HORIZON FOG").
- Frustum culling tests all 4 chunk corners and only culls chunks fully behind camera. This is conservative -- some off-screen chunks render, but no visible popping.
