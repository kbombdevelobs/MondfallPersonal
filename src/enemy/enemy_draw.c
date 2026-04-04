#include "enemy_draw.h"
#include "enemy_draw_death.h"
#include "world.h"
#include "rlgl.h"
#include <math.h>
#include <stddef.h>


void DrawAstronautModel(EnemyManager *em, Enemy *e) {
    Vector3 pos = e->position;
    Color tint = WHITE;
    if (e->hitFlash > 0) {
        unsigned char v = (unsigned char)(150 + e->hitFlash * 105);
        tint = (Color){255, v, v, 255};
    }

    Color visorColor, accentColor, suitBase, suitDark, bootColor, beltColor, buckleColor, helmetColor, gloveColor, backpackColor;

    if (e->type == ENEMY_SOVIET) {
        suitBase     = (Color){205, 0, 0, 255};
        suitDark     = (Color){160, 0, 0, 255};
        accentColor  = (Color){255, 215, 0, 255};
        visorColor   = (Color){255, 200, 50, 230};
        helmetColor  = (Color){220, 185, 40, 255};
        bootColor    = (Color){25, 25, 22, 255};
        beltColor    = (Color){120, 75, 35, 255};
        buckleColor  = (Color){255, 215, 0, 255};
        gloveColor   = (Color){30, 30, 28, 255};
        backpackColor= (Color){140, 10, 10, 255};
    } else {
        suitBase     = (Color){25, 40, 90, 255};
        suitDark     = (Color){18, 28, 65, 255};
        accentColor  = (Color){220, 220, 230, 255};
        visorColor   = (Color){100, 170, 255, 230};
        helmetColor  = (Color){195, 200, 210, 255};
        bootColor    = (Color){140, 115, 75, 255};
        beltColor    = (Color){80, 85, 60, 255};
        buckleColor  = (Color){200, 205, 215, 255};
        gloveColor   = (Color){160, 155, 140, 255};
        backpackColor= (Color){30, 45, 80, 255};
    }

    // === Delegate death states to enemy_draw_death.c ===
    if (e->state == ENEMY_EVISCERATING) {
        DrawAstronautEviscerate(e, em);
        return;
    }
    if (e->state == ENEMY_VAPORIZING) {
        DrawAstronautVaporize(e, em);
        return;
    }

    rlPushMatrix();
    rlTranslatef(pos.x, pos.y, pos.z);
    rlRotatef(e->facingAngle * RAD2DEG, 0, 1, 0);
    if (e->state == ENEMY_DYING) {
        rlRotatef(e->deathAngle, 1, 0, 0);
        rlRotatef(e->deathAngle * 0.7f, 0, 0, 1);
    }

    // === TORSO ===
    DrawCube((Vector3){0, 0, 0}, 0.9f, 1.5f, 0.55f, suitBase);
    DrawCube((Vector3){0, 0.2f, 0.02f}, 0.7f, 0.6f, 0.5f, suitDark);
    DrawCubeWires((Vector3){0, 0, 0}, 0.91f, 1.51f, 0.56f, suitDark);
    DrawCube((Vector3){0, 0.72f, 0}, 0.55f, 0.1f, 0.45f, suitDark);
    DrawCube((Vector3){0, -0.55f, 0}, 0.92f, 0.1f, 0.57f, beltColor);
    DrawCubeWires((Vector3){0, -0.55f, 0}, 0.93f, 0.11f, 0.58f, (Color){beltColor.r/2, beltColor.g/2, beltColor.b/2, 180});
    DrawCube((Vector3){0, -0.55f, 0.29f}, 0.12f, 0.09f, 0.02f, buckleColor);
    DrawCubeWires((Vector3){0, -0.55f, 0.29f}, 0.13f, 0.1f, 0.03f, (Color){buckleColor.r/2, buckleColor.g/2, buckleColor.b/2, 200});
    DrawCube((Vector3){0.5f, 0.6f, 0}, 0.18f, 0.2f, 0.4f, suitDark);
    DrawCube((Vector3){-0.5f, 0.6f, 0}, 0.18f, 0.2f, 0.4f, suitDark);
    DrawCube((Vector3){0.5f, 0.62f, 0.15f}, 0.15f, 0.12f, 0.02f, accentColor);
    DrawCube((Vector3){-0.5f, 0.62f, 0.15f}, 0.15f, 0.12f, 0.02f, accentColor);
    DrawCube((Vector3){0.2f, 0.35f, 0.28f}, 0.12f, 0.12f, 0.02f, accentColor);

    // === HELMET ===
    DrawSphere((Vector3){0, 1.1f, 0}, 0.48f, helmetColor);
    DrawCube((Vector3){0, 0.8f, 0}, 0.65f, 0.08f, 0.55f, suitDark);
    DrawModel(em->mdlVisor, (Vector3){0, 1.12f, 0.32f}, 1.0f, visorColor);
    DrawCubeWires((Vector3){0, 1.12f, 0.34f}, 0.42f, 0.32f, 0.04f, (Color){160,158,152,200});
    if (e->type == ENEMY_SOVIET) {
        DrawSphere((Vector3){0, 1.55f, 0}, 0.06f, accentColor);
    } else {
        DrawCube((Vector3){0, 1.5f, 0}, 0.12f, 0.04f, 0.04f, accentColor);
        DrawCube((Vector3){0, 1.5f, 0}, 0.04f, 0.12f, 0.04f, accentColor);
    }
    DrawCube((Vector3){0.12f, 1.55f, 0}, 0.03f, 0.12f, 0.03f, accentColor);

    // === BACKPACK ===
    DrawCube((Vector3){0, 0.1f, -0.42f}, 0.62f, 0.9f, 0.28f, backpackColor);
    DrawCubeWires((Vector3){0, 0.1f, -0.42f}, 0.63f, 0.91f, 0.29f, suitDark);
    for (int v = 0; v < 3; v++) {
        float vy = -0.1f + v * 0.3f;
        DrawCube((Vector3){0, vy, -0.57f}, 0.45f, 0.05f, 0.01f, suitDark);
    }
    Color hoseColor = (e->type == ENEMY_SOVIET) ? (Color){100,30,30,255} : (Color){60,65,90,255};
    DrawCube((Vector3){0.18f, 0.65f, -0.25f}, 0.05f, 0.5f, 0.05f, hoseColor);
    DrawCube((Vector3){-0.18f, 0.65f, -0.25f}, 0.05f, 0.5f, 0.05f, hoseColor);

    // === GUN ===
    float gunAim = e->shootAnim * -25.0f;
    rlPushMatrix();
    rlTranslatef(0.2f, 0.05f, 0.55f);
    rlRotatef(gunAim, 1, 0, 0);

    if (e->type == ENEMY_SOVIET) {
        Color GM = {40,42,50,255};
        Color BK = {25,27,32,255};
        Color RD = {255,40,20,230};
        DrawCube((Vector3){0,0,0}, 0.16f, 0.14f, 1.0f, GM);
        DrawCubeWires((Vector3){0,0,0}, 0.161f, 0.141f, 1.01f, BK);
        DrawCube((Vector3){0,0.02f,0.6f}, 0.09f, 0.09f, 0.35f, BK);
        DrawCube((Vector3){0,0.02f,0.82f}, 0.13f, 0.13f, 0.07f, GM);
        DrawCubeWires((Vector3){0,0.02f,0.82f}, 0.131f, 0.131f, 0.071f, BK);
        DrawSphere((Vector3){0,-0.16f,0.0f}, 0.13f, GM);
        DrawSphere((Vector3){0,-0.16f,0.12f}, 0.11f, GM);
        DrawCube((Vector3){0.085f,0,0}, 0.008f, 0.04f, 0.7f, RD);
        DrawCube((Vector3){-0.085f,0,0}, 0.008f, 0.04f, 0.7f, RD);
        DrawCube((Vector3){0,0,-0.4f}, 0.06f, 0.12f, 0.3f, BK);
        DrawCube((Vector3){0,-0.02f,-0.55f}, 0.09f, 0.08f, 0.06f, GM);
    } else {
        Color GM = {30,35,50,255};
        Color BK = {20,23,35,255};
        Color BL = {50,130,255,230};
        DrawCube((Vector3){0,0,0}, 0.14f, 0.14f, 0.8f, GM);
        DrawCubeWires((Vector3){0,0,0}, 0.141f, 0.141f, 0.801f, BK);
        DrawCube((Vector3){0,0,0.48f}, 0.18f, 0.18f, 0.12f, GM);
        DrawCube((Vector3){0,0,0.58f}, 0.26f, 0.26f, 0.05f, BK);
        DrawCubeWires((Vector3){0,0,0.58f}, 0.261f, 0.261f, 0.051f, (Color){40,80,180,150});
        DrawCube((Vector3){0,0.12f,-0.05f}, 0.08f, 0.08f, 0.08f, BL);
        DrawSphere((Vector3){0,0.12f,-0.05f}, 0.05f, (Color){120,200,255,200});
        DrawCube((Vector3){0.075f,0,0}, 0.008f, 0.03f, 0.6f, BL);
        DrawCube((Vector3){-0.075f,0,0}, 0.008f, 0.03f, 0.6f, BL);
        DrawCube((Vector3){0,0,-0.35f}, 0.08f, 0.1f, 0.15f, BK);
    }
    if (e->muzzleFlash > 0) {
        float mz = (e->type == ENEMY_SOVIET) ? 0.88f : 0.64f;
        Color fc = (e->type == ENEMY_SOVIET) ?
            (Color){255,180,50,(unsigned char)(e->muzzleFlash*230)} :
            (Color){80,180,255,(unsigned char)(e->muzzleFlash*230)};
        DrawSphere((Vector3){0,0,mz}, e->muzzleFlash*0.18f, fc);
    }
    rlPopMatrix();

    // === ARMS ===
    float armSwing = sinf(e->walkCycle) * 25.0f;
    rlPushMatrix();
    rlTranslatef(0.52f, 0.35f, 0.1f);
    rlRotatef(-25 - armSwing + gunAim * 0.4f, 1, 0, 0);
    rlRotatef(-12, 0, 0, 1);
    DrawModel(em->mdlArm, (Vector3){0, 0, 0}, 1.0f, tint);
    DrawCube((Vector3){0, -0.42f, 0}, 0.24f, 0.16f, 0.24f, gloveColor);
    rlPopMatrix();
    rlPushMatrix();
    rlTranslatef(-0.52f, 0.35f, 0.1f);
    rlRotatef(-25 + armSwing + gunAim * 0.4f, 1, 0, 0);
    rlRotatef(12, 0, 0, 1);
    DrawModel(em->mdlArm, (Vector3){0, 0, 0}, 1.0f, tint);
    DrawCube((Vector3){0, -0.42f, 0}, 0.24f, 0.16f, 0.24f, gloveColor);
    rlPopMatrix();

    // === LEGS ===
    float legSwing = sinf(e->walkCycle) * 35.0f;
    rlPushMatrix();
    rlTranslatef(0.22f, -0.85f, 0);
    rlRotatef(legSwing, 1, 0, 0);
    DrawCube((Vector3){0, -0.05f, 0}, 0.3f, 0.45f, 0.3f, suitBase);
    DrawCubeWires((Vector3){0, -0.05f, 0}, 0.31f, 0.46f, 0.31f, suitDark);
    DrawCube((Vector3){0, -0.3f, 0}, 0.22f, 0.08f, 0.22f, suitDark);
    rlPushMatrix();
    rlTranslatef(0, -0.3f, 0);
    rlRotatef(-legSwing * 0.4f, 1, 0, 0);
    DrawCube((Vector3){0, -0.2f, 0}, 0.28f, 0.4f, 0.28f, suitBase);
    DrawModel(em->mdlBoot, (Vector3){0, -0.44f, 0.04f}, 1.0f, bootColor);
    rlPopMatrix();
    rlPopMatrix();
    rlPushMatrix();
    rlTranslatef(-0.22f, -0.85f, 0);
    rlRotatef(-legSwing, 1, 0, 0);
    DrawCube((Vector3){0, -0.05f, 0}, 0.3f, 0.45f, 0.3f, suitBase);
    DrawCubeWires((Vector3){0, -0.05f, 0}, 0.31f, 0.46f, 0.31f, suitDark);
    DrawCube((Vector3){0, -0.3f, 0}, 0.22f, 0.08f, 0.22f, suitDark);
    rlPushMatrix();
    rlTranslatef(0, -0.3f, 0);
    rlRotatef(legSwing * 0.4f, 1, 0, 0);
    DrawCube((Vector3){0, -0.2f, 0}, 0.28f, 0.4f, 0.28f, suitBase);
    DrawModel(em->mdlBoot, (Vector3){0, -0.44f, 0.04f}, 1.0f, bootColor);
    rlPopMatrix();
    rlPopMatrix();

    rlPopMatrix(); // root

    // Death particles — delegate to enemy_draw_death.c
    if (e->state == ENEMY_DYING) {
        DrawAstronautRagdoll(e);
    }
}

// === LOD 1: Simplified astronaut (~8 draw calls) ===
void DrawAstronautLOD1(Enemy *e) {
    Color suitBase, helmetColor, bootColor;

    if (e->type == ENEMY_SOVIET) {
        suitBase    = (Color){205, 0, 0, 255};
        helmetColor = (Color){220, 185, 40, 255};
        bootColor   = (Color){25, 25, 22, 255};
    } else {
        suitBase    = (Color){25, 40, 90, 255};
        helmetColor = (Color){195, 200, 210, 255};
        bootColor   = (Color){140, 115, 75, 255};
    }

    if (e->hitFlash > 0) {
        unsigned char v = (unsigned char)(150 + e->hitFlash * 105);
        suitBase = (Color){255, v, v, 255};
    }

    rlPushMatrix();
    rlTranslatef(e->position.x, e->position.y, e->position.z);
    rlRotatef(e->facingAngle * RAD2DEG, 0, 1, 0);
    if (e->state == ENEMY_DYING) {
        rlRotatef(e->deathAngle, 1, 0, 0);
    }

    // Torso
    DrawCube((Vector3){0, 0, 0}, 0.9f, 1.5f, 0.55f, suitBase);
    // Head
    DrawSphere((Vector3){0, 1.1f, 0}, 0.48f, helmetColor);
    // Arms
    DrawCube((Vector3){0.55f, 0.1f, 0}, 0.22f, 0.8f, 0.22f, suitBase);
    DrawCube((Vector3){-0.55f, 0.1f, 0}, 0.22f, 0.8f, 0.22f, suitBase);
    // Legs with walk animation
    float legSwing = sinf(e->walkCycle) * 20.0f;
    rlPushMatrix();
    rlTranslatef(0.22f, -0.85f, 0);
    rlRotatef(legSwing, 1, 0, 0);
    DrawCube((Vector3){0, -0.2f, 0}, 0.3f, 0.85f, 0.3f, suitBase);
    DrawCube((Vector3){0, -0.65f, 0.04f}, 0.28f, 0.3f, 0.35f, bootColor);
    rlPopMatrix();
    rlPushMatrix();
    rlTranslatef(-0.22f, -0.85f, 0);
    rlRotatef(-legSwing, 1, 0, 0);
    DrawCube((Vector3){0, -0.2f, 0}, 0.3f, 0.85f, 0.3f, suitBase);
    DrawCube((Vector3){0, -0.65f, 0.04f}, 0.28f, 0.3f, 0.35f, bootColor);
    rlPopMatrix();

    rlPopMatrix();
}

// === LOD 2: Single colored cube (1 draw call) ===
void DrawAstronautLOD2(Enemy *e) {
    Color col = (e->type == ENEMY_SOVIET)
        ? (Color){205, 0, 0, 255}
        : (Color){25, 40, 90, 255};
    DrawCube(e->position, 0.9f, 2.5f, 0.55f, col);
}
