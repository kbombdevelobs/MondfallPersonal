#include "structure_draw.h"
#include "world.h"
#include "rlgl.h"
#include <math.h>

// ============================================================================
// COLOR PALETTE
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

// Interior — Germanic officers' lounge
#define INT_FLOOR_WOOD  (Color){65, 45, 30, 255}
#define INT_FLOOR_DARK  (Color){50, 35, 22, 255}
#define INT_FLOOR_PLANK (Color){55, 38, 25, 255}
#define INT_CARPET_RED  (Color){120, 25, 20, 255}
#define INT_CARPET_DARK (Color){90, 18, 15, 255}
#define INT_CARPET_GOLD (Color){160, 130, 50, 255}
#define INT_WALL_CREAM  (Color){140, 128, 105, 255}
#define INT_WALL_LOWER  (Color){70, 55, 40, 255}
#define INT_WALL_TRIM   (Color){110, 85, 55, 255}
#define INT_WALL_MOULD  (Color){95, 72, 45, 255}
#define INT_CEILING     (Color){120, 110, 95, 255}
#define INT_CEILING_DK  (Color){100, 90, 78, 255}
#define INT_BEAM        (Color){60, 48, 35, 255}
#define INT_CHANDELIER  (Color){180, 160, 80, 255}
#define INT_LIGHT_WARM  (Color){255, 230, 170, 120}
#define INT_COUCH_LEATH (Color){55, 30, 18, 255}
#define INT_COUCH_CUSH  (Color){70, 38, 22, 255}
#define INT_COUCH_ARM   (Color){48, 26, 15, 255}
#define INT_TABLE_WOOD  (Color){60, 42, 28, 255}
#define INT_TABLE_TOP   (Color){50, 35, 22, 255}
#define INT_BAR_WOOD    (Color){55, 38, 25, 255}
#define INT_BAR_TOP     (Color){45, 32, 20, 255}
#define INT_BAR_BRASS   (Color){170, 140, 60, 255}
#define INT_BOTTLE_GRN  (Color){30, 80, 40, 200}
#define INT_BOTTLE_AMB  (Color){140, 90, 30, 200}
#define INT_BOTTLE_CLR  (Color){180, 180, 190, 160}
#define INT_GLASS       (Color){200, 200, 210, 100}
#define INT_PORTRAIT_FR (Color){130, 100, 50, 255}
#define INT_PORTRAIT_BG (Color){60, 55, 45, 255}
#define INT_SKIN        (Color){190, 160, 130, 255}
#define INT_SKIN_DK     (Color){150, 120, 90, 255}
#define INT_UNIFORM_GRN (Color){50, 60, 45, 255}
#define INT_UNIFORM_GRY (Color){80, 80, 75, 255}
#define INT_HAIR_DARK   (Color){40, 35, 30, 255}
#define INT_HAIR_GREY   (Color){120, 115, 110, 255}
#define INT_IRON_CROSS  (Color){30, 30, 30, 255}
#define INT_FLAG_RED    (Color){180, 30, 25, 255}
#define INT_FLAG_WHITE  (Color){200, 200, 195, 255}
#define INT_CLOSET_BODY (Color){45, 55, 45, 255}
#define INT_CLOSET_DOOR (Color){50, 65, 50, 255}
#define INT_CLOSET_GLOW (Color){30, 200, 80, 200}
#define INT_DOOR_FRAME  (Color){60, 48, 35, 255}

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

// Bright yellow for hazard markings — high contrast against lunar gray
#define HAZARD_YELLOW (Color){220, 200, 40, 255}
#define HAZARD_BLACK  (Color){25, 25, 25, 255}

static void DrawExteriorDoor(float cx, float cy, float cz, float angle, bool expended) {
    // Airlock corridor — wide, bold, high-contrast
    float doorDist = MOONBASE_EXTERIOR_RADIUS * 0.95f;
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

// ============================================================================
// INTERIOR — Germanic Officers' Lounge
// ============================================================================

static void DrawInteriorFloor(float y) {
    float halfW = MOONBASE_INTERIOR_W * 0.5f;
    float halfD = MOONBASE_INTERIOR_D * 0.5f;

    // Main wood floor
    DrawCube((Vector3){0, y - 0.1f, 0}, MOONBASE_INTERIOR_W, 0.2f, MOONBASE_INTERIOR_D, INT_FLOOR_WOOD);

    // Plank lines
    for (float fx = -halfW + 1.2f; fx < halfW; fx += 1.2f) {
        DrawCube((Vector3){fx, y + 0.005f, 0}, 0.03f, 0.01f, MOONBASE_INTERIOR_D, INT_FLOOR_DARK);
    }
    // Cross planks
    for (float fz = -halfD + 3.0f; fz < halfD; fz += 6.0f) {
        DrawCube((Vector3){0, y + 0.005f, fz}, MOONBASE_INTERIOR_W, 0.01f, 0.03f, INT_FLOOR_DARK);
    }

    // Large central carpet — deep red with gold border
    float carpW = 10.0f, carpD = 8.0f;
    DrawCube((Vector3){0, y + 0.02f, 0}, carpW, 0.02f, carpD, INT_CARPET_RED);
    // Gold border trim
    float bw = 0.2f;
    DrawCube((Vector3){0, y + 0.03f, carpD * 0.5f - bw * 0.5f}, carpW, 0.01f, bw, INT_CARPET_GOLD);
    DrawCube((Vector3){0, y + 0.03f, -carpD * 0.5f + bw * 0.5f}, carpW, 0.01f, bw, INT_CARPET_GOLD);
    DrawCube((Vector3){carpW * 0.5f - bw * 0.5f, y + 0.03f, 0}, bw, 0.01f, carpD, INT_CARPET_GOLD);
    DrawCube((Vector3){-carpW * 0.5f + bw * 0.5f, y + 0.03f, 0}, bw, 0.01f, carpD, INT_CARPET_GOLD);
    // Inner darker rectangle
    DrawCube((Vector3){0, y + 0.025f, 0}, carpW - 1.5f, 0.015f, carpD - 1.5f, INT_CARPET_DARK);
    // Center diamond motif
    float dSz = 1.2f;
    rlPushMatrix();
    rlTranslatef(0, y + 0.035f, 0);
    rlRotatef(45.0f, 0, 1, 0);
    DrawCube((Vector3){0, 0, 0}, dSz, 0.01f, dSz, INT_CARPET_GOLD);
    DrawCube((Vector3){0, 0.005f, 0}, dSz * 0.6f, 0.01f, dSz * 0.6f, INT_CARPET_RED);
    rlPopMatrix();

    // Small rug in front of bar area (east side)
    DrawCube((Vector3){halfW - 4.0f, y + 0.02f, -2.0f}, 3.0f, 0.02f, 2.0f, INT_CARPET_DARK);
}

static void DrawInteriorWalls(float y) {
    float halfW = MOONBASE_INTERIOR_W * 0.5f;
    float halfD = MOONBASE_INTERIOR_D * 0.5f;
    float h = MOONBASE_INTERIOR_H;
    float thick = 0.3f;

    // Lower wainscoting (dark wood, bottom 1.2m)
    float wainH = 1.2f;
    float wainY = y + wainH * 0.5f;
    // Upper cream plaster
    float upperH = h - wainH;
    float upperY = y + wainH + upperH * 0.5f;

    // ---- NORTH WALL (back, closet here) ----
    DrawCube((Vector3){0, wainY, halfD}, MOONBASE_INTERIOR_W, wainH, thick, INT_WALL_LOWER);
    DrawCube((Vector3){0, upperY, halfD}, MOONBASE_INTERIOR_W, upperH, thick, INT_WALL_CREAM);
    // Moulding rail between wainscot and plaster
    DrawCube((Vector3){0, y + wainH, halfD - 0.01f}, MOONBASE_INTERIOR_W, 0.1f, 0.06f, INT_WALL_MOULD);

    // ---- SOUTH WALL (door 0: center opening) ----
    float doorW = 2.6f;
    float sideW = (MOONBASE_INTERIOR_W - doorW) * 0.5f;
    // Left section
    DrawCube((Vector3){-halfW + sideW * 0.5f, wainY, -halfD}, sideW, wainH, thick, INT_WALL_LOWER);
    DrawCube((Vector3){-halfW + sideW * 0.5f, upperY, -halfD}, sideW, upperH, thick, INT_WALL_CREAM);
    // Right section
    DrawCube((Vector3){halfW - sideW * 0.5f, wainY, -halfD}, sideW, wainH, thick, INT_WALL_LOWER);
    DrawCube((Vector3){halfW - sideW * 0.5f, upperY, -halfD}, sideW, upperH, thick, INT_WALL_CREAM);
    // Above door
    DrawCube((Vector3){0, y + h - 0.5f, -halfD}, doorW + 0.4f, 1.0f, thick, INT_WALL_CREAM);
    // Moulding on south
    DrawCube((Vector3){-halfW + sideW * 0.5f, y + wainH, -halfD + 0.01f}, sideW, 0.1f, 0.06f, INT_WALL_MOULD);
    DrawCube((Vector3){halfW - sideW * 0.5f, y + wainH, -halfD + 0.01f}, sideW, 0.1f, 0.06f, INT_WALL_MOULD);

    // ---- EAST WALL (door 1: opening near north end at Z=2) ----
    float edoorZ = 2.0f;
    float eBelow = edoorZ + halfD;   // wall section from -halfD to door
    float eAbove = halfD - edoorZ - doorW * 0.5f; // wall section from door to +halfD
    // South of door
    if (eBelow - doorW * 0.5f > 0.5f) {
        float secLen = eBelow - doorW * 0.5f;
        float secZ = -halfD + secLen * 0.5f;
        DrawCube((Vector3){halfW, wainY, secZ}, thick, wainH, secLen, INT_WALL_LOWER);
        DrawCube((Vector3){halfW, upperY, secZ}, thick, upperH, secLen, INT_WALL_CREAM);
        DrawCube((Vector3){halfW - 0.01f, y + wainH, secZ}, 0.06f, 0.1f, secLen, INT_WALL_MOULD);
    }
    // North of door
    if (eAbove > 0.5f) {
        float secZ = edoorZ + doorW * 0.5f + eAbove * 0.5f;
        DrawCube((Vector3){halfW, wainY, secZ}, thick, wainH, eAbove, INT_WALL_LOWER);
        DrawCube((Vector3){halfW, upperY, secZ}, thick, upperH, eAbove, INT_WALL_CREAM);
        DrawCube((Vector3){halfW - 0.01f, y + wainH, secZ}, 0.06f, 0.1f, eAbove, INT_WALL_MOULD);
    }
    // Above east door
    DrawCube((Vector3){halfW, y + h - 0.5f, edoorZ}, thick, 1.0f, doorW + 0.4f, INT_WALL_CREAM);

    // ---- WEST WALL (door 2: opening near north end at Z=2) ----
    // South of door
    if (eBelow - doorW * 0.5f > 0.5f) {
        float secLen = eBelow - doorW * 0.5f;
        float secZ = -halfD + secLen * 0.5f;
        DrawCube((Vector3){-halfW, wainY, secZ}, thick, wainH, secLen, INT_WALL_LOWER);
        DrawCube((Vector3){-halfW, upperY, secZ}, thick, upperH, secLen, INT_WALL_CREAM);
        DrawCube((Vector3){-halfW + 0.01f, y + wainH, secZ}, 0.06f, 0.1f, secLen, INT_WALL_MOULD);
    }
    // North of door
    if (eAbove > 0.5f) {
        float secZ = edoorZ + doorW * 0.5f + eAbove * 0.5f;
        DrawCube((Vector3){-halfW, wainY, secZ}, thick, wainH, eAbove, INT_WALL_LOWER);
        DrawCube((Vector3){-halfW, upperY, secZ}, thick, upperH, eAbove, INT_WALL_CREAM);
        DrawCube((Vector3){-halfW + 0.01f, y + wainH, secZ}, 0.06f, 0.1f, eAbove, INT_WALL_MOULD);
    }
    // Above west door
    DrawCube((Vector3){-halfW, y + h - 0.5f, edoorZ}, thick, 1.0f, doorW + 0.4f, INT_WALL_CREAM);

    // Crown moulding along ceiling edge (all walls)
    float crownY = y + h - 0.08f;
    DrawCube((Vector3){0, crownY, halfD - 0.01f}, MOONBASE_INTERIOR_W, 0.12f, 0.08f, INT_WALL_TRIM);
    DrawCube((Vector3){0, crownY, -halfD + 0.01f}, MOONBASE_INTERIOR_W, 0.12f, 0.08f, INT_WALL_TRIM);
    DrawCube((Vector3){halfW - 0.01f, crownY, 0}, 0.08f, 0.12f, MOONBASE_INTERIOR_D, INT_WALL_TRIM);
    DrawCube((Vector3){-halfW + 0.01f, crownY, 0}, 0.08f, 0.12f, MOONBASE_INTERIOR_D, INT_WALL_TRIM);
}

static void DrawInteriorCeiling(float y) {
    float ceilY = y + MOONBASE_INTERIOR_H;
    float halfD = MOONBASE_INTERIOR_D * 0.5f;

    // Main ceiling plaster
    DrawCube((Vector3){0, ceilY + 0.05f, 0}, MOONBASE_INTERIOR_W, 0.1f, MOONBASE_INTERIOR_D, INT_CEILING);

    // Exposed wooden beams
    for (float bz = -halfD + 4.0f; bz < halfD; bz += 5.0f) {
        DrawCube((Vector3){0, ceilY - 0.12f, bz}, MOONBASE_INTERIOR_W - 0.5f, 0.25f, 0.2f, INT_BEAM);
    }

    // Central chandelier — iron cross shape with warm light
    float chY = ceilY - 0.8f;
    // Chain
    DrawCube((Vector3){0, ceilY - 0.4f, 0}, 0.06f, 0.8f, 0.06f, INT_IRON_CROSS);
    // Cross arms
    DrawCube((Vector3){0, chY, 0}, 2.0f, 0.08f, 0.08f, INT_IRON_CROSS);
    DrawCube((Vector3){0, chY, 0}, 0.08f, 0.08f, 2.0f, INT_IRON_CROSS);
    // Candle lights at tips
    Vector3 candlePos[4] = {{1.0f, chY + 0.15f, 0}, {-1.0f, chY + 0.15f, 0},
                            {0, chY + 0.15f, 1.0f}, {0, chY + 0.15f, -1.0f}};
    for (int c = 0; c < 4; c++) {
        DrawCube(candlePos[c], 0.06f, 0.2f, 0.06f, INT_CHANDELIER);
        // Flame glow
        float flicker = 0.7f + 0.3f * sinf((float)GetTime() * 8.0f + (float)c * 1.7f);
        Color flame = {(unsigned char)(255 * flicker), (unsigned char)(200 * flicker),
                       (unsigned char)(100 * flicker), 200};
        DrawCube((Vector3){candlePos[c].x, candlePos[c].y + 0.15f, candlePos[c].z},
                 0.08f, 0.1f, 0.08f, flame);
    }
    // Warm light pool on floor
    DrawCube((Vector3){0, y + 0.04f, 0}, 4.0f, 0.01f, 4.0f, INT_LIGHT_WARM);
}

// Pixel portrait of a general — built from tiny cubes on the wall
static void DrawPixelPortrait(float px, float py, float pz, float faceDir, int seed) {
    // faceDir: 0 = on north wall (facing -Z), 1 = east wall (-X), 2 = west wall (+X), 3 = south wall (+Z)
    float fw = 1.0f;   // frame width
    float fh = 1.4f;   // frame height
    float depth = 0.08f;

    rlPushMatrix();
    rlTranslatef(px, py, pz);
    if (faceDir == 1) rlRotatef(-90.0f, 0, 1, 0);
    else if (faceDir == 2) rlRotatef(90.0f, 0, 1, 0);
    else if (faceDir == 3) rlRotatef(180.0f, 0, 1, 0);

    // Ornate frame
    DrawCube((Vector3){0, 0, 0}, fw + 0.15f, fh + 0.15f, depth, INT_PORTRAIT_FR);
    // Canvas background
    DrawCube((Vector3){0, 0, -0.01f}, fw - 0.05f, fh - 0.05f, depth * 0.5f, INT_PORTRAIT_BG);

    // Pixel art figure — head, torso, shoulders
    float ps = 0.065f; // pixel size

    // Head (oval of skin-colored pixels)
    Color skin = (seed % 2 == 0) ? INT_SKIN : INT_SKIN_DK;
    Color hair = (seed % 3 == 0) ? INT_HAIR_DARK : (seed % 3 == 1) ? INT_HAIR_GREY : INT_HAIR_DARK;
    Color uniform = (seed % 2 == 0) ? INT_UNIFORM_GRN : INT_UNIFORM_GRY;

    float headY = fh * 0.22f;
    // Hair (top of head)
    for (int hx = -2; hx <= 2; hx++) {
        DrawCube((Vector3){hx * ps, headY + ps * 3, -0.02f}, ps, ps, ps * 0.5f, hair);
        if (hx >= -1 && hx <= 1)
            DrawCube((Vector3){hx * ps, headY + ps * 4, -0.02f}, ps, ps, ps * 0.5f, hair);
    }
    // Face
    for (int hy = -1; hy <= 2; hy++) {
        int w = (hy == -1 || hy == 2) ? 2 : 3;
        for (int hx = -(w-1); hx < w; hx++) {
            DrawCube((Vector3){hx * ps, headY + hy * ps, -0.02f}, ps, ps, ps * 0.5f, skin);
        }
    }
    // Eyes (dark dots)
    DrawCube((Vector3){-ps, headY + ps, -0.025f}, ps * 0.7f, ps * 0.5f, ps * 0.3f, INT_HAIR_DARK);
    DrawCube((Vector3){ps, headY + ps, -0.025f}, ps * 0.7f, ps * 0.5f, ps * 0.3f, INT_HAIR_DARK);
    // Mustache (some have one)
    if (seed % 2 == 0) {
        DrawCube((Vector3){0, headY - ps * 0.3f, -0.025f}, ps * 2.5f, ps * 0.4f, ps * 0.3f, INT_HAIR_DARK);
    }

    // Shoulders and torso (uniform)
    float torsoY = headY - ps * 3;
    for (int ty = 0; ty < 5; ty++) {
        int w = (ty < 2) ? 5 : 4;
        for (int tx = -(w-1); tx < w; tx++) {
            DrawCube((Vector3){tx * ps, torsoY - ty * ps, -0.02f}, ps, ps, ps * 0.5f, uniform);
        }
    }

    // Shoulder boards (gold rectangles)
    DrawCube((Vector3){-ps * 3.5f, torsoY, -0.025f}, ps * 1.5f, ps * 0.6f, ps * 0.3f, INT_CARPET_GOLD);
    DrawCube((Vector3){ps * 3.5f, torsoY, -0.025f}, ps * 1.5f, ps * 0.6f, ps * 0.3f, INT_CARPET_GOLD);

    // Iron cross decoration on chest
    if (seed % 3 == 0) {
        float icY = torsoY - ps * 2;
        DrawCube((Vector3){0, icY, -0.025f}, ps * 0.4f, ps * 1.2f, ps * 0.3f, INT_IRON_CROSS);
        DrawCube((Vector3){0, icY, -0.025f}, ps * 1.2f, ps * 0.4f, ps * 0.3f, INT_IRON_CROSS);
    }

    // Medal ribbons
    float ribY = torsoY - ps * 1;
    for (int r = 0; r < 2 + (seed % 2); r++) {
        Color ribCol = (r == 0) ? INT_FLAG_RED : (r == 1) ? INT_CARPET_GOLD : INT_FLAG_WHITE;
        DrawCube((Vector3){-ps * 1.5f + r * ps * 1.2f, ribY, -0.025f},
                 ps * 0.8f, ps * 0.3f, ps * 0.3f, ribCol);
    }

    rlPopMatrix();
}

static void DrawInteriorDoors(float y) {
    float halfW = MOONBASE_INTERIOR_W * 0.5f;
    float halfD = MOONBASE_INTERIOR_D * 0.5f;
    float doorH = 2.6f;
    float doorW = 2.2f;

    // Door 0: south wall center
    DrawCube((Vector3){-doorW * 0.5f - 0.15f, y + doorH * 0.5f, -halfD + 0.12f}, 0.2f, doorH, 0.25f, INT_DOOR_FRAME);
    DrawCube((Vector3){doorW * 0.5f + 0.15f, y + doorH * 0.5f, -halfD + 0.12f}, 0.2f, doorH, 0.25f, INT_DOOR_FRAME);
    DrawCube((Vector3){0, y + doorH + 0.1f, -halfD + 0.12f}, doorW + 0.5f, 0.2f, 0.25f, INT_DOOR_FRAME);
    DrawCube((Vector3){0, y + doorH * 0.5f, -halfD + 0.05f}, doorW - 0.1f, doorH - 0.1f, 0.05f, EXT_DOOR_DARK);
    // Green exit light
    float b0 = (sinf((float)GetTime() * 3.0f) > 0) ? 1.0f : 0.4f;
    DrawCube((Vector3){0, y + doorH + 0.25f, -halfD + 0.08f}, 0.3f, 0.08f, 0.06f,
        (Color){(unsigned char)(30*b0), (unsigned char)(200*b0), (unsigned char)(80*b0), 200});

    // Door 1: east wall at Z=2
    float ez = 2.0f;
    DrawCube((Vector3){halfW - 0.12f, y + doorH * 0.5f, ez - doorW * 0.5f - 0.15f}, 0.25f, doorH, 0.2f, INT_DOOR_FRAME);
    DrawCube((Vector3){halfW - 0.12f, y + doorH * 0.5f, ez + doorW * 0.5f + 0.15f}, 0.25f, doorH, 0.2f, INT_DOOR_FRAME);
    DrawCube((Vector3){halfW - 0.12f, y + doorH + 0.1f, ez}, 0.25f, 0.2f, doorW + 0.5f, INT_DOOR_FRAME);
    DrawCube((Vector3){halfW - 0.05f, y + doorH * 0.5f, ez}, 0.05f, doorH - 0.1f, doorW - 0.1f, EXT_DOOR_DARK);
    float b1 = (sinf((float)GetTime() * 3.0f + 2.0f) > 0) ? 1.0f : 0.4f;
    DrawCube((Vector3){halfW - 0.08f, y + doorH + 0.25f, ez}, 0.06f, 0.08f, 0.3f,
        (Color){(unsigned char)(30*b1), (unsigned char)(200*b1), (unsigned char)(80*b1), 200});

    // Door 2: west wall at Z=2
    DrawCube((Vector3){-halfW + 0.12f, y + doorH * 0.5f, ez - doorW * 0.5f - 0.15f}, 0.25f, doorH, 0.2f, INT_DOOR_FRAME);
    DrawCube((Vector3){-halfW + 0.12f, y + doorH * 0.5f, ez + doorW * 0.5f + 0.15f}, 0.25f, doorH, 0.2f, INT_DOOR_FRAME);
    DrawCube((Vector3){-halfW + 0.12f, y + doorH + 0.1f, ez}, 0.25f, 0.2f, doorW + 0.5f, INT_DOOR_FRAME);
    DrawCube((Vector3){-halfW + 0.05f, y + doorH * 0.5f, ez}, 0.05f, doorH - 0.1f, doorW - 0.1f, EXT_DOOR_DARK);
    float b2 = (sinf((float)GetTime() * 3.0f + 4.0f) > 0) ? 1.0f : 0.4f;
    DrawCube((Vector3){-halfW + 0.08f, y + doorH + 0.25f, ez}, 0.06f, 0.08f, 0.3f,
        (Color){(unsigned char)(30*b2), (unsigned char)(200*b2), (unsigned char)(80*b2), 200});
}

static void DrawPortraits(float y) {
    float halfW = MOONBASE_INTERIOR_W * 0.5f;
    float halfD = MOONBASE_INTERIOR_D * 0.5f;
    float portraitY = y + 3.2f;

    // North wall — 3 portraits (generals)
    DrawPixelPortrait(-4.0f, portraitY, halfD - 0.16f, 0, 0);
    DrawPixelPortrait(0.0f, portraitY + 0.2f, halfD - 0.16f, 0, 1);  // center slightly higher = place of honor
    DrawPixelPortrait(4.0f, portraitY, halfD - 0.16f, 0, 2);

    // East wall — 2 portraits (between furniture)
    DrawPixelPortrait(halfW - 0.16f, portraitY, -4.0f, 1, 3);
    DrawPixelPortrait(halfW - 0.16f, portraitY, 6.0f, 1, 4);

    // West wall — 2 portraits
    DrawPixelPortrait(-halfW + 0.16f, portraitY, -4.0f, 2, 5);
    DrawPixelPortrait(-halfW + 0.16f, portraitY, 6.0f, 2, 6);

    // South wall — 1 portrait above door (large, commanding)
    DrawPixelPortrait(0.0f, y + MOONBASE_INTERIOR_H - 1.2f, -halfD + 0.16f, 3, 7);
}

static void DrawCouch(float cx, float cy, float cz, float rotY) {
    rlPushMatrix();
    rlTranslatef(cx, cy, cz);
    rlRotatef(rotY, 0, 1, 0);

    // Base/frame
    DrawCube((Vector3){0, 0.15f, 0}, 3.0f, 0.3f, 1.0f, INT_COUCH_ARM);
    // Seat cushion
    DrawCube((Vector3){0, 0.4f, 0.05f}, 2.8f, 0.2f, 0.8f, INT_COUCH_CUSH);
    // Backrest
    DrawCube((Vector3){0, 0.7f, -0.4f}, 2.8f, 0.6f, 0.2f, INT_COUCH_LEATH);
    // Arms
    DrawCube((Vector3){-1.4f, 0.45f, 0}, 0.2f, 0.4f, 1.0f, INT_COUCH_ARM);
    DrawCube((Vector3){1.4f, 0.45f, 0}, 0.2f, 0.4f, 1.0f, INT_COUCH_ARM);
    // Arm tops (slightly wider, polished)
    DrawCube((Vector3){-1.4f, 0.7f, 0}, 0.25f, 0.06f, 1.0f, INT_TABLE_WOOD);
    DrawCube((Vector3){1.4f, 0.7f, 0}, 0.25f, 0.06f, 1.0f, INT_TABLE_WOOD);
    // Button tufting on backrest (row of small bumps)
    for (int b = -3; b <= 3; b++) {
        DrawCube((Vector3){b * 0.35f, 0.7f, -0.32f}, 0.08f, 0.08f, 0.04f, INT_COUCH_ARM);
    }

    rlPopMatrix();
}

static void DrawCoffeeTable(float cx, float cy, float cz) {
    // Legs
    float legH = 0.35f;
    DrawCube((Vector3){cx - 0.5f, cy + legH * 0.5f, cz - 0.3f}, 0.08f, legH, 0.08f, INT_TABLE_WOOD);
    DrawCube((Vector3){cx + 0.5f, cy + legH * 0.5f, cz - 0.3f}, 0.08f, legH, 0.08f, INT_TABLE_WOOD);
    DrawCube((Vector3){cx - 0.5f, cy + legH * 0.5f, cz + 0.3f}, 0.08f, legH, 0.08f, INT_TABLE_WOOD);
    DrawCube((Vector3){cx + 0.5f, cy + legH * 0.5f, cz + 0.3f}, 0.08f, legH, 0.08f, INT_TABLE_WOOD);
    // Top
    DrawCube((Vector3){cx, cy + legH + 0.04f, cz}, 1.4f, 0.08f, 0.8f, INT_TABLE_TOP);
}

static void DrawBar(float y) {
    float halfW = MOONBASE_INTERIOR_W * 0.5f;
    // Bar along east wall, south section
    float barX = halfW - 1.5f;
    float barZ = -5.0f;
    float barLen = 6.0f;

    // Bar counter body
    DrawCube((Vector3){barX, y + 0.55f, barZ}, 1.8f, 1.1f, barLen, INT_BAR_WOOD);
    // Bar top (polished dark surface)
    DrawCube((Vector3){barX, y + 1.15f, barZ}, 2.0f, 0.1f, barLen + 0.2f, INT_BAR_TOP);
    // Brass rail along front edge
    DrawCube((Vector3){barX - 0.9f, y + 0.85f, barZ}, 0.06f, 0.06f, barLen - 0.2f, INT_BAR_BRASS);
    // Foot rail
    DrawCube((Vector3){barX - 0.7f, y + 0.15f, barZ}, 0.05f, 0.05f, barLen - 0.4f, INT_BAR_BRASS);

    // Back shelves (against wall)
    DrawCube((Vector3){halfW - 0.4f, y + 1.8f, barZ}, 0.6f, 1.2f, barLen - 0.5f, INT_BAR_WOOD);
    // Shelf dividers
    for (float sz = barZ - barLen * 0.5f + 1.0f; sz < barZ + barLen * 0.5f; sz += 1.5f) {
        DrawCube((Vector3){halfW - 0.4f, y + 1.8f, sz}, 0.55f, 0.04f, 0.04f, INT_BAR_TOP);
    }

    // Bottles on shelves
    float shelfY = y + 1.35f;
    int bottleIdx = 0;
    for (float bz = barZ - barLen * 0.4f; bz < barZ + barLen * 0.4f; bz += 0.5f) {
        Color bottleCol;
        float bh;
        switch (bottleIdx % 4) {
            case 0: bottleCol = INT_BOTTLE_GRN; bh = 0.5f; break;
            case 1: bottleCol = INT_BOTTLE_AMB; bh = 0.45f; break;
            case 2: bottleCol = INT_BOTTLE_CLR; bh = 0.4f; break;
            default: bottleCol = INT_BOTTLE_GRN; bh = 0.48f; break;
        }
        DrawCube((Vector3){halfW - 0.4f, shelfY + bh * 0.5f, bz}, 0.12f, bh, 0.12f, bottleCol);
        // Neck
        DrawCube((Vector3){halfW - 0.4f, shelfY + bh + 0.08f, bz}, 0.05f, 0.15f, 0.05f, bottleCol);
        bottleIdx++;

        // Second row higher
        if (bottleIdx % 3 == 0) {
            Color col2 = INT_BOTTLE_AMB;
            DrawCube((Vector3){halfW - 0.4f, shelfY + 0.7f + 0.2f, bz + 0.2f}, 0.11f, 0.4f, 0.11f, col2);
        }
    }

    // Glasses on bar counter
    for (float gz = barZ - 1.5f; gz < barZ + 1.5f; gz += 1.8f) {
        // Tumbler glass
        DrawCube((Vector3){barX - 0.3f, y + 1.2f + 0.08f, gz}, 0.1f, 0.15f, 0.1f, INT_GLASS);
    }

    // Bar stools (2)
    for (int st = 0; st < 2; st++) {
        float stZ = barZ + (st == 0 ? -1.5f : 1.5f);
        float stX = barX - 1.3f;
        // Legs
        DrawCube((Vector3){stX, y + 0.35f, stZ}, 0.06f, 0.7f, 0.06f, INT_IRON_CROSS);
        // Seat
        DrawCube((Vector3){stX, y + 0.75f, stZ}, 0.5f, 0.08f, 0.5f, INT_COUCH_LEATH);
        // Foot ring
        DrawCube((Vector3){stX, y + 0.25f, stZ}, 0.4f, 0.04f, 0.4f, INT_BAR_BRASS);
    }
}

static void DrawResupplyCloset(float y, float flashTimer, bool expended) {
    float halfD = MOONBASE_INTERIOR_D * 0.5f;
    float closetZ = halfD - 1.0f;
    float closetY = y + 1.5f;

    // Cabinet body — military green, or dark grey if expended
    Color bodyCol = expended ? (Color){55, 50, 48, 255} : INT_CLOSET_BODY;
    Color doorCol = expended ? (Color){60, 55, 52, 255} : INT_CLOSET_DOOR;
    DrawCube((Vector3){0, closetY, closetZ}, 2.4f, 2.8f, 1.2f, bodyCol);
    DrawCube((Vector3){-0.55f, closetY, closetZ - 0.55f}, 1.0f, 2.6f, 0.1f, doorCol);
    DrawCube((Vector3){0.55f, closetY, closetZ - 0.55f}, 1.0f, 2.6f, 0.1f, doorCol);
    DrawCube((Vector3){0, closetY, closetZ - 0.56f}, 0.04f, 2.6f, 0.02f, (Color){30, 30, 30, 255});
    DrawCube((Vector3){-0.15f, closetY, closetZ - 0.62f}, 0.06f, 0.3f, 0.06f, INT_BAR_BRASS);
    DrawCube((Vector3){0.15f, closetY, closetZ - 0.62f}, 0.06f, 0.3f, 0.06f, INT_BAR_BRASS);

    // Glowing panel — green if stocked, dead red if expended
    if (expended) {
        // Dead panel — dim red, no pulse
        DrawCube((Vector3){0, closetY + 1.6f, closetZ - 0.5f}, 1.8f, 0.3f, 0.08f, (Color){80, 20, 15, 150});
    } else {
        float pulse = 0.6f + 0.4f * sinf((float)GetTime() * 2.5f);
        Color glowCol = flashTimer > 0 ?
            (Color){200, 255, 200, 240} :
            (Color){(unsigned char)(30 * pulse), (unsigned char)(200 * pulse), (unsigned char)(80 * pulse), 200};
        DrawCube((Vector3){0, closetY + 1.6f, closetZ - 0.5f}, 1.8f, 0.3f, 0.08f, glowCol);
    }

    // Status lights — green or red
    Color statusCol = expended ? (Color){180, 30, 20, 200} : INT_CLOSET_GLOW;
    DrawCube((Vector3){-1.3f, closetY + 0.8f, closetZ - 0.5f}, 0.1f, 0.1f, 0.1f, statusCol);
    DrawCube((Vector3){1.3f, closetY + 0.8f, closetZ - 0.5f}, 0.1f, 0.1f, 0.1f, statusCol);
    // VERSORGUNG label plaque
    DrawCube((Vector3){0, closetY + 1.95f, closetZ - 0.45f}, 1.4f, 0.2f, 0.04f, EXT_CONCRETE_DK);
}

static void DrawBannerFlag(float px, float py, float pz, float rotY) {
    // Hanging banner — red with white circle motif
    rlPushMatrix();
    rlTranslatef(px, py, pz);
    rlRotatef(rotY, 0, 1, 0);

    // Banner fabric
    DrawCube((Vector3){0, 0, 0}, 0.8f, 2.0f, 0.04f, INT_FLAG_RED);
    // White circle in center
    DrawCube((Vector3){0, 0, -0.01f}, 0.5f, 0.5f, 0.02f, INT_FLAG_WHITE);
    // Horizontal rod at top
    DrawCube((Vector3){0, 1.05f, 0}, 1.0f, 0.05f, 0.05f, INT_BAR_BRASS);
    // Finials
    DrawCube((Vector3){-0.5f, 1.05f, 0}, 0.08f, 0.08f, 0.08f, INT_BAR_BRASS);
    DrawCube((Vector3){0.5f, 1.05f, 0}, 0.08f, 0.08f, 0.08f, INT_BAR_BRASS);
    // Fringe at bottom
    for (int f = -3; f <= 3; f++) {
        DrawCube((Vector3){f * 0.1f, -1.05f, 0}, 0.03f, 0.1f, 0.02f, INT_CARPET_GOLD);
    }

    rlPopMatrix();
}

static void DrawFurnitureAndDecor(float y) {
    float halfW = MOONBASE_INTERIOR_W * 0.5f;
    float halfD = MOONBASE_INTERIOR_D * 0.5f;

    // === SEATING AREA (center-west) ===
    // Two couches facing each other with coffee table between
    DrawCouch(-3.5f, y, -1.0f, 0.0f);     // facing south
    DrawCouch(-3.5f, y, 2.5f, 180.0f);    // facing north

    // Coffee table between them
    DrawCoffeeTable(-3.5f, y, 0.75f);

    // Side table next to west couch
    DrawCube((Vector3){-6.0f, y + 0.3f, 0.75f}, 0.5f, 0.6f, 0.5f, INT_TABLE_WOOD);
    DrawCube((Vector3){-6.0f, y + 0.62f, 0.75f}, 0.55f, 0.04f, 0.55f, INT_TABLE_TOP);
    // Ashtray on side table
    DrawCube((Vector3){-6.0f, y + 0.68f, 0.75f}, 0.15f, 0.04f, 0.15f, INT_GLASS);

    // === BAR (east wall, south section) ===
    DrawBar(y);

    // === MAP TABLE (south-west area) ===
    float mtX = -halfW + 3.5f;
    float mtZ = -halfD + 3.0f;
    // Heavy oak table
    DrawCube((Vector3){mtX - 0.8f, y + 0.35f, mtZ - 0.6f}, 0.12f, 0.7f, 0.12f, INT_TABLE_WOOD);
    DrawCube((Vector3){mtX + 0.8f, y + 0.35f, mtZ - 0.6f}, 0.12f, 0.7f, 0.12f, INT_TABLE_WOOD);
    DrawCube((Vector3){mtX - 0.8f, y + 0.35f, mtZ + 0.6f}, 0.12f, 0.7f, 0.12f, INT_TABLE_WOOD);
    DrawCube((Vector3){mtX + 0.8f, y + 0.35f, mtZ + 0.6f}, 0.12f, 0.7f, 0.12f, INT_TABLE_WOOD);
    DrawCube((Vector3){mtX, y + 0.74f, mtZ}, 2.0f, 0.08f, 1.5f, INT_TABLE_TOP);
    // Map on table (cream colored paper with faint grid)
    DrawCube((Vector3){mtX, y + 0.79f, mtZ}, 1.6f, 0.01f, 1.2f, (Color){200, 190, 170, 255});
    DrawCube((Vector3){mtX, y + 0.795f, mtZ}, 0.8f, 0.005f, 0.005f, (Color){160, 80, 80, 150});
    DrawCube((Vector3){mtX, y + 0.795f, mtZ}, 0.005f, 0.005f, 0.6f, (Color){160, 80, 80, 150});

    // Bookshelf along west wall (north section)
    float bsX = -halfW + 0.5f;
    float bsZ = 6.0f;
    DrawCube((Vector3){bsX, y + 1.2f, bsZ}, 0.8f, 2.4f, 2.5f, INT_TABLE_WOOD);
    // Shelf boards
    for (float sy = 0.6f; sy < 2.4f; sy += 0.6f) {
        DrawCube((Vector3){bsX, y + sy, bsZ}, 0.75f, 0.04f, 2.4f, INT_TABLE_TOP);
    }
    // Books (colored spines)
    Color bookCols[] = {INT_FLAG_RED, INT_UNIFORM_GRN, (Color){40,40,80,255}, INT_CARPET_GOLD,
                        (Color){80,30,30,255}, INT_UNIFORM_GRY};
    int bi = 0;
    for (float sy = 0.1f; sy < 2.2f; sy += 0.6f) {
        for (float bz = bsZ - 1.0f; bz < bsZ + 1.0f; bz += 0.18f) {
            float bh = 0.35f + (bi % 3) * 0.08f;
            DrawCube((Vector3){bsX - 0.15f, y + sy + bh * 0.5f, bz}, 0.12f, bh, 0.14f, bookCols[bi % 6]);
            bi++;
        }
    }

    // === BANNERS ===
    DrawBannerFlag(-2.0f, y + 3.8f, halfD - 0.2f, 0.0f);    // north wall, left of center portrait
    DrawBannerFlag(2.0f, y + 3.8f, halfD - 0.2f, 0.0f);     // north wall, right
    DrawBannerFlag(halfW - 0.2f, y + 3.8f, -1.0f, 90.0f);   // east wall

    // === PIPES along ceiling-wall junction (industrial touch) ===
    DrawCube((Vector3){0, y + MOONBASE_INTERIOR_H - 0.2f, halfD - 0.3f},
             MOONBASE_INTERIOR_W - 1.0f, 0.1f, 0.1f, EXT_ANTENNA);
    DrawCube((Vector3){0, y + MOONBASE_INTERIOR_H - 0.2f, -halfD + 0.3f},
             MOONBASE_INTERIOR_W - 1.0f, 0.08f, 0.08f, EXT_STEEL_LT);
}

void StructureManagerDrawInterior(StructureManager *sm) {
    if (sm->insideIndex < 0) return;
    Structure *s = &sm->structures[sm->insideIndex];
    float y = s->interiorY;

    DrawInteriorFloor(y);
    DrawInteriorWalls(y);
    DrawInteriorCeiling(y);
    DrawInteriorDoors(y);
    DrawPortraits(y);
    DrawResupplyCloset(y, sm->resupplyFlashTimer, s->resuppliesLeft <= 0);
    DrawFurnitureAndDecor(y);
}
