#include "weapon.h"
#include "rlgl.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>

// ---- Procedural sound generation ----
static Wave GenWaveNoise(float duration, float freq, float decay) {
    int sampleRate = 44100;
    int samples = (int)(duration * sampleRate);
    Wave wave = {0};
    wave.frameCount = samples;
    wave.sampleRate = sampleRate;
    wave.sampleSize = 16;
    wave.channels = 1;
    wave.data = RL_CALLOC(samples, sizeof(short));
    short *d = (short *)wave.data;
    for (int i = 0; i < samples; i++) {
        float t = (float)i / sampleRate;
        float env = expf(-t * decay);
        float noise = ((float)rand() / RAND_MAX * 2.0f - 1.0f);
        float tone = sinf(t * freq * 2.0f * PI);
        d[i] = (short)((noise * 0.6f + tone * 0.4f) * env * 28000.0f);
    }
    return wave;
}

static Sound GenSoundGunshot(void) {
    Wave w = GenWaveNoise(0.15f, 120.0f, 25.0f);
    Sound s = LoadSoundFromWave(w);
    UnloadWave(w);
    return s;
}

static Sound GenSoundBeamBlast(void) {
    // 2.5 second sustained energy beam roar
    int sr = 44100, samples = (int)(2.5f * sr);
    Wave wave = {0};
    wave.frameCount = samples; wave.sampleRate = sr; wave.sampleSize = 16; wave.channels = 1;
    wave.data = RL_CALLOC(samples, sizeof(short));
    short *d = (short *)wave.data;
    for (int i = 0; i < samples; i++) {
        float t = (float)i / sr;
        // Ramp up then sustain then fade
        float env = 1.0f;
        if (t < 0.1f) env = t / 0.1f; // ramp up
        if (t > 2.0f) env = (2.5f - t) / 0.5f; // fade out
        // Deep rumble + mid screech + high sizzle
        float rumble = sinf(t * 55.0f * 2.0f * PI) * 0.4f;
        float screech = sinf(t * 280.0f * 2.0f * PI + sinf(t * 8.0f) * 3.0f) * 0.3f;
        float sizzle = ((float)rand() / RAND_MAX * 2.0f - 1.0f) * 0.25f;
        float pulse = 1.0f + sinf(t * 12.0f) * 0.15f; // pulsing intensity
        d[i] = (short)((rumble + screech + sizzle) * env * pulse * 22000.0f);
    }
    Sound s = LoadSoundFromWave(wave);
    UnloadWave(wave);
    return s;
}

static Sound GenSoundJackhammer(void) {
    Wave w = GenWaveNoise(0.12f, 200.0f, 30.0f);
    Sound s = LoadSoundFromWave(w);
    UnloadWave(w);
    return s;
}

static Sound GenSoundReload(void) {
    int sr = 44100, samples = (int)(0.5f * sr);
    Wave wave = {0};
    wave.frameCount = samples; wave.sampleRate = sr; wave.sampleSize = 16; wave.channels = 1;
    wave.data = RL_CALLOC(samples, sizeof(short));
    short *d = (short *)wave.data;
    for (int i = 0; i < samples; i++) {
        float t = (float)i / sr;
        float click1 = (t > 0.05f && t < 0.08f) ? sinf(t * 3000) * 0.8f : 0;
        float click2 = (t > 0.25f && t < 0.3f) ? sinf(t * 2000) * 0.6f : 0;
        float slide = (t > 0.1f && t < 0.2f) ? ((float)rand()/RAND_MAX - 0.5f) * 0.3f : 0;
        d[i] = (short)((click1 + click2 + slide) * 20000.0f);
    }
    Sound s = LoadSoundFromWave(wave);
    UnloadWave(wave);
    return s;
}

static Sound GenSoundExplosion(void) {
    int sr = 44100, samples = (int)(0.6f * sr);
    Wave wave = {0};
    wave.frameCount = samples; wave.sampleRate = sr; wave.sampleSize = 16; wave.channels = 1;
    wave.data = RL_CALLOC(samples, sizeof(short));
    short *d = (short *)wave.data;
    for (int i = 0; i < samples; i++) {
        float t = (float)i / sr;
        float env = expf(-t * 5.0f);
        float boom = sinf(t * 50.0f * 2.0f * PI) * expf(-t * 8.0f);
        float noise = ((float)rand() / RAND_MAX * 2.0f - 1.0f);
        float rumble = sinf(t * 25.0f * 2.0f * PI) * expf(-t * 3.0f);
        d[i] = (short)((boom * 0.4f + noise * 0.3f * env + rumble * 0.3f) * 30000.0f);
    }
    Sound s = LoadSoundFromWave(wave);
    UnloadWave(wave);
    return s;
}

static Sound GenSoundEmpty(void) {
    int sr = 44100, samples = (int)(0.05f * sr);
    Wave wave = {0};
    wave.frameCount = samples; wave.sampleRate = sr; wave.sampleSize = 16; wave.channels = 1;
    wave.data = RL_CALLOC(samples, sizeof(short));
    short *d = (short *)wave.data;
    for (int i = 0; i < samples; i++) {
        float t = (float)i / sr;
        d[i] = (short)(sinf(t * 4000) * expf(-t * 80) * 15000.0f);
    }
    Sound s = LoadSoundFromWave(wave);
    UnloadWave(wave);
    return s;
}

void WeaponInit(Weapon *w) {
    bool hadSound = w->soundLoaded;
    Sound s1=w->sndMp40Fire, s2=w->sndRaketenFire, s3=w->sndJackhammerHit, s4=w->sndReload, s5=w->sndEmpty, s6=w->sndExplosion;

    memset(w, 0, sizeof(Weapon));
    w->current = WEAPON_MOND_MP40;
    w->mp40Mag = 32; w->mp40MagSize = 32; w->mp40Reserve = 128;
    w->mp40FireRate = 0.08f; w->mp40Damage = 25.0f;
    w->raketenMag = 5; w->raketenMagSize = 5; w->raketenReserve = 10;
    w->raketenFireRate = 0.1f; w->raketenDamage = 500.0f; w->raketenRadius = 3.0f;
    w->raketenBeamDuration = 2.0f;
    w->raketenCooldown = 2.0f;
    w->jackhammerDamage = 45.0f; w->jackhammerRange = 3.5f; w->jackhammerFireRate = 0.35f;

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
}

void WeaponUnload(Weapon *w) {
    if (w->soundLoaded) {
        UnloadSound(w->sndMp40Fire); UnloadSound(w->sndRaketenFire);
        UnloadSound(w->sndJackhammerHit); UnloadSound(w->sndReload); UnloadSound(w->sndEmpty);
        UnloadSound(w->sndExplosion);
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
    if (w->jackhammerAnim > 0) w->jackhammerAnim -= dt * 6.0f;

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
                    {0, 220, 255, 180}, 0.1f};
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
            w->jackhammerAnim = 1.0f; w->recoil = 1.5f;
            PlaySound(w->sndJackhammerHit);
            return true;
        default: return false;
    }
}

void WeaponSwitch(Weapon *w, WeaponType type) { w->current = type; }

// ---- PROCEDURAL WW2 WEAPON DRAWING ----
void WeaponDrawFirst(Weapon *w, Camera3D camera) {
    Vector3 forward = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
    float yaw = atan2f(forward.x, forward.z);
    float pitch = asinf(-forward.y);

    float bobX = sinf(w->weaponBobTimer) * 0.02f;
    float bobY = cosf(w->weaponBobTimer * 2.0f) * 0.01f;
    float recoilBack = w->recoil * 0.06f;
    float reloadTilt = w->reloading ? sinf(w->reloadTimer / w->reloadDuration * PI) * 25.0f : 0;

    Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, camera.up));
    Vector3 up = Vector3CrossProduct(right, forward);
    Vector3 anchor = Vector3Add(Vector3Add(Vector3Add(camera.position,
        Vector3Scale(forward, 0.5f - recoilBack)),
        Vector3Scale(right, 0.28f + bobX)),
        Vector3Scale(up, -0.2f + bobY));

    rlPushMatrix();
    rlTranslatef(anchor.x, anchor.y, anchor.z);
    rlRotatef(yaw * RAD2DEG, 0, 1, 0);
    rlRotatef(pitch * RAD2DEG, 1, 0, 0);
    if (reloadTilt != 0) rlRotatef(reloadTilt, 1, 0, 0);

    switch (w->current) {
        case WEAPON_MOND_MP40: {
            // === MOND-MP40: Chrome space SMG with MP40 silhouette ===
            Color CH = {180,185,192,255};  // chrome
            Color ST = {140,145,155,255};  // steel
            Color BK = {35,38,45,255};     // black
            Color GP = {50,45,40,255};     // grip (dark wood)
            Color CY = {0,210,255,230};    // cyan energy

            // --- Receiver body (main shape) ---
            DrawCube((Vector3){0, 0, 0.08f}, 0.07f, 0.075f, 0.48f, CH);
            DrawCube((Vector3){0, 0.005f, 0.08f}, 0.06f, 0.065f, 0.46f, ST); // inner bevel
            // Receiver top flat
            DrawCube((Vector3){0, 0.04f, 0.04f}, 0.058f, 0.012f, 0.38f, CH);
            // Ejection port (right side cutout)
            DrawCube((Vector3){0.037f, 0.01f, 0.12f}, 0.005f, 0.03f, 0.06f, BK);

            // --- Barrel shroud + barrel ---
            DrawCube((Vector3){0, 0, 0.4f}, 0.04f, 0.04f, 0.22f, ST);
            // Perforated shroud (holes = thin dark lines)
            for (int h = 0; h < 6; h++) {
                float z = 0.32f + h * 0.03f;
                DrawCube((Vector3){0.022f, 0, z}, 0.003f, 0.02f, 0.008f, BK);
                DrawCube((Vector3){-0.022f, 0, z}, 0.003f, 0.02f, 0.008f, BK);
            }
            // Inner barrel
            DrawCube((Vector3){0, 0, 0.52f}, 0.02f, 0.02f, 0.08f, BK);
            // Muzzle brake — flared cylinder shape
            DrawCube((Vector3){0, 0, 0.58f}, 0.035f, 0.035f, 0.04f, ST);
            DrawCube((Vector3){0, 0, 0.61f}, 0.04f, 0.04f, 0.015f, CH);
            // Muzzle brake slots
            DrawCube((Vector3){0.02f, 0, 0.58f}, 0.003f, 0.015f, 0.035f, BK);
            DrawCube((Vector3){-0.02f, 0, 0.58f}, 0.003f, 0.015f, 0.035f, BK);

            // --- Cyan energy coils (space-age!) ---
            DrawCube((Vector3){0.04f, 0, 0.25f}, 0.006f, 0.018f, 0.2f, CY);
            DrawCube((Vector3){-0.04f, 0, 0.25f}, 0.006f, 0.018f, 0.2f, CY);
            // Energy glow behind muzzle
            DrawCube((Vector3){0, 0.025f, 0.5f}, 0.015f, 0.006f, 0.06f, CY);

            // --- MAGAZINE (MP40 signature — left side, big and obvious) ---
            rlPushMatrix();
            rlTranslatef(-0.055f, -0.06f, 0.06f);
            rlRotatef(-6, 0, 0, 1);
            // Mag body — dark, chunky, sticks out
            DrawCube((Vector3){0, 0, 0}, 0.035f, 0.2f, 0.055f, BK);
            DrawCubeWires((Vector3){0, 0, 0}, 0.036f, 0.201f, 0.056f, ST);
            // Mag ribs (horizontal lines for detail)
            for (int mr = 0; mr < 4; mr++) {
                float my = -0.07f + mr * 0.04f;
                DrawCube((Vector3){-0.019f, my, 0}, 0.002f, 0.008f, 0.05f, ST);
            }
            // Cyan energy window — bright, glowing, visible
            DrawCube((Vector3){-0.02f, 0, 0}, 0.004f, 0.14f, 0.03f, CY);
            DrawCube((Vector3){-0.021f, 0, 0}, 0.002f, 0.12f, 0.025f, (Color){0,255,255,150}); // inner glow
            // Base plate
            DrawCube((Vector3){0, -0.105f, 0}, 0.04f, 0.015f, 0.06f, ST);
            // Feed lips at top
            DrawCube((Vector3){0, 0.1f, 0}, 0.03f, 0.012f, 0.04f, ST);
            rlPopMatrix();

            // --- Pistol grip (angled back like MP40) ---
            rlPushMatrix();
            rlTranslatef(0, -0.06f, -0.02f);
            rlRotatef(12, 1, 0, 0); // angled back
            DrawCube((Vector3){0, -0.03f, 0}, 0.035f, 0.1f, 0.035f, GP);
            DrawCubeWires((Vector3){0, -0.03f, 0}, 0.036f, 0.101f, 0.036f, BK);
            // Grip texture lines
            DrawCube((Vector3){0.019f, -0.03f, 0}, 0.002f, 0.06f, 0.025f, BK);
            DrawCube((Vector3){-0.019f, -0.03f, 0}, 0.002f, 0.06f, 0.025f, BK);
            rlPopMatrix();
            // Trigger guard
            DrawCube((Vector3){0, -0.045f, 0.02f}, 0.008f, 0.025f, 0.06f, ST);
            // Trigger
            DrawCube((Vector3){0, -0.04f, 0.03f}, 0.005f, 0.015f, 0.008f, BK);

            // --- Folding wire stock ---
            DrawCube((Vector3){0, 0.015f, -0.15f}, 0.008f, 0.008f, 0.15f, ST); // top rod
            DrawCube((Vector3){0, -0.03f, -0.15f}, 0.008f, 0.008f, 0.15f, ST); // bottom rod
            DrawCube((Vector3){0, -0.008f, -0.23f}, 0.025f, 0.05f, 0.008f, ST); // butt plate
            // Stock hinge
            DrawCube((Vector3){0, -0.008f, -0.06f}, 0.015f, 0.015f, 0.015f, ST);

            // --- Sights ---
            // Rear sight (notch)
            DrawCube((Vector3){0, 0.052f, -0.02f}, 0.018f, 0.02f, 0.008f, BK);
            DrawCube((Vector3){0, 0.058f, -0.02f}, 0.006f, 0.008f, 0.008f, BK); // notch gap
            // Front sight (post)
            DrawCube((Vector3){0, 0.048f, 0.5f}, 0.004f, 0.02f, 0.004f, BK);
            DrawCube((Vector3){0, 0.058f, 0.5f}, 0.008f, 0.004f, 0.008f, BK); // hood

            // --- Bolt handle (right side) ---
            DrawCube((Vector3){0.04f, 0.01f, 0.05f}, 0.012f, 0.012f, 0.012f, ST);

            if (w->muzzleFlash > 0)
                DrawSphere((Vector3){0,0,0.64f}, w->muzzleFlash * 0.1f,
                    (Color){0,230,255,(unsigned char)(w->muzzleFlash*220)});
            break;
        }
        case WEAPON_RAKETENFAUST: {
            // === RAKETENFAUST: 60s space Panzerfaust beam weapon ===
            Color TB = {105,115,100,255};  // tube olive
            Color DK = {70,78,65,255};     // dark olive
            Color BK = {35,40,32,255};     // near black
            Color YW = {215,190,40,255};   // hazard yellow
            Color WD = {70,58,42,255};     // wood grip
            Color OR = {255,160,50,210};   // energy orange
            Color GL = {200,220,255,180};  // glass

            // --- Main tube body ---
            DrawCube((Vector3){0, 0, 0}, 0.1f, 0.1f, 0.6f, TB);
            DrawCubeWires((Vector3){0, 0, 0}, 0.101f, 0.101f, 0.601f, DK);
            // Inner tube (darker)
            DrawCube((Vector3){0, 0, 0.02f}, 0.075f, 0.075f, 0.55f, DK);

            // --- Flared bell nozzle (3 steps) ---
            DrawCube((Vector3){0, 0, 0.36f}, 0.12f, 0.12f, 0.06f, TB);
            DrawCube((Vector3){0, 0, 0.42f}, 0.15f, 0.15f, 0.04f, DK);
            DrawCube((Vector3){0, 0, 0.46f}, 0.18f, 0.18f, 0.03f, BK);
            DrawCubeWires((Vector3){0, 0, 0.46f}, 0.181f, 0.181f, 0.031f, DK);
            // Nozzle inner glow
            DrawCube((Vector3){0, 0, 0.47f}, 0.1f, 0.1f, 0.01f, OR);

            // --- Tail fins (swept back, 4) ---
            for (int f = 0; f < 4; f++) {
                float fa = f * 1.5708f;
                float fx = cosf(fa) * 0.07f, fy = sinf(fa) * 0.07f;
                DrawCube((Vector3){fx, fy, -0.18f},
                    (f%2==0) ? 0.04f : 0.006f,
                    (f%2==0) ? 0.006f : 0.04f,
                    0.12f, TB);
                // Fin tips
                DrawCube((Vector3){fx*1.3f, fy*1.3f, -0.22f},
                    (f%2==0) ? 0.03f : 0.005f,
                    (f%2==0) ? 0.005f : 0.03f,
                    0.03f, DK);
            }

            // --- Rear exhaust cone ---
            DrawCube((Vector3){0, 0, -0.25f}, 0.11f, 0.11f, 0.04f, BK);
            DrawCube((Vector3){0, 0, -0.28f}, 0.08f, 0.08f, 0.02f, DK);

            // --- Bubble sight (retro) ---
            DrawSphereEx((Vector3){0, 0.085f, 0.08f}, 0.028f, 6, 6, GL);
            DrawCube((Vector3){0, 0.06f, 0.08f}, 0.012f, 0.02f, 0.012f, DK); // mount
            // Crosshair inside bubble
            DrawCube((Vector3){0, 0.085f, 0.08f}, 0.001f, 0.015f, 0.015f, BK);
            DrawCube((Vector3){0, 0.085f, 0.08f}, 0.015f, 0.001f, 0.015f, BK);

            // --- Pistol grip + trigger ---
            rlPushMatrix();
            rlTranslatef(0, -0.07f, -0.06f);
            rlRotatef(10, 1, 0, 0);
            DrawCube((Vector3){0, -0.02f, 0}, 0.04f, 0.1f, 0.04f, WD);
            DrawCubeWires((Vector3){0, -0.02f, 0}, 0.041f, 0.101f, 0.041f, BK);
            rlPopMatrix();
            DrawCube((Vector3){0, -0.055f, -0.01f}, 0.008f, 0.025f, 0.06f, DK); // trigger guard
            DrawCube((Vector3){0, -0.045f, 0.0f}, 0.005f, 0.015f, 0.008f, BK); // trigger

            // --- Forward grip (left hand hold) ---
            DrawCube((Vector3){0, -0.06f, 0.15f}, 0.03f, 0.06f, 0.03f, WD);

            // --- Hazard stripes ---
            for (int s = 0; s < 5; s++)
                DrawCube((Vector3){0, -0.053f, -0.12f+s*0.065f}, 0.102f, 0.005f, 0.015f, YW);

            // --- Energy cell / rocket visible ---
            DrawSphereEx((Vector3){0, 0, 0.28f}, 0.04f, 5, 5, OR);
            // Glow ring
            DrawCube((Vector3){0, 0, 0.22f}, 0.085f, 0.085f, 0.008f, (Color){255,120,30,120});

            // --- Active beam glow ---
            if (w->raketenFiring) {
                DrawSphereEx((Vector3){0, 0, 0.48f}, 0.12f + sinf(w->raketenBeamTimer*15)*0.03f, 5, 5,
                    (Color){255,220,120,(unsigned char)(180+sinf(w->raketenBeamTimer*20)*50)});
            }

            if (w->muzzleFlash > 0) {
                DrawSphere((Vector3){0,0,0.5f}, w->muzzleFlash*0.15f,
                    (Color){255,180,50,(unsigned char)(w->muzzleFlash*200)});
                DrawSphere((Vector3){0,0,-0.3f}, w->muzzleFlash*0.08f,
                    (Color){255,200,100,(unsigned char)(w->muzzleFlash*120)});
            }
            break;
        }
        case WEAPON_JACKHAMMER: {
            // === JACKHAMMER: Mining tool turned melee weapon ===
            float ao = sinf(w->jackhammerAnim * 20.0f) * w->jackhammerAnim * 0.05f;
            Color YL = {185,175,50,255};
            Color GR = {105,105,95,255};
            Color MT = {155,155,148,255};
            Color RD = {205,50,30,255};
            Color ST = {205,205,198,255};
            Color BK = {40,40,38,255};

            rlPushMatrix();
            rlRotatef(55, 1, 0, 0);

            // Main body shaft
            DrawCube((Vector3){0, ao, 0}, 0.045f, 0.35f, 0.045f, YL);
            DrawCubeWires((Vector3){0, ao, 0}, 0.046f, 0.351f, 0.046f, (Color){150,140,40,255});
            // Motor housing (top)
            DrawCube((Vector3){0, 0.18f+ao, 0}, 0.055f, 0.06f, 0.055f, GR);
            // Grip bars
            DrawCube((Vector3){0, 0.14f+ao, 0}, 0.13f, 0.02f, 0.03f, GR);
            DrawCube((Vector3){0, 0.06f+ao, 0}, 0.11f, 0.02f, 0.03f, GR);
            // Grip rubber texture
            DrawCube((Vector3){-0.06f, 0.1f+ao, 0}, 0.008f, 0.08f, 0.032f, BK);
            DrawCube((Vector3){0.06f, 0.1f+ao, 0}, 0.008f, 0.08f, 0.032f, BK);
            // Piston housing
            DrawCube((Vector3){0, -0.13f+ao, 0}, 0.065f, 0.1f, 0.065f, MT);
            DrawCubeWires((Vector3){0, -0.13f+ao, 0}, 0.066f, 0.101f, 0.066f, GR);
            // Hazard stripes
            DrawCube((Vector3){0.034f, -0.13f+ao, 0}, 0.004f, 0.08f, 0.067f, RD);
            DrawCube((Vector3){-0.034f, -0.13f+ao, 0}, 0.004f, 0.08f, 0.067f, RD);
            DrawCube((Vector3){0, -0.13f+ao, 0.034f}, 0.067f, 0.08f, 0.004f, RD);
            // Chisel bit
            DrawCube((Vector3){0, -0.22f+ao, 0}, 0.035f, 0.1f, 0.035f, ST);
            DrawCube((Vector3){0, -0.3f+ao, 0}, 0.02f, 0.06f, 0.02f, ST);
            // Bit tip (sharp)
            DrawCube((Vector3){0, -0.34f+ao, 0}, 0.01f, 0.03f, 0.025f, (Color){220,220,215,255});

            if (w->jackhammerAnim > 0.5f)
                for (int i = 0; i < 4; i++)
                    DrawSphere((Vector3){(float)GetRandomValue(-5,5)*0.01f,
                        -0.36f+(float)GetRandomValue(-2,3)*0.01f,
                        (float)GetRandomValue(-5,5)*0.01f}, 0.01f, YELLOW);
            rlPopMatrix();
            break;
        }
        default: break;
    }
    rlPopMatrix();
}

// Get the barrel tip position in world space (for beam origin)
Vector3 WeaponGetBarrelWorldPos(Weapon *w, Camera3D camera) {
    Vector3 forward = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
    Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, camera.up));
    Vector3 up = Vector3CrossProduct(right, forward);
    // Same offset as WeaponDrawFirst anchor + barrel tip
    Vector3 barrel = camera.position;
    barrel = Vector3Add(barrel, Vector3Scale(forward, 0.5f + 0.42f)); // forward to barrel tip
    barrel = Vector3Add(barrel, Vector3Scale(right, 0.28f));
    barrel = Vector3Add(barrel, Vector3Scale(up, -0.25f));
    (void)w;
    return barrel;
}

void WeaponSpawnExplosion(Weapon *w, Vector3 pos, float radius) {
    for (int i = 0; i < MAX_EXPLOSIONS; i++) {
        if (w->explosions[i].active) continue;
        w->explosions[i].position = pos;
        w->explosions[i].radius = radius * 1.5f;
        w->explosions[i].timer = 0;
        w->explosions[i].duration = 1.2f;
        w->explosions[i].active = true;
        PlaySound(w->sndExplosion);
        return;
    }
}

void WeaponDrawWorld(Weapon *w) {
    // MEGA BEAM — one thick solid beam, no modulation
    if (w->raketenFiring) {
        Vector3 dir = Vector3Normalize(Vector3Subtract(w->raketenBeamEnd, w->raketenBeamStart));
        Vector3 right = Vector3Normalize(Vector3CrossProduct(dir, (Vector3){0,1,0}));
        Vector3 up = Vector3CrossProduct(right, dir);

        // Thick beam — grid of parallel lines filling a 0.15m radius
        float beamR = 0.15f;
        for (int rx = -2; rx <= 2; rx++) {
            for (int ry = -2; ry <= 2; ry++) {
                Vector3 off = Vector3Add(
                    Vector3Scale(right, (float)rx * beamR * 0.4f),
                    Vector3Scale(up, (float)ry * beamR * 0.4f));
                Color c = (rx == 0 && ry == 0) ? (Color){255, 255, 240, 255} :  // white core
                          (abs(rx) + abs(ry) <= 2) ? (Color){255, 200, 80, 220} : // orange mid
                          (Color){255, 120, 20, 160};  // outer
                DrawLine3D(Vector3Add(w->raketenBeamStart, off),
                           Vector3Add(w->raketenBeamEnd, off), c);
            }
        }
        // Impact point glow
        DrawSphereEx(w->raketenBeamEnd, 0.6f, 4, 4, (Color){255, 240, 180, 180});
    }

    // Beam trails
    for (int i = 0; i < w->beamCount; i++) {
        float a = w->beams[i].life / 0.1f;
        Color c = w->beams[i].color; c.a = (unsigned char)(a * c.a);
        DrawLine3D(w->beams[i].start, w->beams[i].end, c);
        DrawLine3D(Vector3Add(w->beams[i].start, (Vector3){0.02f,0.02f,0}),
                   Vector3Add(w->beams[i].end, (Vector3){0.02f,0.02f,0}), c);
    }
    // Rockets
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        if (!w->projectiles[i].active) continue;
        DrawSphere(w->projectiles[i].position, 0.25f, w->projectiles[i].color);
        DrawSphere(w->projectiles[i].position, 0.4f, (Color){255,220,100,80});
        Vector3 trail = Vector3Subtract(w->projectiles[i].position,
            Vector3Scale(Vector3Normalize(w->projectiles[i].velocity), 1.0f));
        DrawSphere(trail, 0.3f, (Color){180,180,180,60});
    }
    // Explosions
    for (int i = 0; i < MAX_EXPLOSIONS; i++) {
        if (!w->explosions[i].active) continue;
        float t = w->explosions[i].timer / w->explosions[i].duration;
        float r = w->explosions[i].radius * t;
        unsigned char alpha = (unsigned char)((1.0f - t) * 200);
        // Fireball
        DrawSphere(w->explosions[i].position, r * 0.6f, (Color){255, 200, 50, alpha});
        DrawSphere(w->explosions[i].position, r * 0.8f, (Color){255, 120, 20, (unsigned char)(alpha * 0.6f)});
        DrawSphere(w->explosions[i].position, r, (Color){255, 80, 0, (unsigned char)(alpha * 0.3f)});
        // Shockwave ring
        DrawCylinderWires(w->explosions[i].position, r * 1.2f, r * 1.3f, 0.1f, 16,
            (Color){255, 200, 100, (unsigned char)(alpha * 0.5f)});
        // Flying rock debris
        for (int j = 0; j < 8; j++) {
            float angle = (float)j * 0.785f + t * 2.0f;
            float debrisR = r * 1.5f * t;
            float dy = t * 3.0f - t * t * 4.0f; // arc up then down
            Vector3 dp = {
                w->explosions[i].position.x + cosf(angle) * debrisR,
                w->explosions[i].position.y + dy + (float)(j % 3) * 0.3f,
                w->explosions[i].position.z + sinf(angle) * debrisR
            };
            float ds = 0.08f + (1.0f - t) * 0.12f;
            DrawCube(dp, ds, ds, ds, (Color){110, 108, 100, alpha});
        }
    }
}

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
