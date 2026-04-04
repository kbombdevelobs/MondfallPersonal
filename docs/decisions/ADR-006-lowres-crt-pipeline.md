---
title: "ADR-006: Low-Res Render Target with CRT Shader"
status: Accepted
owner_area: Core
created: 2026-04-04
last_updated: 2026-04-04
last_reviewed: 2026-04-04
parent_doc: docs/decisions/README.md
related_docs:
  - docs/rendering-pipeline.md
---

# ADR-006: Low-Res Render Target with CRT Shader

## Context

Mondfall targets a retro aesthetic inspired by early 3D shooters viewed on CRT monitors. The visual style needed to feel authentically low-resolution and analog, not just a modern game with a filter applied as an afterthought. The rendering approach had to be integral to the art direction.

## Decision

Render the 3D scene to a 640x360 `RenderTexture2D`, apply a multi-effect CRT post-processing shader, then scale to the window with nearest-neighbor filtering. The CRT shader (`resources/crt.fs`) applies: Gaussian scanlines, Trinitron phosphor mask, bloom, chromatic aberration, fisheye barrel distortion, film grain, breath fog, and ghost reflection. The HUD renders to a separate `RenderTexture2D` and is curved via a dedicated visor shader (`resources/hud.fs`).

## Alternatives Considered

- **Full-resolution rendering with retro shader only:** Rendering at native resolution and applying CRT effects produces a less authentic look. True low-res rendering creates naturally chunky pixels and aliasing that a shader alone cannot replicate.
- **Pixel art sprites (2D):** Would achieve a retro feel but abandons 3D, losing the FPS perspective and terrain depth that define the gameplay.
- **No post-processing:** Clean 3D rendering at low resolution. Functional but loses the CRT character that gives Mondfall its visual identity.

## Consequences

- Authentically retro look: real low-resolution rendering combined with CRT simulation produces convincing analog-era visuals.
- Good performance: shading only 640x360 pixels (230K) instead of full resolution means the fragment shader budget is generous even with complex post-processing.
- Unique visual identity that distinguishes the game from modern retro-styled titles.
- Trade-off: HUD elements must be designed for 640x360 internal resolution. The `S(px)` scaling macro handles proportional sizing, but text and icons need to be readable at low res.
- The HUD `RenderTexture2D` must be recreated on window resize to keep UI elements positioned correctly.
- Screen scale options (1x through 4x) map to window sizes from 640x360 to 2560x1440, all scaling the same internal render target.
