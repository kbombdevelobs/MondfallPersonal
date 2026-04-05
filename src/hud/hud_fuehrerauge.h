#ifndef HUD_FUEHRERAUGE_H
#define HUD_FUEHRERAUGE_H

#include "raylib.h"

// Draw the Fuehrerauge zoom optic viewport with swing arm,
// trapezoid frame, old-TV shader, and targeting reticle.
void HudDrawFuehrerauge(Texture2D zoomTex, Shader fishbowlShader, float animProgress, int sw, int sh);

#endif
