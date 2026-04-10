#ifndef WEAPON_MODEL_H
#define WEAPON_MODEL_H

#include "raylib.h"
#include "weapon.h"
#include "pickup.h"
#include <stdbool.h>

/* ============================================================================
 * Weapon model loader for Blender-exported .glb weapon models.
 *
 * Loads static (no bones) vertex-colored models from resources/models/weapons/.
 * Falls back gracefully if model files are missing -- the procedural viewmodel
 * draw code continues to work as before.
 * ============================================================================ */

/// All loaded weapon models (player + pickup).
typedef struct {
    Model   playerWeapons[WEAPON_COUNT];       /// Player weapon viewmodels
    bool    playerLoaded[WEAPON_COUNT];         /// true if model loaded OK

    Model   pickupWeapons[PICKUP_TYPE_COUNT];   /// Pickup weapon models
    bool    pickupLoaded[PICKUP_TYPE_COUNT];     /// true if model loaded OK

    bool    available;                           /// true if any models loaded
} WeaponModelSet;

/// Load all weapon models from resources/models/weapons/. Safe to call if
/// directory is missing -- sets available=false and all loaded=false.
void WeaponModelsLoad(WeaponModelSet *set);

/// Unload all loaded weapon models and reset the set.
void WeaponModelsUnload(WeaponModelSet *set);

/// Get the global weapon model set (singleton, initialized by WeaponModelsLoad).
WeaponModelSet *WeaponModelsGet(void);

#endif /* WEAPON_MODEL_H */
