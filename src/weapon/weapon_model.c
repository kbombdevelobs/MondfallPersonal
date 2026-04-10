#include "weapon_model.h"
#include <string.h>
#include <stdio.h>

/* ============================================================================
 * Weapon model loader -- static vertex-colored .glb models
 * ============================================================================ */

#define WEAPON_MODEL_PATH "resources/models/weapons/"

/// File names for player weapon models (indexed by WeaponType)
static const char *PLAYER_WEAPON_FILES[WEAPON_COUNT] = {
    [WEAPON_MOND_MP40]    = "mond_mp40.glb",
    [WEAPON_RAKETENFAUST] = "raketenfaust.glb",
    [WEAPON_JACKHAMMER]   = "jackhammer.glb",
};

/// File names for pickup weapon models (indexed by PickupWeaponType)
static const char *PICKUP_WEAPON_FILES[PICKUP_TYPE_COUNT] = {
    [PICKUP_KOSMOS7]        = "kosmos7.glb",
    [PICKUP_LIBERTY]        = "liberty.glb",
    [PICKUP_KS23_MOLOT]     = "ks23_molot.glb",
    [PICKUP_M8A1_STARHAWK]  = "m8a1_starhawk.glb",
    [PICKUP_ZARYA_TK4]      = "zarya_tk4.glb",
    [PICKUP_ARC9_LONGBOW]   = "arc9_longbow.glb",
};

/// Singleton instance
static WeaponModelSet g_weaponModels = {0};

/// Try to load a single .glb model. Returns true on success.
static bool LoadWeaponModel(Model *out, const char *filename) {
    if (!out || !filename) return false;

    char path[256];
    snprintf(path, sizeof(path), "%s%s", WEAPON_MODEL_PATH, filename);

    if (!FileExists(path)) {
        TraceLog(LOG_INFO, "WeaponModel: file not found: %s", path);
        return false;
    }

    *out = LoadModel(path);
    if (out->meshCount <= 0) {
        TraceLog(LOG_WARNING, "WeaponModel: failed to load: %s", path);
        return false;
    }

    TraceLog(LOG_INFO, "WeaponModel: loaded %s (%d meshes)", path, out->meshCount);
    return true;
}

void WeaponModelsLoad(WeaponModelSet *set) {
    if (!set) return;

    memset(set, 0, sizeof(*set));
    set->available = false;
    int loaded = 0;

    /* Load player weapons */
    for (int i = 0; i < WEAPON_COUNT; i++) {
        if (PLAYER_WEAPON_FILES[i] && LoadWeaponModel(&set->playerWeapons[i], PLAYER_WEAPON_FILES[i])) {
            set->playerLoaded[i] = true;
            loaded++;
            TraceLog(LOG_INFO, "WeaponModel: player weapon %d loaded", i);
        }
    }

    /* Load pickup weapons */
    for (int i = 0; i < PICKUP_TYPE_COUNT; i++) {
        if (PICKUP_WEAPON_FILES[i] && LoadWeaponModel(&set->pickupWeapons[i], PICKUP_WEAPON_FILES[i])) {
            set->pickupLoaded[i] = true;
            loaded++;
            TraceLog(LOG_INFO, "WeaponModel: pickup weapon %d loaded", i);
        }
    }

    if (loaded > 0) {
        set->available = true;
        TraceLog(LOG_INFO, "WeaponModel: %d/%d weapon models loaded", loaded, WEAPON_COUNT + PICKUP_TYPE_COUNT);
    } else {
        TraceLog(LOG_INFO, "WeaponModel: no weapon models found, using procedural fallback");
    }
}

void WeaponModelsUnload(WeaponModelSet *set) {
    if (!set) return;

    for (int i = 0; i < WEAPON_COUNT; i++) {
        if (set->playerLoaded[i]) {
            UnloadModel(set->playerWeapons[i]);
            set->playerLoaded[i] = false;
        }
    }
    for (int i = 0; i < PICKUP_TYPE_COUNT; i++) {
        if (set->pickupLoaded[i]) {
            UnloadModel(set->pickupWeapons[i]);
            set->pickupLoaded[i] = false;
        }
    }
    set->available = false;
    TraceLog(LOG_INFO, "WeaponModel: all models unloaded");
}

WeaponModelSet *WeaponModelsGet(void) {
    return &g_weaponModels;
}
