#include "player_effects.h"
#include "config.h"
#include <math.h>

void DrawGroundPoundDust(Player *player) {
    if (player->groundPoundImpactTimer <= 0.0f) return;

    float gpT = 1.0f - (player->groundPoundImpactTimer / GROUND_POUND_DUST_LIFE);
    float ringRadius = GROUND_POUND_DUST_SPEED * gpT * GROUND_POUND_DUST_LIFE;
    float dustAlpha = (1.0f - gpT);
    dustAlpha = dustAlpha * dustAlpha * dustAlpha; // cubic fade
    /* Flat world: up is always Y, tangent plane is XZ */
    Vector3 gpUp = {0, 1, 0};
    Vector3 gpPos = player->groundPoundImpactPos;
    Vector3 gpTangent = {1, 0, 0};
    Vector3 gpBitangent = {0, 0, 1};

    // Hash helper — cheap per-particle pseudo-random from index
    #define PHASH(i, salt) (((i) * 2654435761u + (salt)) & 0xFFFFu)
    #define PH01(i, salt) ((float)PHASH(i, salt) / 65535.0f)

    // --- Layer 1: Expanding dust ring (varied sizes, colors, speeds) ---
    for (int di = 0; di < GROUND_POUND_DUST_COUNT; di++) {
        float aOff = PH01(di, 37u) * 0.4f - 0.2f;
        float angle = ((float)di / (float)GROUND_POUND_DUST_COUNT) * 2.0f * PI + aOff;
        float spd = 0.4f + PH01(di, 71u) * 1.2f;
        float r = ringRadius * spd;
        Vector3 offset = Vector3Add(
            Vector3Scale(gpTangent, cosf(angle) * r),
            Vector3Scale(gpBitangent, sinf(angle) * r));
        float lift = 0.1f + gpT * (1.0f + PH01(di, 103u) * 4.0f) * spd;
        Vector3 dPos = Vector3Add(gpPos, Vector3Add(offset, Vector3Scale(gpUp, lift)));
        float dSize = (0.3f + PH01(di, 151u) * 0.6f + gpT * (0.8f + PH01(di, 199u) * 2.0f));
        float aFade = dustAlpha * (0.6f + PH01(di, 211u) * 0.4f);
        unsigned char a = (unsigned char)(aFade * 170.0f);
        unsigned char cr = 155 + (unsigned char)(PH01(di, 233u) * 50);
        unsigned char cg = 145 + (unsigned char)(PH01(di, 251u) * 45);
        unsigned char cb = 120 + (unsigned char)(PH01(di, 269u) * 50);
        DrawSphere(dPos, dSize, (Color){cr, cg, cb, a});
    }

    // --- Layer 2: Rock chunks (varied shapes, ballistic arcs) ---
    int rockCount = 18;
    for (int ri = 0; ri < rockCount; ri++) {
        float angle = PH01(ri, 307u) * 2.0f * PI;
        float spd = 3.0f + PH01(ri, 331u) * 10.0f;
        float r = spd * gpT * GROUND_POUND_DUST_LIFE;
        Vector3 offset = Vector3Add(
            Vector3Scale(gpTangent, cosf(angle) * r),
            Vector3Scale(gpBitangent, sinf(angle) * r));
        float launchVel = 2.0f + PH01(ri, 347u) * 7.0f;
        float tSec = gpT * GROUND_POUND_DUST_LIFE;
        float rockY = launchVel * tSec - 0.5f * MOON_GRAVITY * tSec * tSec;
        if (rockY < 0.0f) rockY = 0.0f;
        Vector3 rPos = Vector3Add(gpPos, Vector3Add(offset, Vector3Scale(gpUp, rockY + 0.1f)));
        float rw = 0.06f + PH01(ri, 359u) * 0.2f;
        float rh = 0.04f + PH01(ri, 373u) * 0.15f;
        float rd = 0.05f + PH01(ri, 389u) * 0.18f;
        unsigned char ra = (unsigned char)(dustAlpha * 230.0f);
        unsigned char rc = 75 + (unsigned char)(PH01(ri, 397u) * 50);
        DrawCube(rPos, rw, rh, rd, (Color){rc, (unsigned char)(rc - 5), (unsigned char)(rc - 15), ra});
    }

    // --- Layer 3: Fine mist wisps ---
    int mistCount = 14;
    for (int mi = 0; mi < mistCount; mi++) {
        float mAngle = PH01(mi, 409u) * 2.0f * PI;
        float mDrift = 0.1f + PH01(mi, 421u) * 0.5f;
        float mR = ringRadius * mDrift;
        Vector3 mOff = Vector3Add(
            Vector3Scale(gpTangent, cosf(mAngle) * mR),
            Vector3Scale(gpBitangent, sinf(mAngle) * mR));
        float mLift = 0.3f + gpT * (3.0f + PH01(mi, 433u) * 5.0f);
        Vector3 mPos = Vector3Add(gpPos, Vector3Add(mOff, Vector3Scale(gpUp, mLift)));
        float mSize = (0.6f + PH01(mi, 443u) * 0.8f + gpT * (1.5f + PH01(mi, 457u) * 3.0f));
        float mFade = dustAlpha * dustAlpha * (0.4f + PH01(mi, 461u) * 0.6f);
        unsigned char ma = (unsigned char)(mFade * 130.0f);
        unsigned char mc = 185 + (unsigned char)(PH01(mi, 467u) * 30);
        DrawSphere(mPos, mSize, (Color){mc, (unsigned char)(mc - 8), (unsigned char)(mc - 25), ma});
    }

    // --- Layer 4: Ground-hugging radial dust streaks ---
    int streakCount = 12;
    for (int si = 0; si < streakCount; si++) {
        float sAngle = PH01(si, 479u) * 2.0f * PI;
        float sSpeed = 5.0f + PH01(si, 487u) * 8.0f;
        float sR = sSpeed * gpT * GROUND_POUND_DUST_LIFE;
        float sRInner = sR * (0.3f + PH01(si, 491u) * 0.4f);
        Vector3 sDir = Vector3Add(
            Vector3Scale(gpTangent, cosf(sAngle)),
            Vector3Scale(gpBitangent, sinf(sAngle)));
        float sLift = 0.1f + PH01(si, 499u) * 0.3f;
        Vector3 sStart = Vector3Add(gpPos, Vector3Add(Vector3Scale(sDir, sRInner), Vector3Scale(gpUp, sLift)));
        Vector3 sEnd = Vector3Add(gpPos, Vector3Add(Vector3Scale(sDir, sR), Vector3Scale(gpUp, sLift * 0.5f)));
        unsigned char sa = (unsigned char)(dustAlpha * (60.0f + PH01(si, 503u) * 60.0f));
        DrawLine3D(sStart, sEnd, (Color){170, 160, 140, sa});
    }

    #undef PHASH
    #undef PH01

    // --- Layer 5: Central shockwave flash ---
    if (gpT < 0.2f) {
        float flashAlpha = (1.0f - gpT / 0.2f);
        unsigned char fa = (unsigned char)(flashAlpha * 130.0f);
        float flashR = ringRadius * 0.5f + 2.0f;
        DrawSphere(Vector3Add(gpPos, Vector3Scale(gpUp, 0.5f)), flashR,
            (Color){220, 200, 160, fa});
    }
}

void DrawHe3Trail(Player *player) {
    if (player->he3TrailCount <= 0) return;

    for (int ti = 0; ti < player->he3TrailCount; ti++) {
        int idx = (player->he3TrailHead - player->he3TrailCount + ti + HE3_JET_TRAIL_MAX) % HE3_JET_TRAIL_MAX;
        float age = player->he3TrailAge[idx];
        float life = age / HE3_JET_TRAIL_LIFE;
        if (life > 1.0f) continue;
        float fade = (1.0f - life);
        fade *= fade;
        float seed1 = sinf((float)idx * 7.13f) * 43758.5453f;
        seed1 = seed1 - (int)seed1;
        float seed2 = sinf((float)idx * 11.97f) * 43758.5453f;
        seed2 = seed2 - (int)seed2;
        float seed3 = sinf((float)idx * 3.71f) * 43758.5453f;
        seed3 = seed3 - (int)seed3;
        // Spiral orbit
        float spiralAngle = (float)idx * 0.5f + age * 3.0f;
        float spiralR = 0.4f + age * 2.0f;
        float spiralX = cosf(spiralAngle) * spiralR;
        float spiralZ = sinf(spiralAngle) * spiralR;
        float drift = age * age * 0.5f;
        float dx = spiralX + (seed1 - 0.5f) * drift;
        float dy = (seed3 - 0.5f) * drift * 0.3f;
        float dz = spiralZ + (seed2 - 0.5f) * drift;
        Vector3 pPos = player->he3Trail[idx];
        pPos.x += dx;
        pPos.y += dy;
        pPos.z += dz;
        float pSize = 0.12f + age * 1.4f;
        unsigned char pa = (unsigned char)(fade * 150.0f);
        DrawSphere(pPos, pSize, (Color){70, 210, 240, pa});
        // Wispy secondary puff
        if (ti % 2 == 0) {
            float s2Angle = -(float)idx * 0.4f - age * 2.5f;
            float s2R = 0.3f + age * 1.6f;
            float dx2 = cosf(s2Angle) * s2R + (seed2 - 0.5f) * drift * 0.8f;
            float dz2 = sinf(s2Angle) * s2R + (seed1 - 0.5f) * drift * 0.8f;
            Vector3 p2 = player->he3Trail[idx];
            p2.x += dx2; p2.y += dy * 0.5f; p2.z += dz2;
            float p2Size = pSize * 0.7f;
            DrawSphere(p2, p2Size, (Color){90, 225, 248, (unsigned char)(pa * 0.4f)});
        }
    }
}
