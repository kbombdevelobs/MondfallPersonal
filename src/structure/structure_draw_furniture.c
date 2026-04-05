#include "structure_draw_furniture.h"
#include "rlgl.h"
#include <math.h>

// ============================================================================
// INTERIOR COLOR PALETTE (furniture subset)
// ============================================================================

#define INT_FLOOR_WOOD  (Color){130, 95, 65, 255}
#define INT_CARPET_RED  (Color){200, 55, 40, 255}
#define INT_CARPET_GOLD (Color){240, 200, 90, 255}
#define INT_COUCH_LEATH (Color){115, 70, 40, 255}
#define INT_COUCH_CUSH  (Color){135, 85, 50, 255}
#define INT_COUCH_ARM   (Color){100, 60, 35, 255}
#define INT_TABLE_WOOD  (Color){120, 90, 60, 255}
#define INT_TABLE_TOP   (Color){105, 78, 50, 255}
#define INT_BAR_WOOD    (Color){115, 82, 55, 255}
#define INT_BAR_TOP     (Color){95, 70, 45, 255}
#define INT_BAR_BRASS   (Color){230, 200, 100, 255}
#define INT_BOTTLE_GRN  (Color){60, 150, 80, 220}
#define INT_BOTTLE_AMB  (Color){210, 150, 60, 220}
#define INT_BOTTLE_CLR  (Color){220, 220, 230, 180}
#define INT_GLASS       (Color){230, 230, 240, 130}
#define INT_IRON_CROSS  (Color){60, 60, 60, 255}
#define INT_FLAG_RED    (Color){230, 55, 40, 255}
#define INT_FLAG_WHITE  (Color){240, 240, 235, 255}
#define INT_UNIFORM_GRN (Color){100, 120, 90, 255}
#define INT_UNIFORM_GRY (Color){140, 140, 130, 255}

// Exterior colors needed for pipes
#define EXT_ANTENNA     (Color){120, 118, 112, 255}
#define EXT_STEEL_LT    (Color){110, 112, 115, 255}

// ============================================================================
// FURNITURE — Couches, Tables, Bar, Bookshelves, Banners
// ============================================================================

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

void DrawFurnitureAndDecor(float y) {
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
