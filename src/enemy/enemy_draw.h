#ifndef ENEMY_DRAW_H
#define ENEMY_DRAW_H

#include "enemy.h"

// Draw a single astronaut (all states: alive, dying, vaporizing, eviscerating)
void DrawAstronautModel(EnemyManager *em, Enemy *e);

// LOD 1: Simplified astronaut (~8 draw calls)
void DrawAstronautLOD1(Enemy *e);

// LOD 2: Single colored cube (1 draw call)
void DrawAstronautLOD2(Enemy *e);

#endif
