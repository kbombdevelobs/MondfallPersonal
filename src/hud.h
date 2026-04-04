#ifndef HUD_H
#define HUD_H

#include "raylib.h"
#include "player.h"
#include "weapon.h"
#include "game.h"

void HudDraw(Player *player, Weapon *weapon, Game *game, int screenW, int screenH);

#endif
