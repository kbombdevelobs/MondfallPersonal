#ifndef HUD_H
#define HUD_H

#include "raylib.h"
#include "player.h"
#include "weapon.h"
#include "game.h"
#include "pickup.h"
#include "lander.h"
#include "structure/structure.h"

void HudDraw(Player *player, Weapon *weapon, Game *game, int screenW, int screenH);
void HudDrawPickup(PickupManager *pm, int sw, int sh);
void HudDrawLanderArrows(LanderManager *lm, Camera3D camera, int sw, int sh);
void HudDrawRadioTransmission(float timer, int sw, int sh);
void HudDrawStructurePrompt(StructurePrompt prompt, int sw, int sh);

#endif
