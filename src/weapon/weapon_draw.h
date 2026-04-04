#ifndef WEAPON_DRAW_H
#define WEAPON_DRAW_H

#include "weapon.h"

void WeaponDrawFirst(Weapon *w, Camera3D camera);
void WeaponDrawWorld(Weapon *w);
Vector3 WeaponGetBarrelWorldPos(Weapon *w, Camera3D camera);
void WeaponSpawnExplosion(Weapon *w, Vector3 pos, float radius);

#endif
