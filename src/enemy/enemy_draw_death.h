#ifndef ENEMY_DRAW_DEATH_H
#define ENEMY_DRAW_DEATH_H

#include "enemy.h"

// Death state rendering — extracted from DrawAstronautModel

// Eviscerate death: limbs fly apart with blood spurts and gore
void DrawAstronautEviscerate(Enemy *e, EnemyManager *em);

// Vaporize death: jerk, freeze, optional swell/pop, disintegrate
void DrawAstronautVaporize(Enemy *e, EnemyManager *em);

// Ragdoll/crumple death particles: suit breach jets, blood, blood pools
void DrawAstronautRagdoll(Enemy *e);

#endif
