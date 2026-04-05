---
title: Rendering Pipeline
status: Active
owner_area: Core
created: 2026-04-04
last_updated: 2026-04-04
last_reviewed: 2026-04-04
parent_doc: CLAUDE.md
related_docs:
  - src/world/CLAUDE.md
  - docs/procedural-audio.md
---

# Rendering Pipeline

The rendering pipeline uses a 3-layer approach to produce a retro CRT aesthetic at low resolution, then composites a HUD with visor curvature on top.

## Layer 1: 3D Scene to Low-Res RenderTexture

All 3D gameplay (terrain, enemies, landers, weapons, pickups) renders into a 640x360 `RenderTexture2D` with nearest-neighbor filtering (`TEXTURE_FILTER_POINT`). This produces the pixelated look. The render target dimensions are defined by `RENDER_W` (640) and `RENDER_H` (360) in `config.h`.

During gameplay states (`STATE_PLAYING`, `STATE_PAUSED`, `STATE_GAME_OVER`, or `STATE_SETTINGS` when accessed from pause), the pipeline:

1. Begins texture mode on the low-res target
2. Clears to black
3. Enters 3D mode with the player camera
4. Draws sky, terrain chunks, ECS enemies, landers, pickups, world-space weapon effects
5. Draws the first-person viewmodel (pickup weapon or standard weapon)
6. Ends 3D and texture mode

## Layer 2: CRT Post-Processing Shader

The low-res texture is drawn to the window through `resources/crt.fs`, a GLSL 330 fragment shader. The shader receives a `time` uniform updated each frame.

CRT shader effects:
- **Fishbowl visor distortion** -- radial UV displacement simulating a curved helmet visor
- **Pixelation** -- quantizes to 320x180 grid for an extra-chunky look
- **Radial chromatic aberration** -- R/G/B channels offset radially from center, stronger at edges
- **Fake bloom** -- 5-tap cross-pattern sampling that brightens areas above 0.5 luminance
- **Scanlines** -- Gaussian-weighted horizontal darkening
- **Trinitron phosphor mask** -- per-pixel RGB sub-pixel pattern
- **Film grain** -- time-based noise overlay
- **Breath fog** -- subtle haze effect
- **Horizon fog** -- radial fade to black at screen edges

The texture is scaled to fill the window while maintaining aspect ratio, centered with letterboxing if needed.

## Layer 3: HUD Visor Shader

The HUD renders to a separate `RenderTexture2D` that matches the current window dimensions (recreated on window resize). HUD elements include health, ammo, wave counter, reload bar, lander directional arrows, pickup indicators, and radio transmission display.

This HUD texture is drawn through `resources/hud.fs`, which applies:
- **Fishbowl curvature** -- edges pinch inward to simulate viewing through a helmet visor (0.12 distortion factor)
- **Glow boost** -- bright HUD elements get a 1.05x brightness multiplier
- **No pixelation** -- text and HUD elements remain crisp at full window resolution

## Menu / Overlay Screens

Menu, pause, settings, intro, and game-over screens draw at full window resolution directly on top of everything, after the CRT and HUD passes. All UI elements use the `S(px)` macro which scales pixel values relative to `WINDOW_H` (540), keeping proportions consistent at any window size.

## Key Files

| File | Role |
|------|------|
| `src/main.c` | Orchestrates the full render pipeline, manages render textures and shaders |
| `resources/crt.fs` | CRT post-processing fragment shader (GLSL 330) |
| `resources/hud.fs` | HUD visor curvature fragment shader (GLSL 330) |
| `src/hud.c` | HUD element drawing (health, ammo, wave, arrows, radio) |
| `src/config.h` | `RENDER_W`, `RENDER_H`, `WINDOW_W`, `WINDOW_H`, `CAMERA_FOV` |
