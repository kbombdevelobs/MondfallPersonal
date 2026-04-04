# World System — src/world/

Split from `world.c` (was 660 lines) into subfolder per 500-line rule.

## Files

| File | Lines | Purpose |
|------|-------|---------|
| `world_noise.h/c` | ~163 | Noise functions: Perlin gradient noise, domain warping, rilles, maria biomes, `WorldGetHeight()`, `WorldGetMareFactor()` |
| `world_draw.h/c` | ~190 | All world rendering: `WorldDrawSky()` (stars, Earth billboard), `DrawChunk()` (boulders, crater rims), `WorldDraw()` (frustum culling + chunk iteration) |

## Remaining in src/world.c (~375 lines)

Core chunk management: texture generation, crater profiles (terraced walls + central peaks), terrain mesh generation (vertices, normals, slope-based vertex colors, baked sun lighting, maria darkening), chunk create/find/evict, `WorldInit`, `WorldUpdate`, `WorldUnload`, `WorldCheckCollision`.

## Key Types (defined in src/world.h)

- `World` — global state: chunk array, textures, star positions
- `Chunk` — one terrain tile: mesh, craters, rocks, grid coordinates
- `Crater` — position, radius, depth
- `Rock` — position, size, rotation, color

## Key Functions

| Function | File | Description |
|----------|------|-------------|
| `WorldGetHeight(x, z)` | world_noise.c | Domain-warped 4-octave Perlin noise + rilles + maria flattening |
| `WorldGetMareFactor(x, z)` | world_noise.c | Cell-noise biome factor: 0 = highlands, 1 = mare center |
| `GradientNoise(x, z)` | world_noise.c | Core Perlin noise with permutation table |
| `RilleDepth(x, z, seed)` | world_noise.c | Sinuous lunar channel depth (static) |
| `WorldDraw(world, pos, cam)` | world_draw.c | Renders visible chunks with 4-corner frustum culling |
| `WorldDrawSky(world, cam)` | world_draw.c | Stars + Earth billboard |
| `WorldInit/Update/Unload` | world.c | Lifecycle management |
| `WorldCheckCollision` | world.c | Boulder AABB collision (X/Z only, NULL-safe) |
| `CraterProfile(t, r, depth)` | world.c | Terraced walls + central peaks for large craters |
| `CraterHeight` | world.c | Min-depth crater overlay across chunk borders |
| `GenTerrainMesh` | world.c | Mesh gen with slope colors, maria darkening, baked sun lighting |
