#ifndef ENEMY_DRAW_H
#define ENEMY_DRAW_H

#include "enemy.h"

/// Draw a single astronaut in any state (alive, dying, vaporizing, eviscerating, decapitating) using data-driven body defs.
void DrawAstronautModel(EnemyManager *em, Enemy *e);

/// Draw a simplified astronaut at medium distance (~15 draw calls, wireframe parts skipped).
/// Pass em for skeletal model support (NULL falls back to procedural).
void DrawAstronautLOD1(EnemyManager *em, Enemy *e);

/// Draw a minimal astronaut at far distance as a single faction-colored cube (1 draw call).
void DrawAstronautLOD2(Enemy *e);

#endif
