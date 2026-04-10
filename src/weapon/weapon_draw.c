#include "weapon.h"
#include "weapon_draw.h"
#include "weapon_model.h"
#include "rlgl.h"
#include <math.h>
#include <stdlib.h>

// Draw a cached mesh at position with uniform scale and color (no per-frame mesh generation)
static void DrawCachedMesh(Mesh mesh, Material *mat, Vector3 pos, float scale, Color color) {
    if (scale < 0.001f) return; // skip degenerate zero-scale draws
    mat->maps[MATERIAL_MAP_DIFFUSE].color = color;
    Matrix transform = MatrixMultiply(
        MatrixScale(scale, scale, scale),
        MatrixTranslate(pos.x, pos.y, pos.z));
    DrawMesh(mesh, *mat, transform);
}

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

    /* If a .glb model is loaded for this weapon, draw it instead of procedural */
    WeaponModelSet *wms = WeaponModelsGet();
    if (wms->available && wms->playerLoaded[w->current]) {
        /* Models built in Blender with barrel along +Y. glTF Y-up export maps
         * Blender Y → loaded -Z. 180° around Y flips barrel forward. */
        float modelScale = 1.8f;
        if (w->current == WEAPON_JACKHAMMER) modelScale = 1.6f;

        rlPushMatrix();
        rlRotatef(180.0f, 0.0f, 1.0f, 0.0f);
        DrawModel(wms->playerWeapons[w->current], (Vector3){0, 0, 0}, modelScale, WHITE);
        rlPopMatrix();

        /* Muzzle flash effects on top of the model */
        if (w->muzzleFlash > 0) {
            Vector3 flashPos;
            float flashSize;
            Color flashColor;
            if (w->current == WEAPON_RAKETENFAUST) {
                flashPos = (Vector3){0, 0, 0.7f};
                flashSize = w->muzzleFlash * 0.15f;
                flashColor = (Color){255,180,50,(unsigned char)(w->muzzleFlash*200)};
            } else if (w->current == WEAPON_JACKHAMMER) {
                /* No muzzle flash for melee */
                flashSize = 0;
                flashPos = (Vector3){0,0,0};
                flashColor = (Color){0,0,0,0};
            } else {
                flashPos = (Vector3){0, 0, 0.8f};
                flashSize = w->muzzleFlash * 0.1f;
                flashColor = (Color){0,230,255,(unsigned char)(w->muzzleFlash*220)};
            }
            if (flashSize > 0.001f) DrawSphere(flashPos, flashSize, flashColor);
        }
        rlPopMatrix();
        return;
    }

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
            // === JACKHAMMER: Heavy pneumatic war-hammer / industrial impaler ===
            float ao = sinf(w->jackhammerAnim * 20.0f) * w->jackhammerAnim * 0.08f;
            // Lunge thrust animation — push the weapon forward hard
            float lungeThrust = 0;
            if (w->jackhammerLunging) {
                float lt = w->jackhammerLunge / JACKHAMMER_LUNGE_DURATION;
                lungeThrust = sinf((1.0f - lt) * PI) * 0.25f; // thrust forward then back
            }

            Color CH = {180,185,195,255};  // chrome
            Color GN = {50,65,40,255};     // military green
            Color GR = {85,90,82,255};     // dark green-gray
            Color MT = {170,170,162,255};   // brushed metal
            Color RD = {220,40,25,255};     // hazard red
            Color ST = {215,215,208,255};   // polished steel
            Color BK = {30,32,28,255};      // black
            Color YW = {210,190,40,255};    // hazard yellow
            Color OR = {255,140,30,200};    // energy orange
            Color BL = {140,10,5,255};      // blood/rust

            rlPushMatrix();
            rlRotatef(50, 1, 0, 0);
            rlTranslatef(0, 0, -lungeThrust); // lunge pushes weapon forward

            // === Main body — heavy industrial shaft ===
            DrawCube((Vector3){0, ao, 0}, 0.06f, 0.42f, 0.06f, GN);
            DrawCubeWires((Vector3){0, ao, 0}, 0.061f, 0.421f, 0.061f, BK);
            // Reinforcement ribs along shaft
            for (int r = 0; r < 4; r++) {
                float ry = -0.12f + r * 0.09f + ao;
                DrawCube((Vector3){0, ry, 0}, 0.065f, 0.015f, 0.065f, GR);
            }

            // === Motor housing (top) — bigger, industrial ===
            DrawCube((Vector3){0, 0.22f+ao, 0}, 0.075f, 0.08f, 0.075f, CH);
            DrawCubeWires((Vector3){0, 0.22f+ao, 0}, 0.076f, 0.081f, 0.076f, GR);
            // Exhaust vents on motor
            DrawCube((Vector3){0.04f, 0.24f+ao, 0}, 0.005f, 0.04f, 0.05f, BK);
            DrawCube((Vector3){-0.04f, 0.24f+ao, 0}, 0.005f, 0.04f, 0.05f, BK);
            // Power indicator light
            float glow = 0.5f + sinf(GetTime() * 6.0f) * 0.5f;
            DrawCube((Vector3){0, 0.27f+ao, 0.035f}, 0.015f, 0.015f, 0.005f,
                (Color){(unsigned char)(255*glow), (unsigned char)(40*glow), 0, 255});

            // === Grip — T-handle with rubber wrapping ===
            DrawCube((Vector3){0, 0.16f+ao, 0}, 0.16f, 0.025f, 0.035f, MT);
            DrawCubeWires((Vector3){0, 0.16f+ao, 0}, 0.161f, 0.026f, 0.036f, GR);
            // Rubber grip wraps
            DrawCube((Vector3){-0.07f, 0.16f+ao, 0}, 0.025f, 0.028f, 0.038f, BK);
            DrawCube((Vector3){0.07f, 0.16f+ao, 0}, 0.025f, 0.028f, 0.038f, BK);
            // Lower fore-grip
            DrawCube((Vector3){0, 0.04f+ao, 0}, 0.035f, 0.08f, 0.04f, BK);

            // === Piston housing — heavy, exposed pistons ===
            DrawCube((Vector3){0, -0.15f+ao, 0}, 0.08f, 0.12f, 0.08f, MT);
            DrawCubeWires((Vector3){0, -0.15f+ao, 0}, 0.081f, 0.121f, 0.081f, GR);
            // Visible piston rods (animated)
            float pistonTravel = ao * 3.0f;
            DrawCube((Vector3){0.03f, -0.12f+pistonTravel, 0.03f}, 0.012f, 0.08f, 0.012f, ST);
            DrawCube((Vector3){-0.03f, -0.12f+pistonTravel, -0.03f}, 0.012f, 0.08f, 0.012f, ST);
            // Hydraulic lines
            DrawCube((Vector3){0.045f, -0.08f+ao, 0}, 0.008f, 0.2f, 0.008f, RD);
            DrawCube((Vector3){-0.045f, -0.08f+ao, 0}, 0.008f, 0.2f, 0.008f, RD);

            // === Hazard stripes — front face ===
            for (int s = 0; s < 3; s++) {
                float sy = -0.13f + s * 0.04f + ao;
                DrawCube((Vector3){0, sy, 0.042f}, 0.082f, 0.012f, 0.003f, YW);
            }
            DrawCube((Vector3){0.042f, -0.15f+ao, 0}, 0.003f, 0.1f, 0.082f, YW);

            // === SPIKE HEAD — brutal war-pick style ===
            // Transition collar
            DrawCube((Vector3){0, -0.22f+ao, 0}, 0.07f, 0.03f, 0.07f, CH);
            // Main spike shaft — thick, tapers to point
            DrawCube((Vector3){0, -0.28f+ao, 0}, 0.04f, 0.1f, 0.04f, ST);
            DrawCube((Vector3){0, -0.35f+ao, 0}, 0.025f, 0.08f, 0.025f, ST);
            DrawCube((Vector3){0, -0.42f+ao, 0}, 0.012f, 0.06f, 0.018f, (Color){230,230,225,255});
            // Sharp tip
            DrawCube((Vector3){0, -0.46f+ao, 0}, 0.005f, 0.03f, 0.01f, WHITE);
            // Side prongs — war-pick flanges
            DrawCube((Vector3){0.04f, -0.26f+ao, 0}, 0.06f, 0.015f, 0.02f, ST);
            DrawCube((Vector3){-0.04f, -0.26f+ao, 0}, 0.06f, 0.015f, 0.02f, ST);
            DrawCube((Vector3){0.07f, -0.27f+ao, 0}, 0.02f, 0.025f, 0.015f, ST); // flange tips
            DrawCube((Vector3){-0.07f, -0.27f+ao, 0}, 0.02f, 0.025f, 0.015f, ST);
            // Blood/rust stains on spike
            DrawCube((Vector3){0.005f, -0.38f+ao, 0.008f}, 0.02f, 0.04f, 0.008f, BL);
            DrawCube((Vector3){-0.008f, -0.32f+ao, -0.005f}, 0.015f, 0.03f, 0.01f, BL);

            // === Sparks on impact ===
            if (w->jackhammerAnim > 0.4f) {
                for (int i = 0; i < 8; i++) {
                    float sx = (float)GetRandomValue(-8,8)*0.012f;
                    float sy = -0.48f + (float)GetRandomValue(-4,4)*0.01f;
                    float sz = (float)GetRandomValue(-8,8)*0.012f;
                    Color sc = (i % 3 == 0) ? OR : YW;
                    DrawSphere((Vector3){sx, sy+ao, sz}, 0.012f, sc);
                }
            }
            // Energy glow when lunging
            if (w->jackhammerLunging) {
                float lg = w->jackhammerLunge / JACKHAMMER_LUNGE_DURATION;
                DrawSphere((Vector3){0, -0.44f+ao, 0}, 0.08f * lg,
                    (Color){255, 100, 20, (unsigned char)(lg * 200)});
            }

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
        DrawCachedMesh(w->meshSphere, &w->matDefault, w->raketenBeamEnd, 0.6f, (Color){255, 240, 180, 180});
    }

    // Beam trails — width-aware rendering
    for (int i = 0; i < w->beamCount; i++) {
        float ml = w->beams[i].maxLife > 0 ? w->beams[i].maxLife : BEAM_TRAIL_LIFE;
        float a = w->beams[i].life / ml;
        Color c = w->beams[i].color; c.a = (unsigned char)(a * c.a);
        float bw = w->beams[i].width;

        if (bw >= 3.0f) {
            // Fat Liberty Blaster rail beam: multiple offset lines + bright core + glow spheres
            for (int ox = -2; ox <= 2; ox++) {
                for (int oy = -2; oy <= 2; oy++) {
                    float d = sqrtf((float)(ox*ox + oy*oy));
                    if (d > 2.5f) continue;
                    float s = 0.025f * bw;
                    Vector3 off = {ox * s, oy * s, 0};
                    Color lc = c;
                    if (d < 1.0f) lc = (Color){255, 255, 255, (unsigned char)(a * 255)}; // white core
                    else lc.a = (unsigned char)(a * c.a * (1.0f - d / 3.0f));
                    DrawLine3D(Vector3Add(w->beams[i].start, off),
                               Vector3Add(w->beams[i].end, off), lc);
                }
            }
            // Impact glow at endpoint
            DrawCachedMesh(w->meshSphere, &w->matDefault, w->beams[i].end,
                0.4f * a * bw * 0.3f, (Color){c.r, c.g, c.b, (unsigned char)(a * 120)});
        } else if (bw >= 1.5f) {
            // PPSh spread tracers: thin lines with slight random offsets baked in
            DrawLine3D(w->beams[i].start, w->beams[i].end, c);
            Vector3 off1 = {0.015f, 0.01f, 0};
            Vector3 off2 = {-0.01f, 0.015f, 0};
            Color dim = c; dim.a = (unsigned char)(a * c.a * 0.6f);
            DrawLine3D(Vector3Add(w->beams[i].start, off1),
                       Vector3Add(w->beams[i].end, off1), dim);
            DrawLine3D(Vector3Add(w->beams[i].start, off2),
                       Vector3Add(w->beams[i].end, off2), dim);
        } else {
            // Standard thin beam (MP40 etc.)
            DrawLine3D(w->beams[i].start, w->beams[i].end, c);
            DrawLine3D(Vector3Add(w->beams[i].start, (Vector3){0.02f,0.02f,0}),
                       Vector3Add(w->beams[i].end, (Vector3){0.02f,0.02f,0}), c);
        }
    }
    // Rockets
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        if (!w->projectiles[i].active) continue;
        DrawCachedMesh(w->meshSphere, &w->matDefault, w->projectiles[i].position, 0.25f, w->projectiles[i].color);
        DrawCachedMesh(w->meshSphere, &w->matDefault, w->projectiles[i].position, 0.4f, (Color){255,220,100,80});
        Vector3 trail = Vector3Subtract(w->projectiles[i].position,
            Vector3Scale(Vector3Normalize(w->projectiles[i].velocity), 1.0f));
        DrawCachedMesh(w->meshSphere, &w->matDefault, trail, 0.3f, (Color){180,180,180,60});
    }
    // Explosions
    for (int i = 0; i < MAX_EXPLOSIONS; i++) {
        if (!w->explosions[i].active) continue;
        float t = w->explosions[i].timer / w->explosions[i].duration;
        float r = w->explosions[i].radius * t;
        unsigned char alpha = (unsigned char)((1.0f - t) * 200);
        // Fireball
        DrawCachedMesh(w->meshSphere, &w->matDefault, w->explosions[i].position, r * 0.6f, (Color){255, 200, 50, alpha});
        DrawCachedMesh(w->meshSphere, &w->matDefault, w->explosions[i].position, r * 0.8f, (Color){255, 120, 20, (unsigned char)(alpha * 0.6f)});
        DrawCachedMesh(w->meshSphere, &w->matDefault, w->explosions[i].position, r, (Color){255, 80, 0, (unsigned char)(alpha * 0.3f)});
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
            DrawCachedMesh(w->meshCube, &w->matDefault, dp, ds, (Color){110, 108, 100, alpha});
        }
    }
}

