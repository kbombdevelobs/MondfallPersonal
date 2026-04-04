#include "structure_draw.h"
#include "structure_draw_furniture.h"
#include "rlgl.h"
#include <math.h>

// ============================================================================
// INTERIOR COLOR PALETTE
// ============================================================================

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
#define INT_IRON_CROSS  (Color){30, 30, 30, 255}
#define INT_PORTRAIT_FR (Color){130, 100, 50, 255}
#define INT_PORTRAIT_BG (Color){60, 55, 45, 255}
#define INT_SKIN        (Color){190, 160, 130, 255}
#define INT_SKIN_DK     (Color){150, 120, 90, 255}
#define INT_UNIFORM_GRN (Color){50, 60, 45, 255}
#define INT_UNIFORM_GRY (Color){80, 80, 75, 255}
#define INT_HAIR_DARK   (Color){40, 35, 30, 255}
#define INT_HAIR_GREY   (Color){120, 115, 110, 255}
#define INT_FLAG_RED    (Color){180, 30, 25, 255}
#define INT_FLAG_WHITE  (Color){200, 200, 195, 255}
#define INT_CLOSET_BODY (Color){45, 55, 45, 255}
#define INT_CLOSET_DOOR (Color){50, 65, 50, 255}
#define INT_CLOSET_GLOW (Color){30, 200, 80, 200}
#define INT_DOOR_FRAME  (Color){60, 48, 35, 255}
#define INT_BAR_BRASS   (Color){170, 140, 60, 255}

// Exterior colors used by doors and closet
#define EXT_DOOR_DARK   (Color){12, 12, 16, 255}
#define EXT_CONCRETE_DK (Color){100, 95, 90, 255}

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
