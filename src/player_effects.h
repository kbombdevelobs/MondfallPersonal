#ifndef PLAYER_EFFECTS_H
#define PLAYER_EFFECTS_H

#include "raylib.h"
#include "player.h"

/// Draw ground pound dust cloud, rock chunks, mist, streaks, and shockwave flash.
/// Call inside BeginMode3D. No-op if groundPoundImpactTimer <= 0.
void DrawGroundPoundDust(Player *player);

/// Draw He-3 rocket exhaust spiral trail beneath the player.
/// Call inside BeginMode3D. No-op if he3TrailCount == 0.
void DrawHe3Trail(Player *player);

#endif
