#include "enemy_draw_death.h"
#include "enemy_model_loader.h"
#include "enemy_model_bones.h"
#include "enemy_bodydef.h"
#include "world.h"
#include "rlgl.h"
#include "raymath.h"
#include <math.h>
#include <stddef.h>

typedef struct {
    Color suitBase, suitDark, visorColor, helmetColor;
    Color bootColor, gloveColor, backpackColor;
} FactionColors;

static FactionColors GetFactionColors(EnemyType type) {
    FactionColors c;
    if (type == ENEMY_SOVIET) {
        c.suitBase     = (Color){205, 0, 0, 255};
        c.suitDark     = (Color){160, 0, 0, 255};
        c.visorColor   = (Color){255, 200, 50, 230};
        c.helmetColor  = (Color){220, 185, 40, 255};
        c.bootColor    = (Color){25, 25, 22, 255};
        c.gloveColor   = (Color){30, 30, 28, 255};
        c.backpackColor= (Color){140, 10, 10, 255};
    } else {
        c.suitBase     = (Color){25, 40, 90, 255};
        c.suitDark     = (Color){18, 28, 65, 255};
        c.visorColor   = (Color){100, 170, 255, 230};
        c.helmetColor  = (Color){195, 200, 210, 255};
        c.bootColor    = (Color){140, 115, 75, 255};
        c.gloveColor   = (Color){160, 155, 140, 255};
        c.backpackColor= (Color){30, 45, 80, 255};
    }
    return c;
}

// === EVISCERATE — limbs fly apart, blood spurts, weapon dropped ===
void DrawAstronautEviscerate(Enemy *e, EnemyManager *em) {
    FactionColors fc = GetFactionColors(e->type);
    Vector3 pos = e->position;
    float t = e->evisTimer;
    float fade = (t > 7.0f) ? 1.0f - (t - 7.0f) / 3.0f : 1.0f;
    if (fade < 0) fade = 0;
    unsigned char alpha = (unsigned char)(255 * fade);

    Color limbCol = {fc.suitBase.r, fc.suitBase.g, fc.suitBase.b, alpha};
    Color boneCol = {200, 190, 175, alpha};
    Color bloodCol = {180, 10, 5, alpha};
    Color darkBlood = {100, 5, 2, alpha};
    Color brightBlood = {220, 20, 8, alpha};

    // 0=head
    {
        Vector3 hp = Vector3Add(pos, e->evisLimbPos[0]);
        float spin = t * 400.0f;
        rlPushMatrix();
        rlTranslatef(hp.x, hp.y, hp.z);
        rlRotatef(spin, 1, 0.3f, 0);
        DrawSphere((Vector3){0, 0, 0}, 0.45f * fade, fc.helmetColor);
        DrawModel(em->mdlVisor, (Vector3){0, 0.02f, 0.3f}, fade, fc.visorColor);
        DrawCube((Vector3){0, -0.35f, 0}, 0.25f, 0.18f, 0.25f, bloodCol);
        DrawCube((Vector3){0.05f, -0.38f, 0.05f}, 0.1f, 0.1f, 0.1f, darkBlood);
        DrawCube((Vector3){-0.06f, -0.36f, -0.04f}, 0.08f, 0.08f, 0.08f, brightBlood);
        rlPopMatrix();
        if (t < 6.0f) {
            float spurtPhase = sinf(t * 12.0f);
            for (int b = 0; b < 8; b++) {
                float bt = e->evisBloodTimer[0] * 2.5f + b * 0.2f;
                float spurt = (1.0f + spurtPhase * 0.5f) * 0.4f;
                Vector3 bp = {hp.x + sinf(bt*5)*spurt, hp.y - 0.4f - bt*0.4f, hp.z + cosf(bt*3)*spurt};
                float sz = 0.08f * (1.0f - bt/4.0f);
                if (sz > 0) DrawSphereEx(bp, sz, 3, 3, (b%2==0) ? bloodCol : brightBlood);
            }
            for (int d = 0; d < 5; d++) {
                float dt2 = t * 1.5f + d * 0.4f;
                Vector3 dp = {hp.x + sinf(dt2*3)*0.15f, hp.y - 0.5f - d*0.2f, hp.z + cosf(dt2*2)*0.15f};
                DrawSphereEx(dp, 0.04f, 3, 3, darkBlood);
            }
        }
    }
    // 1=torso
    {
        Vector3 tp = Vector3Add(pos, e->evisLimbPos[1]);
        float spin = t * 120.0f;
        rlPushMatrix();
        rlTranslatef(tp.x, tp.y, tp.z);
        rlRotatef(spin, 0.5f, 1, 0.2f);
        DrawCube((Vector3){0, 0, 0}, 0.85f * fade, 1.2f * fade, 0.5f * fade, limbCol);
        DrawCube((Vector3){0, 0.55f, 0}, 0.5f, 0.15f, 0.4f, bloodCol);
        DrawCube((Vector3){0.1f, 0.58f, 0.08f}, 0.15f, 0.08f, 0.12f, brightBlood);
        DrawCube((Vector3){0, -0.55f, 0}, 0.55f, 0.15f, 0.45f, bloodCol);
        DrawCube((Vector3){-0.1f, -0.58f, -0.05f}, 0.2f, 0.08f, 0.15f, darkBlood);
        DrawCube((Vector3){0, 0.6f, -0.05f}, 0.06f, 0.1f, 0.06f, boneCol);
        DrawCube((Vector3){0, -0.6f, -0.05f}, 0.06f, 0.1f, 0.06f, boneCol);
        DrawCube((Vector3){0.48f, 0.3f, 0}, 0.15f, 0.12f, 0.15f, bloodCol);
        DrawCube((Vector3){-0.48f, 0.3f, 0}, 0.15f, 0.12f, 0.15f, bloodCol);
        DrawCube((Vector3){0, 0.1f, -0.38f}, 0.55f * fade, 0.8f * fade, 0.25f * fade, fc.backpackColor);
        rlPopMatrix();
        if (t < 6.0f) {
            for (int s = 0; s < 10; s++) {
                float sa = (float)s * 0.63f + t * 6.0f;
                float sr = 0.15f + sinf(t * 8.0f + s) * 0.1f;
                Vector3 sp = {tp.x + cosf(sa)*sr, tp.y + 0.6f + sinf(sa*2)*0.3f, tp.z + sinf(sa)*sr};
                DrawSphereEx(sp, 0.06f, 3, 3, brightBlood);
            }
            for (int s = 0; s < 8; s++) {
                float sa = (float)s * 0.78f + t * 5.0f;
                float sr = 0.2f + sinf(t * 7.0f + s) * 0.15f;
                Vector3 sp = {tp.x + cosf(sa)*sr, tp.y - 0.6f - sinf(sa)*0.2f, tp.z + sinf(sa)*sr};
                DrawSphereEx(sp, 0.05f, 3, 3, bloodCol);
            }
        }
    }
    // 2=right arm, 3=left arm
    for (int ai = 2; ai <= 3; ai++) {
        Vector3 ap = Vector3Add(pos, e->evisLimbPos[ai]);
        float spin = t * (250.0f + ai * 80.0f);
        rlPushMatrix();
        rlTranslatef(ap.x, ap.y, ap.z);
        rlRotatef(spin, ai == 2 ? 1 : -1, 0.5f, 0.3f);
        DrawCube((Vector3){0, -0.15f, 0}, 0.2f * fade, 0.75f * fade, 0.2f * fade, limbCol);
        DrawCube((Vector3){0, -0.55f, 0}, 0.22f * fade, 0.14f * fade, 0.22f * fade, fc.gloveColor);
        DrawCube((Vector3){0, 0.25f, 0}, 0.18f, 0.12f, 0.18f, bloodCol);
        DrawCube((Vector3){0.03f, 0.28f, 0.03f}, 0.08f, 0.06f, 0.06f, brightBlood);
        DrawCube((Vector3){0, 0.3f, 0}, 0.04f, 0.06f, 0.04f, boneCol);
        rlPopMatrix();
        if (t < 5.5f) {
            float pulse = sinf(t * 10.0f + ai * 2.0f);
            for (int b = 0; b < 6; b++) {
                float bt = e->evisBloodTimer[ai] * 2.0f + b * 0.18f;
                float spurt = 0.3f + pulse * 0.15f;
                Vector3 bp = {ap.x + sinf(bt*8)*spurt, ap.y + 0.3f + cosf(bt*4)*0.2f, ap.z + cosf(bt*6)*spurt};
                DrawSphereEx(bp, 0.06f, 3, 3, (b%3==0) ? brightBlood : bloodCol);
            }
            for (int d = 0; d < 4; d++) {
                float dt2 = t * 2.0f + d * 0.5f + ai;
                Vector3 dp = {ap.x + sinf(dt2)*0.1f, ap.y - 0.3f - d*0.15f, ap.z + cosf(dt2)*0.1f};
                DrawSphereEx(dp, 0.03f, 3, 3, darkBlood);
            }
        }
    }
    // 4=right leg, 5=left leg
    for (int li = 4; li <= 5; li++) {
        Vector3 lp = Vector3Add(pos, e->evisLimbPos[li]);
        float spin = t * (180.0f + li * 60.0f);
        rlPushMatrix();
        rlTranslatef(lp.x, lp.y, lp.z);
        rlRotatef(spin, 0.3f, li == 4 ? 1 : -1, 0.5f);
        DrawCube((Vector3){0, -0.05f, 0}, 0.28f * fade, 0.4f * fade, 0.28f * fade, limbCol);
        DrawCube((Vector3){0, -0.45f, 0}, 0.26f * fade, 0.4f * fade, 0.26f * fade, limbCol);
        DrawCube((Vector3){0, -0.7f, 0.04f}, 0.26f * fade, 0.2f * fade, 0.32f * fade, fc.bootColor);
        DrawCube((Vector3){0, 0.2f, 0}, 0.22f, 0.12f, 0.22f, bloodCol);
        DrawCube((Vector3){0.04f, 0.23f, 0.04f}, 0.1f, 0.06f, 0.08f, brightBlood);
        DrawCube((Vector3){0, 0.26f, 0}, 0.05f, 0.08f, 0.05f, boneCol);
        rlPopMatrix();
        if (t < 5.0f) {
            float pulse = sinf(t * 9.0f + li * 1.5f);
            for (int b = 0; b < 7; b++) {
                float bt = e->evisBloodTimer[li] * 2.2f + b * 0.15f;
                float spurt = 0.35f + pulse * 0.2f;
                Vector3 bp = {lp.x + sinf(bt*7)*spurt, lp.y + 0.2f + bt*0.5f, lp.z + cosf(bt*5)*spurt};
                float sz = 0.07f * (1.0f - bt/3.0f);
                if (sz > 0) DrawSphereEx(bp, sz, 3, 3, (b%2==0) ? bloodCol : brightBlood);
            }
            for (int d = 0; d < 5; d++) {
                float dt2 = t * 1.8f + d * 0.35f + li;
                Vector3 dp = {lp.x + sinf(dt2*2)*0.2f, lp.y - d*0.2f, lp.z + cosf(dt2*3)*0.2f};
                DrawSphereEx(dp, 0.04f, 3, 3, darkBlood);
            }
        }
    }

    // === MASSIVE INITIAL GORE EXPLOSION ===
    if (t < 0.5f) {
        float burst = t / 0.5f;
        for (int p = 0; p < 24; p++) {
            float pa = (float)p * 0.262f;
            float pr = burst * 3.5f;
            float py = sinf(pa * 1.5f + t * 8.0f) * pr * 0.4f;
            Vector3 pp = {pos.x + cosf(pa)*pr, pos.y + py, pos.z + sinf(pa)*pr};
            float sz = 0.15f * (1.0f - burst);
            Color bc = (p % 3 == 0) ? brightBlood : (p % 3 == 1) ? bloodCol : darkBlood;
            DrawSphereEx(pp, sz, 3, 3, bc);
        }
        DrawSphereEx(pos, 2.5f * (1.0f - burst), 5, 5,
            (Color){255, 20, 5, (unsigned char)((1.0f - burst) * 200)});
        if (t < 0.1f) {
            DrawSphereEx(pos, 1.0f * (1.0f - t/0.1f), 4, 4,
                (Color){255, 200, 180, (unsigned char)((1.0f - t/0.1f) * 250)});
        }
        for (int m = 0; m < 16; m++) {
            float ma = (float)m * 0.393f + t * 5.0f;
            float mr = burst * 4.0f + (float)m * 0.1f;
            float my = burst * 2.0f - burst * burst * 1.5f;
            Vector3 mp = {pos.x + cosf(ma)*mr, pos.y + my + sinf(ma*2)*0.5f, pos.z + sinf(ma)*mr};
            DrawSphereEx(mp, 0.06f + (1.0f - burst) * 0.06f, 3, 3,
                (Color){160, 15, 8, (unsigned char)((1.0f - burst) * 230)});
        }
    }

    // === PERSISTENT BLOOD MIST CLOUD ===
    if (t < 5.0f) {
        float mistFade = (t < 1.0f) ? t : (t < 3.5f) ? 1.0f : 1.0f - (t - 3.5f) / 1.5f;
        for (int m = 0; m < 14; m++) {
            float ma = (float)m * 0.449f + t * 1.5f;
            float mr = 0.5f + t * 0.8f + sinf(ma * 3.0f) * 0.3f;
            float my = sinf(t * 2.0f + ma) * 0.5f + 0.3f;
            Vector3 mp = {pos.x + cosf(ma)*mr, pos.y + my, pos.z + sinf(ma)*mr};
            DrawSphereEx(mp, 0.12f + sinf(t*3+m)*0.04f, 3, 3,
                (Color){150, 10, 5, (unsigned char)(mistFade * 80)});
        }
    }

    // Bone + meat fragments
    if (t < 4.0f) {
        for (int b = 0; b < 12; b++) {
            float ba = (float)b * 0.55f + t * 4.0f;
            float br = t * 2.5f + (float)b * 0.25f;
            float by = t * 2.0f - t * t * 0.4f;
            Vector3 bp2 = {pos.x + cosf(ba)*br, pos.y + by, pos.z + sinf(ba)*br};
            if (b % 2 == 0) {
                DrawSphereEx(bp2, 0.035f, 3, 3, boneCol);
            } else {
                DrawSphereEx(bp2, 0.05f, 3, 3, (Color){160, 12, 6, alpha});
            }
        }
    }
}

// === VAPORIZE — 3 phases: jerk, freeze, disintegrate ===
void DrawAstronautVaporize(Enemy *e, EnemyManager *em) {
    (void)em; // visor model not used in vaporize path
    Vector3 pos = e->position;
    float t = e->vaporizeTimer;
    float sc = e->vaporizeScale;

    rlPushMatrix();
    rlTranslatef(pos.x, pos.y, pos.z);
    rlRotatef(e->facingAngle * RAD2DEG, 0, 1, 0);

    if (t < 0.8f) {
        float intensity = 1.0f - t / 0.8f;
        float jerk = sinf(t * 40.0f) * intensity;
        rlRotatef(jerk * 20.0f, 1, 0, 0);
        rlRotatef(sinf(t * 30.0f) * 12.0f * intensity, 0, 0, 1);
        unsigned char flash = (unsigned char)(200 + sinf(t * 50.0f) * 55 * intensity);
        Color jCol = {flash, flash, flash, 255};
        DrawCube((Vector3){0, 0, 0}, 0.9f, 1.5f, 0.55f, jCol);
        DrawSphere((Vector3){0, 1.1f, 0}, 0.48f, jCol);
        float armJerk = sinf(t * 35.0f) * 70.0f * intensity;
        rlPushMatrix(); rlTranslatef(0.52f, 0.3f, 0); rlRotatef(armJerk, 1, 0, 0);
        DrawCube((Vector3){0, -0.2f, 0}, 0.22f, 0.8f, 0.22f, jCol); rlPopMatrix();
        rlPushMatrix(); rlTranslatef(-0.52f, 0.3f, 0); rlRotatef(-armJerk*0.7f, 1, 0, 0);
        DrawCube((Vector3){0, -0.2f, 0}, 0.22f, 0.8f, 0.22f, jCol); rlPopMatrix();
        float legJerk = sinf(t * 45.0f) * 55.0f * intensity;
        rlPushMatrix(); rlTranslatef(0.22f, -0.85f, 0); rlRotatef(legJerk, 1, 0, 0);
        DrawCube((Vector3){0, -0.2f, 0}, 0.3f, 0.75f, 0.3f, jCol); rlPopMatrix();
        rlPushMatrix(); rlTranslatef(-0.22f, -0.85f, 0); rlRotatef(-legJerk*0.6f, 1, 0, 0);
        DrawCube((Vector3){0, -0.2f, 0}, 0.3f, 0.75f, 0.3f, jCol); rlPopMatrix();
    } else if ((e->deathStyle == 0 && t < 2.0f) || (e->deathStyle == 1 && t < 2.0f)) {
        float glow = (t - 0.8f) / 1.2f;
        unsigned char gr = 255;
        unsigned char gg = (unsigned char)(240 - glow * 100);
        unsigned char gb = (unsigned char)(180 - glow * 130);
        Color fCol = {gr, gg, gb, 255};
        rlRotatef(4.0f, 1, 0, 0);
        rlRotatef(2.0f, 0, 0, 1);
        DrawCube((Vector3){0, 0, 0}, 0.9f, 1.5f, 0.55f, fCol);
        DrawSphere((Vector3){0, 1.1f, 0}, 0.48f, fCol);
        DrawCube((Vector3){0.52f, 0.1f, 0}, 0.22f, 0.8f, 0.22f, fCol);
        DrawCube((Vector3){-0.52f, 0.12f, 0.08f}, 0.22f, 0.8f, 0.22f, fCol);
        DrawCube((Vector3){0.22f, -0.9f, 0.08f}, 0.3f, 0.75f, 0.3f, fCol);
        DrawCube((Vector3){-0.22f, -0.88f, -0.04f}, 0.3f, 0.75f, 0.3f, fCol);
        for (int c = 0; c < 6; c++) {
            float ca = sinf(GetTime() * 8.0f + c * 2.5f);
            Vector3 c1 = {ca * 0.5f, -0.6f + c * 0.35f, ca * 0.25f};
            Vector3 c2 = {-ca * 0.35f, -0.4f + c * 0.35f, -ca * 0.3f};
            unsigned char la = (unsigned char)(180 - glow * 60);
            DrawLine3D(Vector3Add(pos, c1), Vector3Add(pos, c2), (Color){255, 255, 200, la});
        }
        DrawSphereWires(pos, 1.2f + sinf(GetTime() * 3.0f) * 0.1f, 6, 6,
            (Color){255, gg, gb, (unsigned char)(40 + glow * 30)});
    } else if (e->deathStyle == 1 && t < 2.5f) {
        float swellT = (t - 2.0f) / 0.5f;
        float swSc = 1.0f + swellT * 1.0f;
        rlScalef(swSc, swSc * 0.85f, swSc);
        rlRotatef(4.0f, 1, 0, 0);
        Color swCol = {255, (unsigned char)(255 - swellT * 60), (unsigned char)(200 - swellT * 120), 255};
        DrawCube((Vector3){0, 0, 0}, 0.9f, 1.5f, 0.55f, swCol);
        DrawSphere((Vector3){0, 1.1f, 0}, 0.48f, swCol);
        DrawCube((Vector3){0.52f, 0.1f, 0}, 0.22f, 0.8f, 0.22f, swCol);
        DrawCube((Vector3){-0.52f, 0.1f, 0}, 0.22f, 0.8f, 0.22f, swCol);
        DrawCube((Vector3){0.22f, -0.9f, 0}, 0.3f, 0.75f, 0.3f, swCol);
        DrawCube((Vector3){-0.22f, -0.9f, 0}, 0.3f, 0.75f, 0.3f, swCol);
    } else if (e->deathStyle == 1 && t < 2.6f) {
        float popT = (t - 2.5f) / 0.1f;
        DrawSphereEx(pos, 2.0f * (1.0f - popT), 5, 5,
            (Color){255, 255, 220, (unsigned char)((1.0f - popT) * 250)});
        rlRotatef(4.0f, 1, 0, 0);
        Color popCol = {200, 100, 50, 255};
        DrawCube((Vector3){0, 0, 0}, 0.9f, 1.5f, 0.55f, popCol);
        DrawSphere((Vector3){0, 1.1f, 0}, 0.48f, popCol);
    } else {
        float startDis = (e->deathStyle == 1) ? 2.6f : 2.0f;
        float disDur = (e->deathStyle == 1) ? 3.2f : 2.3f;
        float disT = (t - startDis) / disDur;
        if (disT > 1.0f) disT = 1.0f;
        unsigned char da = (unsigned char)(255 * (1.0f - disT));
        Color dCol = {255, (unsigned char)(160 * (1.0f - disT)),
                     (unsigned char)(60 * (1.0f - disT)), da};
        rlScalef(sc, sc, sc);
        rlRotatef(4.0f, 1, 0, 0);
        if (disT < 0.2f)
            DrawSphere((Vector3){0, 1.1f, 0}, 0.48f * (1.0f - disT/0.2f), dCol);
        if (disT < 0.35f) {
            float as = 1.0f - disT / 0.35f;
            DrawCube((Vector3){0.52f, 0.1f, 0}, 0.22f*as, 0.8f*as, 0.22f*as, dCol);
            DrawCube((Vector3){-0.52f, 0.1f, 0}, 0.22f*as, 0.8f*as, 0.22f*as, dCol);
        }
        if (disT < 0.55f) {
            float ls = 1.0f - disT / 0.55f;
            DrawCube((Vector3){0.22f, -0.9f, 0}, 0.3f*ls, 0.75f*ls, 0.3f*ls, dCol);
            DrawCube((Vector3){-0.22f, -0.9f, 0}, 0.3f*ls, 0.75f*ls, 0.3f*ls, dCol);
        }
        float ts = 1.0f - disT;
        if (ts > 0) DrawCube((Vector3){0, 0, 0}, 0.9f*ts, 1.5f*ts, 0.55f*ts, dCol);
    }
    rlPopMatrix();

    float particleT = t / 2.4f;
    for (int p = 0; p < 10; p++) {
        float pa = (float)p * 0.628f + t * 3.0f;
        float pr = particleT * 3.5f + (float)p * 0.2f;
        float py = particleT * 1.5f + sinf(pa * 2.0f) * 0.5f;
        Vector3 pp = {pos.x + cosf(pa) * pr, pos.y + py, pos.z + sinf(pa) * pr};
        float ps = 0.06f + (1.0f - particleT) * 0.08f;
        DrawSphereEx(pp, ps, 3, 3, (Color){255, 200, 120, (unsigned char)((1.0f - particleT) * 160)});
    }
    for (int b = 0; b < 6; b++) {
        float ba = (float)b * 1.05f + t * 2.5f;
        float br = particleT * 2.0f + (float)b * 0.15f;
        float by = particleT * 0.8f + sinf(ba) * 0.3f - 0.3f;
        DrawSphereEx((Vector3){pos.x + cosf(ba)*br, pos.y + by, pos.z + sinf(ba)*br},
            0.05f, 3, 3, (Color){150, 12, 8, (unsigned char)((1.0f - particleT) * 140)});
    }
    if (t < 0.15f) {
        DrawSphereEx(pos, 2.0f * (1.0f - t / 0.15f), 4, 4,
            (Color){255, 255, 240, (unsigned char)((1.0f - t / 0.15f) * 220)});
    }
}

// === RAGDOLL/CRUMPLE death particles ===
void DrawAstronautRagdoll(Enemy *e) {
    Vector3 pos = e->position;

    if (e->deathStyle == 0) {
        float elapsed = DEATH_BODY_PERSIST_TIME - e->deathTimer;
        // Suit breach point — deterministic per enemy
        int limbSeed = (int)((size_t)e % 4);
        Vector3 leakOff = {0, 0, 0};
        Vector3 jetDir = {0, 1, 0};
        switch (limbSeed) {
            case 0: leakOff = (Vector3){0, 1.1f, 0}; jetDir = (Vector3){0.2f, 1, 0.1f}; break;
            case 1: leakOff = (Vector3){0.5f, 0.3f, 0.1f}; jetDir = (Vector3){1, 0.3f, 0.2f}; break;
            case 2: leakOff = (Vector3){-0.2f, -0.5f, 0}; jetDir = (Vector3){-0.5f, -0.3f, 0.8f}; break;
            case 3: leakOff = (Vector3){0, 0.1f, -0.4f}; jetDir = (Vector3){0.1f, 0.5f, -1}; break;
        }
        // Pressurized air jet
        float pressure = (elapsed < 6.0f) ? 1.0f - elapsed / 6.0f : 0.0f;
        if (pressure > 0) {
            for (int p = 0; p < 14; p++) {
                float pt = (float)p * 0.25f + elapsed * 12.0f;
                float life = fmodf(pt, 3.5f) / 3.5f;
                float speed = 1.5f + life * 3.0f;
                float jitter = sinf(pt * 7.3f) * 0.15f * pressure;
                float px = pos.x + leakOff.x + jetDir.x * speed * life + jitter;
                float py = pos.y + leakOff.y + jetDir.y * speed * life + cosf(pt * 5.1f) * 0.1f;
                float pz = pos.z + leakOff.z + jetDir.z * speed * life + sinf(pt * 6.7f) * 0.1f;
                float sz = (0.04f + life * 0.12f) * pressure;
                unsigned char alpha = (unsigned char)((1.0f - life) * 200 * pressure);
                DrawSphereEx((Vector3){px, py, pz}, sz, 3, 3,
                    (Color){230, 235, 240, alpha});
            }
            DrawSphereEx((Vector3){pos.x + leakOff.x, pos.y + leakOff.y, pos.z + leakOff.z},
                0.12f * pressure, 4, 4, (Color){255, 255, 255, (unsigned char)(180 * pressure)});
        }
        // Blood spurting from breach
        float bloodFade = (elapsed < 8.0f) ? 1.0f : 1.0f - (elapsed - 8.0f) / 2.0f;
        if (bloodFade < 0) bloodFade = 0;
        if (bloodFade > 0) {
            float pulse = sinf(elapsed * 10.0f) * 0.5f + 0.5f;
            for (int b = 0; b < 10; b++) {
                float bt = (float)b * 0.35f + elapsed * 5.0f;
                float life = fmodf(bt, 2.5f) / 2.5f;
                float spurt = (0.2f + pulse * 0.3f) * bloodFade;
                float grav = life * life * 1.2f;
                Vector3 bp = {
                    pos.x + leakOff.x + sinf(bt * 3.0f) * spurt + jetDir.x * life * 0.5f,
                    pos.y + leakOff.y + life * 0.4f * spurt - grav,
                    pos.z + leakOff.z + cosf(bt * 2.5f) * spurt + jetDir.z * life * 0.5f
                };
                float bGH = WorldGetHeight(bp.x, bp.z) + 0.08f;
                if (bp.y < bGH) bp.y = bGH;
                float sz = 0.04f + (1.0f - life) * 0.04f;
                DrawSphereEx(bp, sz * bloodFade, 3, 3,
                    (b % 3 == 0) ? (Color){220, 20, 8, (unsigned char)(200 * bloodFade)} :
                                   (Color){160, 10, 5, (unsigned char)(180 * bloodFade)});
            }
            for (int d = 0; d < 6; d++) {
                float dt2 = elapsed * 2.0f + (float)d * 1.1f;
                float dlife = fmodf(dt2, 2.0f) / 2.0f;
                float dx = pos.x + leakOff.x + sinf(dt2 * 1.5f) * 0.2f;
                float dz = pos.z + leakOff.z + cosf(dt2 * 1.3f) * 0.2f;
                float dy = pos.y + leakOff.y - dlife * 2.0f;
                float dGH = WorldGetHeight(dx, dz) + 0.06f;
                if (dy < dGH) dy = dGH;
                DrawSphereEx((Vector3){dx, dy, dz}, 0.03f * bloodFade, 3, 3,
                    (Color){140, 8, 4, (unsigned char)(160 * bloodFade)});
            }
        }

        // === RAGDOLL BLOOD POOL (after body settles) — same terrain-conforming system ===
        if (elapsed > 3.0f) {
            float poolTime = elapsed - 3.0f;
            float poolR = 0.15f + poolTime * 0.06f;   // slow spread
            if (poolR > 2.5f) poolR = 2.5f;
            int segs = 12;
            int rings = 4;
            float pcx = pos.x, pcz = pos.z;
            for (int ring = rings - 1; ring >= 0; ring--) {
                float outerFrac = (float)(ring + 1) / (float)rings;
                float innerFrac = (float)ring / (float)rings;
                float outerR = poolR * outerFrac;
                float innerR = poolR * innerFrac;
                float colorT = (float)ring / (float)(rings - 1);
                unsigned char cr = (unsigned char)(25 + colorT * 45);   // very dark
                unsigned char cg = (unsigned char)(1 + colorT * 4);
                unsigned char cb = (unsigned char)(1 + colorT * 3);
                unsigned char ca = (unsigned char)(230 - colorT * 90);  // very opaque
                for (int s = 0; s < segs; s++) {
                    float a0 = (float)s / segs * 2.0f * PI;
                    float a1 = (float)(s + 1) / segs * 2.0f * PI;
                    float j0 = 1.0f + sinf(a0 * 3.0f + e->facingAngle * 7.13f) * 0.15f * outerFrac;
                    float j1 = 1.0f + sinf(a1 * 3.0f + e->facingAngle * 7.13f) * 0.15f * outerFrac;
                    float ox0 = pcx + cosf(a0)*outerR*j0, oz0 = pcz + sinf(a0)*outerR*j0;
                    float ox1 = pcx + cosf(a1)*outerR*j1, oz1 = pcz + sinf(a1)*outerR*j1;
                    float ix0 = pcx + cosf(a0)*innerR*j0, iz0 = pcz + sinf(a0)*innerR*j0;
                    float ix1 = pcx + cosf(a1)*innerR*j1, iz1 = pcz + sinf(a1)*innerR*j1;
                    float oh0=WorldGetHeight(ox0,oz0)+0.04f, oh1=WorldGetHeight(ox1,oz1)+0.04f;
                    float ih0=WorldGetHeight(ix0,iz0)+0.04f+(float)ring*0.005f;
                    float ih1=WorldGetHeight(ix1,iz1)+0.04f+(float)ring*0.005f;
                    Color rc = {cr, cg, cb, ca};
                    DrawTriangle3D((Vector3){ix0,ih0,iz0},(Vector3){ox1,oh1,oz1},(Vector3){ox0,oh0,oz0},rc);
                    DrawTriangle3D((Vector3){ix0,ih0,iz0},(Vector3){ix1,ih1,iz1},(Vector3){ox1,oh1,oz1},rc);
                }
            }
            // Occasional blood spurts from the body
            float t_ = (float)GetTime();
            float spurtSeed = e->facingAngle * 11.37f;
            float spurtPhase = sinf(t_ * 0.8f + spurtSeed);
            if (spurtPhase > 0.7f) {
                float spurtStr = (spurtPhase - 0.7f) / 0.3f;
                for (int sp = 0; sp < 6; sp++) {
                    float sa = (float)sp / 6.0f * PI * 2.0f + t_ * 2.0f;
                    float sr = 0.1f + spurtStr * 0.3f;
                    float sy = 0.1f + spurtStr * 0.5f - (float)sp * 0.05f;
                    float sx = pcx + cosf(sa) * sr;
                    float sz = pcz + sinf(sa) * sr;
                    float sGH = WorldGetHeight(sx, sz) + 0.08f;
                    if (pos.y + sy < sGH) sy = sGH - pos.y;
                    DrawSphereEx((Vector3){sx, pos.y + sy, sz}, 0.04f * spurtStr, 3, 3,
                        (Color){200, 15, 5, (unsigned char)(200 * spurtStr)});
                }
            }
        }
    } else {
        float elapsed = DEATH_BODY_PERSIST_TIME - e->deathTimer;
        float poolTime = elapsed - 0.5f;
        if (poolTime < 0) poolTime = 0;
        float poolR = 0.2f + poolTime * 0.08f;   // slow spread
        if (poolR > 3.0f) poolR = 3.0f;

        int segs = 14;
        int rings = 5;  // concentric rings for gradient
        float cx = pos.x, cz = pos.z;

        // Draw concentric rings from outside in, each sampling terrain height
        // Outer: thin, transparent. Inner: thick, dark, opaque.
        for (int ring = rings - 1; ring >= 0; ring--) {
            float outerFrac = (float)(ring + 1) / (float)rings;
            float innerFrac = (float)ring / (float)rings;
            float outerR = poolR * outerFrac;
            float innerR = poolR * innerFrac;

            // Gradient: center is darkest/most opaque, edges are lighter/transparent
            // Also add slight per-ring irregularity from facing angle seed
            float colorT = (float)ring / (float)(rings - 1);  // 0=center, 1=edge
            unsigned char cr = (unsigned char)(25 + colorT * 45);    // 25→70 (very dark)
            unsigned char cg = (unsigned char)(1 + colorT * 4);      // 1→5
            unsigned char cb = (unsigned char)(1 + colorT * 3);      // 1→4
            unsigned char ca = (unsigned char)(240 - colorT * 90);   // 240→150 (very opaque)

            for (int s = 0; s < segs; s++) {
                float a0 = (float)s / segs * 2.0f * PI;
                float a1 = (float)(s + 1) / segs * 2.0f * PI;

                // Per-segment radius jitter for organic edge (stronger at outer rings)
                float jitter0 = 1.0f + sinf(a0 * 3.0f + e->facingAngle * 7.13f) * 0.15f * outerFrac;
                float jitter1 = 1.0f + sinf(a1 * 3.0f + e->facingAngle * 7.13f) * 0.15f * outerFrac;

                // Outer edge vertices (sample terrain height at each point)
                float ox0 = cx + cosf(a0) * outerR * jitter0;
                float oz0 = cz + sinf(a0) * outerR * jitter0;
                float oh0 = WorldGetHeight(ox0, oz0) + 0.04f;

                float ox1 = cx + cosf(a1) * outerR * jitter1;
                float oz1 = cz + sinf(a1) * outerR * jitter1;
                float oh1 = WorldGetHeight(ox1, oz1) + 0.04f;

                // Inner edge vertices
                float ix0 = cx + cosf(a0) * innerR * jitter0;
                float iz0 = cz + sinf(a0) * innerR * jitter0;
                float ih0 = WorldGetHeight(ix0, iz0) + 0.04f + (float)ring * 0.005f;

                float ix1 = cx + cosf(a1) * innerR * jitter1;
                float iz1 = cz + sinf(a1) * innerR * jitter1;
                float ih1 = WorldGetHeight(ix1, iz1) + 0.04f + (float)ring * 0.005f;

                Color ringCol = {cr, cg, cb, ca};

                // Two triangles per quad (outer-inner ring strip)
                DrawTriangle3D(
                    (Vector3){ix0, ih0, iz0},
                    (Vector3){ox1, oh1, oz1},
                    (Vector3){ox0, oh0, oz0},
                    ringCol);
                DrawTriangle3D(
                    (Vector3){ix0, ih0, iz0},
                    (Vector3){ix1, ih1, iz1},
                    (Vector3){ox1, oh1, oz1},
                    ringCol);
            }
        }

        if (elapsed < 8.0f) {
            float dripFade = (elapsed < 6.0f) ? 1.0f : 1.0f - (elapsed - 6.0f) / 2.0f;
            for (int d = 0; d < 5; d++) {
                float dt2 = elapsed * 3.0f + (float)d * 1.3f;
                float dlife = fmodf(dt2, 1.5f) / 1.5f;
                float dx = cx + sinf(dt2 * 0.7f) * 0.3f;
                float dz = cz + cosf(dt2 * 0.9f) * 0.3f;
                float groundH = WorldGetHeight(cx, cz) + 0.06f;
                float dy = pos.y + 0.3f - dlife * (pos.y - groundH + 0.5f);
                float dGH = WorldGetHeight(dx, dz) + 0.08f;
                if (dy < dGH) dy = dGH;
                DrawSphereEx((Vector3){dx, dy, dz}, 0.03f * dripFade, 3, 3,
                    (Color){180, 12, 6, (unsigned char)(200 * dripFade)});
            }
        }

        // Bubbles in pool
        for (int b = 0; b < 4; b++) {
            float bt = GetTime() * 3.0f + (float)b * 1.6f;
            float bx = cx + cosf(bt) * poolR * 0.35f;
            float bz = cz + sinf(bt) * poolR * 0.35f;
            float bH = WorldGetHeight(bx, bz) + 0.12f;
            float bubble = sinf(bt * 2.0f) * 0.03f;
            DrawSphereEx((Vector3){bx, bH + bubble, bz},
                0.035f, 3, 3, (Color){40, 5, 3, 160});
        }

        // Occasional blood spurts from crumpled body
        {
            float t_ = (float)GetTime();
            float spurtSeed = e->facingAngle * 9.73f;
            float spurtPhase = sinf(t_ * 0.6f + spurtSeed);
            if (spurtPhase > 0.75f && elapsed < 30.0f) {
                float spurtStr = (spurtPhase - 0.75f) / 0.25f;
                for (int sp = 0; sp < 5; sp++) {
                    float sa = (float)sp / 5.0f * PI * 2.0f + t_ * 1.5f;
                    float sr = 0.08f + spurtStr * 0.2f;
                    float sy = 0.15f + spurtStr * 0.4f - (float)sp * 0.04f;
                    float sx = cx + cosf(sa) * sr;
                    float sz = cz + sinf(sa) * sr;
                    float sGH = WorldGetHeight(sx, sz) + 0.08f;
                    float drawY = pos.y + sy;
                    if (drawY < sGH) drawY = sGH;
                    DrawSphereEx((Vector3){sx, drawY, sz}, 0.035f * spurtStr, 3, 3,
                        (Color){50, 5, 2, (unsigned char)(220 * spurtStr)});
                }
            }
        }

        if (elapsed < 5.0f) {
            float gasFade = 1.0f - elapsed / 5.0f;
            for (int g = 0; g < 6; g++) {
                float gt = elapsed * 6.0f + (float)g * 1.05f;
                float glife = fmodf(gt, 2.0f) / 2.0f;
                float gx = cx + sinf(gt * 2.0f) * 0.2f;
                float gz = cz + cosf(gt * 1.7f) * 0.2f;
                float gy = pos.y + 0.2f + glife * 1.5f;
                float gsz = (0.05f + glife * 0.15f) * gasFade;
                DrawSphereEx((Vector3){gx, gy, gz}, gsz, 3, 3,
                    (Color){220, 225, 230, (unsigned char)((1.0f - glife) * 120 * gasFade)});
            }
        }
    }
}

// === DECAPITATE — chunk blown off head, blood pouring, ragdoll blowout ===
void DrawAstronautDecapitate(Enemy *e) {
    FactionColors fc = GetFactionColors(e->type);
    Vector3 pos = e->position;
    float t = e->decapTimer;

    // Fade out in last 3 seconds
    float fade = (e->deathTimer < 3.0f) ? e->deathTimer / 3.0f : 1.0f;
    if (fade < 0) fade = 0;
    unsigned char alpha = (unsigned char)(255 * fade);

    // Flat world: basis is identity with Y up
    Vector3 localUp    = {0, 1, 0};
    Vector3 localFwd   = {sinf(e->facingAngle), 0, cosf(e->facingAngle)};
    Vector3 localRight = {cosf(e->facingAngle), 0, -sinf(e->facingAngle)};

    // Per-enemy deterministic chunk direction (seeded from facingAngle)
    float chunkAngle = e->facingAngle * 7.13f;
    float chunkDirR = cosf(chunkAngle);
    float chunkDirF = sinf(chunkAngle);

    // === INITIAL CHUNK BURST (first 3 seconds) — skull/helmet fragments ===
    if (t < 3.0f) {
        float burst = t / 3.0f;
        for (int f = 0; f < 8; f++) {
            float fa = (float)f / 8.0f * PI + 0.3f;
            float fr = t * 4.0f + (float)f * 0.25f;
            float fy = t * 2.5f - t * t * 0.5f + sinf(fa) * 0.3f;
            Vector3 fp = Vector3Add(pos,
                Vector3Add(Vector3Scale(localRight, chunkDirR * (0.3f + cosf(fa) * fr * 0.6f)),
                Vector3Add(Vector3Scale(localUp, HEADSHOT_HEAD_CENTER_Y + fy),
                           Vector3Scale(localFwd, chunkDirF * (0.2f + sinf(fa) * fr * 0.4f)))));
            float fsz = 0.09f * (1.0f - burst * 0.5f) * fade;
            unsigned char fa2 = (unsigned char)((1.0f - burst) * 230 * fade);
            if (f < 4) {
                DrawCubeV(fp, (Vector3){fsz, fsz * 0.5f, fsz * 0.7f},
                    (Color){fc.helmetColor.r, fc.helmetColor.g, fc.helmetColor.b, fa2});
            } else {
                Color chunkCol = (f % 2 == 0) ? (Color){200, 185, 170, fa2} : (Color){190, 15, 8, fa2};
                DrawCubeV(fp, (Vector3){fsz * 0.5f, fsz * 0.4f, fsz * 0.5f}, chunkCol);
            }
        }
        // Blood burst cloud from chunk direction
        for (int p = 0; p < 20; p++) {
            float pa = (float)p / 20.0f * PI * 1.5f - 0.3f;
            float pr = burst * 3.0f + (float)p * 0.05f;
            float py = sinf(pa * 1.5f + t * 5.0f) * pr * 0.3f;
            Vector3 pp = Vector3Add(pos,
                Vector3Add(Vector3Scale(localRight, chunkDirR * (0.2f + cosf(pa) * pr * 0.5f)),
                Vector3Add(Vector3Scale(localUp, HEADSHOT_HEAD_CENTER_Y + py),
                           Vector3Scale(localFwd, chunkDirF * (0.1f + sinf(pa) * pr * 0.4f)))));
            float sz = (0.12f - burst * 0.04f) * fade;
            unsigned char ba = (unsigned char)((1.0f - burst * 0.6f) * 200 * fade);
            Color col = (p % 3 == 0) ? (Color){230, 20, 8, ba} :
                        (p % 3 == 1) ? (Color){170, 10, 5, ba} :
                                       (Color){110, 5, 2, ba};
            DrawSphereEx(pp, sz, 3, 3, col);
        }
    }

    // === PRESSURIZED AIR JET from head wound (0-6 seconds) ===
    if (t < 6.0f) {
        float pressure = 1.0f - t / 6.0f;
        for (int g = 0; g < 14; g++) {
            float gt = (float)g / 14.0f;
            float glife = fmodf(t * 3.0f + gt * 1.5f, 1.5f);
            float gspeed = (1.5f + glife * 3.0f) * pressure;
            float gjitter = sinf(gt * 7.3f + t * 15.0f) * 0.15f * pressure;
            Vector3 gp = Vector3Add(pos,
                Vector3Add(Vector3Scale(localRight, chunkDirR * (0.25f + gspeed * 0.3f) + gjitter),
                Vector3Add(Vector3Scale(localUp, HEADSHOT_HEAD_CENTER_Y + glife * 0.5f),
                           Vector3Scale(localFwd, chunkDirF * (0.15f + gspeed * 0.2f) + gjitter * 0.7f))));
            float gsz = (0.04f + glife * 0.12f) * pressure * fade;
            unsigned char ga = (unsigned char)((1.0f - glife / 1.5f) * 180 * pressure * fade);
            DrawSphereEx(gp, gsz, 3, 3, (Color){230, 235, 240, ga});
        }
    }

    // === BLOOD POURING FROM HEAD WOUND ===
    if (t < HEADSHOT_BLOOD_DURATION) {
        float fountainFade = 1.0f - t / HEADSHOT_BLOOD_DURATION;
        float pulse = sinf(t * 12.0f);

        for (int s = 0; s < 16; s++) {
            float st = (float)s / 16.0f;
            float outward = 0.3f + st * 0.4f * (1.0f + pulse * 0.3f);
            float sa = st * 8.0f + t * 10.0f + (float)s;
            Vector3 sp = Vector3Add(pos,
                Vector3Add(Vector3Scale(localUp, HEADSHOT_HEAD_CENTER_Y - 0.1f - st * 0.8f),
                Vector3Add(Vector3Scale(localRight, chunkDirR * outward + sinf(sa) * 0.1f),
                           Vector3Scale(localFwd, chunkDirF * outward * 0.5f + cosf(sa) * 0.1f))));
            float ssz = (0.08f + st * 0.05f) * fountainFade * fade;
            unsigned char sa2 = (unsigned char)(240 * fountainFade * (1.0f - st * 0.3f) * fade);
            Color col = (s % 2 == 0) ? (Color){220, 20, 8, sa2} : (Color){170, 10, 5, sa2};
            DrawSphereEx(sp, ssz, 3, 3, col);
        }

        for (int d = 0; d < 10; d++) {
            float dt2 = fmodf(t * 2.5f + (float)d * 0.4f, 2.0f);
            float dfall = dt2 * dt2 * 1.0f;
            float dout = 0.25f + dt2 * 0.2f;
            float da = (float)d / 10.0f * PI;
            Vector3 dp = Vector3Add(pos,
                Vector3Add(Vector3Scale(localUp, HEADSHOT_HEAD_CENTER_Y - 0.2f - dfall),
                Vector3Add(Vector3Scale(localRight, chunkDirR * dout + cosf(da) * 0.1f),
                           Vector3Scale(localFwd, chunkDirF * dout * 0.5f + sinf(da) * 0.1f))));
            float dsz = 0.05f * fountainFade * fade;
            unsigned char da2 = (unsigned char)(200 * fountainFade * fade);
            DrawSphereEx(dp, dsz, 3, 3, (Color){200, 12, 5, da2});
        }

        for (int r = 0; r < 6; r++) {
            float rt = fmodf(t * 1.2f + (float)r * 0.5f, 2.5f);
            float ry = HEADSHOT_HEAD_CENTER_Y - 0.3f - rt * 0.6f;
            float rx = chunkDirR * (0.2f + sinf((float)r * 2.1f) * 0.15f);
            float rz2 = chunkDirF * 0.1f;
            Vector3 rp = Vector3Add(pos,
                Vector3Add(Vector3Scale(localUp, ry),
                Vector3Add(Vector3Scale(localRight, rx),
                           Vector3Scale(localFwd, rz2))));
            float rsz = 0.04f * fountainFade * fade;
            unsigned char ra = (unsigned char)(160 * fountainFade * fade);
            DrawSphereEx(rp, rsz, 3, 3, (Color){160, 8, 3, ra});
        }
    }

    // === BODY WITH DAMAGED HEAD — ragdoll spin ===
    rlPushMatrix();
    rlTranslatef(pos.x, pos.y, pos.z);
    rlRotatef(e->facingAngle * RAD2DEG, 0, 1, 0);
    rlRotatef(e->deathAngle, 1, 0, 0);
    rlRotatef(e->spinX * 0.02f + e->spinZ * 0.5f, 0, 0, 1);  // per-enemy random topple
    rlRotatef(e->spinZ * 0.5f, 0, 1, 0);

    Color suitCol = {fc.suitBase.r, fc.suitBase.g, fc.suitBase.b, alpha};
    Color darkCol = {fc.suitDark.r, fc.suitDark.g, fc.suitDark.b, alpha};
    Color bloodCol2 = {180, 10, 5, alpha};

    // Damaged helmet — chunk blown off
    float cr = chunkDirR;
    float cf2 = chunkDirF;
    rlPushMatrix();
    rlTranslatef(0, 1.1f, 0);
    DrawSphere((Vector3){-cr * 0.1f, 0, -cf2 * 0.1f}, 0.42f, fc.helmetColor);
    DrawCube((Vector3){cr * 0.15f, 0.1f, cf2 * 0.1f}, 0.08f, 0.12f, 0.06f, fc.helmetColor);
    DrawCube((Vector3){cr * 0.12f, -0.05f, cf2 * -0.08f}, 0.06f, 0.1f, 0.05f, fc.helmetColor);
    DrawCube((Vector3){cr * 0.18f, 0.0f, cf2 * 0.02f}, 0.05f, 0.08f, 0.07f, fc.helmetColor);
    DrawSphere((Vector3){cr * 0.2f, 0.05f, cf2 * 0.05f}, 0.25f, (Color){100, 5, 2, alpha});
    DrawSphere((Vector3){cr * 0.15f, 0, cf2 * 0.08f}, 0.18f, (Color){140, 8, 3, alpha});
    DrawSphere((Vector3){cr * 0.22f, 0.1f, cf2 * 0.08f}, 0.1f, bloodCol2);
    DrawSphere((Vector3){cr * 0.18f, -0.08f, cf2 * 0.03f}, 0.08f, (Color){200, 15, 5, alpha});
    DrawCube((Vector3){-cr * 0.1f, 0.02f, 0.34f}, 0.28f, 0.25f, 0.04f,
        (Color){fc.visorColor.r, fc.visorColor.g, fc.visorColor.b, (unsigned char)(alpha * 0.7f)});
    rlPopMatrix();

    DrawCube((Vector3){0, 0.8f, 0}, 0.65f, 0.08f, 0.55f, darkCol);
    DrawCube((Vector3){cr * 0.2f, 0.7f, cf2 * 0.1f}, 0.2f, 0.3f, 0.15f, bloodCol2);
    DrawCube((Vector3){cr * 0.3f, 0.5f, cf2 * 0.05f}, 0.15f, 0.25f, 0.12f, (Color){160, 8, 3, alpha});

    // Torso
    DrawCube((Vector3){0, 0, 0}, 0.9f, 1.5f, 0.55f, suitCol);
    DrawCubeWires((Vector3){0, 0, 0}, 0.91f, 1.51f, 0.56f, darkCol);
    DrawCube((Vector3){0, 0.2f, 0.02f}, 0.7f, 0.6f, 0.5f, darkCol);
    DrawCube((Vector3){0.5f, 0.6f, 0}, 0.18f, 0.2f, 0.4f, darkCol);
    DrawCube((Vector3){-0.5f, 0.6f, 0}, 0.18f, 0.2f, 0.4f, darkCol);
    Color beltCol2 = (e->type == ENEMY_SOVIET) ? (Color){120, 75, 35, alpha} : (Color){80, 85, 60, alpha};
    DrawCube((Vector3){0, -0.55f, 0}, 0.92f, 0.1f, 0.57f, beltCol2);
    Color bpCol = {fc.backpackColor.r, fc.backpackColor.g, fc.backpackColor.b, alpha};
    DrawCube((Vector3){0, 0.1f, -0.42f}, 0.62f, 0.9f, 0.28f, bpCol);

    // Arms — violent jerking that dies down
    float jerkDamp = (t < 2.0f) ? 1.0f : (t < 5.0f) ? (5.0f - t) / 3.0f : 0.0f;
    for (int side = 0; side < 2; side++) {
        float sx = side == 0 ? 0.52f : -0.52f;
        float armSign = side == 0 ? 1.0f : -1.0f;
        rlPushMatrix();
        rlTranslatef(sx, 0.35f, 0.1f);
        float jerk2 = sinf(t * 25.0f * armSign + e->deathAngle * 0.1f) * 40.0f * jerkDamp;
        float drift = sinf(t * 0.4f * armSign + 1.0f) * 15.0f;
        float armSwing2 = sinf(e->deathAngle * 0.07f * armSign + t * 4.0f * jerkDamp) * 70.0f * jerkDamp
                        + jerk2 + drift * (1.0f - jerkDamp);
        float armFlop = cosf(e->deathAngle * 0.05f + armSign * 1.5f) * 40.0f * jerkDamp
                      + (-30.0f * armSign) * (1.0f - jerkDamp);
        float armTwist = sinf(t * 18.0f * armSign + 2.0f) * 25.0f * jerkDamp;
        rlRotatef(armSwing2, 1, 0, 0);
        rlRotatef(armFlop, 0, 0, 1);
        rlRotatef(armTwist, 0, 1, 0);
        DrawCube((Vector3){0, 0, 0}, 0.22f, 0.8f, 0.22f, suitCol);
        Color gCol = {fc.gloveColor.r, fc.gloveColor.g, fc.gloveColor.b, alpha};
        DrawCube((Vector3){0, -0.42f, 0}, 0.24f, 0.16f, 0.24f, gCol);
        rlPopMatrix();
    }

    // Legs — floppy ragdoll
    for (int side = 0; side < 2; side++) {
        float lx = side == 0 ? 0.22f : -0.22f;
        float legSign = side == 0 ? 1.0f : -1.0f;
        rlPushMatrix();
        rlTranslatef(lx, -0.85f, 0);
        float legDrift = sinf(t * 0.3f * legSign + 0.5f) * 8.0f;
        float legSwing2 = sinf(e->deathAngle * 0.04f * legSign + t * 2.0f) * 25.0f * jerkDamp
                        + legDrift * (1.0f - jerkDamp);
        rlRotatef(legSwing2, 1, 0, 0);
        DrawCube((Vector3){0, -0.05f, 0}, 0.3f, 0.45f, 0.3f, suitCol);
        DrawCube((Vector3){0, -0.3f, 0}, 0.22f, 0.08f, 0.22f, darkCol);
        rlPushMatrix();
        rlTranslatef(0, -0.3f, 0);
        float kneeFlop = sinf(e->deathAngle * 0.06f + legSign * 2.0f) * 20.0f * jerkDamp;
        rlRotatef(kneeFlop, 1, 0, 0);
        DrawCube((Vector3){0, -0.2f, 0}, 0.28f, 0.4f, 0.28f, suitCol);
        Color btCol = {fc.bootColor.r, fc.bootColor.g, fc.bootColor.b, alpha};
        DrawCube((Vector3){0, -0.44f, 0.04f}, 0.28f, 0.3f, 0.35f, btCol);
        rlPopMatrix();
        rlPopMatrix();
    }

    rlPopMatrix(); // root
}

// === SKELETAL HEADSHOT — uses per-rank headshot .glb model with blood spray ===
void DrawAstronautDecapitateSkeletal(Enemy *e, EnemyManager *em) {
    if (!em || !em->astroModels) return;
    AstroModelSet *ms = em->astroModels;
    int fi = (int)e->type;
    int ri = (int)e->rank;

    Vector3 pos = e->position;
    float t = e->decapTimer;
    float fade = (e->deathTimer < 3.0f) ? e->deathTimer / 3.0f : 1.0f;
    if (fade < 0) fade = 0;
    unsigned char alpha = (unsigned char)(255 * fade);

    /* Flat world: up = Y, forward from facingAngle */
    Vector3 localUp = {0, 1, 0};
    float cosF = cosf(e->facingAngle);
    float sinF = sinf(e->facingAngle);
    Vector3 localFwd = {sinF, 0, cosF};
    Vector3 localRight = {cosF, 0, -sinF};

    #define HEAD_HEIGHT 1.1f
    Vector3 headWorldPos = Vector3Add(pos, Vector3Scale(localUp, HEAD_HEIGHT));

    /* Hole direction for blood — right side of helmet */
    Vector3 holeDir = Vector3Add(
        Vector3Add(Vector3Scale(localRight, 0.6f),
                   Vector3Scale(localFwd, 0.2f)),
        Vector3Scale(localUp, 0.3f));
    Vector3 holePos = Vector3Add(headWorldPos, Vector3Scale(holeDir, 0.3f));

    /* === BODY: character model with damaged helmet === */
    if (fi >= 0 && fi < 2 && ri >= 0 && ri < 3) {
        AstroModel *am = &ms->characters[fi][ri];
        if (am->loaded && e->hasLimbState) {
            const AnimProfile *ap = AnimProfileGet(e->type);
            AstroModelApplySpringState(am, &e->limbState, ap, 0, 0, 0,
                                       false, true, t);

            /* Zero original head — replaced by damaged helmet model */
            if (am->bones[BONE_HEAD] >= 0 && am->model.meshes[0].boneMatrices)
                am->model.meshes[0].boneMatrices[am->bones[BONE_HEAD]] = MatrixScale(0, 0, 0);
            if (am->bones[BONE_NECK] >= 0 && am->model.meshes[0].boneMatrices)
                am->model.meshes[0].boneMatrices[am->bones[BONE_NECK]] = MatrixScale(0, 0, 0);

            rlPushMatrix();
            rlTranslatef(pos.x, pos.y, pos.z);
            rlRotatef(e->facingAngle * RAD2DEG, 0, 1, 0);
            /* Per-enemy random topple direction using spinX/spinZ */
            rlRotatef(e->deathAngle * 0.3f + e->spinX * 0.01f, 1, 0, 0);
            rlRotatef(e->spinZ * 0.4f, 0, 0, 1);
            rlRotatef(180.0f, 0, 1, 0);

            DrawModel(am->model, (Vector3){0, 0, 0}, 1.0f, (Color){alpha,alpha,alpha,alpha});

            /* Per-rank damaged helmet at head position */
            AstroModel *dmgHead = &ms->headshot[fi][ri];
            if (dmgHead && dmgHead->loaded) {
                DrawModel(dmgHead->model, (Vector3){0, HEAD_HEIGHT, 0}, 1.0f,
                    (Color){alpha,alpha,alpha,alpha});
            }

            rlPopMatrix();
        }
    }

    /* === HEAVY BLOOD SPRAY from the helmet hole === */
    if (t < HEADSHOT_BLOOD_DURATION) {
        float bf = 1.0f - t / HEADSHOT_BLOOD_DURATION;
        float pulse = sinf(t * 10.0f);

        /* Thick arterial spray — pressurized, pulsing */
        for (int s = 0; s < 22; s++) {
            float st = (float)s / 22.0f;
            float spurt = (0.2f + st * 0.6f) * (1.0f + pulse * 0.4f);
            float fall = st * st * 1.2f;
            float spread = sinf(s * 1.7f + t * 8.0f) * st * 0.15f;
            Vector3 sp = Vector3Add(holePos,
                Vector3Add(Vector3Scale(holeDir, spurt),
                Vector3Add(Vector3Scale(localUp, -fall),
                           Vector3Scale(localRight, spread))));
            float ssz = (0.09f + st * 0.04f) * bf * fade;
            unsigned char sa = (unsigned char)(240 * bf * (1.0f - st * 0.2f) * fade);
            Color col = (s%3==0) ? (Color){230,15,5,sa} :
                        (s%3==1) ? (Color){190,8,3,sa} : (Color){150,5,2,sa};
            DrawSphereEx(sp, ssz, 3, 3, col);
        }

        /* Thick drips cascading down */
        for (int d = 0; d < 14; d++) {
            float dt2 = fmodf(t * 1.8f + d * 0.28f, 2.5f);
            float dfall = dt2 * dt2 * 0.7f;
            float dspread = sinf(d * 1.3f + t * 0.5f) * 0.1f;
            Vector3 dp = Vector3Add(headWorldPos,
                Vector3Add(Vector3Scale(holeDir, 0.12f + sinf(d*2.1f)*0.06f),
                Vector3Add(Vector3Scale(localUp, -0.1f - dfall),
                           Vector3Scale(localRight, dspread))));
            float dsz = (0.05f + dt2 * 0.015f) * bf * fade;
            DrawSphereEx(dp, dsz, 3, 3,
                (Color){210,10,4,(unsigned char)(200*bf*fade)});
        }
    }

    /* === Air hissing from helmet hole (0-6s) === */
    if (t < 6.0f) {
        float pressure = 1.0f - t / 6.0f;
        for (int g = 0; g < 16; g++) {
            float gt = (float)g / 16.0f;
            float glife = fmodf(t * 3.0f + gt * 1.0f, 1.5f);
            float gjitter = sinf(gt * 5.0f + t * 12.0f) * 0.08f * pressure;
            Vector3 gp = Vector3Add(holePos,
                Vector3Scale(holeDir, glife * 1.5f * pressure));
            gp = Vector3Add(gp, Vector3Scale(localUp, glife * 0.3f));
            gp = Vector3Add(gp, Vector3Scale(localRight, gjitter));
            float gsz = (0.04f + glife * 0.1f) * pressure * fade;
            unsigned char ga = (unsigned char)((1.0f - glife/1.5f) * 180 * pressure * fade);
            DrawSphereEx(gp, gsz, 3, 3, (Color){220, 225, 235, ga});
        }
    }

    /* === BIG blood cloud at the head (first 2.5s) === */
    if (t < 2.5f) {
        float burst = t / 2.5f;
        for (int p = 0; p < 30; p++) {
            float pa = (float)p / 30.0f * PI * 2.0f;
            float pr = burst * 2.0f + (float)(p%5) * 0.06f;
            float py = sinf(pa * 1.3f + t * 3.0f) * pr * 0.3f;
            Vector3 pp = Vector3Add(holePos,
                Vector3Add(Vector3Scale(localRight, cosf(pa) * pr * 0.5f),
                Vector3Add(Vector3Scale(localUp, py + sinf(pa) * pr * 0.2f),
                           Vector3Scale(localFwd, sinf(pa) * pr * 0.4f))));
            float sz = (0.12f + (float)(p%3)*0.03f) * (1.0f - burst * 0.4f) * fade;
            unsigned char ba = (unsigned char)((1.0f - burst * 0.6f) * 200 * fade);
            Color col = (p%3==0) ? (Color){230,20,8,ba} :
                        (p%3==1) ? (Color){180,10,5,ba} : (Color){120,5,2,ba};
            DrawSphereEx(pp, sz, 3, 3, col);
        }
        /* Thick central blood mass */
        if (t < 0.8f) {
            float cf2 = 1.0f - t / 0.8f;
            DrawSphereEx(holePos, 0.6f * cf2 * fade, 5, 5,
                (Color){210,10,5,(unsigned char)(180*cf2*fade)});
            DrawSphereEx(Vector3Add(holePos, Vector3Scale(holeDir, 0.3f)),
                0.35f * cf2 * fade, 4, 4,
                (Color){180,8,3,(unsigned char)(150*cf2*fade)});
        }
    }
    #undef HEAD_HEIGHT
}

// === SKELETAL EVISCERATE — Blender dismemberment models fly apart ===
void DrawAstronautEviscerateSkeletal(Enemy *e, EnemyManager *em) {
    if (!em || !em->astroModels) return;
    AstroModelSet *ms = em->astroModels;
    int fi = (int)e->type;

    Vector3 pos = e->position;
    float t = e->evisTimer;
    float fade = (t > 7.0f) ? 1.0f - (t - 7.0f) / 3.0f : 1.0f;
    if (fade < 0) fade = 0;
    unsigned char alpha = (unsigned char)(255 * fade);

    Color bloodCol = {180, 10, 5, alpha};
    Color darkBlood = {100, 5, 2, alpha};
    Color brightBlood = {220, 20, 8, alpha};

    /* Map dismemberment parts to evisLimbPos indices:
     * 0=head, 1=torso, 2=arm_R, 3=arm_L, 4=leg_R, 5=leg_L */
    static const DismemberPart LIMB_MAP[6] = {
        DISMEMBER_HEAD, DISMEMBER_TORSO, DISMEMBER_ARM_R,
        DISMEMBER_ARM_L, DISMEMBER_LEG_R, DISMEMBER_LEG_L
    };
    static const float SPIN_SPEEDS[6] = {400, 120, 250, 330, 180, 240};
    static const Vector3 SPIN_AXES[6] = {
        {1, 0.3f, 0}, {0.5f, 1, 0.2f}, {1, 0.5f, 0.3f},
        {-1, 0.5f, 0.3f}, {0.3f, 1, 0.5f}, {0.3f, -1, 0.5f}
    };

    /* Draw each dismemberment part spinning through the air */
    for (int li = 0; li < 6; li++) {
        AstroModel *dm = &ms->dismember[fi][LIMB_MAP[li]];
        if (!dm->loaded) continue;

        Vector3 lp = Vector3Add(pos, e->evisLimbPos[li]);
        float spin = t * SPIN_SPEEDS[li];

        rlPushMatrix();
        rlTranslatef(lp.x, lp.y, lp.z);
        rlRotatef(spin, SPIN_AXES[li].x, SPIN_AXES[li].y, SPIN_AXES[li].z);
        rlScalef(fade, fade, fade);
        rlRotatef(180.0f, 0, 1, 0); /* Blender face fix */
        DrawModel(dm->model, (Vector3){0, 0, 0}, 1.0f, WHITE);
        rlPopMatrix();

        /* Blood spurts from each limb */
        if (t < 6.0f) {
            float pulse = sinf(t * 10.0f + li * 2.0f);
            for (int b = 0; b < 6; b++) {
                float bt = e->evisBloodTimer[li] * 2.2f + b * 0.18f;
                float spurt = 0.3f + pulse * 0.15f;
                Vector3 bp = {lp.x + sinf(bt*7)*spurt, lp.y + 0.2f + cosf(bt*4)*0.2f, lp.z + cosf(bt*5)*spurt};
                float sz = 0.06f * (1.0f - bt/3.0f);
                if (sz > 0) DrawSphereEx(bp, sz, 3, 3, (b%2==0) ? bloodCol : brightBlood);
            }
        }
    }

    /* === GORE EXPLOSION (first 0.5s) === */
    if (t < 0.5f) {
        float burst = t / 0.5f;
        for (int p = 0; p < 24; p++) {
            float pa = (float)p * 0.262f;
            float pr = burst * 3.5f;
            float py = sinf(pa * 1.5f + t * 8.0f) * pr * 0.4f;
            Vector3 pp = {pos.x + cosf(pa)*pr, pos.y + py, pos.z + sinf(pa)*pr};
            float sz = 0.15f * (1.0f - burst);
            Color bc = (p % 3 == 0) ? brightBlood : (p % 3 == 1) ? bloodCol : darkBlood;
            DrawSphereEx(pp, sz, 3, 3, bc);
        }
        DrawSphereEx(pos, 2.5f * (1.0f - burst), 5, 5,
            (Color){255, 20, 5, (unsigned char)((1.0f - burst) * 200)});
    }

    /* === BLOOD MIST === */
    if (t < 5.0f) {
        float mistFade = (t < 1.0f) ? t : (t < 3.5f) ? 1.0f : 1.0f - (t - 3.5f) / 1.5f;
        for (int m = 0; m < 14; m++) {
            float ma = (float)m * 0.449f + t * 1.5f;
            float mr = 0.5f + t * 0.8f + sinf(ma * 3.0f) * 0.3f;
            float my = sinf(t * 2.0f + ma) * 0.5f + 0.3f;
            Vector3 mp = {pos.x + cosf(ma)*mr, pos.y + my, pos.z + sinf(ma)*mr};
            DrawSphereEx(mp, 0.12f + sinf(t*3+m)*0.04f, 3, 3,
                (Color){150, 10, 5, (unsigned char)(mistFade * 80)});
        }
    }

    /* Bone fragments */
    if (t < 4.0f) {
        for (int b = 0; b < 12; b++) {
            float ba = (float)b * 0.55f + t * 4.0f;
            float br = t * 2.5f + (float)b * 0.25f;
            float by = t * 2.0f - t * t * 0.4f;
            Vector3 bp2 = {pos.x + cosf(ba)*br, pos.y + by, pos.z + sinf(ba)*br};
            Color bc = (b % 2 == 0) ? (Color){200, 190, 175, alpha} : (Color){160, 12, 6, alpha};
            DrawSphereEx(bp2, (b % 2 == 0) ? 0.035f : 0.05f, 3, 3, bc);
        }
    }
}
