#ifndef STRUCTURE_DRAW_H
#define STRUCTURE_DRAW_H

#include "structure.h"

// Draw all exterior structures in world space
void StructureManagerDraw(StructureManager *sm, Vector3 playerPos);

// Draw the interior of the structure the player is inside
void StructureManagerDrawInterior(StructureManager *sm);

#endif
