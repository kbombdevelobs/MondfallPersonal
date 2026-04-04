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

## File Size Rule — MANDATORY
**Any source file exceeding 500 lines MUST be split into smaller files in a subfolder.**
- Create `src/<system>/` subfolder (e.g. `src/enemy/`, `src/weapon/`)
- Split by concern: logic vs rendering vs AI vs sound
- Add a `README.md` in each subfolder listing files, their purpose, key types, and key functions
- Update all `#include` paths and the `Makefile`
- Subfolder READMEs are the primary navigation aid for agents — keep them accurate

## Project Structure
```
Mondfall/
├── src/
│   ├── config.h        — ALL tunable game constants in one place
│   ├── main.c          — Game loop, state machine, input, rendering pipeline
│   ├── player.c/h      — FPS camera, WASD + mouse, moon gravity, jump, ground pound
│   ├── weapon.c/h      — Weapon logic: init, update, fire, reload, switch, getters
│   ├── weapon/          — Weapon rendering & sound (see src/weapon/README.md)
│   │   ├── weapon_sound.h/c — Procedural sound generation (6 weapon sounds)
│   │   ├── weapon_draw.h/c  — Viewmodels, beams, explosions, barrel positions
│   │   └── README.md
│   ├── combat.c/h      — Hit processing, damage application, weapon fire dispatch
│   ├── enemy/           — Enemy system (see src/enemy/README.md)
│   │   ├── enemy.h     — Types, state machine, API
│   │   ├── enemy.c     — AI, spawning, hit detection, damage, death logic
│   │   ├── enemy_draw.h — Draw API
│   │   ├── enemy_draw.c — All enemy rendering (alive, dying, vaporizing, eviscerating)
│   │   └── README.md   — System overview for agents
│   ├── world.c/h       — Core chunk management, terrain mesh gen, craters, collision
│   ├── world/           — World rendering & noise (see src/world/README.md)
│   │   ├── world_noise.h/c — Perlin gradient noise, ValueNoise, WorldGetHeight()
│   │   ├── world_draw.h/c  — Sky rendering, chunk drawing, frustum culling
│   │   └── README.md
│   ├── structure/       — Moon base structures (see src/structure/README.md)
│   │   ├── structure.h/c    — Structure manager, enter/exit teleport, resupply, collision
│   │   ├── structure_draw.h/c — Exterior hab dome + interior room rendering
│   │   └── README.md
│   ├── lander.c/h      — Moon lander wave system with descent, deployment, self-destruct
│   ├── pickup.c/h      — Dropped enemy weapons (KOSMOS-7 SMG, LIBERTY BLASTER)
│   ├── hud.c/h         — Health, ammo, wave counter, reload bar, ACHTUNG alert, radio transmission
│   ├── audio.c/h       — Erika march music (synthesized from MIDI), radio static overlay
│   ├── game.c/h        — Game state (menu/intro/settings/playing/paused/game over), wave management, settings, intro lore screen
│   ├── sound_gen.c/h   — Procedural audio waveform generation utilities
├── assets/
│   ├── crt.fs          — Main CRT post-processing fragment shader (GLSL 330)
│   ├── hud.fs          — HUD visor curve fragment shader
│   ├── march.wav       — Generated at runtime (Erika march)
│   ├── soviet_death_sounds/  — Soviet faction radio death sounds (mp3)
│   ├── american_death_sounds/ — American faction radio death sounds (mp3)
│   ├── intro.txt       — Intro lore text (directives: @STYLE, @PAUSE, @CLEAR)
├── tests/
│   └── test_game.c     — ~71 unit tests (no GPU required)
├── Makefile
└── .gitignore
```

## Architecture

### Rendering Pipeline
1. 3D scene renders to a 640x360 `RenderTexture2D` (low-res target)
2. CRT shader (`crt.fs`) post-processes with: Gaussian scanlines, Trinitron phosphor mask, bloom, chromatic aberration, fishbowl distortion, film grain, breath fog, ghost reflection
3. Scaled up to window with nearest-neighbor filtering
4. HUD renders to separate `RenderTexture2D`, curved via `hud.fs` visor shader
5. HUD render texture recreated on window resize to keep elements positioned correctly
6. Menu/pause/settings/game-over screens drawn at full window resolution with DPI-aware scaling

### Performance Optimizations
- **Cached particle meshes:** Unit sphere and unit cube meshes generated once in `WeaponInit()`, reused via `DrawMesh()` with transform matrices for all projectiles, explosions, and beam glows — eliminates ~200 per-frame mesh regenerations
- **Frustum culling:** `WorldDraw()` culls chunks behind camera or outside ~80 degree cone, cutting terrain draw calls roughly in half

### Game Loop (main.c)
- State machine: `STATE_MENU` → `STATE_INTRO` → `STATE_PLAYING` → `STATE_PAUSED` / `STATE_GAME_OVER` / `STATE_SETTINGS`
- Assets load on first frame (loading screen shown first)
- Wave system: `GameUpdate` triggers waves → `LanderSpawnWave` sends landers → landers deploy enemies

### World System (world.c + world/)
- Infinite terrain via chunk system (60x60 unit chunks, 5x5 render grid)
- `WorldGetHeight(x, z)` — deterministic multi-octave Perlin gradient noise with domain warping for organic shapes
- **Domain warping:** large-scale octaves fed through secondary noise to break repetition; fine detail stays crisp
- **Rilles:** two sinuous lunar channels at different angles carved into heightmap — natural gameplay trenches
- **Maria:** cell-noise biome regions where terrain flattens and darkens (dark basalt "seas")
- **Wrinkle ridges:** narrow raised linear features in mare regions, modulated along length
- **Ejecta rays:** bright radial streaks in vertex colors around large craters (radius > 4.5)
- **Craters:** `CraterProfile()` with terraced walls and central peaks for large craters (radius > 5), simple bowls for small; min-depth overlap resolution
- Baked directional lighting: analytical normals from WorldGetHeight (seamless across chunks) with low-angle sun dot product (0.15 ambient floor)
- Slope-based vertex coloring: steep terrain darkened by sampling 4 neighboring heights
- Maria darkening: vertex colors reduced in dark sea biome regions
- Frustum culling in `WorldDraw()` — tests all 4 chunk corners, only culls chunks fully behind camera
- Vertex-displaced meshes with world-space UVs and vertex colors for seamless chunks
- Sphere-cluster boulders with LOD (detail reduces with distance)
- Boulder collision checks Y-axis so players can jump over rocks
- Horizon fog in CRT shader: subtle radial fade to black at screen edges

### Enemy System (enemy.c)
- Two factions: Soviet (red uniforms, gold helmets) and American (navy blue, silver helmets)
- AI behaviors: `AI_ADVANCE`, `AI_STRAFE`, `AI_DODGE`, `AI_RETREAT`
- Soviet: aggressive rushers, wide spread, circle-strafe close
- American: tactical, seek cover behind rocks, maintain distance, retreat when hurt
- Four death types: ragdoll blowout (60%), crumple + blood pool (40%), vaporize (beam only), eviscerate (jackhammer only)
- **Dead bodies persist 10-12 seconds** and are clamped to terrain surface via continuous `WorldGetHeight` sampling — never sink through ground
- **Ragdoll deaths**: pressurized suit breach with directional gas jet (fades over 6s), pulsing arterial blood spurts with gravity, settling behavior locks body to terrain when velocity drops
- **Crumple deaths**: blood drips from wounds to terrain, gas wisps leak from suit (5s), terrain-conforming blood pool grows to radius 3.5 with 10-segment mesh
- Blood pools are terrain-conforming triangle fan meshes — each vertex placed at `WorldGetHeight`, draping over slopes and craters
- Vaporize sequence: jerk → freeze → optional swell/pop (15%) → disintegrate
- Eviscerate sequence: limbs separate with physics (head/torso/arms/legs fly apart), blood spurts from stumps (6s), bone fragments (4s), blood mist cloud (5s), blood pool forms under torso, enemy drops weapon

### Weapon System (weapon.c)
- **Mond-MP40:** Hitscan SMG, 32-round mag, fast fire, cyan energy beams
- **Raketenfaust:** Death beam, holds for 2 seconds, kills everything in path, massive knockback
- **Jackhammer:** Pneumatic war-pick, one-hit eviscerate kill, forward lunge on attack, enemy torn apart with blood spurts + weapon drop
- Reload system: R key, auto-reload on empty, visual tilt animation
- All procedural 3D viewmodels drawn with `rlPushMatrix`/`DrawCube`/`rlPopMatrix`
- Cached `meshSphere`/`meshCube` for all particle effects — projectile glows, explosion fireballs, debris use `DrawMesh()` with pre-uploaded GPU geometry instead of regenerating each frame

### Pickup Weapons (pickup.c)
- **KOSMOS-7 SMG (PPSh):** Fastest fire rate in game (0.05s), 35 damage, 3 spread tracers per shot, snappy recoil
- **LIBERTY BLASTER:** One-shot vaporize kill, wide hitbox for forgiving aim, massive recoil kick, thick lingering rail beam, heavy rail-gun sound
- Pickup buffs only apply in player's hands — enemy weapon behavior unchanged

### Moon Base Structure System (structure/)
- **Exterior**: Geodesic dome (5 rings x 12 segments) on a raised cylinder shaft that tunnels underground. Dome radius 4.5 units. Cylinder visible 1.0 unit above terrain, extends 8 units underground with steel rim and cap.
- **3 Airlock Doors**: Player-height (2.0 unit) corridors at 120-degree intervals around the dome, 2.8 units long, with ribbed steel reinforcement, red warning stripes, green indicator lights.
- **Interior**: Separate-space technique — player teleports to Y=500 hidden room (24x20 units, much bigger than exterior). Germanic officers' lounge: dark wood wainscoting, cream plaster walls, crown moulding, red carpet with gold border, leather Chesterfield couches, iron chandelier with flickering candles, full bar with bottles/glasses/stools, bookshelf, map table, 8 pixel-art general portraits, hanging banners, exposed ceiling beams.
- **Resupply**: Military green cabinet on north wall — press E to refill all weapon ammo (MP40, Raketenfaust, pickup weapons).
- **Safe Zone**: When inside, ALL simulation freezes: `EnemyManagerUpdate`, `LanderManagerUpdate`, `GameUpdate`, `CombatProcess*`, `PickupManagerUpdate` all skipped. Enemies have no knowledge of player position.
- **Wave Pause**: Wave timer does not advance while inside. Existing enemies and landers freeze in place, resume on exit.
- **Multi-door**: Enter/exit through any of the 3 doors. Exit places player outside the door they walked through.
- **HUD Prompts**: "PRESS [E] TO ENTER BASE", "PRESS [E] TO EXIT BASE", "PRESS [E] TO RESUPPLY" — pulsing green text at screen center.
- **Adding structures**: New `StructureType` enum + unique `interiorY` offset. Teleport, collision, and freeze systems work automatically. See `src/structure/README.md`.

### Settings System (game.c)
- Mouse sensitivity: stored in `Game.mouseSensitivity`, synced to `Player.mouseSensitivity` each frame
- Music volume: applied via `GameAudioSetMusicVolume()` each frame
- SFX volume: applied via per-manager `SetSFXVolume()` functions (`WeaponSetSFXVolume`, `EnemyManagerSetSFXVolume`, `LanderManagerSetSFXVolume`, `PickupManagerSetSFXVolume`)
- Screen scale: 1x–4x (640x360 to 2560x1440), applied via `SetWindowSize()` on change
- All UI elements use `S(px)` macro scaling relative to `WINDOW_H` (menus) or `RENDER_H` (HUD) to stay proportional at any resolution

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
| E | Pick up dropped enemy weapon / Enter-exit base / Resupply |
| V | Jackhammer (alternate) |
| X | Ground pound (airborne: slam down / grounded: hop + slam) |
| ESC | Pause / Resume |
| Enter | Select menu option |
| Up/Down or W/S | Navigate menus |
| I | Toggle intro skip (main menu only) |
| Left/Right or A/D | Adjust settings |

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
- `WINDOW_W/H` — 960x540 (default window size, used as UI scale reference)

## Adding Content

### New Enemy Death Sounds
Drop `.mp3` files in `assets/soviet_death_sounds/` or `assets/american_death_sounds/`. Update the file path arrays in `EnemyManagerInit()` in `enemy.c`. They auto-degrade to scratchy radio quality at load time.

### Modifying Weapons
All weapon stats are in `WeaponInit()` in `weapon.c`. Viewmodels are drawn in `WeaponDrawFirst()`. Enemy weapon visuals are in `DrawAstronautModel()` in `enemy.c`.

### Modifying Terrain
Height function is `WorldGetHeight()` in `src/world/world_noise.c` — uses domain-warped 4-octave Perlin gradient noise + rille channels + maria flattening. Adjust warp strength (0.15 multiplier), octave scales/amplitudes, or rille angles/widths for different terrain character. `WorldGetMareFactor()` controls biome regions — tune the cell noise scale (0.002) for larger/smaller maria. Crater profiles are in `CraterProfile()` in `src/world.c` — terracing and central peaks only activate for radius > 5. Sun direction vector is in `GenTerrainMesh()` — modify for different shadow angles. Rock sizes in `GenerateChunk()`. The CRT shader horizon fog is in `assets/crt.fs` (search for "HORIZON FOG").

### Modifying Intro Lore
Edit `assets/intro.txt` — no recompile needed, it's loaded at startup. One display line per text line. Directives:
- `@STYLE TITLE` — next line renders large and red
- `@STYLE DIM` — next line renders subdued gray
- `@PAUSE 1.5` — extra delay (seconds) after the previous line finishes
- `@CLEAR` — fade out all previous text before showing next line
- `#` lines are comments, blank lines add a small pause
The intro plays once on first PLAY from main menu (tile-by-tile cipher reveal). SPACE/ENTER skips. I key on menu toggles show/skip. Restarts from game over bypass it.

### Shaders
`assets/crt.fs` is the main post-processing shader — edit and reload (no recompile needed, it's loaded at runtime). `assets/hud.fs` curves the HUD overlay.
