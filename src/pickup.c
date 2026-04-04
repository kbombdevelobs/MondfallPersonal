#include "pickup.h"
#include "world.h"
#include "sound_gen.h"
#include "rlgl.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>

void PickupManagerInit(PickupManager *pm) {
    memset(pm, 0, sizeof(PickupManager));
    // Generate fire sounds
    {
        int sr = AUDIO_SAMPLE_RATE;
        Wave w = SoundGenCreateWave(sr, 0.12f);
        short *d = (short *)w.data;
        for (int i = 0; i < (int)w.frameCount; i++) {
            float t = (float)i/sr;
            d[i] = (short)(((float)rand()/RAND_MAX*2-1)*expf(-t*30)*16000 + sinf(t*140*6.283f)*expf(-t*20)*10000);
        }
        pm->sndSovietFire = LoadSoundFromWave(w); UnloadWave(w);
    }
    {
        // Liberty Blaster: heavy rail-gun crack — bass thud + electric zap + reverb tail
        int sr = AUDIO_SAMPLE_RATE;
        Wave w = SoundGenCreateWave(sr, 0.4f);
        short *d = (short *)w.data;
        for (int i = 0; i < (int)w.frameCount; i++) {
            float t = (float)i/sr;
            float crack = sinf(t*900*6.283f)*expf(-t*40)*0.4f; // sharp electric zap
            float thud = sinf(t*60*6.283f)*expf(-t*8)*0.35f;   // deep bass punch
            float sizzle = ((float)rand()/RAND_MAX*2-1)*expf(-t*15)*0.2f; // energy crackle
            float tail = sinf(t*200*6.283f)*expf(-t*5)*0.1f;   // reverb hum
            d[i] = (short)((crack + thud + sizzle + tail) * 28000);
        }
        pm->sndAmericanFire = LoadSoundFromWave(w); UnloadWave(w);
    }
    pm->soundLoaded = true;
}

void PickupDrop(PickupManager *pm, Vector3 pos, EnemyType type) {
    for (int i = 0; i < MAX_PICKUPS; i++) {
        if (pm->guns[i].active) continue;
        pm->guns[i].position = pos;
        pm->guns[i].position.y = WorldGetHeight(pos.x, pos.z) + 0.5f;
        pm->guns[i].gunType = type;
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
    // Pickup weapon timers
    if (pm->hasPickup) {
        if (pm->pickupTimer > 0) pm->pickupTimer -= dt;
        if (pm->pickupMuzzleFlash > 0) pm->pickupMuzzleFlash -= dt * 8.0f;
        // Liberty Blaster recoil recovers slower for a heavier feel
        float recoilDecay = (pm->pickupType == ENEMY_AMERICAN) ? 4.0f : 10.0f;
        if (pm->pickupRecoil > 0) pm->pickupRecoil -= dt * recoilDecay;
        // Auto-drop when empty
        if (pm->pickupAmmo <= 0) pm->hasPickup = false;
    }
}

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
    pm->pickupType = g->gunType;
    pm->pickupAmmo = (g->gunType == ENEMY_SOVIET) ? PICKUP_SOVIET_AMMO : PICKUP_AMERICAN_AMMO;
    pm->pickupFireRate = (g->gunType == ENEMY_SOVIET) ? PICKUP_SOVIET_FIRE_RATE : PICKUP_AMERICAN_FIRE_RATE;
    pm->pickupDamage = (g->gunType == ENEMY_SOVIET) ? PICKUP_SOVIET_DAMAGE : PICKUP_AMERICAN_DAMAGE;
    pm->pickupTimer = 0;
    pm->pickupMuzzleFlash = 0;
    pm->pickupRecoil = 0;
    g->active = false;
    return true;
}

bool PickupFire(PickupManager *pm, Vector3 origin, Vector3 dir) {
    (void)origin; (void)dir;
    if (!pm->hasPickup || pm->pickupAmmo <= 0 || pm->pickupTimer > 0) return false;
    pm->pickupAmmo--;
    pm->pickupTimer = pm->pickupFireRate;
    pm->pickupMuzzleFlash = 1.0f;
    // Per-weapon visual recoil: Liberty Blaster kicks hard
    pm->pickupRecoil = (pm->pickupType == ENEMY_SOVIET) ? 0.8f : 2.0f;
    if (pm->pickupType == ENEMY_SOVIET) PlaySound(pm->sndSovietFire);
    else PlaySound(pm->sndAmericanFire);
    return true;
}

void PickupDrawFirstPerson(PickupManager *pm, Camera3D camera, float weaponBobTimer) {
    if (!pm->hasPickup) return;

    Vector3 forward = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
    Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, camera.up));
    Vector3 up = Vector3CrossProduct(right, forward);

    float bobX = sinf(weaponBobTimer) * 0.02f;
    float bobY = cosf(weaponBobTimer * 2.0f) * 0.01f;
    // Liberty Blaster gets a much bigger visual kickback
    float recoilMult = (pm->pickupType == ENEMY_AMERICAN) ? 0.12f : 0.05f;
    float recoilBack = pm->pickupRecoil * recoilMult;
    float recoilUp = (pm->pickupType == ENEMY_AMERICAN) ? pm->pickupRecoil * 0.06f : pm->pickupRecoil * 0.02f;

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

    if (pm->pickupType == ENEMY_SOVIET) {
        Color GM = {45,48,55,255};
        Color BK = {30,32,38,255};
        Color RD = {255,50,30,220};
        // Soviet PPSh viewmodel — same as enemy but first person
        DrawCube((Vector3){0,0,0.08f}, 0.07f, 0.06f, 0.5f, GM);
        DrawCubeWires((Vector3){0,0,0.08f}, 0.071f, 0.061f, 0.501f, BK);
        DrawCube((Vector3){0,0,0.4f}, 0.04f, 0.04f, 0.18f, BK);
        DrawCube((Vector3){0,0,0.52f}, 0.055f, 0.055f, 0.04f, GM);
        DrawSphereEx((Vector3){0,-0.08f,0.02f}, 0.06f, 4, 4, GM); // drum mag
        DrawCube((Vector3){0,0.035f,0.08f}, 0.005f, 0.012f, 0.35f, RD);
        DrawCube((Vector3){0,-0.06f,-0.1f}, 0.03f, 0.08f, 0.03f, (Color){50,45,40,255}); // grip
        if (pm->pickupMuzzleFlash > 0)
            DrawSphere((Vector3){0,0,0.56f}, pm->pickupMuzzleFlash*0.08f,
                (Color){255,180,50,(unsigned char)(pm->pickupMuzzleFlash*220)});
    } else {
        Color GM = {35,40,55,255};
        Color BK = {25,28,40,255};
        Color BL = {60,140,255,220};
        // American ray gun viewmodel
        DrawCube((Vector3){0,0,0.06f}, 0.06f, 0.06f, 0.4f, GM);
        DrawCubeWires((Vector3){0,0,0.06f}, 0.061f, 0.061f, 0.401f, BK);
        DrawCube((Vector3){0,0,0.3f}, 0.08f, 0.08f, 0.06f, GM);
        DrawCube((Vector3){0,0,0.36f}, 0.11f, 0.11f, 0.03f, BK);
        DrawCube((Vector3){0,0.06f,-0.02f}, 0.04f, 0.04f, 0.04f, BL); // energy cube
        DrawCube((Vector3){0,0.035f,0.06f}, 0.005f, 0.012f, 0.3f, BL);
        DrawCube((Vector3){0,-0.06f,-0.06f}, 0.03f, 0.08f, 0.03f, (Color){50,50,55,255});
        if (pm->pickupMuzzleFlash > 0) {
            DrawSphere((Vector3){0,0,0.4f}, pm->pickupMuzzleFlash*0.18f,
                (Color){80,180,255,(unsigned char)(pm->pickupMuzzleFlash*255)});
            // Secondary flash ring for one-shot power feel
            DrawSphere((Vector3){0,0,0.38f}, pm->pickupMuzzleFlash*0.12f,
                (Color){200,230,255,(unsigned char)(pm->pickupMuzzleFlash*150)});
        }
    }

    rlPopMatrix();
}

void PickupManagerDraw(PickupManager *pm) {
    for (int i = 0; i < MAX_PICKUPS; i++) {
        DroppedGun *g = &pm->guns[i];
        if (!g->active) continue;

        Vector3 p = g->position;
        float bob = sinf(g->bobTimer * 3.0f) * 0.15f;
        float spin = g->bobTimer * 90.0f;
        p.y += 0.5f + bob;

        // Blinking when about to despawn
        if (g->life < 2.0f && sinf(g->life * 12.0f) < 0) continue;

        rlPushMatrix();
        rlTranslatef(p.x, p.y, p.z);
        rlRotatef(spin, 0, 1, 0);

        if (g->gunType == ENEMY_SOVIET) {
            // Floating Soviet PPSh
            Color GM = {45,48,55,255};
            Color RD = {255,50,30,220};
            DrawCube((Vector3){0,0,0}, 0.12f, 0.1f, 0.7f, GM);
            DrawCube((Vector3){0,0,0.42f}, 0.07f, 0.07f, 0.2f, (Color){30,32,38,255});
            DrawSphereEx((Vector3){0,-0.12f,0}, 0.1f, 4, 4, GM);
            DrawCube((Vector3){0,0.055f,0}, 0.01f, 0.02f, 0.5f, RD);
        } else {
            // Floating American ray gun
            Color GM = {35,40,55,255};
            Color BL = {60,140,255,220};
            DrawCube((Vector3){0,0,0}, 0.1f, 0.1f, 0.55f, GM);
            DrawCube((Vector3){0,0,0.34f}, 0.14f, 0.14f, 0.08f, GM);
            DrawCube((Vector3){0,0,0.42f}, 0.2f, 0.2f, 0.04f, (Color){25,28,40,255});
            DrawCube((Vector3){0,0.09f,-0.05f}, 0.06f, 0.06f, 0.06f, BL);
        }

        // Pickup prompt glow
        DrawSphereEx(p, 0.6f + bob * 0.3f, 4, 4, (Color){255, 255, 200, 30});

        rlPopMatrix();
    }
}
