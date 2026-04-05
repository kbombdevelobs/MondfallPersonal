# HUD System — src/hud/

Dieselpunk retro HUD drawing split by concern. The parent `src/hud.c` contains the main HUD layout functions; this subfolder contains reusable primitives and the Führerauge zoom optic.

## Files

| File | Purpose | Key Functions |
|------|---------|---------------|
| `hud_primitives.h/c` | Reusable drawing primitives: rivets, iron crosses, eagle wings, art deco bars, analog gauges, mechanical tickers | `HudDrawRivet`, `HudDrawIronCross`, `HudDrawEagle`, `HudDrawDecoBar`, `HudDrawGauge`, `HudDrawTicker` |
| `hud_fuehrerauge.h/c` | Führerauge zoom optic: swing arm, trapezoid frame, old-TV shader viewport, targeting reticle | `HudDrawFuehrerauge` |

## Architecture

- `src/hud.c` — Main HUD entry points (`HudDraw`, `HudDrawPickup`, `HudDrawLanderArrows`, `HudDrawRadioTransmission`, `HudDrawRankKill`, `HudDrawStructurePrompt`). Calls primitives from `hud_primitives.h`.
- `src/hud/hud_primitives.c` — All reusable visual elements. No game state dependencies — pure drawing functions taking position, size, color, scale.
- `src/hud/hud_fuehrerauge.c` — Self-contained zoom optic. Takes a zoom texture + shader + animation progress and handles all arm/frame/reticle drawing.
- `src/hud.h` — Public API. Re-exports `HudDrawFuehrerauge` via `#include "hud/hud_fuehrerauge.h"`.

## Color Palette

All dieselpunk colors defined in `src/config.h`: `COLOR_BRASS`, `COLOR_BRASS_DIM`, `COLOR_BRASS_BRIGHT`, `COLOR_COPPER`, `COLOR_DARK_METAL`, `COLOR_RIVET`, `COLOR_GAUGE_BG`, `COLOR_GAUGE_NEEDLE`, `COLOR_GAUGE_MARK`, `COLOR_HUD_TEXT`.

## Dependencies

- `raylib.h` — all drawing functions
- `config.h` — color defines, `RENDER_H`, `FUEHRERAUGE_*` constants
- `math.h` — trig for gauges and swing arm
