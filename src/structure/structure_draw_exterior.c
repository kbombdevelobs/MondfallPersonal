#include "structure_draw.h"
#include "world.h"
#include "rlgl.h"
#include <math.h>

// ============================================================================
// EXTERIOR COLOR PALETTE
// ============================================================================

// Exterior — lunar concrete & steel
#define EXT_CONCRETE    (Color){145, 140, 135, 255}
#define EXT_CONCRETE_DK (Color){100, 95, 90, 255}
#define EXT_STEEL       (Color){75, 78, 82, 255}
#define EXT_STEEL_LT    (Color){110, 112, 115, 255}
#define EXT_ACCENT_RED  (Color){160, 35, 30, 255}
#define EXT_DOOR_DARK   (Color){12, 12, 16, 255}
#define EXT_DOOR_FRAME  (Color){55, 58, 62, 255}
#define EXT_LIGHT_GREEN (Color){30, 200, 80, 180}
#define EXT_ANTENNA     (Color){120, 118, 112, 255}

// Bright yellow for hazard markings — high contrast against lunar gray
#define HAZARD_YELLOW (Color){220, 200, 40, 255}
#define HAZARD_BLACK  (Color){25, 25, 25, 255}

// ============================================================================
// GEODESIC DOME EXTERIOR
// ============================================================================

// Draw a geodesic-style dome from triangular facets using cubes at angles
static void DrawGeodesicDome(float cx, float cy, float cz, float radius, float height, Structure *s) {
    int rings = 5;
    int segs = MOONBASE_GEODESIC_SEGS;

    // Solid opaque inner fill — prevents seeing void through gaps
    // Kept smaller than dome so airlock corridors aren't buried
    float bodyH = height * 0.55f;
    float bodyR = radius * 0.7f; // well inside dome shell, won't hide doors
    for (int i = 0; i < 8; i++) {
        float a = (float)i / 8.0f * PI;
        float cA = cosf(a);
        float sA = sinf(a);
        DrawCube((Vector3){cx, cy + bodyH * 0.5f, cz}, bodyR * 2.0f * fabsf(cA) + bodyR * 0.6f,
                 bodyH, bodyR * 2.0f * fabsf(sA) + bodyR * 0.6f, EXT_CONCRETE_DK);
    }
    // Upper fill — also kept narrow
    DrawCube((Vector3){cx, cy + bodyH + (height - bodyH) * 0.4f, cz},
             radius * 0.75f, (height - bodyH) * 0.8f, radius * 0.75f, EXT_CONCRETE_DK);

    // Geodesic panels on top of the solid fill
    for (int ring = 0; ring < rings; ring++) {
        float t0 = (float)ring / (float)rings;
        float t1 = (float)(ring + 1) / (float)rings;
        float a0 = t0 * PI * 0.5f;
        float a1 = t1 * PI * 0.5f;
        float r0 = radius * cosf(a0);
        float r1 = radius * cosf(a1);
        float y0 = cy + height * sinf(a0);
        float y1 = cy + height * sinf(a1);

        int shade = 135 + (ring % 2) * 15;
        Color ringCol = {(unsigned char)shade, (unsigned char)(shade - 3), (unsigned char)(shade - 6), 255};

        for (int seg = 0; seg < segs; seg++) {
            float ang0 = (float)seg / (float)segs * 2.0f * PI;
            float ang1 = (float)(seg + 1) / (float)segs * 2.0f * PI;
            float midAng = (ang0 + ang1) * 0.5f;
            float midR = (r0 + r1) * 0.5f;
            float midY = (y0 + y1) * 0.5f;

            // Skip panels that overlap with door openings (bottom ring only)
            if (ring == 0) {
                bool skipForDoor = false;
                for (int d = 0; d < s->doorCount; d++) {
                    float doorAng = s->doorAngles[d];
                    float diff = midAng - doorAng;
                    // Normalize to [-PI, PI]
                    while (diff > PI) diff -= 2.0f * PI;
                    while (diff < -PI) diff += 2.0f * PI;
                    if (fabsf(diff) < (2.0f * PI / (float)segs) * 0.9f) {
                        skipForDoor = true;
                        break;
                    }
                }
                if (skipForDoor) continue;
            }

            float px = cx + cosf(midAng) * midR;
            float pz = cz + sinf(midAng) * midR;

            float panelW = 2.0f * midR * sinf(PI / (float)segs) * 1.08f;
            float panelH = (y1 - y0) * 1.15f;
            if (panelW < 0.1f) panelW = 0.1f;

            rlPushMatrix();
            rlTranslatef(px, midY, pz);
            rlRotatef(-midAng * 180.0f / PI + 90.0f, 0, 1, 0);
            float tilt = (a0 + a1) * 0.5f * 180.0f / PI;
            rlRotatef(tilt, 1, 0, 0);
            DrawCube((Vector3){0, 0, 0}, panelW, panelH, 0.18f, ringCol);
            DrawCubeWires((Vector3){0, 0, 0}, panelW, panelH, 0.19f, EXT_STEEL);
            rlPopMatrix();
        }
    }

    // Cap at top
    DrawCube((Vector3){cx, cy + height, cz}, radius * 0.35f, 0.25f, radius * 0.35f, EXT_STEEL);
}

static void DrawExteriorDoor(float cx, float cy, float cz, float angle, bool expended) {
    // Airlock corridor — wide, bold, high-contrast
    float doorDist = MOONBASE_EXTERIOR_RADIUS * 1.1f;
    float dx = cx + cosf(angle) * doorDist;
    float dz = cz + sinf(angle) * doorDist;

    rlPushMatrix();
    rlTranslatef(dx, cy, dz);
    rlRotatef(-angle * 180.0f / PI + 90.0f, 0, 1, 0);

    float corrLen = 2.8f;
    float corrW = 2.0f;    // wider corridor
    float corrH = 2.2f;    // player height + clearance
    float corrMidZ = -corrLen * 0.5f;

    // Corridor shell — bright white/light gray to contrast with dark dome
    Color shellCol = {180, 178, 172, 255};
    DrawCube((Vector3){-corrW * 0.5f, corrH * 0.5f, corrMidZ}, 0.25f, corrH, corrLen, shellCol);
    DrawCube((Vector3){corrW * 0.5f, corrH * 0.5f, corrMidZ}, 0.25f, corrH, corrLen, shellCol);
    DrawCube((Vector3){0, corrH + 0.12f, corrMidZ}, corrW + 0.3f, 0.25f, corrLen, shellCol);
    DrawCube((Vector3){0, -0.06f, corrMidZ}, corrW + 0.3f, 0.12f, corrLen, EXT_STEEL);

    // Dark interior fill
    DrawCube((Vector3){0, corrH * 0.5f, corrMidZ + 0.05f}, corrW - 0.3f, corrH - 0.2f, corrLen - 0.2f, EXT_DOOR_DARK);

    // Yellow/black hazard chevrons along corridor roof
    for (float rz = -0.1f; rz > -corrLen + 0.2f; rz -= 0.6f) {
        int idx = (int)((-rz) / 0.6f);
        Color chevCol = (idx % 2 == 0) ? HAZARD_YELLOW : HAZARD_BLACK;
        DrawCube((Vector3){0, corrH + 0.2f, rz}, corrW + 0.1f, 0.08f, 0.25f, chevCol);
    }

    // Ribbed steel reinforcement — thicker, more visible
    for (float rz = -0.1f; rz > -corrLen; rz -= 0.7f) {
        DrawCube((Vector3){-corrW * 0.5f - 0.08f, corrH * 0.5f, rz}, 0.15f, corrH + 0.2f, 0.12f, EXT_STEEL);
        DrawCube((Vector3){corrW * 0.5f + 0.08f, corrH * 0.5f, rz}, 0.15f, corrH + 0.2f, 0.12f, EXT_STEEL);
    }

    // Outer door frame — bold, dark steel
    float endZ = -corrLen;
    float fw = 0.25f;
    DrawCube((Vector3){-corrW * 0.5f - fw, corrH * 0.5f, endZ}, fw * 2, corrH + 0.3f, 0.35f, EXT_DOOR_FRAME);
    DrawCube((Vector3){corrW * 0.5f + fw, corrH * 0.5f, endZ}, fw * 2, corrH + 0.3f, 0.35f, EXT_DOOR_FRAME);
    DrawCube((Vector3){0, corrH + 0.2f, endZ}, corrW + fw * 4 + 0.2f, 0.35f, 0.35f, EXT_DOOR_FRAME);

    // Red/yellow hazard stripes on door frame
    for (int stripe = 0; stripe < 4; stripe++) {
        float sy = 0.3f + (float)stripe * 0.5f;
        Color sc = (stripe % 2 == 0) ? EXT_ACCENT_RED : HAZARD_YELLOW;
        DrawCube((Vector3){-corrW * 0.5f - fw - 0.02f, sy, endZ - 0.12f}, 0.1f, 0.2f, 0.1f, sc);
        DrawCube((Vector3){corrW * 0.5f + fw + 0.02f, sy, endZ - 0.12f}, 0.1f, 0.2f, 0.1f, sc);
    }

    // Door opening
    float openW = corrW - 0.4f;
    float openH = 2.0f;
    DrawCube((Vector3){0, openH * 0.5f, endZ - 0.06f}, openW, openH, 0.1f, EXT_DOOR_DARK);

    // Threshold step
    DrawCube((Vector3){0, 0.06f, endZ - 0.25f}, corrW + 0.2f, 0.12f, 0.5f, EXT_STEEL);

    // Indicator light — green stocked / red expended, large and bright
    float blink = (sinf((float)GetTime() * 3.5f + angle * 2.0f) > 0) ? 1.0f : 0.4f;
    Color g;
    if (expended)
        g = (Color){(unsigned char)(220 * blink), (unsigned char)(30 * blink), (unsigned char)(20 * blink), 230};
    else
        g = (Color){(unsigned char)(30 * blink), (unsigned char)(220 * blink), (unsigned char)(80 * blink), 230};
    DrawCube((Vector3){0, corrH + 0.45f, endZ - 0.12f}, 0.5f, 0.15f, 0.1f, g);

    // Wall-mounted lights inside corridor
    DrawCube((Vector3){-corrW * 0.5f + 0.07f, corrH - 0.2f, -corrLen * 0.35f}, 0.08f, 0.15f, 0.08f,
        (Color){220, 200, 140, 200});
    DrawCube((Vector3){corrW * 0.5f - 0.07f, corrH - 0.2f, -corrLen * 0.35f}, 0.08f, 0.15f, 0.08f,
        (Color){220, 200, 140, 200});
    DrawCube((Vector3){-corrW * 0.5f + 0.07f, corrH - 0.2f, -corrLen * 0.75f}, 0.08f, 0.15f, 0.08f,
        (Color){220, 200, 140, 200});
    DrawCube((Vector3){corrW * 0.5f - 0.07f, corrH - 0.2f, -corrLen * 0.75f}, 0.08f, 0.15f, 0.08f,
        (Color){220, 200, 140, 200});

    rlPopMatrix();
}

static void DrawMoonBaseExterior(Structure *s) {
    float x = s->worldPos.x;
    float y = s->worldPos.y;
    float z = s->worldPos.z;
    float r = MOONBASE_EXTERIOR_RADIUS;

    // === CYLINDER going into the ground ===
    // Visible portion rises 1.0 above terrain, then 8 units deep underground
    float cylR = r * 0.65f;
    float cylAbove = 1.0f;  // how much cylinder sticks up above terrain
    float cylBelow = 8.0f;  // how deep it goes underground
    float cylTotal = cylAbove + cylBelow;
    float cylCenterY = y + cylAbove - cylTotal * 0.5f;

    // Cylinder body (approximated with 8 rotated cubes)
    for (int i = 0; i < 8; i++) {
        float a = (float)i / 8.0f * PI;
        float cA = cosf(a);
        float sA = sinf(a);
        float w = cylR * 2.0f * fabsf(cA) + cylR * 0.7f;
        float d = cylR * 2.0f * fabsf(sA) + cylR * 0.7f;
        DrawCube((Vector3){x, cylCenterY, z}, w, cylTotal, d, EXT_CONCRETE_DK);
    }

    // Steel rim ring at the top of the cylinder
    float cylTopY = y + cylAbove;
    for (int seg = 0; seg < 20; seg++) {
        float a = (float)seg / 20.0f * 2.0f * PI;
        float px = x + cosf(a) * (cylR + 0.1f);
        float pz = z + sinf(a) * (cylR + 0.1f);
        rlPushMatrix();
        rlTranslatef(px, cylTopY, pz);
        rlRotatef(-a * 180.0f / PI + 90.0f, 0, 1, 0);
        DrawCube((Vector3){0, 0, 0}, cylR * 0.45f, 0.3f, 0.2f, EXT_STEEL);
        rlPopMatrix();
    }
    // Flat cap on cylinder top
    DrawCube((Vector3){x, cylTopY + 0.1f, z}, cylR * 2.1f, 0.2f, cylR * 2.1f, EXT_STEEL);

    // === DOME sits on TOP of cylinder ===
    float baseY = cylTopY + 0.2f; // dome base = top of cylinder cap
    float domeH = r * 0.85f;
    DrawGeodesicDome(x, baseY, z, r, domeH, s);

    // Red band around dome base
    for (int seg = 0; seg < 20; seg++) {
        float a = (float)seg / 20.0f * 2.0f * PI;
        float px = x + cosf(a) * (r + 0.03f);
        float pz = z + sinf(a) * (r + 0.03f);
        rlPushMatrix();
        rlTranslatef(px, baseY + 0.25f, pz);
        rlRotatef(-a * 180.0f / PI + 90.0f, 0, 1, 0);
        DrawCube((Vector3){0, 0, 0}, r * 0.35f, 0.15f, 0.06f, EXT_ACCENT_RED);
        rlPopMatrix();
    }

    // === AIRLOCK DOORS on top of cylinder ===
    bool expended = (s->resuppliesLeft <= 0);
    for (int d = 0; d < s->doorCount; d++) {
        DrawExteriorDoor(x, baseY, z, s->doorAngles[d], expended);
    }

    // === ANTENNA ===
    float domeTop = baseY + domeH;
    DrawCube((Vector3){x, domeTop + 1.5f, z}, 0.06f, 3.0f, 0.06f, EXT_ANTENNA);
    DrawCube((Vector3){x, domeTop + 3.2f, z}, 0.5f, 0.03f, 0.5f, EXT_STEEL);
    float blink = (sinf((float)GetTime() * 4.0f) > 0) ? 1.0f : 0.25f;
    DrawCube((Vector3){x, domeTop + 3.4f, z}, 0.1f, 0.1f, 0.1f,
        (Color){(unsigned char)(220 * blink), (unsigned char)(40 * blink), (unsigned char)(30 * blink), 220});
}

void StructureManagerDraw(StructureManager *sm, Vector3 playerPos) {
    for (int i = 0; i < sm->count; i++) {
        Structure *s = &sm->structures[i];
        if (!s->active) continue;
        float dx = playerPos.x - s->worldPos.x;
        float dz = playerPos.z - s->worldPos.z;
        if (dx * dx + dz * dz > 200.0f * 200.0f) continue;
        DrawMoonBaseExterior(s);
    }
}
