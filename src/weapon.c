#include "weapon.h"
#include "weapon/weapon_sound.h"
#include "weapon/weapon_draw.h"
#include "weapon/weapon_model.h"
#include "sound_gen.h"
#include "rlgl.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>

void WeaponInit(Weapon *w) {
    bool hadSound = w->soundLoaded;
    Sound s1=w->sndMp40Fire, s2=w->sndRaketenFire, s3=w->sndJackhammerHit, s4=w->sndReload, s5=w->sndEmpty, s6=w->sndExplosion;
    bool hadMeshes = w->meshesLoaded;
    Mesh saveSphere = w->meshSphere;
    Mesh saveCube = w->meshCube;
    Material saveMat = w->matDefault;

    memset(w, 0, sizeof(Weapon));
    w->current = WEAPON_MOND_MP40;
    w->mp40Mag = MP40_MAG_SIZE; w->mp40MagSize = MP40_MAG_SIZE; w->mp40Reserve = MP40_RESERVE;
    w->mp40FireRate = MP40_FIRE_RATE; w->mp40Damage = MP40_DAMAGE;
    w->raketenMag = RAKETEN_MAG_SIZE; w->raketenMagSize = RAKETEN_MAG_SIZE; w->raketenReserve = RAKETEN_RESERVE;
    w->raketenFireRate = RAKETEN_FIRE_RATE; w->raketenDamage = RAKETEN_DAMAGE; w->raketenRadius = RAKETEN_BLAST_RADIUS;
    w->raketenBeamDuration = RAKETEN_BEAM_DURATION;
    w->raketenCooldown = RAKETEN_COOLDOWN;
    w->jackhammerDamage = JACKHAMMER_DAMAGE; w->jackhammerRange = JACKHAMMER_RANGE; w->jackhammerFireRate = JACKHAMMER_FIRE_RATE;

    if (hadSound) {
        w->sndMp40Fire=s1; w->sndRaketenFire=s2; w->sndJackhammerHit=s3; w->sndReload=s4; w->sndEmpty=s5;
        w->sndExplosion = s6;
        w->soundLoaded = true;
    } else {
        w->sndMp40Fire = GenSoundGunshot();
        w->sndRaketenFire = GenSoundBeamBlast();
        w->sndJackhammerHit = GenSoundJackhammer();
        w->sndReload = GenSoundReload();
        w->sndEmpty = GenSoundEmpty();
        w->sndExplosion = GenSoundExplosion();
        w->soundLoaded = true;
    }

    if (hadMeshes) {
        w->meshSphere = saveSphere;
        w->meshCube = saveCube;
        w->matDefault = saveMat;
        w->meshesLoaded = true;
    } else {
        w->meshSphere = GenMeshSphere(1.0f, 8, 8);
        w->meshCube = GenMeshCube(1.0f, 1.0f, 1.0f);
        w->matDefault = LoadMaterialDefault();
        w->meshesLoaded = true;
    }

    /* Load .glb weapon models (graceful fallback if missing) */
    WeaponModelsLoad(WeaponModelsGet());
}

void WeaponUnload(Weapon *w) {
    WeaponModelsUnload(WeaponModelsGet());

    if (w->soundLoaded) {
        UnloadSound(w->sndMp40Fire); UnloadSound(w->sndRaketenFire);
        UnloadSound(w->sndJackhammerHit); UnloadSound(w->sndReload); UnloadSound(w->sndEmpty);
        UnloadSound(w->sndExplosion);
    }
    if (w->meshesLoaded) {
        UnloadMesh(w->meshSphere);
        UnloadMesh(w->meshCube);
        UnloadMaterial(w->matDefault);
    }
}


void WeaponReload(Weapon *w) {
    if (w->reloading) return;
    switch (w->current) {
        case WEAPON_MOND_MP40:
            if (w->mp40Mag >= w->mp40MagSize || w->mp40Reserve <= 0) return;
            w->reloading = true; w->reloadDuration = 1.5f; w->reloadTimer = 0;
            PlaySound(w->sndReload); break;
        case WEAPON_RAKETENFAUST:
            if (w->raketenMag >= w->raketenMagSize || w->raketenReserve <= 0) return;
            w->reloading = true; w->reloadDuration = 2.5f; w->reloadTimer = 0;
            PlaySound(w->sndReload); break;
        default: break;
    }
}

static void FinishReload(Weapon *w) {
    w->reloading = false;
    switch (w->current) {
        case WEAPON_MOND_MP40: {
            int need = w->mp40MagSize - w->mp40Mag;
            int take = (need < w->mp40Reserve) ? need : w->mp40Reserve;
            w->mp40Mag += take; w->mp40Reserve -= take; break; }
        case WEAPON_RAKETENFAUST: {
            int need = w->raketenMagSize - w->raketenMag;
            int take = (need < w->raketenReserve) ? need : w->raketenReserve;
            w->raketenMag += take; w->raketenReserve -= take; break; }
        default: break;
    }
}

void WeaponUpdate(Weapon *w, float dt) {
    if (w->mp40Timer > 0) w->mp40Timer -= dt;
    if (w->raketenTimer > 0) w->raketenTimer -= dt;
    if (w->jackhammerTimer > 0) w->jackhammerTimer -= dt;
    if (w->muzzleFlash > 0) w->muzzleFlash -= dt * 8.0f;
    if (w->recoil > 0) w->recoil -= dt * 10.0f;
    else if (w->recoil < 0) { w->recoil += dt * 16.0f; if (w->recoil > 0) w->recoil = 0; }
    if (w->jackhammerAnim > 0) w->jackhammerAnim -= dt * 6.0f;
    if (w->jackhammerLunge > 0) {
        w->jackhammerLunge -= dt;
        if (w->jackhammerLunge <= 0) w->jackhammerLunging = false;
    }

    if (w->reloading) { w->reloadTimer += dt; if (w->reloadTimer >= w->reloadDuration) FinishReload(w); }

    // Raketenfaust mega beam — violent shudder
    if (w->raketenFiring) {
        w->raketenBeamTimer += dt;
        w->muzzleFlash = 2.0f;
        // Hard random shudder — not smooth sine, actual violent shake
        w->recoil = 2.0f + ((float)rand() / RAND_MAX) * 3.0f;
        if (w->raketenBeamTimer >= w->raketenBeamDuration) {
            w->raketenFiring = false;
            w->raketenTimer = w->raketenCooldown;
            w->recoil = 0;
        }
    }

    // Update explosions
    for (int i = 0; i < MAX_EXPLOSIONS; i++) {
        if (!w->explosions[i].active) continue;
        w->explosions[i].timer += dt;
        if (w->explosions[i].timer >= w->explosions[i].duration) w->explosions[i].active = false;
    }

    for (int i = 0; i < w->beamCount; i++) {
        w->beams[i].life -= dt;
        if (w->beams[i].life <= 0) { w->beams[i] = w->beams[--w->beamCount]; i--; }
    }
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        if (!w->projectiles[i].active) continue;
        w->projectiles[i].position = Vector3Add(w->projectiles[i].position,
            Vector3Scale(w->projectiles[i].velocity, dt));
        w->projectiles[i].life -= dt;
        if (w->projectiles[i].life <= 0) w->projectiles[i].active = false;
    }

    WeaponType prev = w->current;
    if (IsKeyPressed(KEY_ONE)) WeaponSwitch(w, WEAPON_MOND_MP40);
    if (IsKeyPressed(KEY_TWO)) WeaponSwitch(w, WEAPON_RAKETENFAUST);
    if (IsKeyPressed(KEY_THREE) || IsKeyPressed(KEY_V)) WeaponSwitch(w, WEAPON_JACKHAMMER);
    float scroll = GetMouseWheelMove();
    if (scroll != 0) WeaponSwitch(w, (WeaponType)(((int)w->current + (scroll > 0 ? 1 : -1) + WEAPON_COUNT) % WEAPON_COUNT));
    if (w->current != prev) w->reloading = false;

    if (IsKeyPressed(KEY_R)) WeaponReload(w);
    if (!w->reloading) {
        if (w->current == WEAPON_MOND_MP40 && w->mp40Mag <= 0 && w->mp40Reserve > 0) WeaponReload(w);
        if (w->current == WEAPON_RAKETENFAUST && w->raketenMag <= 0 && w->raketenReserve > 0) WeaponReload(w);
    }
}

bool WeaponFire(Weapon *w, Vector3 origin, Vector3 direction) {
    if (w->reloading) return false;
    switch (w->current) {
        case WEAPON_MOND_MP40:
            if (w->mp40Timer > 0) return false;
            if (w->mp40Mag <= 0) { PlaySound(w->sndEmpty); return false; }
            w->mp40Mag--; w->mp40Timer = w->mp40FireRate;
            w->muzzleFlash = 1.0f; w->recoil = 0.6f;
            PlaySound(w->sndMp40Fire);
            if (w->beamCount < MAX_BEAM_TRAILS) {
                w->beams[w->beamCount] = (BeamTrail){origin,
                    Vector3Add(origin, Vector3Scale(direction, 100.0f)),
                    {0, 220, 255, 180}, BEAM_TRAIL_LIFE, BEAM_TRAIL_LIFE, 1.0f};
                w->beamCount++;
            }
            return true;
        case WEAPON_RAKETENFAUST:
            if (w->raketenFiring) return false; // already firing
            if (w->raketenTimer > 0) return false; // cooldown
            if (w->raketenMag <= 0) { PlaySound(w->sndEmpty); return false; }
            // Start the mega beam
            w->raketenMag--;
            w->raketenFiring = true;
            w->raketenBeamTimer = 0;
            w->muzzleFlash = 2.0f;
            w->recoil = 5.0f;
            PlaySound(w->sndRaketenFire);
            w->raketenBeamStart = origin;
            w->raketenBeamEnd = Vector3Add(origin, Vector3Scale(direction, 100.0f));
            return true;
        case WEAPON_JACKHAMMER:
            if (w->jackhammerTimer > 0) return false;
            w->jackhammerTimer = w->jackhammerFireRate;
            w->jackhammerAnim = 1.0f;
            w->recoil = -8.0f; // NEGATIVE = thrust viewmodel FORWARD (huge punch)
            w->jackhammerLunge = JACKHAMMER_LUNGE_DURATION;
            w->jackhammerLunging = true;
            PlaySound(w->sndJackhammerHit);
            return true;
        default: return false;
    }
}

void WeaponSwitch(Weapon *w, WeaponType type) { w->current = type; }

// ---- PROCEDURAL WW2 WEAPON DRAWING ----

const char *WeaponGetName(Weapon *w) {
    switch (w->current) {
        case WEAPON_MOND_MP40: return "MOND-MP40";
        case WEAPON_RAKETENFAUST: return "RAKETENFAUST";
        case WEAPON_JACKHAMMER: return "JACKHAMMER";
        default: return "?";
    }
}
int WeaponGetAmmo(Weapon *w) {
    switch (w->current) {
        case WEAPON_MOND_MP40: return w->mp40Mag;
        case WEAPON_RAKETENFAUST: return w->raketenMag;
        case WEAPON_JACKHAMMER: return -1;
        default: return 0;
    }
}
int WeaponGetMaxAmmo(Weapon *w) {
    switch (w->current) {
        case WEAPON_MOND_MP40: return w->mp40MagSize;
        case WEAPON_RAKETENFAUST: return w->raketenMagSize;
        case WEAPON_JACKHAMMER: return -1;
        default: return 0;
    }
}
int WeaponGetReserve(Weapon *w) {
    switch (w->current) {
        case WEAPON_MOND_MP40: return w->mp40Reserve;
        case WEAPON_RAKETENFAUST: return w->raketenReserve;
        case WEAPON_JACKHAMMER: return -1;
        default: return 0;
    }
}
bool WeaponIsReloading(Weapon *w) { return w->reloading; }
float WeaponReloadProgress(Weapon *w) {
    return (!w->reloading || w->reloadDuration <= 0) ? 0 : w->reloadTimer / w->reloadDuration;
}

void WeaponSetSFXVolume(Weapon *w, float vol) {
    if (!w->soundLoaded) return;
    SetSoundVolume(w->sndMp40Fire, vol);
    SetSoundVolume(w->sndRaketenFire, vol);
    SetSoundVolume(w->sndJackhammerHit, vol);
    SetSoundVolume(w->sndReload, vol);
    SetSoundVolume(w->sndEmpty, vol);
    SetSoundVolume(w->sndExplosion, vol);
}
