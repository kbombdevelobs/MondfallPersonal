#ifndef HUD_PRIMITIVES_H
#define HUD_PRIMITIVES_H

#include "raylib.h"
#include "config.h"

// Reusable dieselpunk HUD drawing primitives
void HudDrawRivet(int x, int y, float s);
void HudDrawIronCross(int cx, int cy, int size, Color col);
void HudDrawEagle(int cx, int cy, int size, Color col);
void HudDrawDecoBar(int x, int y, int w, int h, float s);
void HudDrawGauge(int cx, int cy, int radius, float value, float s, const char *label);
void HudDrawTicker(int x, int y, int w, int h, const char *label, const char *value, float s);

#endif
