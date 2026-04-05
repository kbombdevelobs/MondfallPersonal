#include "pickup.h"
#include "world.h"
#include "sound_gen.h"
#include "rlgl.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>

// ============================================================================
// Weapon name lookup
// ============================================================================
const char *PickupGetName(PickupWeaponType type) {
    switch (type) {
        case PICKUP_KOSMOS7:       return "KOSMOS-7 SMG";
        case PICKUP_LIBERTY:       return "LIBERTY BLASTER";
        case PICKUP_KS23_MOLOT:    return "KS-23 MOLOT";
        case PICKUP_M8A1_STARHAWK: return "M8A1 STARHAWK";
        case PICKUP_ZARYA_TK4:     return "ZARYA TK-4";
        case PICKUP_ARC9_LONGBOW:  return "ARC-9 LONGBOW";
        default:                   return "UNKNOWN";
    }
}

PickupWeaponType PickupTypeFromRank(EnemyType faction, EnemyRank rank) {
    if (faction == ENEMY_SOVIET) {
        if (rank == RANK_NCO)     return PICKUP_KS23_MOLOT;
        if (rank == RANK_OFFICER) return PICKUP_ZARYA_TK4;
        return PICKUP_KOSMOS7;
    } else {
        if (rank == RANK_NCO)     return PICKUP_M8A1_STARHAWK;
        if (rank == RANK_OFFICER) return PICKUP_ARC9_LONGBOW;
        return PICKUP_LIBERTY;
    }
}

// ============================================================================
// Sound generation
// ============================================================================
void PickupManagerInit(PickupManager *pm) {
    memset(pm, 0, sizeof(PickupManager));

    int sr = AUDIO_SAMPLE_RATE;

    // KOSMOS-7 SMG (existing Soviet trooper)
    {
        Wave w = SoundGenCreateWave(sr, 0.12f);
        short *d = (short *)w.data;
        for (int i = 0; i < (int)w.frameCount; i++) {
            float t = (float)i/sr;
            d[i] = (short)(((float)rand()/RAND_MAX*2-1)*expf(-t*30)*16000
                + sinf(t*140*6.283f)*expf(-t*20)*10000);
        }
        pm->sndSovietFire = LoadSoundFromWave(w); UnloadWave(w);
    }
    // LIBERTY BLASTER (existing American trooper)
    {
        Wave w = SoundGenCreateWave(sr, 0.4f);
        short *d = (short *)w.data;
        for (int i = 0; i < (int)w.frameCount; i++) {
            float t = (float)i/sr;
            float crack = sinf(t*900*6.283f)*expf(-t*40)*0.4f;
            float thud = sinf(t*60*6.283f)*expf(-t*8)*0.35f;
            float sizzle = ((float)rand()/RAND_MAX*2-1)*expf(-t*15)*0.2f;
            float tail = sinf(t*200*6.283f)*expf(-t*5)*0.1f;
            d[i] = (short)((crack + thud + sizzle + tail) * 28000);
        }
        pm->sndAmericanFire = LoadSoundFromWave(w); UnloadWave(w);
    }
    // KS-23 Molot — deep boom + brass tinkle
    {
        Wave w = SoundGenCreateWave(sr, 0.35f);
        short *d = (short *)w.data;
        for (int i = 0; i < (int)w.frameCount; i++) {
            float t = (float)i/sr;
            float boom = sinf(t*50*6.283f)*expf(-t*6)*0.5f;
            float crack = ((float)rand()/RAND_MAX*2-1)*expf(-t*25)*0.4f;
            float brass = sinf(t*3200*6.283f)*expf(-t*40)*0.1f; // high shell tinkle
            d[i] = (short)((boom + crack + brass) * 30000);
        }
        pm->sndMolotFire = LoadSoundFromWave(w); UnloadWave(w);
    }
    // M8A1 Starhawk — sharp triple-crack
    {
        Wave w = SoundGenCreateWave(sr, 0.15f);
        short *d = (short *)w.data;
        for (int i = 0; i < (int)w.frameCount; i++) {
            float t = (float)i/sr;
            float snap = sinf(t*600*6.283f)*expf(-t*50)*0.5f;
            float hiss = ((float)rand()/RAND_MAX*2-1)*expf(-t*35)*0.3f;
            d[i] = (short)((snap + hiss) * 26000);
        }
        pm->sndStarhawkFire = LoadSoundFromWave(w); UnloadWave(w);
    }
    // Zarya TK-4 — rising whine + thunderclap
    {
        Wave w = SoundGenCreateWave(sr, 0.5f);
        short *d = (short *)w.data;
        for (int i = 0; i < (int)w.frameCount; i++) {
            float t = (float)i/sr;
            float thunder = sinf(t*80*6.283f)*expf(-t*4)*0.4f;
            float zap = sinf(t*1200*6.283f)*expf(-t*20)*0.3f;
            float rumble = ((float)rand()/RAND_MAX*2-1)*expf(-t*8)*0.25f;
            d[i] = (short)((thunder + zap + rumble) * 30000);
        }
        pm->sndZaryaFire = LoadSoundFromWave(w); UnloadWave(w);
    }
    // ARC-9 Longbow — electric lance crack + hum
    {
        Wave w = SoundGenCreateWave(sr, 0.6f);
        short *d = (short *)w.data;
        for (int i = 0; i < (int)w.frameCount; i++) {
            float t = (float)i/sr;
            float lance = sinf(t*1500*6.283f)*expf(-t*30)*0.35f;
            float hum = sinf(t*120*6.283f)*expf(-t*3)*0.3f;
            float crackle = ((float)rand()/RAND_MAX*2-1)*expf(-t*12)*0.2f;
            d[i] = (short)((lance + hum + crackle) * 28000);
        }
        pm->sndLongbowFire = LoadSoundFromWave(w); UnloadWave(w);
    }
    pm->soundLoaded = true;
}

// ============================================================================
// Drop and Update
// ============================================================================
void PickupDrop(PickupManager *pm, Vector3 pos, PickupWeaponType type) {
    for (int i = 0; i < MAX_PICKUPS; i++) {
        if (pm->guns[i].active) continue;
        pm->guns[i].position = pos;
        pm->guns[i].position.y = WorldGetHeight(pos.x, pos.z) + 0.5f;
        pm->guns[i].weaponType = type;
        pm->guns[i].life = PICKUP_LIFESPAN;
        pm->guns[i].active = true;
        pm->guns[i].bobTimer = 0;
        return;
    }
}

void PickupManagerUpdate(PickupManager *pm, Vector3 playerPos, float dt) {
    (void)playerPos;
    for (int i = 0; i < MAX_PICKUPS; i++) {
        if (!pm->guns[i].active) continue;
        pm->guns[i].life -= dt;
        pm->guns[i].bobTimer += dt;
        if (pm->guns[i].life <= 0) pm->guns[i].active = false;
    }
    if (pm->hasPickup) {
        if (pm->pickupTimer > 0) pm->pickupTimer -= dt;
        if (pm->pickupMuzzleFlash > 0) pm->pickupMuzzleFlash -= dt * 8.0f;

        // Recoil decay (heavier for Liberty and Zarya)
        float recoilDecay = 10.0f;
        if (pm->pickupType == PICKUP_LIBERTY || pm->pickupType == PICKUP_ZARYA_TK4)
            recoilDecay = 4.0f;
        else if (pm->pickupType == PICKUP_KS23_MOLOT)
            recoilDecay = 5.0f;
        if (pm->pickupRecoil > 0) pm->pickupRecoil -= dt * recoilDecay;

        // Starhawk burst processing
        if (pm->pickupType == PICKUP_M8A1_STARHAWK && pm->burstRemaining > 0) {
            pm->burstTimer -= dt;
            if (pm->burstTimer <= 0 && pm->pickupAmmo > 0) {
                pm->burstRemaining--;
                pm->pickupAmmo--;
                pm->pickupMuzzleFlash = 1.0f;
                pm->pickupRecoil = PICKUP_STARHAWK_RECOIL * 0.7f;
                PlaySound(pm->sndStarhawkFire);
                if (pm->burstRemaining > 0) {
                    pm->burstTimer = PICKUP_STARHAWK_FIRE_RATE;
                } else {
                    pm->pickupTimer = PICKUP_STARHAWK_BURST_CD;
                }
            }
        }

        // Zarya charge tracking
        if (pm->pickupType == PICKUP_ZARYA_TK4 && pm->charging) {
            pm->chargeTime += dt;
            if (pm->chargeTime > PICKUP_ZARYA_CHARGE_TIME)
                pm->chargeTime = PICKUP_ZARYA_CHARGE_TIME;
        }

        // Auto-drop when empty
        if (pm->pickupAmmo <= 0 && pm->burstRemaining <= 0) pm->hasPickup = false;
    }
}

// ============================================================================
// Grab
// ============================================================================
bool PickupTryGrab(PickupManager *pm, Vector3 playerPos) {
    float bestDist = PICKUP_GRAB_RANGE;
    int bestIdx = -1;
    for (int i = 0; i < MAX_PICKUPS; i++) {
        if (!pm->guns[i].active) continue;
        float d = Vector3Distance(playerPos, pm->guns[i].position);
        if (d < bestDist) { bestDist = d; bestIdx = i; }
    }
    if (bestIdx < 0) return false;

    DroppedGun *g = &pm->guns[bestIdx];
    pm->hasPickup = true;
    pm->pickupType = g->weaponType;
    pm->pickupTimer = 0;
    pm->pickupMuzzleFlash = 0;
    pm->pickupRecoil = 0;
    pm->burstRemaining = 0;
    pm->burstTimer = 0;
    pm->chargeTime = 0;
    pm->charging = false;

    switch (g->weaponType) {
        case PICKUP_KOSMOS7:
            pm->pickupAmmo = PICKUP_SOVIET_AMMO;
            pm->pickupFireRate = PICKUP_SOVIET_FIRE_RATE;
            pm->pickupDamage = PICKUP_SOVIET_DAMAGE;
            break;
        case PICKUP_LIBERTY:
            pm->pickupAmmo = PICKUP_AMERICAN_AMMO;
            pm->pickupFireRate = PICKUP_AMERICAN_FIRE_RATE;
            pm->pickupDamage = PICKUP_AMERICAN_DAMAGE;
            break;
        case PICKUP_KS23_MOLOT:
            pm->pickupAmmo = PICKUP_MOLOT_AMMO;
            pm->pickupFireRate = PICKUP_MOLOT_FIRE_RATE;
            pm->pickupDamage = PICKUP_MOLOT_DAMAGE;
            break;
        case PICKUP_M8A1_STARHAWK:
            pm->pickupAmmo = PICKUP_STARHAWK_AMMO;
            pm->pickupFireRate = PICKUP_STARHAWK_BURST_CD;
            pm->pickupDamage = PICKUP_STARHAWK_DAMAGE;
            break;
        case PICKUP_ZARYA_TK4:
            pm->pickupAmmo = PICKUP_ZARYA_AMMO;
            pm->pickupFireRate = 0.1f; // minimal, charge-based
            pm->pickupDamage = PICKUP_ZARYA_DMG_MIN;
            break;
        case PICKUP_ARC9_LONGBOW:
            pm->pickupAmmo = PICKUP_LONGBOW_AMMO;
            pm->pickupFireRate = PICKUP_LONGBOW_FIRE_RATE;
            pm->pickupDamage = PICKUP_LONGBOW_DAMAGE;
            break;
        default: break;
    }
    g->active = false;
    return true;
}

// ============================================================================
// Fire
// ============================================================================
bool PickupFire(PickupManager *pm, Vector3 origin, Vector3 dir) {
    (void)origin; (void)dir;
    if (!pm->hasPickup || pm->pickupAmmo <= 0 || pm->pickupTimer > 0) return false;

    // Zarya TK-4: hold to charge, fires on release (handled by combat_ecs)
    if (pm->pickupType == PICKUP_ZARYA_TK4) {
        if (!pm->charging) {
            pm->charging = true;
            pm->chargeTime = 0;
            return false; // don't fire yet, just start charging
        }
        return false; // still charging
    }

    pm->pickupAmmo--;
    pm->pickupTimer = pm->pickupFireRate;
    pm->pickupMuzzleFlash = 1.0f;

    switch (pm->pickupType) {
        case PICKUP_KOSMOS7:
            pm->pickupRecoil = PICKUP_SOVIET_RECOIL;
            PlaySound(pm->sndSovietFire);
            break;
        case PICKUP_LIBERTY:
            pm->pickupRecoil = PICKUP_AMERICAN_RECOIL;
            PlaySound(pm->sndAmericanFire);
            break;
        case PICKUP_KS23_MOLOT:
            pm->pickupRecoil = PICKUP_MOLOT_RECOIL;
            PlaySound(pm->sndMolotFire);
            break;
        case PICKUP_M8A1_STARHAWK:
            // Start burst: first round fires now, remaining auto-fire
            pm->pickupRecoil = PICKUP_STARHAWK_RECOIL;
            pm->burstRemaining = PICKUP_STARHAWK_BURST_SIZE - 1;
            pm->burstTimer = PICKUP_STARHAWK_FIRE_RATE;
            PlaySound(pm->sndStarhawkFire);
            break;
        case PICKUP_ARC9_LONGBOW:
            pm->pickupRecoil = PICKUP_LONGBOW_RECOIL;
            PlaySound(pm->sndLongbowFire);
            break;
        default: break;
    }
    return true;
}

// Zarya TK-4: release charge to fire
bool PickupFireZaryaRelease(PickupManager *pm) {
    if (!pm->hasPickup || pm->pickupType != PICKUP_ZARYA_TK4) return false;
    if (!pm->charging) return false;

    pm->charging = false;
    pm->pickupAmmo--;
    pm->pickupMuzzleFlash = 1.0f;
    pm->pickupTimer = 0.3f; // short cooldown after release

    // Damage scales with charge time
    float chargeRatio = pm->chargeTime / PICKUP_ZARYA_CHARGE_TIME;
    if (chargeRatio > 1.0f) chargeRatio = 1.0f;
    pm->pickupDamage = PICKUP_ZARYA_DMG_MIN + (PICKUP_ZARYA_DMG_MAX - PICKUP_ZARYA_DMG_MIN) * chargeRatio;
    pm->pickupRecoil = PICKUP_ZARYA_RECOIL * (0.5f + 0.5f * chargeRatio);

    PlaySound(pm->sndZaryaFire);
    pm->chargeTime = 0;
    return true;
}

// ============================================================================
// First-person viewmodels
// ============================================================================
void PickupDrawFirstPerson(PickupManager *pm, Camera3D camera, float weaponBobTimer) {
    if (!pm->hasPickup) return;

    Vector3 forward = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
    Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, camera.up));
    Vector3 up = Vector3CrossProduct(right, forward);

    float bobX = sinf(weaponBobTimer) * 0.02f;
    float bobY = cosf(weaponBobTimer * 2.0f) * 0.01f;
    float recoilMult = 0.05f, recoilUpMult = 0.02f;
    if (pm->pickupType == PICKUP_LIBERTY || pm->pickupType == PICKUP_ZARYA_TK4) {
        recoilMult = 0.12f; recoilUpMult = 0.06f;
    } else if (pm->pickupType == PICKUP_KS23_MOLOT) {
        recoilMult = 0.10f; recoilUpMult = 0.04f;
    }
    float recoilBack = pm->pickupRecoil * recoilMult;
    float recoilUp = pm->pickupRecoil * recoilUpMult;

    Vector3 anchor = Vector3Add(Vector3Add(Vector3Add(camera.position,
        Vector3Scale(forward, 0.5f - recoilBack)),
        Vector3Scale(right, 0.28f + bobX)),
        Vector3Scale(up, -0.2f + bobY + recoilUp));

    float yaw = atan2f(forward.x, forward.z);
    float pitch = asinf(-forward.y);

    rlPushMatrix();
    rlTranslatef(anchor.x, anchor.y, anchor.z);
    rlRotatef(yaw * RAD2DEG, 0, 1, 0);
    rlRotatef(pitch * RAD2DEG, 1, 0, 0);

    switch (pm->pickupType) {
    case PICKUP_KOSMOS7: {
        Color GM = {45,48,55,255};
        Color BK = {30,32,38,255};
        Color RD = {255,50,30,220};
        DrawCube((Vector3){0,0,0.08f}, 0.07f, 0.06f, 0.5f, GM);
        DrawCubeWires((Vector3){0,0,0.08f}, 0.071f, 0.061f, 0.501f, BK);
        DrawCube((Vector3){0,0,0.4f}, 0.04f, 0.04f, 0.18f, BK);
        DrawCube((Vector3){0,0,0.52f}, 0.055f, 0.055f, 0.04f, GM);
        DrawSphereEx((Vector3){0,-0.08f,0.02f}, 0.06f, 4, 4, GM);
        DrawCube((Vector3){0,0.035f,0.08f}, 0.005f, 0.012f, 0.35f, RD);
        DrawCube((Vector3){0,-0.06f,-0.1f}, 0.03f, 0.08f, 0.03f, (Color){50,45,40,255});
        if (pm->pickupMuzzleFlash > 0)
            DrawSphere((Vector3){0,0,0.56f}, pm->pickupMuzzleFlash*0.08f,
                (Color){255,180,50,(unsigned char)(pm->pickupMuzzleFlash*220)});
        break;
    }
    case PICKUP_LIBERTY: {
        Color GM = {35,40,55,255};
        Color BK = {25,28,40,255};
        Color BL = {60,140,255,220};
        DrawCube((Vector3){0,0,0.06f}, 0.06f, 0.06f, 0.4f, GM);
        DrawCubeWires((Vector3){0,0,0.06f}, 0.061f, 0.061f, 0.401f, BK);
        DrawCube((Vector3){0,0,0.3f}, 0.08f, 0.08f, 0.06f, GM);
        DrawCube((Vector3){0,0,0.36f}, 0.11f, 0.11f, 0.03f, BK);
        DrawCube((Vector3){0,0.06f,-0.02f}, 0.04f, 0.04f, 0.04f, BL);
        DrawCube((Vector3){0,0.035f,0.06f}, 0.005f, 0.012f, 0.3f, BL);
        DrawCube((Vector3){0,-0.06f,-0.06f}, 0.03f, 0.08f, 0.03f, (Color){50,50,55,255});
        if (pm->pickupMuzzleFlash > 0) {
            DrawSphere((Vector3){0,0,0.4f}, pm->pickupMuzzleFlash*0.18f,
                (Color){80,180,255,(unsigned char)(pm->pickupMuzzleFlash*255)});
            DrawSphere((Vector3){0,0,0.38f}, pm->pickupMuzzleFlash*0.12f,
                (Color){200,230,255,(unsigned char)(pm->pickupMuzzleFlash*150)});
        }
        break;
    }
    case PICKUP_KS23_MOLOT: {
        // Chunky double-barrel shotgun, brass accents, red faction
        Color GM = {50,45,42,255};
        Color BR = {180,150,80,255}; // brass
        Color RD = {255,50,30,220};
        Color BK = {30,28,25,255};
        // Double barrel
        DrawCube((Vector3){-0.025f,0,0.1f}, 0.04f, 0.05f, 0.55f, GM);
        DrawCube((Vector3){ 0.025f,0,0.1f}, 0.04f, 0.05f, 0.55f, GM);
        DrawCubeWires((Vector3){0,0,0.1f}, 0.09f, 0.06f, 0.56f, BK);
        // Receiver
        DrawCube((Vector3){0,0,-0.05f}, 0.08f, 0.07f, 0.15f, GM);
        // Brass trim rings
        DrawCube((Vector3){0,0,0.42f}, 0.07f, 0.06f, 0.02f, BR);
        DrawCube((Vector3){0,0,0.0f}, 0.085f, 0.075f, 0.02f, BR);
        // Pump grip
        DrawCube((Vector3){0,-0.04f,0.2f}, 0.06f, 0.04f, 0.1f, BK);
        // Red sight
        DrawCube((Vector3){0,0.035f,0.1f}, 0.005f, 0.01f, 0.3f, RD);
        // Stock
        DrawCube((Vector3){0,-0.02f,-0.15f}, 0.04f, 0.06f, 0.12f, (Color){60,50,40,255});
        if (pm->pickupMuzzleFlash > 0) {
            DrawSphere((Vector3){-0.025f,0,0.44f}, pm->pickupMuzzleFlash*0.1f,
                (Color){255,150,30,(unsigned char)(pm->pickupMuzzleFlash*255)});
            DrawSphere((Vector3){ 0.025f,0,0.44f}, pm->pickupMuzzleFlash*0.1f,
                (Color){255,150,30,(unsigned char)(pm->pickupMuzzleFlash*255)});
        }
        break;
    }
    case PICKUP_M8A1_STARHAWK: {
        // Sleek angular rifle, blue energy rails, chrome barrel
        Color CH = {180,185,195,255}; // chrome
        Color BL = {60,140,255,220};
        Color DK = {40,42,50,255};
        // Main body
        DrawCube((Vector3){0,0,0.08f}, 0.05f, 0.06f, 0.55f, DK);
        DrawCubeWires((Vector3){0,0,0.08f}, 0.051f, 0.061f, 0.551f, (Color){20,22,30,255});
        // Chrome barrel
        DrawCube((Vector3){0,0,0.42f}, 0.03f, 0.03f, 0.2f, CH);
        // Angular receiver
        DrawCube((Vector3){0,0,-0.02f}, 0.06f, 0.07f, 0.12f, DK);
        // Blue energy rails (two parallel)
        DrawCube((Vector3){-0.03f,0.035f,0.1f}, 0.005f, 0.01f, 0.4f, BL);
        DrawCube((Vector3){ 0.03f,0.035f,0.1f}, 0.005f, 0.01f, 0.4f, BL);
        // Scope
        DrawCube((Vector3){0,0.055f,0.05f}, 0.025f, 0.025f, 0.08f, CH);
        // Grip
        DrawCube((Vector3){0,-0.06f,-0.04f}, 0.025f, 0.07f, 0.025f, (Color){50,52,58,255});
        if (pm->pickupMuzzleFlash > 0) {
            DrawSphere((Vector3){0,0,0.56f}, pm->pickupMuzzleFlash*0.06f,
                (Color){80,180,255,(unsigned char)(pm->pickupMuzzleFlash*255)});
        }
        break;
    }
    case PICKUP_ZARYA_TK4: {
        // Ornate pistol, red glowing charge core, gold trim
        Color GM = {45,40,38,255};
        Color GD = {200,170,50,255}; // gold
        Color BK = {30,28,25,255};
        // Barrel
        DrawCube((Vector3){0,0,0.05f}, 0.04f, 0.04f, 0.3f, GM);
        DrawCubeWires((Vector3){0,0,0.05f}, 0.041f, 0.041f, 0.301f, BK);
        // Receiver
        DrawCube((Vector3){0,0,-0.08f}, 0.06f, 0.06f, 0.1f, GM);
        // Gold trim
        DrawCube((Vector3){0,0,0.0f}, 0.065f, 0.065f, 0.015f, GD);
        DrawCube((Vector3){0,0,0.22f}, 0.045f, 0.045f, 0.015f, GD);
        // Red energy core (glows brighter when charging)
        float chargeGlow = pm->charging ? (0.5f + 0.5f * (pm->chargeTime / PICKUP_ZARYA_CHARGE_TIME)) : 0.3f;
        unsigned char coreAlpha = (unsigned char)(chargeGlow * 255);
        DrawSphere((Vector3){0,0.04f,-0.02f}, 0.025f,
            (Color){255, 50, 30, coreAlpha});
        if (pm->charging) {
            // Pulsing charge glow
            float pulse = 0.5f + 0.5f * sinf(pm->chargeTime * 12.0f);
            DrawSphere((Vector3){0,0.04f,-0.02f}, 0.035f * pulse,
                (Color){255, 100, 50, (unsigned char)(pulse * 100)});
        }
        // Grip
        DrawCube((Vector3){0,-0.05f,-0.06f}, 0.03f, 0.07f, 0.03f, (Color){55,48,40,255});
        if (pm->pickupMuzzleFlash > 0) {
            DrawSphere((Vector3){0,0,0.24f}, pm->pickupMuzzleFlash*0.15f,
                (Color){255,80,30,(unsigned char)(pm->pickupMuzzleFlash*255)});
        }
        break;
    }
    case PICKUP_ARC9_LONGBOW: {
        // Long thin rod, blue-white energy coils, lens at tip
        Color CH = {180,185,195,255};
        Color BW = {200,220,255,220}; // blue-white
        Color DK = {35,38,48,255};
        // Long barrel rod
        DrawCube((Vector3){0,0,0.15f}, 0.025f, 0.025f, 0.7f, CH);
        // Receiver block
        DrawCube((Vector3){0,0,-0.05f}, 0.06f, 0.06f, 0.12f, DK);
        DrawCubeWires((Vector3){0,0,-0.05f}, 0.061f, 0.061f, 0.121f, (Color){20,22,30,255});
        // Energy coils wrapped around barrel (3 rings)
        for (int c = 0; c < 3; c++) {
            float cz = 0.1f + (float)c * 0.18f;
            DrawCube((Vector3){0,0,cz}, 0.04f, 0.04f, 0.015f, BW);
        }
        // Lens at tip
        DrawSphere((Vector3){0,0,0.54f}, 0.02f, BW);
        // Grip
        DrawCube((Vector3){0,-0.05f,-0.03f}, 0.025f, 0.06f, 0.025f, (Color){45,48,55,255});
        if (pm->pickupMuzzleFlash > 0) {
            // Piercing beam flash — long and narrow
            DrawSphere((Vector3){0,0,0.56f}, pm->pickupMuzzleFlash*0.08f,
                (Color){150,200,255,(unsigned char)(pm->pickupMuzzleFlash*255)});
            DrawSphere((Vector3){0,0,0.6f}, pm->pickupMuzzleFlash*0.04f,
                (Color){255,255,255,(unsigned char)(pm->pickupMuzzleFlash*200)});
        }
        break;
    }
    default: break;
    }

    rlPopMatrix();
}

// ============================================================================
// World pickup models (floating spinning guns)
// ============================================================================
void PickupManagerDraw(PickupManager *pm) {
    for (int i = 0; i < MAX_PICKUPS; i++) {
        DroppedGun *g = &pm->guns[i];
        if (!g->active) continue;

        Vector3 p = g->position;
        float bob = sinf(g->bobTimer * 3.0f) * 0.15f;
        float spin = g->bobTimer * 90.0f;
        p.y += 0.5f + bob;

        if (g->life < 2.0f && sinf(g->life * 12.0f) < 0) continue;

        rlPushMatrix();
        rlTranslatef(p.x, p.y, p.z);
        rlRotatef(spin, 0, 1, 0);

        switch (g->weaponType) {
        case PICKUP_KOSMOS7: {
            Color GM = {45,48,55,255};
            Color RD = {255,50,30,220};
            DrawCube((Vector3){0,0,0}, 0.12f, 0.1f, 0.7f, GM);
            DrawCube((Vector3){0,0,0.42f}, 0.07f, 0.07f, 0.2f, (Color){30,32,38,255});
            DrawSphereEx((Vector3){0,-0.12f,0}, 0.1f, 4, 4, GM);
            DrawCube((Vector3){0,0.055f,0}, 0.01f, 0.02f, 0.5f, RD);
            break;
        }
        case PICKUP_LIBERTY: {
            Color GM = {35,40,55,255};
            Color BL = {60,140,255,220};
            DrawCube((Vector3){0,0,0}, 0.1f, 0.1f, 0.55f, GM);
            DrawCube((Vector3){0,0,0.34f}, 0.14f, 0.14f, 0.08f, GM);
            DrawCube((Vector3){0,0,0.42f}, 0.2f, 0.2f, 0.04f, (Color){25,28,40,255});
            DrawCube((Vector3){0,0.09f,-0.05f}, 0.06f, 0.06f, 0.06f, BL);
            break;
        }
        case PICKUP_KS23_MOLOT: {
            Color GM = {50,45,42,255};
            Color BR = {180,150,80,255};
            DrawCube((Vector3){-0.04f,0,0}, 0.06f, 0.08f, 0.7f, GM);
            DrawCube((Vector3){ 0.04f,0,0}, 0.06f, 0.08f, 0.7f, GM);
            DrawCube((Vector3){0,0,0.38f}, 0.1f, 0.09f, 0.03f, BR);
            DrawCube((Vector3){0,0,-0.1f}, 0.12f, 0.1f, 0.15f, GM);
            break;
        }
        case PICKUP_M8A1_STARHAWK: {
            Color CH = {180,185,195,255};
            Color BL = {60,140,255,220};
            Color DK = {40,42,50,255};
            DrawCube((Vector3){0,0,0}, 0.07f, 0.08f, 0.65f, DK);
            DrawCube((Vector3){0,0,0.38f}, 0.04f, 0.04f, 0.2f, CH);
            DrawCube((Vector3){-0.04f,0.05f,0}, 0.005f, 0.01f, 0.5f, BL);
            DrawCube((Vector3){ 0.04f,0.05f,0}, 0.005f, 0.01f, 0.5f, BL);
            break;
        }
        case PICKUP_ZARYA_TK4: {
            Color GM = {45,40,38,255};
            Color GD = {200,170,50,255};
            DrawCube((Vector3){0,0,0}, 0.06f, 0.06f, 0.4f, GM);
            DrawCube((Vector3){0,0,0.0f}, 0.08f, 0.08f, 0.015f, GD);
            DrawSphere((Vector3){0,0.05f,-0.05f}, 0.035f, (Color){255,50,30,180});
            DrawCube((Vector3){0,0,-0.1f}, 0.08f, 0.08f, 0.1f, GM);
            break;
        }
        case PICKUP_ARC9_LONGBOW: {
            Color CH = {180,185,195,255};
            Color BW = {200,220,255,220};
            Color DK = {35,38,48,255};
            DrawCube((Vector3){0,0,0}, 0.035f, 0.035f, 0.8f, CH);
            DrawCube((Vector3){0,0,-0.2f}, 0.08f, 0.08f, 0.12f, DK);
            for (int c = 0; c < 3; c++) {
                float cz = -0.1f + (float)c * 0.2f;
                DrawCube((Vector3){0,0,cz}, 0.05f, 0.05f, 0.015f, BW);
            }
            DrawSphere((Vector3){0,0,0.42f}, 0.025f, BW);
            break;
        }
        default: break;
        }

        rlPopMatrix();
        // Pickup prompt glow (drawn in world space, after matrix pop)
        DrawSphereEx(p, 0.6f + bob * 0.3f, 4, 4, (Color){255, 255, 200, 30});
    }
}

void PickupManagerSetSFXVolume(PickupManager *pm, float vol) {
    if (!pm->soundLoaded) return;
    SetSoundVolume(pm->sndSovietFire, vol);
    SetSoundVolume(pm->sndAmericanFire, vol);
    SetSoundVolume(pm->sndMolotFire, vol);
    SetSoundVolume(pm->sndStarhawkFire, vol);
    SetSoundVolume(pm->sndZaryaFire, vol);
    SetSoundVolume(pm->sndLongbowFire, vol);
}
