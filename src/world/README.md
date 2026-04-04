# World System — src/world/

Split from `world.c` (was 660 lines) into subfolder per 500-line rule.

## Files

| File | Lines | Purpose |
|------|-------|---------|
| `world_noise.h/c` | ~106 | Noise functions: Perlin gradient noise (permutation table, quintic fade), legacy ValueNoise/Hash2D for texture gen, `WorldGetHeight()` |
| `world_draw.h/c` | ~187 | All world rendering: `WorldDrawSky()` (stars, Earth billboard), `DrawChunk()` (boulders, crater rims), `WorldDraw()` (frustum culling + chunk iteration) |

## Remaining in src/world.c (~338 lines)

Core chunk management: texture generation, crater height computation, terrain mesh generation (vertices, normals, slope-based vertex colors, baked sun lighting), chunk create/find/evict, `WorldInit`, `WorldUpdate`, `WorldUnload`, `WorldCheckCollision`.

## Key Types (defined in src/world.h)

- `World` — global state: chunk array, textures, star positions
- `Chunk` — one terrain tile: mesh, craters, rocks, grid coordinates
- `Crater` — position, radius, depth
- `Rock` — position, size, rotation, color

## Key Functions

| Function | File | Description |
|----------|------|-------------|
| `WorldGetHeight(x, z)` | world_noise.c | 4-octave Perlin gradient noise heightmap |
| `GradientNoise(x, z)` | world_noise.c | Core Perlin noise with permutation table |
| `WorldDraw(world, pos, cam)` | world_draw.c | Renders visible chunks with frustum culling |
| `WorldDrawSky(world, cam)` | world_draw.c | Stars + Earth billboard |
| `WorldInit/Update/Unload` | world.c | Lifecycle management |
| `WorldCheckCollision` | world.c | Boulder AABB collision (X/Z only) |
| `GenTerrainMesh` | world.c | Mesh gen with slope colors + baked sun lighting |
| `CraterHeight` | world.c | Min-depth crater overlay across chunk borders |
