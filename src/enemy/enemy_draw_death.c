#include "enemy_draw_death.h"
#include "world.h"
#include "rlgl.h"
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
        float elapsed = 10.0f - e->deathTimer;
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
    } else {
        float elapsed = 12.0f - e->deathTimer;
        float poolTime = elapsed - 0.5f;
        if (poolTime < 0) poolTime = 0;
        float poolR = 0.3f + poolTime * 0.25f;
        if (poolR > 3.5f) poolR = 3.5f;

        int segs = 10;
        float cx = pos.x, cz = pos.z;
        float cH = WorldGetHeight(cx, cz) + 0.06f;

        for (int s = 0; s < segs; s++) {
            float a0 = (float)s / segs * 2.0f * PI;
            float a1 = (float)(s + 1) / segs * 2.0f * PI;

            float x0 = cx + cosf(a0) * poolR;
            float z0 = cz + sinf(a0) * poolR;
            float h0 = WorldGetHeight(x0, z0) + 0.06f;

            float x1 = cx + cosf(a1) * poolR;
            float z1 = cz + sinf(a1) * poolR;
            float h1 = WorldGetHeight(x1, z1) + 0.06f;

            DrawTriangle3D(
                (Vector3){cx, cH, cz},
                (Vector3){x1, h1, z1},
                (Vector3){x0, h0, z0},
                (Color){120, 8, 5, 200});

            float ix0 = cx + cosf(a0) * poolR * 0.6f;
            float iz0 = cz + sinf(a0) * poolR * 0.6f;
            float ih0 = WorldGetHeight(ix0, iz0) + 0.07f;
            float ix1 = cx + cosf(a1) * poolR * 0.6f;
            float iz1 = cz + sinf(a1) * poolR * 0.6f;
            float ih1 = WorldGetHeight(ix1, iz1) + 0.07f;

            DrawTriangle3D(
                (Vector3){cx, cH + 0.01f, cz},
                (Vector3){ix1, ih1, iz1},
                (Vector3){ix0, ih0, iz0},
                (Color){160, 15, 10, 220});
        }

        if (elapsed < 8.0f) {
            float dripFade = (elapsed < 6.0f) ? 1.0f : 1.0f - (elapsed - 6.0f) / 2.0f;
            for (int d = 0; d < 5; d++) {
                float dt2 = elapsed * 3.0f + (float)d * 1.3f;
                float dlife = fmodf(dt2, 1.5f) / 1.5f;
                float dx = cx + sinf(dt2 * 0.7f) * 0.3f;
                float dz = cz + cosf(dt2 * 0.9f) * 0.3f;
                float dy = pos.y + 0.3f - dlife * (pos.y - cH + 0.5f);
                float dGH = WorldGetHeight(dx, dz) + 0.08f;
                if (dy < dGH) dy = dGH;
                DrawSphereEx((Vector3){dx, dy, dz}, 0.03f * dripFade, 3, 3,
                    (Color){180, 12, 6, (unsigned char)(200 * dripFade)});
            }
        }

        for (int b = 0; b < 4; b++) {
            float bt = GetTime() * 3.0f + (float)b * 1.6f;
            float bx = cx + cosf(bt) * poolR * 0.35f;
            float bz = cz + sinf(bt) * poolR * 0.35f;
            float bH = WorldGetHeight(bx, bz) + 0.12f;
            float bubble = sinf(bt * 2.0f) * 0.03f;
            DrawSphereEx((Vector3){bx, bH + bubble, bz},
                0.035f, 3, 3, (Color){160, 15, 10, 160});
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
