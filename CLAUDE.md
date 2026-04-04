# Mondfall — Nazis on the Moon FPS

## What This Is
A raylib-based first-person shooter set on the Moon. You play as a Nazi moon defender fighting waves of Soviet and American cosmowarrior astronauts who arrive in Apollo-style landers.

## Tech Stack
- **Language:** C (C99)
- **Framework:** raylib 5.5
- **Platform:** macOS ARM64 (Apple Silicon)
- **Build:** `make` (clang)
- **Audio:** Procedurally generated (no audio files required except optional death sounds)

## Quick Start
```bash
# Requires: raylib installed via homebrew
brew install raylib

# Build and run
make run

# Just build
make

# Run unit tests (ALWAYS run before committing)
make test

# Clean
make clean
```

## Testing Rules
- **Always run `make test` before committing.** All tests must pass.
- Tests are in `tests/test_game.c` — ~35 tests covering config sanity, player physics, weapon logic, pickup system, enemy hit detection, world height, and game state.
- Tests run without GPU/window — they exercise pure game logic only.
- When adding new gameplay features, add corresponding tests.

## Project Structure
```
Mondfall/
├── src/
│   ├── main.c          — Game loop, state machine, input, rendering pipeline
│   ├── player.c/h      — FPS camera, WASD + mouse, moon gravity, jump, ground pound
│   ├── weapon.c/h      — 3 weapons: Mond-MP40, Raketenfaust beam, Jackhammer
│   ├── enemy.c/h       — Astronaut enemies, AI (Soviet rush / American tactical), animations
│   ├── world.c/h       — Infinite chunked terrain, heightmap, craters, boulders, sky
│   ├── lander.c/h      — Moon lander wave system with descent, deployment, self-destruct
│   ├── pickup.c/h      — Dropped enemy weapons (KOSMOS-7 SMG, LIBERTY BLASTER)
│   ├── hud.c/h         — Health, ammo, wave counter, reload bar, ACHTUNG alert, radio transmission
│   ├── audio.c/h       — Erika march music (synthesized from MIDI), radio static overlay
│   ├── game.c/h        — Game state (menu/playing/paused/game over), wave management
├── assets/
│   ├── crt.fs          — Main CRT post-processing fragment shader (GLSL 330)
│   ├── hud.fs          — HUD visor curve fragment shader
│   ├── march.wav       — Generated at runtime (Erika march)
│   ├── soviet_death_sounds/  — Soviet faction radio death sounds (mp3)
│   ├── american_death_sounds/ — American faction radio death sounds (mp3)
├── Makefile
└── .gitignore
```

## Architecture

### Rendering Pipeline
1. 3D scene renders to a 640x360 `RenderTexture2D` (low-res target)
2. CRT shader (`crt.fs`) post-processes with: Gaussian scanlines, Trinitron phosphor mask, bloom, chromatic aberration, fishbowl distortion, film grain, breath fog, ghost reflection
3. Scaled up to window with nearest-neighbor filtering
4. HUD renders to separate `RenderTexture2D`, curved via `hud.fs` visor shader
5. Menu/pause/game-over screens drawn at full window resolution

### Performance Optimizations
- **Cached particle meshes:** Unit sphere and unit cube meshes generated once in `WeaponInit()`, reused via `DrawMesh()` with transform matrices for all projectiles, explosions, and beam glows — eliminates ~200 per-frame mesh regenerations

### Game Loop (main.c)
- State machine: `STATE_MENU` → `STATE_PLAYING` → `STATE_PAUSED` / `STATE_GAME_OVER`
- Assets load on first frame (loading screen shown first)
- Wave system: `GameUpdate` triggers waves → `LanderSpawnWave` sends landers → landers deploy enemies

### World System (world.c)
- Infinite terrain via chunk system (60x60 unit chunks, 5x5 render grid)
- `WorldGetHeight(x, z)` — deterministic multi-octave value noise, used by all systems
- Vertex-displaced meshes with world-space UVs and vertex colors for seamless chunks
- Craters baked into mesh via `CraterHeight()` which checks all loaded chunks
- Sphere-cluster boulders with LOD (detail reduces with distance)

### Enemy System (enemy.c)
- Two factions: Soviet (red uniforms, gold helmets) and American (navy blue, silver helmets)
- AI behaviors: `AI_ADVANCE`, `AI_STRAFE`, `AI_DODGE`, `AI_RETREAT`
- Soviet: aggressive rushers, wide spread, circle-strafe close
- American: tactical, seek cover behind rocks, maintain distance, retreat when hurt
- Three death types: ragdoll blowout (60%), crumple + blood pool (40%), vaporize (beam only)
- Vaporize sequence: jerk → freeze → optional swell/pop (15%) → disintegrate

### Weapon System (weapon.c)
- **Mond-MP40:** Hitscan SMG, 32-round mag, fast fire, cyan energy beams
- **Raketenfaust:** Death beam, holds for 2 seconds, kills everything in path, massive knockback
- **Jackhammer:** Melee mining tool, sparks on impact
- Reload system: R key, auto-reload on empty, visual tilt animation
- All procedural 3D viewmodels drawn with `rlPushMatrix`/`DrawCube`/`rlPopMatrix`
- Cached `meshSphere`/`meshCube` for all particle effects — projectile glows, explosion fireballs, debris use `DrawMesh()` with pre-uploaded GPU geometry instead of regenerating each frame

### Pickup Weapons (pickup.c)
- **KOSMOS-7 SMG (PPSh):** Fastest fire rate in game (0.05s), 35 damage, 3 spread tracers per shot, snappy recoil
- **LIBERTY BLASTER:** One-shot vaporize kill, wide hitbox for forgiving aim, massive recoil kick, thick lingering rail beam, heavy rail-gun sound
- Pickup buffs only apply in player's hands — enemy weapon behavior unchanged

### Sound
- All sounds procedurally generated from waveforms (no files needed for core game)
- Erika march synthesized from MIDI note data with brass square waves + drums + radio static
- Enemy death sounds loaded from mp3, degraded to 8kHz scratchy radio quality
- Klaxon siren for incoming waves

## Controls
| Key | Action |
|-----|--------|
| WASD | Move |
| Mouse | Look |
| Space | Jump (floaty moon gravity) |
| Shift | Sprint |
| Left Click | Fire (or fire pickup weapon if holding one) |
| 1/2/3 | Switch weapons |
| R | Reload |
| E | Pick up dropped enemy weapon |
| V | Jackhammer (alternate) |
| X | Ground pound (airborne: slam down / grounded: hop + slam) |
| ESC | Pause |
| Enter | Start game / Restart |

## Key Constants
- `CHUNK_SIZE` — 60 units per terrain chunk
- `RENDER_CHUNKS` — 5x5 grid (25 chunks visible)
- `MAX_CHUNKS_PER_FRAME` — 3 (lazy generation)
- `MAX_ENEMIES` — 50
- `MAX_LANDERS` — 4
- `MAX_PICKUPS` — 30
- `PLAYER_HEIGHT` — 1.8 units
- `MOON_GRAVITY` — 1.62 m/s²
- `RENDER_W/H` — 640x360 (internal render resolution)

## Adding Content

### New Enemy Death Sounds
Drop `.mp3` files in `assets/soviet_death_sounds/` or `assets/american_death_sounds/`. Update the file path arrays in `EnemyManagerInit()` in `enemy.c`. They auto-degrade to scratchy radio quality at load time.

### Modifying Weapons
All weapon stats are in `WeaponInit()` in `weapon.c`. Viewmodels are drawn in `WeaponDrawFirst()`. Enemy weapon visuals are in `DrawAstronautModel()` in `enemy.c`.

### Modifying Terrain
Height function is `WorldGetHeight()` in `world.c`. Adjust noise octaves/scales for different terrain. Rock sizes in `GenerateChunk()`. Crater parameters there too.

### Shaders
`assets/crt.fs` is the main post-processing shader — edit and reload (no recompile needed, it's loaded at runtime). `assets/hud.fs` curves the HUD overlay.
