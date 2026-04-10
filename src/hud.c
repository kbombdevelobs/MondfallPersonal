#include "hud.h"
#include "structure/structure.h"
#include "rlgl.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

// ---- Helper: draw a single rivet (small brass circle) ----
static void DrawRivet(int x, int y, float s) {
    int r = (int)(2.5f * s);
    if (r < 1) r = 1;
    DrawCircle(x, y, (float)r, (Color){140, 125, 95, 255});
    DrawCircle(x, y, (float)(r - 1 > 0 ? r - 1 : 1), (Color){100, 90, 70, 255});
}

// ---- Helper: draw a stylized iron cross ----
static void DrawIronCross(int cx, int cy, int size, Color col) {
    // Four arms, each wider at the tip than the center
    int armW = size / 3;       // width at center
    int tipW = size / 2;       // width at tip
    int armLen = size;         // length from center to tip
    if (armW < 1) armW = 1;
    if (tipW < 2) tipW = 2;

    // Right arm
    DrawTriangle(
        (Vector2){(float)(cx + armW / 2), (float)(cy - armW / 2)},
        (Vector2){(float)(cx + armLen), (float)(cy - tipW / 2)},
        (Vector2){(float)(cx + armLen), (float)(cy + tipW / 2)},
        col);
    DrawTriangle(
        (Vector2){(float)(cx + armW / 2), (float)(cy - armW / 2)},
        (Vector2){(float)(cx + armLen), (float)(cy + tipW / 2)},
        (Vector2){(float)(cx + armW / 2), (float)(cy + armW / 2)},
        col);
    // Left arm
    DrawTriangle(
        (Vector2){(float)(cx - armW / 2), (float)(cy + armW / 2)},
        (Vector2){(float)(cx - armLen), (float)(cy + tipW / 2)},
        (Vector2){(float)(cx - armLen), (float)(cy - tipW / 2)},
        col);
    DrawTriangle(
        (Vector2){(float)(cx - armW / 2), (float)(cy + armW / 2)},
        (Vector2){(float)(cx - armLen), (float)(cy - tipW / 2)},
        (Vector2){(float)(cx - armW / 2), (float)(cy - armW / 2)},
        col);
    // Top arm
    DrawTriangle(
        (Vector2){(float)(cx + armW / 2), (float)(cy - armW / 2)},
        (Vector2){(float)(cx - armW / 2), (float)(cy - armW / 2)},
        (Vector2){(float)(cx - tipW / 2), (float)(cy - armLen)},
        col);
    DrawTriangle(
        (Vector2){(float)(cx + armW / 2), (float)(cy - armW / 2)},
        (Vector2){(float)(cx - tipW / 2), (float)(cy - armLen)},
        (Vector2){(float)(cx + tipW / 2), (float)(cy - armLen)},
        col);
    // Bottom arm
    DrawTriangle(
        (Vector2){(float)(cx - armW / 2), (float)(cy + armW / 2)},
        (Vector2){(float)(cx + armW / 2), (float)(cy + armW / 2)},
        (Vector2){(float)(cx + tipW / 2), (float)(cy + armLen)},
        col);
    DrawTriangle(
        (Vector2){(float)(cx - armW / 2), (float)(cy + armW / 2)},
        (Vector2){(float)(cx + tipW / 2), (float)(cy + armLen)},
        (Vector2){(float)(cx - tipW / 2), (float)(cy + armLen)},
        col);
    // Center square
    DrawRectangle(cx - armW / 2, cy - armW / 2, armW, armW, col);
}

// ---- Helper: draw spread-wing eagle (wings only, clean V shape) ----
static void DrawEagle(int cx, int cy, int size, Color col) {
    Color dark = {(unsigned char)(col.r * 0.5f), (unsigned char)(col.g * 0.5f),
                  (unsigned char)(col.b * 0.5f), col.a};

    float S = (float)size;
    float X = (float)cx, Y = (float)cy;

    // Simple clean swept wings — two thick lines angled upward from center
    float wingSpan = S * 1.8f;
    float wingRise = S * 0.5f;
    float thick = S * 0.14f;
    if (thick < 2.0f) thick = 2.0f;

    // Left wing — thick swept line
    DrawLineEx((Vector2){X, Y}, (Vector2){X - wingSpan, Y - wingRise}, thick, col);
    // Thinner trailing edge underneath
    DrawLineEx((Vector2){X, Y + thick * 0.4f},
               (Vector2){X - wingSpan * 0.85f, Y - wingRise + thick * 1.2f},
               thick * 0.5f, dark);

    // Right wing
    DrawLineEx((Vector2){X, Y}, (Vector2){X + wingSpan, Y - wingRise}, thick, col);
    DrawLineEx((Vector2){X, Y + thick * 0.4f},
               (Vector2){X + wingSpan * 0.85f, Y - wingRise + thick * 1.2f},
               thick * 0.5f, dark);

    // Center body dot
    DrawCircle(cx, cy, thick * 0.6f, col);
}

// ---- Helper: art deco framed bar with rivets and corner cuts ----
static void DrawDecoBar(int x, int y, int w, int h, float s) {
    int cut = (int)(8 * s); // corner cut size
    Color bg = (Color){0, 0, 0, 190};
    Color border = (Color)COLOR_BRASS_DIM;

    // Background
    DrawRectangle(x, y, w, h, bg);

    // Border lines (top, bottom, left, right) with brass
    DrawLine(x + cut, y, x + w - cut, y, border);           // top
    DrawLine(x + cut, y + h, x + w - cut, y + h, border);   // bottom
    DrawLine(x, y + cut, x, y + h - cut, border);           // left
    DrawLine(x + w, y + cut, x + w, y + h - cut, border);   // right

    // 45-degree corner cuts
    DrawLine(x, y + cut, x + cut, y, border);                       // top-left
    DrawLine(x + w - cut, y, x + w, y + cut, border);               // top-right
    DrawLine(x, y + h - cut, x + cut, y + h, border);               // bottom-left
    DrawLine(x + w - cut, y + h, x + w, y + h - cut, border);       // bottom-right

    // Rivets at midpoints of edges
    DrawRivet(x + w / 2, y, s);
    DrawRivet(x + w / 2, y + h, s);
    DrawRivet(x, y + h / 2, s);
    DrawRivet(x + w, y + h / 2, s);

    // Rivets near corners (just inside the cuts)
    DrawRivet(x + cut + (int)(3 * s), y + (int)(2 * s), s);
    DrawRivet(x + w - cut - (int)(3 * s), y + (int)(2 * s), s);
    DrawRivet(x + cut + (int)(3 * s), y + h - (int)(2 * s), s);
    DrawRivet(x + w - cut - (int)(3 * s), y + h - (int)(2 * s), s);
}

// ---- Helper: analog gauge dial (semicircular, brass housing — matches Mondfall) ----
static void DrawAnalogGauge(HudState *state, Player *player, int cx, int cy, int radius, float s) {
    float hpPct = (player->maxHealth > 0.001f) ? (player->health / player->maxHealth) : 0.0f;
    float t = (float)GetTime();
    (void)state;

    // Upside-down semicircle: arches upward, flat bottom
    float arcStart = 180.0f;
    float arcTotal = 180.0f;

    // Outer brass housing — top semicircle (arches upward, flat bottom at bar edge)
    int housingExtra = (int)(4 * s);
    DrawCircleSector((Vector2){(float)cx, (float)cy}, (float)(radius + housingExtra),
        arcStart, arcStart + arcTotal, 36, (Color)COLOR_DARK_METAL);
    // Brass base bar across the bottom
    int baseH = (int)(6 * s);
    int baseW = (radius + housingExtra) * 2 + (int)(6 * s);
    int baseX = cx - baseW / 2;
    int baseY = cy - (int)(1 * s);
    DrawRectangle(baseX, baseY, baseW, baseH, (Color){55, 50, 40, 245});
    DrawRectangle(baseX + 1, baseY + 1, baseW - 2, (int)(2 * s), (Color){80, 72, 55, 100});
    DrawLine(baseX, baseY, baseX + baseW, baseY, (Color)COLOR_BRASS);
    DrawLine(baseX, baseY + baseH, baseX + baseW, baseY + baseH, (Color)COLOR_BRASS_DIM);
    // Rivets on the base bar ends
    DrawRivet(baseX + (int)(3 * s), baseY + baseH / 2, s * 0.5f);
    DrawRivet(baseX + baseW - (int)(3 * s), baseY + baseH / 2, s * 0.5f);

    // Brass arc outline (bottom half only)
    for (int i = 0; i <= 36; i++) {
        float a1 = (arcStart + arcTotal * (float)i / 36.0f) * DEG2RAD;
        float a2 = (arcStart + arcTotal * (float)(i + 1) / 36.0f) * DEG2RAD;
        float rr = radius + housingExtra;
        DrawLine(cx + (int)(rr * cosf(a1)), cy + (int)(rr * sinf(a1)),
                 cx + (int)(rr * cosf(a2)), cy + (int)(rr * sinf(a2)), (Color)COLOR_BRASS_DIM);
    }

    // Gear-tooth decorations around bottom semicircle only
    int numTeeth = 16;
    for (int i = 0; i < numTeeth; i++) {
        float frac = (float)i / (float)(numTeeth - 1);
        float angle = (arcStart + arcTotal * frac) * DEG2RAD;
        float toothR = radius + 5.0f * s;
        float toothLen = 2.5f * s;
        int tx1 = cx + (int)(toothR * cosf(angle));
        int ty1 = cy + (int)(toothR * sinf(angle));
        int tx2 = cx + (int)((toothR + toothLen) * cosf(angle));
        int ty2 = cy + (int)((toothR + toothLen) * sinf(angle));
        DrawLine(tx1, ty1, tx2, ty2, (Color){100, 88, 60, 160});
    }

    // Background arc — dark gauge face
    DrawCircleSector((Vector2){(float)cx, (float)cy}, (float)radius,
        arcStart, arcStart + arcTotal, 48, (Color)COLOR_GAUGE_BG);

    // Color zones — BRIGHTER, more saturated for visibility
    Color zoneRed = {220, 50, 30, 220};
    Color zoneYellow = {230, 200, 50, 200};
    Color zoneGreen = {80, 200, 80, 200};

    float outerFrac = 0.96f;
    float innerFrac = 0.62f;

    // Draw zones as thick arcs (outer ring only)
    DrawCircleSector((Vector2){(float)cx, (float)cy}, (float)(radius * outerFrac),
        arcStart, arcStart + arcTotal * 0.25f, 16, zoneRed);
    DrawCircleSector((Vector2){(float)cx, (float)cy}, (float)(radius * outerFrac),
        arcStart + arcTotal * 0.25f, arcStart + arcTotal * 0.5f, 16, zoneYellow);
    DrawCircleSector((Vector2){(float)cx, (float)cy}, (float)(radius * outerFrac),
        arcStart + arcTotal * 0.5f, arcStart + arcTotal, 24, zoneGreen);

    // Inner dark disc to create the ring effect (bottom semicircle only)
    DrawCircleSector((Vector2){(float)cx, (float)cy}, (float)(radius * innerFrac),
        arcStart, arcStart + arcTotal, 24, (Color)COLOR_GAUGE_BG);
    // Flat bottom fill for inner disc
    DrawRectangle(cx - (int)(radius * innerFrac), cy - 1,
        (int)(radius * innerFrac * 2), 2, (Color)COLOR_GAUGE_BG);

    // Inner brass arc (bottom half)
    for (int j = 0; j <= 24; j++) {
        float a1 = (arcStart + arcTotal * (float)j / 24.0f) * DEG2RAD;
        float a2 = (arcStart + arcTotal * (float)(j + 1) / 24.0f) * DEG2RAD;
        float ir = radius * innerFrac;
        DrawLine(cx + (int)(ir * cosf(a1)), cy + (int)(ir * sinf(a1)),
                 cx + (int)(ir * cosf(a2)), cy + (int)(ir * sinf(a2)),
                 (Color){120, 100, 60, 160});
    }

    // Tick marks at 10% intervals — thicker, brighter
    for (int i = 0; i <= 10; i++) {
        float frac = (float)i / 10.0f;
        float angle = (arcStart + arcTotal * frac) * DEG2RAD;
        float tickInnerR = radius * (innerFrac + 0.03f);
        float tickOuterR = radius * (outerFrac - 0.02f);
        int x1 = cx + (int)(tickInnerR * cosf(angle));
        int y1 = cy + (int)(tickInnerR * sinf(angle));
        int x2 = cx + (int)(tickOuterR * cosf(angle));
        int y2 = cy + (int)(tickOuterR * sinf(angle));
        Color mc = (Color)COLOR_BRASS_BRIGHT;
        if (i % 5 == 0) {
            DrawLineEx((Vector2){(float)x1, (float)y1}, (Vector2){(float)x2, (float)y2},
                2.5f * s, mc);
        } else {
            DrawLineEx((Vector2){(float)x1, (float)y1}, (Vector2){(float)x2, (float)y2},
                1.0f * s, (Color)COLOR_GAUGE_MARK);
        }
    }

    // Needle — thicker, with shadow for depth
    float needleAngle = (arcStart + arcTotal * hpPct) * DEG2RAD;
    float needleLen = radius * 0.88f;
    int nx = cx + (int)(needleLen * cosf(needleAngle));
    int ny = cy + (int)(needleLen * sinf(needleAngle));

    // Shadow
    DrawLineEx((Vector2){(float)(cx + 1), (float)(cy + 1)}, (Vector2){(float)(nx + 1), (float)(ny + 1)},
        3.5f * s, (Color){0, 0, 0, 120});

    // Needle body
    Color needleCol = (Color)COLOR_GAUGE_NEEDLE;
    if (hpPct < 0.25f) {
        float pulse = 0.5f + 0.5f * sinf(t * 6.0f);
        needleCol = (Color){(unsigned char)(220 + 35 * pulse), (unsigned char)(40 * pulse),
                           (unsigned char)(20 * pulse), 255};
    }
    DrawLineEx((Vector2){(float)cx, (float)cy}, (Vector2){(float)nx, (float)ny},
        2.5f * s, needleCol);

    // Needle tip highlight
    DrawCircle(nx, ny, 1.5f * s, needleCol);

    // Pivot — brass bolt with highlight
    DrawCircle(cx, cy, 4.0f * s, (Color){50, 45, 35, 255});
    DrawCircle(cx, cy, 3.0f * s, (Color)COLOR_BRASS_DIM);
    DrawCircle(cx, cy, 1.5f * s, (Color){70, 60, 45, 255});
    // Specular highlight on pivot
    DrawCircle(cx - (int)(1 * s), cy - (int)(1 * s), 1.0f * s, (Color){200, 180, 120, 120});

    // HP percentage text — centered in the lower semicircle area
    {
        char hpBuf[8];
        snprintf(hpBuf, sizeof(hpBuf), "%d%%", (int)(hpPct * 100));
        int hpFs = (int)(radius * 0.30f);
        if (hpFs < 8) hpFs = 8;
        int hpW = MeasureText(hpBuf, hpFs);
        Color hpCol;
        if (hpPct > 0.5f) hpCol = (Color){120, 220, 120, 240};
        else if (hpPct > 0.25f) hpCol = (Color){220, 200, 80, 240};
        else {
            float pulse = 0.5f + 0.5f * sinf(t * 8.0f);
            hpCol = (Color){(unsigned char)(220 + 35 * pulse), (unsigned char)(50 * pulse),
                           (unsigned char)(30 * pulse), 255};
        }
        DrawText(hpBuf, cx - hpW / 2, cy - (int)(radius * 0.50f), hpFs, hpCol);
    }

    // Rivets at the ends of the flat bottom edge
    DrawRivet(cx + radius + housingExtra - (int)(2 * s), cy, s * 0.7f);
    DrawRivet(cx - radius - housingExtra + (int)(2 * s), cy, s * 0.7f);
    // Rivet at top of semicircle arc
    DrawRivet(cx, cy - radius - housingExtra, s * 0.7f);
}

// ---- He-3 Fuel Gauge — horizontal glass tube with cyan fill (from Mondfall) ----
static void DrawHe3Gauge(Player *player, int cx, int cy, int gaugeW, float s) {
    if (!player) return;
    float fuelPct = player->he3Fuel / HE3_MAX_FUEL;
    if (fuelPct < 0.0f) fuelPct = 0.0f;
    if (fuelPct > 1.0f) fuelPct = 1.0f;
    float t = (float)GetTime();

    int tubeW = gaugeW;
    int tubeH = (int)(8 * s);
    int tubeX = cx - tubeW / 2;
    int tubeY = cy - tubeH / 2;

    // Housing frame — dark metal with brass border
    int frameP = (int)(3 * s);
    DrawRectangle(tubeX - frameP, tubeY - frameP, tubeW + frameP * 2, tubeH + frameP * 2, (Color)COLOR_DARK_METAL);
    DrawRectangleLinesEx((Rectangle){(float)(tubeX - frameP), (float)(tubeY - frameP),
        (float)(tubeW + frameP * 2), (float)(tubeH + frameP * 2)}, s, (Color)COLOR_BRASS_DIM);

    // Glass tube background
    DrawRectangle(tubeX, tubeY, tubeW, tubeH, (Color){15, 25, 30, 220});

    // Fuel fill — from left to right
    int fillW = (int)(tubeW * fuelPct);

    // Cyan gradient fill — brighter at right (meniscus), darker at left
    for (int col = 0; col < fillW; col++) {
        float colPct = (float)col / (float)(fillW > 1 ? fillW - 1 : 1);
        unsigned char cr = (unsigned char)(35 + colPct * 45);
        unsigned char cg = (unsigned char)(150 + colPct * 70);
        unsigned char cb = (unsigned char)(190 + colPct * 50);
        // Bubble shimmer
        float shimmer = sinf(t * 3.0f + colPct * 10.0f) * 0.1f + 0.9f;
        DrawRectangle(tubeX + col, tubeY, 1, tubeH,
            (Color){(unsigned char)(cr * shimmer), (unsigned char)(cg * shimmer),
                    (unsigned char)(cb * shimmer), 230});
    }

    // Meniscus highlight at liquid edge
    if (fillW > 2) {
        DrawRectangle(tubeX + fillW - (int)(2 * s), tubeY + 1, (int)(2 * s), tubeH - 2,
            (Color){140, 240, 255, 160});
    }

    // Glass reflection stripe (top edge)
    DrawRectangle(tubeX, tubeY + 1, tubeW, (int)(2 * s), (Color){200, 220, 230, 35});

    // Tick marks — 4 divisions along top
    for (int i = 0; i <= 4; i++) {
        int tickX = tubeX + (int)((float)tubeW * (float)i / 4.0f);
        int tickH = (i % 2 == 0) ? frameP + (int)(3 * s) : frameP + (int)(1 * s);
        DrawLine(tickX, tubeY - frameP, tickX, tubeY - frameP + tickH, (Color)COLOR_BRASS);
    }

    // Label — left of tube
    const char *label = "He\xc2\xb3"; // He3 in UTF-8
    int labelFs = (int)(7 * s);
    if (labelFs < 6) labelFs = 6;
    int labelW = MeasureText(label, labelFs);
    DrawText(label, tubeX - frameP - labelW - (int)(4 * s), cy - labelFs / 2, labelFs, (Color)COLOR_HUD_TEXT);

    // Low fuel warning — flashing red border
    if (fuelPct < HE3_LOW_THRESHOLD && fuelPct > 0.0f) {
        float pulse = 0.5f + 0.5f * sinf(t * 6.0f);
        unsigned char wa = (unsigned char)(pulse * 200);
        DrawRectangleLinesEx((Rectangle){(float)(tubeX - frameP - 1), (float)(tubeY - frameP - 1),
            (float)(tubeW + frameP * 2 + 2), (float)(tubeH + frameP * 2 + 2)},
            s * 1.5f, (Color){255, 60, 30, wa});
    }

    // Charging indicator — yellow bar growing along bottom
    if (player->he3Charging) {
        float chargeRatio = player->he3ChargeTimer / HE3_CHARGE_MAX_TIME;
        int chargeW = (int)(tubeW * chargeRatio);
        unsigned char ca = (unsigned char)(100 + sinf(t * 12.0f) * 50);
        DrawRectangle(tubeX, tubeY + tubeH + frameP, chargeW,
            (int)(2 * s), (Color){255, 240, 100, ca});
    }

    // Rivets at corners
    DrawRivet(tubeX - frameP + (int)(1 * s), tubeY - frameP + (int)(1 * s), s * 0.4f);
    DrawRivet(tubeX + tubeW + frameP - (int)(1 * s), tubeY - frameP + (int)(1 * s), s * 0.4f);
}

// ---- Helper: mechanical ticker box ----
static void DrawTicker(int x, int y, int w, int h, const char *label, const char *value, float s) {
    // Dark inset box with brass frame
    DrawRectangle(x, y, w, h, (Color)COLOR_GAUGE_BG);
    DrawRectangleLinesEx((Rectangle){(float)x, (float)y, (float)w, (float)h}, s, (Color)COLOR_BRASS_DIM);

    // Value text (large, centered)
    int vfs = h * 2 / 3;
    if (vfs < 6) vfs = 6;
    int vw = MeasureText(value, vfs);
    DrawText(value, x + w / 2 - vw / 2, y + h / 2 - vfs / 2, vfs, (Color)COLOR_BRASS_BRIGHT);

    // Label above
    if (label) {
        int lfs = (int)(7 * s);
        if (lfs < 5) lfs = 5;
        int lw = MeasureText(label, lfs);
        DrawText(label, x + w / 2 - lw / 2, y - lfs - (int)(2 * s), lfs, (Color)COLOR_HUD_TEXT);
    }

    // Corner rivets
    DrawRivet(x + (int)(3 * s), y + (int)(3 * s), s * 0.7f);
    DrawRivet(x + w - (int)(3 * s), y + (int)(3 * s), s * 0.7f);
}

// ---- Helper: draw ammo bullets as visual indicators ----
static void DrawAmmoBullets(int x, int y, int ammo, int maxAmmo, float s, int maxH) {
    // Fit within maxH pixels: 2 rows of 16, compact sizing
    int cols = 16;
    int gap = (int)(1.0f * s);
    if (gap < 1) gap = 1;
    int bh = (maxH - gap) / 2;
    if (bh < 2) bh = 2;
    int bw = (int)(2 * s);
    if (bw < 1) bw = 1;

    for (int i = 0; i < maxAmmo && i < 40; i++) {
        int col = i % cols;
        int row = i / cols;
        int bx = x + col * (bw + gap);
        int by = y + row * (bh + gap);
        Color c = (i < ammo) ? (Color)COLOR_BRASS : (Color){40, 38, 32, 150};
        DrawRectangle(bx, by, bw, bh, c);
        if (i < ammo) {
            DrawRectangle(bx, by, bw, (int)(1.0f * s), (Color)COLOR_BRASS_BRIGHT);
        }
    }
}

void HudDrawLanderArrows(LanderManager *lm, Camera3D camera, int sw, int sh) {
    int cx = sw / 2;
    float s = (float)sh / (float)WINDOW_H;

    bool incoming = false;
    for (int i = 0; i < MAX_LANDERS; i++)
        if (lm->landers[i].state == LANDER_WAITING)
            incoming = true;

    if (incoming) {
        float pulse = (sinf(GetTime() * 6.0f) + 1.0f) * 0.5f;
        unsigned char pa = (unsigned char)(pulse * 255);

        // Red flashing bar with deco frame
        int barY = sh / 8;
        int barH = sh / 6;
        DrawRectangle(0, barY, sw, barH, (Color){180, 20, 10, (unsigned char)(pulse * 80)});

        // Deco border lines on alert bar
        Color border = (Color){200, 60, 30, (unsigned char)(pulse * 180)};
        DrawLineEx((Vector2){0, (float)barY}, (Vector2){(float)sw, (float)barY}, 2.0f * s, border);
        DrawLineEx((Vector2){0, (float)(barY + barH)}, (Vector2){(float)sw, (float)(barY + barH)}, 2.0f * s, border);

        // Angular deco accents at ends
        int accent = (int)(20 * s);
        DrawLine(0, barY, accent, barY + barH / 2, border);
        DrawLine(sw, barY, sw - accent, barY + barH / 2, border);

        const char *alert = "ACHTUNG SOLDATEN";
        int alertFs = sh / 8;
        int alertW = MeasureText(alert, alertFs);
        DrawText(alert, cx - alertW / 2, barY + sh / 30, alertFs, (Color){255, 50, 30, pa});

        const char *sub = "FEINDLICHE LANDUNG ERKANNT";
        int subFs = sh / 18;
        int subW = MeasureText(sub, subFs);
        DrawText(sub, cx - subW / 2, barY + sh / 30 + alertFs + (int)(4 * s), subFs, (Color){255, 200, 50, pa});
    }
    (void)camera;
}

void HudDrawPickup(PickupManager *pm, int sw, int sh) {
    if (!pm->hasPickup) return;
    int cx = sw / 2;
    float s = (float)sh / (float)WINDOW_H;

    int py = sh - sh / 30 - sh / 14 - sh / 18;
    int pw = sw / 4;
    int px = cx - pw / 2;
    int ph = sh / 22;

    DrawRectangle(px, py, pw, ph, (Color)COLOR_DARK_METAL);
    bool isSoviet = (pm->pickupType == PICKUP_KOSMOS7 || pm->pickupType == PICKUP_KS23_MOLOT || pm->pickupType == PICKUP_ZARYA_TK4);
    Color borderCol = isSoviet ? (Color){200, 80, 40, 200} : (Color){80, 140, 200, 200};
    DrawRectangleLinesEx((Rectangle){(float)px, (float)py, (float)pw, (float)ph}, s, borderCol);

    // Rivets
    DrawRivet(px + (int)(4 * s), py + ph / 2, s * 0.7f);
    DrawRivet(px + pw - (int)(4 * s), py + ph / 2, s * 0.7f);

    const char *name = PickupGetName(pm->pickupType);
    char txt[64];
    snprintf(txt, sizeof(txt), "%s  [%d]", name, pm->pickupAmmo);
    int fs = ph * 2 / 3;
    int tw = MeasureText(txt, fs);
    Color tc = isSoviet ? (Color){255, 120, 60, 255} : (Color){100, 180, 255, 255};

    if (pm->pickupType == PICKUP_ZARYA_TK4 && pm->charging) {
        float ratio = pm->chargeTime / PICKUP_ZARYA_CHARGE_TIME;
        if (ratio > 1.0f) ratio = 1.0f;
        DrawRectangle(px + 2, py + ph - 4, (int)((pw - 4) * ratio), 3, (Color){255, 80, 30, 255});
    }
    DrawText(txt, cx - tw / 2, py + ph / 2 - fs / 2, fs, tc);
}

void HudDraw(HudState *state, Player *player, Weapon *weapon, Game *game, int sw, int sh) {
    int cx = sw / 2, cy = sh / 2;
    float s = (float)sh / (float)WINDOW_H;
    int barInset = sw / 7;
    int topL = barInset, topR = sw - barInset;
    int topW = topR - topL;

    // ======== CROSSHAIR — brass spinning T-ticks, hidden when Fuehrerauge active ========
    if (state->fuehreraugeAnim <= 0.0f && player->fuehreraugeAnim <= 0.0f) {
        float hpPct = (player->maxHealth > 0.001f) ? (player->health / player->maxHealth) : 0.0f;
        (void)hpPct;

        // Normal: brass. Kill confirm: flash red.
        Color col;
        if (state->killConfirmTimer > 0) {
            float flash = state->killConfirmTimer / HUD_CROSSHAIR_KILL_FLASH;
            col = (Color){(unsigned char)(200 + 55 * flash), (unsigned char)(30 + 40 * flash), 20, 230};
        } else {
            col = (Color)COLOR_BRASS;
        }

        // Center dot
        DrawCircle(cx, cy, 1.5f * s, col);

        // Spinning T-shaped ticks
        float rot = state->crossRotation * DEG2RAD;
        float spread = state->crossSpread * s;
        int dist = (int)(10 * s + spread * 0.5f);
        int stem = (int)(4 * s);
        int bar = (int)(3 * s);

        for (int i = 0; i < 4; i++) {
            float angle = rot + (float)i * PI * 0.5f;
            float cosA = cosf(angle);
            float sinA = sinf(angle);
            int ox = cx + (int)(dist * cosA);
            int oy = cy + (int)(dist * sinA);
            int ix = cx + (int)((dist - stem) * cosA);
            int iy = cy + (int)((dist - stem) * sinA);
            DrawLine(ix, iy, ox, oy, col);
            float perpX = -sinA;
            float perpY =  cosA;
            DrawLine(ox - (int)(bar * perpX), oy - (int)(bar * perpY),
                     ox + (int)(bar * perpX), oy + (int)(bar * perpY), col);
        }
    }

    // ======== He-3 Charge Ring around crosshair ========
    if (player->he3Charging && player->he3ChargeTimer > HE3_TAP_THRESHOLD) {
        float chargeRatio = player->he3ChargeTimer / HE3_CHARGE_MAX_TIME;
        if (chargeRatio > 1.0f) chargeRatio = 1.0f;
        float t_ch = (float)GetTime();
        int ringR = (int)(24 * s);
        float arcDeg = chargeRatio * 360.0f;
        float thickness = 3.5f * s;
        unsigned char ca = (unsigned char)(180 + sinf(t_ch * 6.0f) * 40);
        // Thick filled arc
        DrawCircleSector((Vector2){(float)cx, (float)cy}, (float)(ringR + (int)(thickness * 0.5f)),
            -90.0f, -90.0f + arcDeg, (int)(arcDeg / 8) + 4,
            (Color){50, 200, 230, (unsigned char)(ca * 0.5f)});
        DrawCircleSector((Vector2){(float)cx, (float)cy}, (float)(ringR - (int)(thickness * 0.5f)),
            -90.0f, -90.0f + arcDeg, (int)(arcDeg / 8) + 4,
            (Color)COLOR_GAUGE_BG);
        // Bright thick arc edge
        for (int seg = 0; seg <= (int)(arcDeg / 4); seg++) {
            float a1 = (-90.0f + (float)seg * 4.0f) * DEG2RAD;
            float a2 = (-90.0f + fminf((float)(seg + 1) * 4.0f, arcDeg)) * DEG2RAD;
            DrawLineEx(
                (Vector2){(float)cx + ringR * cosf(a1), (float)cy + ringR * sinf(a1)},
                (Vector2){(float)cx + ringR * cosf(a2), (float)cy + ringR * sinf(a2)},
                thickness, (Color){80, 240, 255, ca});
        }
        // Full charge flash
        if (chargeRatio >= 0.99f) {
            float flashPulse = 0.5f + 0.5f * sinf(t_ch * 10.0f);
            unsigned char fa = (unsigned char)(flashPulse * 120);
            DrawCircleSector((Vector2){(float)cx, (float)cy}, (float)(ringR + (int)(thickness)),
                -90.0f, 270.0f, 36, (Color){100, 255, 255, fa});
        }
    }

    // ======== TOP BAR — Art Deco frame ========
    int topY = sh / 20;
    int topH = sh / 14;
    int pad = (int)(12 * s);
    DrawDecoBar(topL, topY, topW, topH, s);

    // ---- Decorative eagle wings above top bar ----
    DrawEagle(topL + topW / 2, topY - (int)(5 * s), (int)(16 * s),
        (Color){200, 170, 80, 200});

    // ---- Health gauge (left) — semicircular, flush with bottom of bar ----
    {
        int gaugeR = (int)(32 * s);
        int gaugeCX = topL + pad + gaugeR + (int)(8 * s);
        int gaugeCY = topY + topH; // flush with bottom of bar, semicircle arches upward
        DrawAnalogGauge(state, player, gaugeCX, gaugeCY, gaugeR, s);

        // He-3 fuel tube — horizontal, below health gauge
        int he3X = gaugeCX;
        int he3Y = gaugeCY + (int)(12 * s);
        DrawHe3Gauge(player, he3X, he3Y, (int)(gaugeR * 1.8f), s);
    }

    // ---- Iron cross between health gauge and wave ticker ----
    {
        int icSize = (int)(8 * s);
        int icX = topL + topW / 4;
        int icY = topY + topH / 2;
        DrawIronCross(icX, icY, icSize, (Color){160, 140, 80, 120});
    }

    // ---- Iron cross between wave and kills tickers ----
    {
        int icSize = (int)(8 * s);
        int icX = topL + topW * 3 / 4;
        int icY = topY + topH / 2;
        DrawIronCross(icX, icY, icSize, (Color){160, 140, 80, 120});
    }

    // ---- Wave ticker (center) ----
    {
        char waveText[16];
        snprintf(waveText, sizeof(waveText), "%d", game->wave);
        int tw = (int)(55 * s), th = topH - (int)(4 * s);
        int tx = topL + topW / 2 - tw / 2;
        int ty = topY + topH / 2 - th / 2;
        DrawTicker(tx, ty, tw, th, "WAVE", waveText, s);
    }

    // ---- Kills ticker (right) ----
    {
        char killText[16];
        snprintf(killText, sizeof(killText), "%d", game->killCount);
        int tw = (int)(55 * s), th = topH - (int)(4 * s);
        int tx = topR - pad - tw;
        int ty = topY + topH / 2 - th / 2;
        DrawTicker(tx, ty, tw, th, "KILLS", killText, s);

        // Enemy count next to kill ticker
        if (state->enemyCount > 0) {
            float t_e = (float)GetTime();
            char ecBuf[16];
            snprintf(ecBuf, sizeof(ecBuf), "[%d]", state->enemyCount);
            int ecFs = topH / 3;
            int ecW = MeasureText(ecBuf, ecFs);
            float pulse = 0.6f + 0.4f * sinf(t_e * 3.0f);
            unsigned char ea = (unsigned char)(200 * pulse);
            DrawText(ecBuf, tx - ecW - (int)(6 * s), topY + topH / 2 - ecFs / 2, ecFs,
                (Color){220, 60, 30, ea});
        }
    }

    // ======== BOTTOM BAR — Art Deco frame ========
    int barH = sh / 14;
    int barY = sh - sh / 30 - barH;
    DrawDecoBar(topL, barY, topW, barH, s);

    // Eagle wings centered above ammo bar
    DrawEagle(topL + topW / 2, barY - (int)(7 * s), (int)(18 * s),
        (Color){200, 170, 80, 200});

    // ---- Weapon name (left) — brass deco tab ----
    {
        const char *weapName = WeaponGetName(weapon);
        int wns = barH * 2 / 3;
        int wnx = topL + (int)(14 * s);
        // Small deco tab behind name
        int nameW = MeasureText(weapName, wns) + (int)(10 * s);
        DrawRectangle(wnx - (int)(5 * s), barY + barH / 2 - wns / 2 - (int)(2 * s),
                      nameW, wns + (int)(4 * s), (Color){35, 32, 28, 160});
        DrawText(weapName, wnx, barY + barH / 2 - wns / 2, wns, (Color)COLOR_BRASS);
    }

    // ---- Ammo display (center) — ticker-style readout ----
    {
        int ammo = WeaponGetAmmo(weapon);
        int maxAmmo = WeaponGetMaxAmmo(weapon);
        int reserve = WeaponGetReserve(weapon);

        char ammoText[64];
        if (ammo < 0)
            snprintf(ammoText, sizeof(ammoText), "MELEE");
        else
            snprintf(ammoText, sizeof(ammoText), "%d / %d  [ %d ]", ammo, maxAmmo, reserve);

        int afs = barH / 2;
        if (afs < 6) afs = 6;
        int atw = MeasureText(ammoText, afs);
        int ax = topL + topW / 2 - atw / 2;
        int ay = barY + barH / 2 - afs / 2;

        // Nixie tube rolling digit ammo display: MAG / MAXMAG  [RESERVE]
        if (ammo >= 0) {
            int nixieX = ax;
            int nixieY = ay;
            HudDrawNixie(state, ammo, nixieX, nixieY, s);
            // "/ maxMag" right after nixie digits
            int digitW = (int)(20 * s);
            int digitGap = (int)(3 * s);
            int nixiePad = (int)(8 * s);
            int nixieW = digitW * 4 + digitGap * 3 + nixiePad;
            int rfs = afs * 2 / 3;
            if (rfs < 8) rfs = 8;
            char magText[32];
            snprintf(magText, sizeof(magText), "/ %d", maxAmmo);
            int magTW = MeasureText(magText, rfs);
            DrawText(magText, nixieX + nixieW + (int)(4 * s), nixieY + afs / 2 - rfs / 2, rfs, (Color)COLOR_BRASS_BRIGHT);
            // "[reserve]" further right in dim
            char resText[32];
            snprintf(resText, sizeof(resText), "[%d]", reserve);
            DrawText(resText, nixieX + nixieW + (int)(4 * s) + magTW + (int)(8 * s), nixieY + afs / 2 - rfs / 2, rfs, (Color)COLOR_BRASS_DIM);
        } else {
            // Melee weapon: just draw text
            int inPad = (int)(4 * s);
            DrawRectangle(ax - inPad, ay - inPad / 2, atw + inPad * 2, afs + inPad, (Color)COLOR_GAUGE_BG);
            DrawRectangleLinesEx(
                (Rectangle){(float)(ax - inPad), (float)(ay - inPad / 2),
                             (float)(atw + inPad * 2), (float)(afs + inPad)},
                s * 0.5f, (Color)COLOR_BRASS_DIM);
            DrawText(ammoText, ax, ay, afs, (Color)COLOR_BRASS_BRIGHT);
        }
    }

    // ---- Hints (right) ----
    {
        const char *hints = "[1][2][3] [R] [X]";
        int hfs = barH / 3;
        int hw = MeasureText(hints, hfs);
        DrawText(hints, topR - hw - pad, barY + barH / 2 - hfs / 2, hfs, (Color)COLOR_BRASS_DIM);
    }

    // ---- Small iron crosses flanking bottom bar ----
    {
        int icSize = (int)(6 * s);
        DrawIronCross(topL + (int)(8 * s), barY + barH / 2, icSize, (Color){140, 120, 70, 80});
        DrawIronCross(topR - (int)(8 * s), barY + barH / 2, icSize, (Color){140, 120, 70, 80});
    }

    // ---- Eagle emblem centered just above bottom bar ----
    {
        int eagleSize = barH + (int)(4 * s);
        int eagleX = topL + topW / 2;
        int eagleY = barY - eagleSize / 4;
        DrawEagle(eagleX, eagleY, eagleSize, (Color)COLOR_BRASS);
    }

    // ======== RELOAD — circular gauge near crosshair ========
    if (WeaponIsReloading(weapon)) {
        float prog = WeaponReloadProgress(weapon);
        float t_r = (float)GetTime();
        int reloadR = (int)(18 * s);
        int reloadCX = cx;
        int reloadCY = cy + (int)(35 * s);

        // Background arc
        DrawCircleSector((Vector2){(float)reloadCX, (float)reloadCY}, (float)reloadR,
            -30.0f, -30.0f + 240.0f, 24, (Color)COLOR_GAUGE_BG);
        // Progress arc
        DrawCircleSector((Vector2){(float)reloadCX, (float)reloadCY}, (float)(reloadR - 1),
            -30.0f, -30.0f + 240.0f * prog, 24, (Color){180, 150, 60, 200});
        // Inner disc
        DrawCircle(reloadCX, reloadCY, (float)(reloadR * 0.5f), (Color)COLOR_GAUGE_BG);
        // Pivot rivet
        DrawRivet(reloadCX, reloadCY, s * 0.7f);

        // "RELOADING" label
        int rfs = (int)(8 * s);
        if (rfs < 6) rfs = 6;
        const char *rl = "RELOADING";
        float rlPulse = 0.6f + 0.4f * sinf(t_r * 6.0f);
        DrawText(rl, reloadCX - MeasureText(rl, rfs) / 2, reloadCY - reloadR - rfs - (int)(4 * s),
            rfs, (Color){200, 180, 130, (unsigned char)(240 * rlPulse)});
    }

    // ======== Beam cooldown ========
    if (weapon->raketenTimer > 0 && !weapon->raketenFiring && weapon->current == WEAPON_RAKETENFAUST) {
        float cd = weapon->raketenTimer / weapon->raketenCooldown;
        int cdW = sw / 8, cdH = sh / 60;
        int cdX = cx - cdW / 2, cdY = cy + (int)(20 * s);
        DrawRectangle(cdX, cdY, cdW, cdH, (Color){40, 30, 15, 180});
        DrawRectangle(cdX, cdY, (int)(cdW * (1.0f - cd)), cdH, (Color)COLOR_BRASS);
    }

    // ======== Ground pound ready prompt — boxed, above bottom bar eagle ========
    if (player->groundPoundReady) {
        float t = (float)GetTime();
        float gpPulse = 0.7f + 0.3f * sinf(t * 4.0f);
        unsigned char gpA = (unsigned char)(230 * gpPulse);
        int gpFs = (int)(14 * s);
        if (gpFs < 10) gpFs = 10;
        const char *gpLine1 = "DER ADLER ERHEBT SICH";
        const char *gpLine2 = "SLAM [X] TO USE YOUR TALONS";
        int gp1W = MeasureText(gpLine1, gpFs);
        int gp2W = MeasureText(gpLine2, gpFs);
        int gpMaxW = gp1W > gp2W ? gp1W : gp2W;
        int gpPad = (int)(6 * s);
        int gpBoxW = gpMaxW + gpPad * 2;
        int gpBoxH = gpFs * 2 + (int)(2 * s) + gpPad * 2;
        int gpBoxX = cx - gpBoxW / 2;
        int gpBoxY = barY - (int)(70 * s);
        DrawRectangle(gpBoxX, gpBoxY, gpBoxW, gpBoxH, (Color){25, 22, 18, 200});
        DrawRectangleLinesEx((Rectangle){(float)gpBoxX, (float)gpBoxY,
            (float)gpBoxW, (float)gpBoxH}, s, (Color){200, 170, 80, (unsigned char)(180 * gpPulse)});
        int gpTextY = gpBoxY + gpPad;
        DrawText(gpLine1, cx - gp1W / 2, gpTextY, gpFs, (Color){200, 170, 80, gpA});
        DrawText(gpLine2, cx - gp2W / 2, gpTextY + gpFs + (int)(2 * s), gpFs, (Color){220, 190, 90, gpA});
    }

    // ======== Damage flash — warm red tint with vignette ========
    if (player->damageFlashTimer > 0) {
        unsigned char a = (unsigned char)(player->damageFlashTimer / DAMAGE_FLASH_DURATION * 120);
        DrawRectangle(0, 0, sw, sh, (Color){180, 30, 10, a});
        int vigW = sw / 6;
        DrawRectangle(0, 0, vigW, sh, (Color){140, 20, 5, (unsigned char)(a * 0.5f)});
        DrawRectangle(sw - vigW, 0, vigW, sh, (Color){140, 20, 5, (unsigned char)(a * 0.5f)});
    }
}

void HudDrawRadioTransmission(float timer, int sw, int sh) {
    if (timer <= 0) return;
    float s = (float)sh / (float)WINDOW_H;

    float alpha = (timer > 0.5f) ? 1.0f : timer / 0.5f;
    unsigned char a = (unsigned char)(alpha * 220);

    int boxW = sw / 5;
    int boxH = sh / 8;
    int boxX = sw / 20;
    int boxY = sh * 2 / 3;
    int inPad = (int)(8 * s);
    int smPad = (int)(4 * s);

    // Dark background with brass border
    DrawRectangle(boxX, boxY, boxW, boxH, (Color){10, 10, 8, (unsigned char)(a * 0.8f)});
    DrawRectangleLinesEx((Rectangle){(float)boxX, (float)boxY, (float)boxW, (float)boxH},
                         s, (Color){180, 150, 70, a});
    DrawRectangleLinesEx((Rectangle){(float)(boxX + 1), (float)(boxY + 1), (float)(boxW - 2), (float)(boxH - 2)},
                         s * 0.5f, (Color){140, 120, 60, (unsigned char)(a * 0.5f)});

    // Rivets
    DrawRivet(boxX + smPad, boxY + smPad, s * 0.6f);
    DrawRivet(boxX + boxW - smPad, boxY + smPad, s * 0.6f);

    // "TRANSMISSION" header
    int hfs = boxH / 4;
    const char *hdr = "TRANSMISSION";
    int hw = MeasureText(hdr, hfs);
    float blink = sinf(GetTime() * 8.0f);
    Color hdrCol = (blink > 0) ? (Color){240, 200, 100, a} : (Color){180, 150, 70, a};
    DrawText(hdr, boxX + boxW / 2 - hw / 2, boxY + smPad, hfs, hdrCol);

    // Amber waveform
    int waveY = boxY + boxH / 3 + smPad;
    int waveH = boxH / 3;
    int waveL = boxX + inPad;
    int waveR = boxX + boxW - inPad;
    int waveStep = (int)(2 * s); if (waveStep < 1) waveStep = 1;
    DrawLine(waveL, waveY + waveH / 2, waveR, waveY + waveH / 2, (Color){80, 70, 40, (unsigned char)(a * 0.4f)});
    for (int x = waveL; x < waveR; x += waveStep) {
        float t = (float)(x - waveL) / (float)(waveR - waveL);
        float wave = sinf(t * 20.0f + GetTime() * 15.0f) * 0.6f;
        wave += sinf(t * 50.0f + GetTime() * 25.0f) * 0.3f;
        wave *= alpha;
        int y1 = waveY + waveH / 2;
        int y2 = y1 + (int)(wave * waveH * 0.45f);
        DrawLine(x, y1, x, y2, (Color){205, 170, 80, a});
    }

    // "RECEIVED" footer
    int ffs = boxH / 5;
    const char *ftr = "RECEIVED";
    int fw = MeasureText(ftr, ffs);
    DrawText(ftr, boxX + boxW / 2 - fw / 2, boxY + boxH - ffs - smPad, ffs, (Color){180, 160, 120, a});
}

void HudDrawRankKill(float timer, int rankType, int sw, int sh) {
    if (timer <= 0 || rankType <= 0) return;
    int cx = sw / 2;
    float s = (float)sh / (float)WINDOW_H;
    float alpha = (timer > 0.5f) ? 1.0f : timer / 0.5f;
    unsigned char a = (unsigned char)(alpha * 255);

    float sc = s;
    float t = (float)GetTime();

    const char *text = (rankType == 4) ? "WUMMS! BLUT UND MONDSTAUB!" :
                       (rankType == 3) ? "KOPF AB! SAUBERE ARBEIT!" :
                       (rankType == 2) ? "OFFIZIER ELIMINIERT!" : "UNTEROFFIZIER ELIMINIERT!";
    // Smaller font, positioned just below top bar
    int fs = sh / 22;
    int tw = MeasureText(text, fs);
    // Top bar bottom edge is at sh/20 + sh/14. Place rank kill just below.
    int ty = sh / 20 + sh / 14 + (int)(8 * sc);

    float expand = (timer > 1.5f) ? (2.0f - timer) * 2.0f : 0.0f;
    if (expand < 0) expand = 0;
    int extraW = (int)(expand * 8);
    int boxX = cx - tw / 2 - (int)(12 * sc) - extraW;
    int boxY = ty - (int)(4 * sc);
    int boxW = tw + (int)(24 * sc) + extraW * 2;
    int boxH = fs + (int)(8 * sc);

    // Deco-style backing with corner cuts
    DrawDecoBar(boxX, boxY, boxW, boxH, sc * 0.7f);

    Color borderGlow = (rankType == 4) ? (Color){180, 160, 120, (unsigned char)(a * 0.9f)} :
                       (rankType == 3) ? (Color){220, 30, 10, (unsigned char)(a * 0.8f)} :
                       (rankType == 2) ? (Color){220, 180, 50, (unsigned char)(a * 0.7f)}
                                        : (Color){200, 160, 60, (unsigned char)(a * 0.5f)};
    DrawRectangleLinesEx(
        (Rectangle){(float)boxX, (float)boxY, (float)boxW, (float)boxH}, sc, borderGlow);

    Color col = (rankType == 4) ? (Color){200, 180, 140, a} :
                (rankType == 3) ? (Color){255, 60, 20, a} :
                (rankType == 2) ? (Color){240, 200, 100, a} : (Color){220, 180, 80, a};
    int jit = (int)(sinf(t * 20.0f) * 1.0f * alpha);
    DrawText(text, cx - tw / 2 + jit, ty, fs, col);
}

void HudDrawStructurePrompt(StructurePrompt prompt, int resuppliesLeft, float emptyTimer, int sw, int sh) {
    int cx = sw / 2;
    int cy = sh / 2;
    float s = (float)sh / (float)WINDOW_H;

    // "MEIN GOTT!" message
    if (emptyTimer > 0) {
        const char *line1 = "MEIN GOTT!";
        const char *line2 = "THE CUPBOARD IS BARE, KAMERAD!";
        int fs1 = sh / 12;
        int fs2 = sh / 18;
        int tw1 = MeasureText(line1, fs1);
        int tw2 = MeasureText(line2, fs2);
        int boxW = (tw1 > tw2 ? tw1 : tw2) + sh / 15;
        int boxH = fs1 + fs2 + sh / 12;
        int boxX = cx - boxW / 2;
        int boxY = cy - boxH / 2 - sh / 10;

        float fade = (emptyTimer > 2.5f) ? 1.0f : emptyTimer / 2.5f;
        unsigned char a = (unsigned char)(fade * 255);

        DrawRectangle(boxX, boxY, boxW, boxH, (Color){40, 10, 5, (unsigned char)(fade * 200)});
        DrawRectangleLinesEx((Rectangle){(float)boxX, (float)boxY, (float)boxW, (float)boxH},
                             s, (Color){200, 50, 30, a});

        float shake = sinf((float)GetTime() * 25.0f) * 2.0f * fade;
        DrawText(line1, cx - tw1 / 2 + (int)shake, boxY + sh / 30, fs1, (Color){255, 60, 30, a});
        DrawText(line2, cx - tw2 / 2, boxY + sh / 30 + fs1 + sh / 40, fs2, (Color){255, 200, 50, a});
    }

    if (prompt == PROMPT_NONE) return;

    const char *text = NULL;
    Color col = (Color)COLOR_BRASS;

    switch (prompt) {
        case PROMPT_ENTER:    text = "PRESS [E] TO ENTER BASE"; break;
        case PROMPT_EXIT:     text = "PRESS [E] TO EXIT BASE"; break;
        case PROMPT_RESUPPLY: {
            static char resupplyBuf[64];
            snprintf(resupplyBuf, sizeof(resupplyBuf), "PRESS [E] TO RESUPPLY [%d]", resuppliesLeft);
            text = resupplyBuf;
            col = (Color)COLOR_BRASS_BRIGHT;
            break;
        }
        case PROMPT_EMPTY:
            text = "VERSORGUNG ERSCHOEPFT";
            col = (Color){200, 60, 40, 220};
            break;
        case PROMPT_REFUEL: {
            static char refuelBuf[64];
            snprintf(refuelBuf, sizeof(refuelBuf), "PRESS [E] — He-3 BETANKEN [%d]", resuppliesLeft);
            text = refuelBuf;
            col = (Color){80, 220, 240, 240};
            break;
        }
        case PROMPT_REFUEL_EMPTY:
            text = "He-3 VORRAT ERSCHOEPFT";
            col = (Color){200, 60, 40, 220};
            break;
        default: return;
    }

    int fs = sh / 20;
    int tw = MeasureText(text, fs);
    int promptPad = sh / 40;

    // Dark metal backing
    DrawRectangle(cx - tw / 2 - promptPad, cy + sh / 6 - promptPad / 2, tw + promptPad * 2, fs + promptPad, (Color)COLOR_DARK_METAL);
    DrawRectangleLinesEx(
        (Rectangle){(float)(cx - tw / 2 - promptPad), (float)(cy + sh / 6 - promptPad / 2),
                     (float)(tw + promptPad * 2), (float)(fs + promptPad)},
        s * 0.5f, (Color)COLOR_BRASS_DIM);

    float pulse = 0.7f + 0.3f * sinf((float)GetTime() * 4.0f);
    Color drawCol = {(unsigned char)(col.r * pulse), (unsigned char)(col.g * pulse), col.b, col.a};
    DrawText(text, cx - tw / 2, cy + sh / 6, fs, drawCol);
}

// ======== FUEHRERAUGE (Leader Eye Targeting Computer) ========
// Swing arm + trapezoid frame + old-TV shader
//
// Animation phases (t = 0..1):
//   Phase 1 (0.0-0.35): Arm swings down from stowed-right to deployed position
//   Phase 2 (0.35-1.0): Viewport telescopically expands toward the player (slow)
void HudDrawFuehrerauge(Texture2D zoomTexture, Shader zoomShader, float anim, int sw, int sh) {
    float s = (float)sh / (float)WINDOW_H;
    float t = (anim > 0.0f) ? anim : 0.0f;
    float time = (float)GetTime();

    // --- Animation phase breakdown ---
    // Phase 1: arm swing (t 0..0.35 -> swingT 0..1)
    float swingT = (t < 0.35f) ? (t / 0.35f) : 1.0f;
    float swingEase = swingT * swingT * (3.0f - 2.0f * swingT);

    // Phase 2: slow telescopic expand (t 0.35..1.0 -> expandT 0..1)
    float expandT = 0.0f;
    if (t > 0.35f) expandT = (t - 0.35f) / 0.65f;
    if (expandT > 1.0f) expandT = 1.0f;
    float expandEase = expandT * expandT * (3.0f - 2.0f * expandT);

    // --- Pivot bolt at right edge of top bar ---
    int barInset = sw / 7;
    float pivotX = (float)(sw - barInset);
    float pivotY = (float)(sh / 20 + sh / 28);

    // Deployed target = screen center (slightly above center)
    float targetCX = sw * 0.5f;
    float targetCY = sh * 0.42f;

    // Arm geometry
    float dx = pivotX - targetCX;
    float dy = targetCY - pivotY;
    float armLen = sqrtf(dx * dx + dy * dy);
    float deployedAngle = atan2f(dx, dy);

    // Stowed: arm extends horizontally to the right
    float stowedAngle = -(float)(PI * 0.5);
    float angle = stowedAngle + swingEase * (deployedAngle - stowedAngle);

    // Lens center on the arc
    float lensCX = pivotX - sinf(angle) * armLen;
    float lensCY = pivotY + cosf(angle) * armLen;

    // --- Viewport sizing: telescopic expand in phase 2 ---
    float sizeMin = 0.35f;
    float sizeFrac = sizeMin + (1.0f - sizeMin) * expandEase;

    int vpWFull = (int)(sw * FUEHRERAUGE_WIDTH_FRAC);
    int vpHFull = (int)(sh * FUEHRERAUGE_HEIGHT_FRAC);
    int vpW = (int)(vpWFull * sizeFrac);
    int vpH = (int)(vpHFull * sizeFrac);

    int vpX = (int)(lensCX - vpW / 2.0f);
    int vpY = (int)(lensCY - vpH / 2.0f);

    // --- Draw Swing Arm (always visible) ---
    float armThick = 6.0f * s;
    Color armEdge = {60, 55, 45, 200};
    Color armMain = {90, 80, 65, 220};
    DrawLineEx((Vector2){pivotX, pivotY}, (Vector2){lensCX, lensCY}, armThick + 2*s, armEdge);
    DrawLineEx((Vector2){pivotX, pivotY}, (Vector2){lensCX, lensCY}, armThick, armMain);

    // Secondary support strut
    {
        float strutOff = 3.0f * s;
        DrawLineEx((Vector2){pivotX + strutOff, pivotY + strutOff},
                   (Vector2){lensCX + strutOff, lensCY + strutOff}, 2.5f * s, (Color){50, 45, 38, 150});
    }

    // Pivot bolt (3 concentric circles)
    DrawCircle((int)pivotX, (int)pivotY, 5*s, (Color)COLOR_DARK_METAL);
    DrawCircle((int)pivotX, (int)pivotY, 3.5f*s, (Color)COLOR_BRASS_DIM);
    DrawCircle((int)pivotX, (int)pivotY, 1.5f*s, (Color){25, 22, 18, 255});

    // Bracket near lens end
    float perpXf = cosf(angle);
    float perpYf = sinf(angle);
    float bracketDist = armLen * 0.85f;
    float bCX = pivotX - sinf(angle) * bracketDist;
    float bCY = pivotY + cosf(angle) * bracketDist;
    float bLen = 8.0f * s;
    DrawLineEx((Vector2){bCX - perpXf * bLen, bCY - perpYf * bLen},
               (Vector2){bCX + perpXf * bLen, bCY + perpYf * bLen},
               2.0f * s, armEdge);

    // Second bracket closer to pivot
    float b2Dist = armLen * 0.35f;
    float b2CX = pivotX - sinf(angle) * b2Dist;
    float b2CY = pivotY + cosf(angle) * b2Dist;
    DrawLineEx((Vector2){b2CX - perpXf * bLen * 0.6f, b2CY - perpYf * bLen * 0.6f},
               (Vector2){b2CX + perpXf * bLen * 0.6f, b2CY + perpYf * bLen * 0.6f},
               1.5f * s, armEdge);

    // --- Housing only once arm is mostly down ---
    if (swingEase < 0.35f) return;

    // Fade-in alpha for housing
    float housingAlpha = (swingEase < 0.5f) ? (swingEase - 0.35f) / 0.15f : 1.0f;
    if (housingAlpha > 1.0f) housingAlpha = 1.0f;

    // --- Thick Brass Housing ---
    int frame = (int)(FUEHRERAUGE_FRAME_THICK * s * 2.5f);
    int trapInset = (int)(vpW * 0.10f);

    Vector2 tl = {(float)(vpX - frame), (float)(vpY - frame)};
    Vector2 tr = {(float)(vpX + vpW + frame), (float)(vpY - frame)};
    Vector2 bl = {(float)(vpX - frame + trapInset), (float)(vpY + vpH + frame)};
    Vector2 br = {(float)(vpX + vpW + frame - trapInset), (float)(vpY + vpH + frame)};

    unsigned char ha = (unsigned char)(250 * housingAlpha);
    Color housingDark = {45, 40, 32, ha};
    Color housingMid = {65, 58, 44, (unsigned char)(245 * housingAlpha)};
    Color housingBrass = {110, 95, 65, (unsigned char)(230 * housingAlpha)};

    // Top band
    DrawRectangle((int)tl.x, (int)tl.y, (int)(tr.x - tl.x), frame, housingMid);
    // Bottom band
    DrawRectangle((int)bl.x, vpY + vpH, (int)(br.x - bl.x), frame, housingDark);
    // Left band (scanline for trapezoid)
    for (int row = 0; row < vpH + frame * 2; row++) {
        int ry = (int)tl.y + row;
        float rowFrac = (float)row / (float)(vpH + frame * 2);
        int leftEdge = (int)(tl.x + rowFrac * trapInset);
        int bandW = vpX - leftEdge;
        if (bandW > 0) DrawRectangle(leftEdge, ry, bandW, 1, housingDark);
    }
    // Right band
    for (int row = 0; row < vpH + frame * 2; row++) {
        int ry = (int)tr.y + row;
        float rowFrac = (float)row / (float)(vpH + frame * 2);
        int rightEdge = (int)(tr.x - rowFrac * trapInset);
        int bandX = vpX + vpW;
        int bandW = rightEdge - bandX;
        if (bandW > 0) DrawRectangle(bandX, ry, bandW, 1, housingDark);
    }

    // Brass highlight strip
    DrawRectangle((int)tl.x + 2, (int)tl.y + 1, (int)(tr.x - tl.x) - 4, (int)(2 * s),
        (Color){100, 88, 62, (unsigned char)(80 * housingAlpha)});

    // Brass accent stripe along inner edge
    int insetB = (int)(3 * s);
    Color accentLine = {140, 120, 70, (unsigned char)(160 * housingAlpha)};
    DrawLine(vpX - insetB, vpY - insetB, vpX + vpW + insetB, vpY - insetB, accentLine);
    DrawLine(vpX - insetB, vpY + vpH + insetB, vpX + vpW + insetB, vpY + vpH + insetB, accentLine);
    DrawLine(vpX - insetB, vpY - insetB, vpX - insetB, vpY + vpH + insetB, accentLine);
    DrawLine(vpX + vpW + insetB, vpY - insetB, vpX + vpW + insetB, vpY + vpH + insetB, accentLine);

    // Outer brass border
    Color borderCol = (Color)COLOR_BRASS;
    DrawLineEx(tl, tr, 3.0f * s, borderCol);
    DrawLineEx(bl, br, 3.0f * s, borderCol);
    DrawLineEx(tl, bl, 2.5f * s, borderCol);
    DrawLineEx(tr, br, 2.5f * s, borderCol);
    Color borderInner = (Color)COLOR_BRASS_DIM;
    float bOff = 3.0f * s;
    DrawLineEx((Vector2){tl.x + bOff, tl.y + bOff}, (Vector2){tr.x - bOff, tr.y + bOff}, 1.5f * s, borderInner);
    DrawLineEx((Vector2){bl.x + bOff, bl.y - bOff}, (Vector2){br.x - bOff, br.y - bOff}, 1.5f * s, borderInner);

    // Gear-tooth detailing
    {
        int numTeeth = 16;
        float topLen = tr.x - tl.x;
        for (int i = 0; i < numTeeth; i++) {
            float frac = ((float)i + 0.5f) / numTeeth;
            int tx = (int)(tl.x + frac * topLen);
            int tyTop = (int)tl.y;
            DrawLine(tx, tyTop - (int)(2 * s), tx, tyTop,
                (Color){100, 88, 60, (unsigned char)(120 * housingAlpha)});
        }
    }

    // Rivets
    DrawRivet((int)tl.x + (int)(4*s), (int)tl.y + (int)(4*s), s);
    DrawRivet((int)tr.x - (int)(4*s), (int)tr.y + (int)(4*s), s);
    DrawRivet((int)bl.x + (int)(4*s), (int)bl.y - (int)(4*s), s);
    DrawRivet((int)br.x - (int)(4*s), (int)br.y - (int)(4*s), s);
    DrawRivet((int)((tl.x + tr.x) / 2), (int)tl.y + (int)(3*s), s);
    DrawRivet((int)((bl.x + br.x) / 2), (int)bl.y - (int)(3*s), s);
    DrawRivet((int)((tl.x + bl.x) / 2), (int)((tl.y + bl.y) / 2), s * 0.7f);
    DrawRivet((int)((tr.x + br.x) / 2), (int)((tr.y + br.y) / 2), s * 0.7f);

    // === DISTANCE DIAL ===
    {
        int dialR = (int)(12 * s);
        int dialCX = (int)(bl.x + (vpX - bl.x) / 2 + 2 * s);
        int dialCY = (int)(vpY + vpH + (bl.y - vpY - vpH) / 2);

        DrawCircle(dialCX, dialCY, (float)(dialR + (int)(2*s)), housingBrass);
        DrawCircle(dialCX, dialCY, (float)dialR, (Color)COLOR_GAUGE_BG);

        for (int i = 0; i <= 8; i++) {
            float frac = (float)i / 8.0f;
            float a = (45.0f + 270.0f * frac) * DEG2RAD;
            float r1 = dialR * 0.65f;
            float r2 = dialR * 0.90f;
            DrawLine(dialCX + (int)(r1 * cosf(a)), dialCY + (int)(r1 * sinf(a)),
                     dialCX + (int)(r2 * cosf(a)), dialCY + (int)(r2 * sinf(a)),
                     (Color)COLOR_GAUGE_MARK);
        }

        float distNeedle = 0.3f + 0.15f * sinf(time * 1.2f) + 0.05f * sinf(time * 3.7f);
        float nAngle = (45.0f + 270.0f * distNeedle) * DEG2RAD;
        float nLen = dialR * 0.75f;
        DrawLineEx((Vector2){(float)dialCX, (float)dialCY},
                   (Vector2){dialCX + cosf(nAngle) * nLen, dialCY + sinf(nAngle) * nLen},
                   1.5f * s, (Color)COLOR_GAUGE_NEEDLE);
        DrawCircle(dialCX, dialCY, 2.0f * s, (Color)COLOR_BRASS_DIM);
        DrawCircle(dialCX, dialCY, 1.0f * s, (Color){50, 45, 35, 255});
        DrawCircleLines(dialCX, dialCY, (float)(dialR + (int)(2*s)), (Color)COLOR_BRASS);

        int dfs = (int)(5 * s);
        if (dfs < 4) dfs = 4;
        const char *dlbl = "DIST";
        int dlw = MeasureText(dlbl, dfs);
        DrawText(dlbl, dialCX - dlw / 2, dialCY - dialR - dfs - (int)(1*s), dfs, (Color){180, 50, 35, 240});
    }

    // === STEAM PUFFS ===
    {
        float ventX = pivotX - sinf(angle) * armLen * 0.7f;
        float ventY = pivotY + cosf(angle) * armLen * 0.7f;

        for (int i = 0; i < 6; i++) {
            float phase = time * 2.0f + (float)i * 1.1f;
            float life = fmodf(phase, 2.5f) / 2.5f;
            float alpha = (1.0f - life) * 0.7f;
            if (alpha <= 0.02f) continue;

            float riseY = life * 40.0f * s;
            float driftX = sinf(phase * 3.1f + i * 0.7f) * 10.0f * s;
            float puffSize = (3.0f + life * 8.0f) * s;
            int ppx = (int)(ventX + driftX);
            int ppy = (int)(ventY - riseY);

            DrawCircle(ppx, ppy, puffSize,
                (Color){220, 220, 215, (unsigned char)(alpha * 200)});
            DrawCircle(ppx + (int)(s), ppy - (int)(s), puffSize * 0.5f,
                (Color){245, 245, 240, (unsigned char)(alpha * 150)});
        }
    }

    // === STATUS INDICATOR LIGHTS ===
    {
        int dotR = (int)(2.5f * s);
        int lightY = (int)(tl.y + frame / 2);
        float gPulse = 0.6f + 0.4f * sinf(time * 3.0f);
        DrawCircle((int)(tl.x + 16 * s), lightY, (float)dotR, (Color){50, 45, 35, 255});
        DrawCircle((int)(tl.x + 16 * s), lightY, (float)(dotR - 1),
            (Color){(unsigned char)(60 * gPulse), (unsigned char)(200 * gPulse), (unsigned char)(60 * gPulse), 255});
        float aPulse = 0.5f + 0.5f * sinf(time * 5.0f);
        DrawCircle((int)(tl.x + 28 * s), lightY, (float)dotR, (Color){50, 45, 35, 255});
        DrawCircle((int)(tl.x + 28 * s), lightY, (float)(dotR - 1),
            (Color){(unsigned char)(220 * aPulse), (unsigned char)(160 * aPulse), (unsigned char)(30 * aPulse), 255});

        // Brass nameplate
        int pfs = (int)(frame * 0.55f);
        if (pfs < 6) pfs = 6;
        const char *plbl = "FUEHRERAUGE";
        int plw = MeasureText(plbl, pfs);
        int plateW = plw + (int)(16 * s);
        int plateH = pfs + (int)(4 * s);
        int plateX = vpX + vpW / 2 - plateW / 2;
        int plateY = (int)tl.y + frame / 2 - plateH / 2;
        DrawRectangle(plateX, plateY, plateW, plateH, housingBrass);
        DrawRectangleLinesEx((Rectangle){(float)plateX, (float)plateY, (float)plateW, (float)plateH},
            s * 0.5f, (Color)COLOR_BRASS);
        DrawText(plbl, plateX + plateW / 2 - plw / 2, plateY + plateH / 2 - pfs / 2,
            pfs, (Color){180, 40, 30, 255});
    }

    // --- Draw zoom texture through old-TV shader ---
    BeginShaderMode(zoomShader);
    DrawTexturePro(zoomTexture,
        (Rectangle){0, 0, (float)zoomTexture.width, -(float)zoomTexture.height},
        (Rectangle){(float)vpX, (float)vpY, (float)vpW, (float)vpH},
        (Vector2){0, 0}, 0.0f, WHITE);
    EndShaderMode();

    // --- Digital Targeting Overlay --- fades in with expand ---
    if (expandEase < 0.3f) return;
    float overlayAlpha = (expandEase - 0.3f) / 0.7f;
    if (overlayAlpha > 1.0f) overlayAlpha = 1.0f;

    int rcx = vpX + vpW / 2;
    int rcy = vpY + vpH / 2;

    unsigned char digA = (unsigned char)(overlayAlpha * 180);
    Color digMid    = {0, 200, 60, digA};
    Color digDim    = {0, 160, 50, (unsigned char)(overlayAlpha * 100)};
    Color digFaint  = {0, 120, 40, (unsigned char)(overlayAlpha * 60)};

    int px = (int)(2.0f * s);
    if (px < 1) px = 1;

    // Pixel cross with gap
    int gapPx = (int)(6 * s);
    int armPx = (int)(20 * s);
    DrawRectangle(rcx - armPx, rcy - px/2, armPx - gapPx, px, digMid);
    DrawRectangle(rcx + gapPx, rcy - px/2, armPx - gapPx, px, digMid);
    DrawRectangle(rcx - px/2, rcy - armPx, px, armPx - gapPx, digMid);
    DrawRectangle(rcx - px/2, rcy + gapPx, px, armPx - gapPx, digMid);

    // Center pip
    float blink = (sinf(time * 10.0f) > 0.0f) ? 1.0f : 0.6f;
    Color centerCol = {0, (unsigned char)(255 * blink * overlayAlpha), (unsigned char)(80 * blink * overlayAlpha), 240};
    DrawRectangle(rcx - px, rcy - px, px * 2, px * 2, centerCol);

    // Scanning line
    {
        float scanCycle = fmodf(time * 0.6f, 1.0f);
        int scanY = vpY + (int)(scanCycle * vpH);
        DrawRectangle(vpX, scanY, vpW, px, digFaint);
    }

    // Range markers
    for (int i = 1; i <= 4; i++) {
        int hashX = gapPx + i * (int)(4 * s);
        int hashH = (int)(3 * s);
        DrawRectangle(rcx - hashX - px/2, rcy - hashH, px, hashH * 2, digDim);
        DrawRectangle(rcx + hashX - px/2, rcy - hashH, px, hashH * 2, digDim);
    }

    // Elevation drop lines
    for (int i = 1; i <= 3; i++) {
        int dropY = gapPx + i * (int)(5 * s);
        int dropW = (int)(4 * s) - i;
        if (dropW < 2) dropW = 2;
        DrawRectangle(rcx - dropW, rcy + dropY, dropW * 2, px, digDim);
    }

    // Rotating digital bracket ring
    {
        float ringRad = 28.0f * s;
        float ringRot = time * 1.2f;
        int bracketArm = (int)(6 * s);
        int bracketGap = (int)(2 * s);

        for (int i = 0; i < 4; i++) {
            float ringAngle = ringRot + (float)i * PI * 0.5f;
            float cosA = cosf(ringAngle);
            float sinA = sinf(ringAngle);
            float rperpX = -sinA;
            float rperpY =  cosA;

            int rx1 = rcx + (int)((ringRad - bracketArm) * cosA);
            int ry1 = rcy + (int)((ringRad - bracketArm) * sinA);
            int rx2 = rcx + (int)(ringRad * cosA);
            int ry2 = rcy + (int)(ringRad * sinA);
            DrawLine(rx1, ry1, rx2, ry2, digMid);

            int cx1 = rx2 + (int)(bracketGap * rperpX);
            int cy1 = ry2 + (int)(bracketGap * rperpY);
            int cx2 = rx2 - (int)(bracketGap * rperpX);
            int cy2 = ry2 - (int)(bracketGap * rperpY);
            DrawLine(cx1, cy1, cx2, cy2, digMid);
        }
    }

    // Scrolling data readout
    {
        int fontSize = (int)(6 * s);
        if (fontSize < 5) fontSize = 5;

        int coordX = ((int)(time * 47.0f)) % 9999;
        int coordY = ((int)(time * 31.0f)) % 9999;
        char buf[32];
        snprintf(buf, sizeof(buf), "X:%04d Y:%04d", coordX, coordY);
        DrawText(buf, vpX + (int)(4 * s), vpY + vpH - fontSize - (int)(3 * s), fontSize, digDim);

        if (sinf(time * 4.0f) > -0.3f) {
            DrawText("TGT", vpX + (int)(4 * s), vpY + (int)(3 * s), fontSize, digMid);
        }

        int dist = 100 + ((int)(time * 13.0f)) % 900;
        snprintf(buf, sizeof(buf), "%dM", dist);
        int tw = MeasureText(buf, fontSize);
        DrawText(buf, vpX + vpW - tw - (int)(4 * s), vpY + (int)(3 * s), fontSize, digDim);
    }

    // Corner brackets
    {
        int cbLen = (int)(10 * s);
        int cbIn = (int)(4 * s);
        Color bc = digMid;
        DrawRectangle(vpX + cbIn, vpY + cbIn, cbLen, px, bc);
        DrawRectangle(vpX + cbIn, vpY + cbIn, px, cbLen, bc);
        DrawRectangle(vpX + vpW - cbIn - cbLen, vpY + cbIn, cbLen, px, bc);
        DrawRectangle(vpX + vpW - cbIn - px, vpY + cbIn, px, cbLen, bc);
        DrawRectangle(vpX + cbIn, vpY + vpH - cbIn - px, cbLen, px, bc);
        DrawRectangle(vpX + cbIn, vpY + vpH - cbIn - cbLen, px, cbLen, bc);
        DrawRectangle(vpX + vpW - cbIn - cbLen, vpY + vpH - cbIn - px, cbLen, px, bc);
        DrawRectangle(vpX + vpW - cbIn - px, vpY + vpH - cbIn - cbLen, px, cbLen, bc);
    }
}

// ============================================================================
// EKG Heartbeat Waveform Sample
// ============================================================================

static float EkgSample(float phase) {
    float p = phase - floorf(phase);

    if (p < 0.15f) {
        float t = p / 0.15f;
        return 0.15f * sinf(t * PI);
    }
    if (p < 0.25f) return 0.0f;
    if (p < 0.30f) {
        float t = (p - 0.25f) / 0.05f;
        return -0.2f * sinf(t * PI);
    }
    if (p < 0.38f) {
        float t = (p - 0.30f) / 0.08f;
        return 1.0f * sinf(t * PI);
    }
    if (p < 0.43f) {
        float t = (p - 0.38f) / 0.05f;
        return -0.3f * sinf(t * PI);
    }
    if (p < 0.55f) return 0.0f;
    if (p < 0.70f) {
        float t = (p - 0.55f) / 0.15f;
        return 0.25f * sinf(t * PI);
    }
    return 0.0f;
}

// ============================================================================
// Nixie Tube Digit — warm amber glow in dark box
// ============================================================================

static void DrawNixieDigit(int digit, float rollOffset, int x, int y, int w, int h, float s) {
    DrawRectangle(x, y, w, h, (Color){15, 10, 5, 220});
    DrawRectangleLines(x, y, w, h, (Color){60, 40, 20, 180});

    int glowPad = (int)(2 * s);
    DrawRectangle(x + glowPad, y + glowPad, w - glowPad * 2, h - glowPad * 2,
        (Color){40, 25, 5, 100});

    int fs = h * 3 / 4;
    if (fs < 8) fs = 8;
    char buf[2] = { '0' + (digit % 10), '\0' };

    int yOff = 0;
    if (rollOffset > 0) {
        yOff = (int)(rollOffset * h * 0.5f);
        if (digit % 2) yOff = -yOff;
    }

    int tx = x + w / 2 - MeasureText(buf, fs) / 2;
    int ty = y + h / 2 - fs / 2 + yOff;

    if (ty >= y - fs && ty <= y + h) {
        float flicker = 0.9f + 0.1f * sinf(GetTime() * 30.0f + digit * 1.7f);
        unsigned char r = (unsigned char)(255 * flicker);
        unsigned char g = (unsigned char)(180 * flicker);
        unsigned char b = (unsigned char)(50 * flicker);
        DrawText(buf, tx, ty, fs, (Color){r, g, b, 255});
    }
}

// ============================================================================
// Flicker Jitter — pseudo-random pixel offset for CRT feel
// ============================================================================

static int FlickerJitter(float time) {
    float v = sinf(time * 47.3f) * sinf(time * 13.7f);
    if (v > 0.85f) return 1;
    if (v > 0.95f) return 2;
    return 0;
}

// ============================================================================
// Frame Ornaments — brass corner brackets with rivets
// ============================================================================

static void DrawFrameOrnaments(int x, int y, int w, int h, Color col, float s) {
    int len = (int)(10 * s);

    float pulse = 0.7f + 0.3f * sinf((float)GetTime() * PI * HUD_PANEL_GLOW_SPEED);
    Color c = {col.r, col.g, col.b, (unsigned char)(col.a * pulse)};

    DrawLine(x, y, x + len, y, c);
    DrawLine(x, y, x, y + len, c);
    DrawRivet(x, y, s * 0.6f);

    DrawLine(x + w, y, x + w - len, y, c);
    DrawLine(x + w, y, x + w, y + len, c);
    DrawRivet(x + w, y, s * 0.6f);

    DrawLine(x, y + h, x + len, y + h, c);
    DrawLine(x, y + h, x, y + h - len, c);
    DrawRivet(x, y + h, s * 0.6f);

    DrawLine(x + w, y + h, x + w - len, y + h, c);
    DrawLine(x + w, y + h, x + w, y + h - len, c);
    DrawRivet(x + w, y + h, s * 0.6f);
}

// ============================================================================
// Extract digits from an integer (up to 4 digits)
// ============================================================================

static void ExtractDigits(int value, int out[4]) {
    if (value < 0) value = 0;
    if (value > 9999) value = 9999;
    out[0] = (value / 1000) % 10;
    out[1] = (value / 100) % 10;
    out[2] = (value / 10) % 10;
    out[3] = value % 10;
}

// ============================================================================
// HudInit
// ============================================================================

void HudInit(HudState *state) {
    if (!state) return;
    memset(state, 0, sizeof(HudState));
    state->interferenceCountdown = HUD_INTERFERENCE_MIN +
        ((float)rand() / RAND_MAX) * (HUD_INTERFERENCE_MAX - HUD_INTERFERENCE_MIN);
    state->prevAmmo = -999;
    state->lastWave = -1;
    state->prevKillCount = 0;
    for (int i = 0; i < 4; i++) {
        state->displayDigits[i] = 0;
        state->targetDigits[i] = 0;
    }
    for (int i = 0; i < HUD_EKG_BUFFER_SIZE; i++) {
        state->ekgBuffer[i] = 0.0f;
    }
    state->initialized = true;
}

// ============================================================================
// HudUpdate
// ============================================================================

void HudUpdate(HudState *state, Player *player, Weapon *weapon, Game *game, float dt) {
    if (!state) return;
    if (!state->initialized) HudInit(state);

    // --- Crosshair ---
    state->crossRotation += HUD_CROSSHAIR_ROTATE_SPEED * dt;
    if (state->crossRotation > 360.0f) state->crossRotation -= 360.0f;

    if (state->crossSpread > 0) {
        state->crossSpread -= state->crossSpread * HUD_CROSSHAIR_SPREAD_DECAY * dt;
        if (state->crossSpread < 0.1f) state->crossSpread = 0;
    }
    if (weapon->muzzleFlash > 0.8f) {
        state->crossSpread = 8.0f;
    }
    if (state->killConfirmTimer > 0) state->killConfirmTimer -= dt;

    // --- Health EKG ---
    float hpPct = (player->maxHealth > 0.001f) ? (player->health / player->maxHealth) : 0.0f;
    float ekgSpeed = HUD_EKG_BASE_SPEED + (1.0f - hpPct) * (HUD_EKG_CRIT_SPEED - HUD_EKG_BASE_SPEED);
    state->ekgPhase += ekgSpeed * dt;
    if (state->ekgPhase > 1.0f) state->ekgPhase -= 1.0f;

    // --- Compass ---
    float targetYaw = player->yaw;
    float yawDiff = targetYaw - state->compassYaw;
    while (yawDiff > PI) yawDiff -= 2.0f * PI;
    while (yawDiff < -PI) yawDiff += 2.0f * PI;
    state->compassYaw += yawDiff * 10.0f * dt;

    // --- Nixie ammo ---
    int ammo = WeaponGetAmmo(weapon);
    bool reloading = WeaponIsReloading(weapon);

    if (ammo != state->prevAmmo || state->prevAmmo == -999) {
        int newDigits[4];
        ExtractDigits(ammo < 0 ? 0 : ammo, newDigits);
        for (int i = 0; i < 4; i++) {
            if (newDigits[i] != state->targetDigits[i] || state->prevAmmo == -999) {
                state->targetDigits[i] = newDigits[i];
                if (state->prevAmmo != -999) {
                    state->digitRoll[i] = 1.0f;
                } else {
                    state->displayDigits[i] = newDigits[i];
                }
            }
        }
        state->prevAmmo = ammo;
    }

    state->reloadSpin = reloading;
    state->nixieRollTimer += dt;

    for (int i = 0; i < 4; i++) {
        if (state->digitRoll[i] > 0) {
            float speed = reloading ? HUD_NIXIE_RELOAD_SPIN : HUD_NIXIE_ROLL_SPEED;
            state->digitRoll[i] -= speed * dt;
            if (state->digitRoll[i] <= 0) {
                state->digitRoll[i] = 0;
                state->displayDigits[i] = state->targetDigits[i];
            }
        }
        if (reloading && state->digitRoll[i] <= 0) {
            state->digitRoll[i] = 1.0f;
            state->displayDigits[i] = (state->displayDigits[i] + 1) % 10;
        }
    }

    // --- Low ammo blink ---
    int maxAmmo = WeaponGetMaxAmmo(weapon);
    if (maxAmmo > 0 && ammo >= 0) {
        float ratio = (float)ammo / (float)maxAmmo;
        if (ratio < HUD_LOW_AMMO_THRESHOLD) {
            state->lowAmmoFlash += HUD_LOW_AMMO_BLINK_SPEED * dt;
        } else {
            state->lowAmmoFlash = 0;
        }
    }

    // --- Radar ---
    state->radarAngle += (HUD_RADAR_RPM / 60.0f) * 2.0f * PI * dt;
    if (state->radarAngle > 2.0f * PI) state->radarAngle -= 2.0f * PI;

    for (int i = 0; i < state->blipCount; i++) {
        state->blipTimers[i] -= dt;
    }

    // --- Wave splash ---
    if (game->wave != state->lastWave && game->wave > 0) {
        state->waveSplashTimer = HUD_WAVE_SPLASH_HOLD + HUD_WAVE_SPLASH_SHRINK;
        state->waveSplashWave = game->wave;
        state->waveTypewriterChars = 0;
        state->waveTypewriterTimer = 0;
        state->lastWave = game->wave;
    }
    if (state->waveSplashTimer > 0) {
        state->waveSplashTimer -= dt;
        // Typewriter effect
        state->waveTypewriterTimer += dt;
        if (state->waveTypewriterTimer > 0.06f) {
            state->waveTypewriterTimer = 0;
            state->waveTypewriterChars++;
        }
    }

    // --- Kill feed ---
    if (game->killCount > state->prevKillCount) {
        int newKills = game->killCount - state->prevKillCount;
        for (int k = 0; k < newKills && k < HUD_KILL_FEED_SLOTS; k++) {
            state->killFeedTimers[state->killFeedHead] = HUD_KILL_FEED_LIFE;
            state->killFeedHead = (state->killFeedHead + 1) % HUD_KILL_FEED_SLOTS;
        }
        state->killConfirmTimer = HUD_CROSSHAIR_KILL_FLASH;

        // Streak tracking
        if (state->streakTimer > 0) {
            state->streakCount += newKills;
        } else {
            state->streakCount = newKills;
        }
        state->streakTimer = HUD_STREAK_WINDOW;
        if (state->streakCount >= 3) {
            state->streakDisplayTimer = HUD_STREAK_DISPLAY;
            state->streakLevel = state->streakCount;
        }

        state->prevKillCount = game->killCount;
    }

    for (int i = 0; i < HUD_KILL_FEED_SLOTS; i++) {
        if (state->killFeedTimers[i] > 0) state->killFeedTimers[i] -= dt;
    }
    if (state->streakTimer > 0) state->streakTimer -= dt;
    if (state->streakDisplayTimer > 0) state->streakDisplayTimer -= dt;

    // --- Scanline interference ---
    if (!state->interferenceActive) {
        state->interferenceCountdown -= dt;
        if (state->interferenceCountdown <= 0) {
            state->interferenceActive = true;
            state->interferenceY = 0;
            state->interferenceIntensity = 0.5f + ((float)rand() / RAND_MAX) * 0.5f;
            state->interferenceTimer = 0;
        }
    } else {
        state->interferenceY += dt / HUD_INTERFERENCE_SPEED;
        state->interferenceTimer += dt;
        if (state->interferenceY > 1.0f) {
            state->interferenceActive = false;
            state->interferenceCountdown = HUD_INTERFERENCE_MIN +
                ((float)rand() / RAND_MAX) * (HUD_INTERFERENCE_MAX - HUD_INTERFERENCE_MIN);
        }
    }

    // --- Panel glow ---
    state->panelGlowPhase += HUD_PANEL_GLOW_SPEED * dt * 2.0f * PI;
    if (state->panelGlowPhase > 2.0f * PI) state->panelGlowPhase -= 2.0f * PI;
}

// ============================================================================
// Dieselpunk Compass — brass tinted, above radar
// ============================================================================

static void DrawCompass(HudState *state, int radarCX, int radarR, int sh, float s) {
    float yaw = state->compassYaw;
    float t = (float)GetTime();
    int jit = FlickerJitter(t + 2.0f);

    float yawDeg = fmodf(-yaw * 180.0f / PI + 360.0f, 360.0f);

    const char *dirNames[] = {"N", "NO", "O", "SO", "S", "SW", "W", "NW"};
    int dirIdx = (int)((yawDeg + 22.5f) / 45.0f) % 8;

    char buf[16];
    snprintf(buf, sizeof(buf), "%s %03d", dirNames[dirIdx], (int)yawDeg);

    int fs = (int)(10 * s);
    if (fs < 7) fs = 7;
    int tw = MeasureText(buf, fs);

    // Place just above the radar
    int compX = radarCX - tw / 2;
    int compY = sh - (int)(sh / 30.0f) - sh / 14 - (int)(65 * s) - radarR - fs - (int)(10 * s);

    int padX = (int)(6 * s);
    int padY = (int)(2 * s);
    DrawRectangle(compX - padX, compY - padY, tw + padX * 2, fs + padY * 2, (Color)COLOR_GAUGE_BG);
    DrawRectangleLinesEx((Rectangle){(float)(compX - padX), (float)(compY - padY),
        (float)(tw + padX * 2), (float)(fs + padY * 2)}, s, (Color)COLOR_BRASS_DIM);

    DrawText(buf, compX + jit, compY, fs, (Color)COLOR_BRASS_BRIGHT);

    DrawRivet(compX - padX, compY + fs / 2, s * 0.5f);
    DrawRivet(compX + tw + padX, compY + fs / 2, s * 0.5f);
}

// ============================================================================
// Radar Sweep Display — flat-world X/Z projection
// ============================================================================

void HudDrawRadar(HudState *state, Player *player, HudRadarBlip *blips, int blipCount, int sw, int sh) {
    if (!state || !player) return;
    float s = (float)sh / (float)WINDOW_H;
    float t = (float)GetTime();

    int radarR = (int)(45 * s);
    int radarCX = sw - (int)(70 * s);
    int radarCY = sh - sh / 30 - sh / 14 - (int)(65 * s);

    // Draw compass above radar
    DrawCompass(state, radarCX, radarR, sh, s);

    // Dark metal background
    DrawCircle(radarCX, radarCY, (float)(radarR + 4), (Color)COLOR_DARK_METAL);
    DrawCircle(radarCX, radarCY, (float)(radarR + 2), (Color){50, 45, 38, 240});
    DrawCircle(radarCX, radarCY, (float)radarR, (Color)COLOR_GAUGE_BG);

    // Range rings — brass tinted
    for (int i = 1; i <= 3; i++) {
        float ringR = radarR * (float)i / 3.0f;
        DrawCircleLines(radarCX, radarCY, ringR, (Color){100, 85, 50, 80});
    }

    // Cross lines (N/S/E/W) — brass
    Color crossLine = {100, 85, 50, 60};
    DrawLine(radarCX, radarCY - radarR, radarCX, radarCY + radarR, crossLine);
    DrawLine(radarCX - radarR, radarCY, radarCX + radarR, radarCY, crossLine);

    // Flat world: use simple X/Z distance with player yaw for rotation
    float cosYaw = cosf(player->yaw);
    float sinYaw = sinf(player->yaw);

    state->blipCount = 0;
    for (int i = 0; i < blipCount && i < HUD_MAX_RADAR_BLIPS && state->blipCount < HUD_MAX_RADAR_BLIPS; i++) {
        float dx = blips[i].position.x - player->position.x;
        float dz = blips[i].position.z - player->position.z;
        float dist = sqrtf(dx * dx + dz * dz);

        if (dist > HUD_RADAR_RANGE) continue;

        float normDist = dist / HUD_RADAR_RANGE;

        // Rotate into player-relative coords (forward = up on radar)
        float relX = dx * cosYaw - dz * sinYaw;
        float relZ = dx * sinYaw + dz * cosYaw;
        float relAngle = atan2f(relX, relZ);

        float bx = sinf(relAngle) * normDist * radarR;
        float by = -cosf(relAngle) * normDist * radarR;

        int idx = state->blipCount;
        state->blipPositions[idx] = (Vector2){(float)radarCX + bx, (float)radarCY + by};
        state->blipFactions[idx] = blips[i].faction;
        state->blipIsLander[idx] = blips[i].isLander;
        state->blipTimers[idx] = HUD_RADAR_FADE;
        state->blipCount++;
    }

    // Draw blips
    for (int i = 0; i < state->blipCount; i++) {
        float timer = state->blipTimers[i];
        if (timer <= 0) continue;

        float fadeAlpha = timer / HUD_RADAR_FADE;
        Vector2 pos = state->blipPositions[i];

        float bdx = pos.x - radarCX;
        float bdy = pos.y - radarCY;
        if (bdx * bdx + bdy * bdy > (float)(radarR * radarR)) continue;

        if (state->blipIsLander[i]) {
            int triSize = (int)(4 * s);
            Color lc = {220, 180, 50, (unsigned char)(fadeAlpha * 255)};
            DrawTriangle(
                (Vector2){pos.x, pos.y - triSize},
                (Vector2){pos.x - triSize, pos.y + triSize},
                (Vector2){pos.x + triSize, pos.y + triSize},
                lc
            );
        } else {
            int dotR = (int)(2 * s);
            Color ec;
            if (state->blipFactions[i] == 0) {
                ec = (Color){220, 60, 40, (unsigned char)(fadeAlpha * 220)};  // Soviet = red
            } else {
                ec = (Color){60, 120, 220, (unsigned char)(fadeAlpha * 220)};  // American = blue
            }
            DrawCircle((int)pos.x, (int)pos.y, (float)dotR, ec);
        }
    }

    // Sweep line — amber/brass phosphor
    float sweepAngle = state->radarAngle;
    int sweepEndX = radarCX + (int)(radarR * sinf(sweepAngle));
    int sweepEndY = radarCY - (int)(radarR * cosf(sweepAngle));
    DrawLine(radarCX, radarCY, sweepEndX, sweepEndY, (Color)COLOR_BRASS_BRIGHT);

    // Sweep trail (phosphor afterglow)
    for (int i = 1; i <= 8; i++) {
        float trailAngle = sweepAngle - (float)i * 0.08f;
        int tx = radarCX + (int)(radarR * sinf(trailAngle));
        int ty = radarCY - (int)(radarR * cosf(trailAngle));
        unsigned char ta = (unsigned char)(180 - i * 20);
        DrawLine(radarCX, radarCY, tx, ty, (Color){180, 150, 60, ta});
    }

    // Player center dot — brass
    DrawCircle(radarCX, radarCY, 2.0f * s, (Color)COLOR_BRASS_BRIGHT);

    // Outer ring — double brass border
    DrawCircleLines(radarCX, radarCY, (float)radarR, (Color)COLOR_BRASS_DIM);
    DrawCircleLines(radarCX, radarCY, (float)(radarR + 1), (Color){120, 100, 50, 100});

    // Rivets at cardinal points
    DrawRivet(radarCX, radarCY - radarR - (int)(2 * s), s * 0.5f);
    DrawRivet(radarCX, radarCY + radarR + (int)(2 * s), s * 0.5f);
    DrawRivet(radarCX - radarR - (int)(2 * s), radarCY, s * 0.5f);
    DrawRivet(radarCX + radarR + (int)(2 * s), radarCY, s * 0.5f);

    // Subtle scan line
    float scanPos = fmodf(t * 0.3f, 1.0f);
    int scanY = radarCY - radarR + (int)(scanPos * radarR * 2);
    float scanDist = fabsf(scanPos - 0.5f) * 2.0f;
    DrawLine(radarCX - radarR, scanY, radarCX + radarR, scanY,
        (Color){120, 100, 50, (unsigned char)(30 * (1.0f - scanDist))});
}

// ============================================================================
// EKG Heart Display — scrolling waveform behind health gauge
// ============================================================================

void HudDrawEKG(HudState *state, float health, float maxHealth, int x, int y, int w, int h) {
    if (!state) return;

    // Green waveform line on dark background
    DrawRectangle(x, y, w, h, (Color){10, 15, 10, 160});
    DrawRectangleLinesEx((Rectangle){(float)x, (float)y, (float)w, (float)h},
        1.0f, (Color){40, 80, 40, 120});

    float hpPct = (maxHealth > 0.001f) ? (health / maxHealth) : 0.0f;
    float phase = state->ekgPhase;
    int midY = y + h / 2;

    // Draw scrolling EKG waveform
    int step = 2;
    for (int px = 0; px < w - step; px += step) {
        float frac1 = (float)px / (float)w;
        float frac2 = (float)(px + step) / (float)w;

        float sample1 = EkgSample(phase + frac1);
        float sample2 = EkgSample(phase + frac2);

        // Scale amplitude with health
        float amp = (float)h * 0.35f * hpPct;
        int y1 = midY - (int)(sample1 * amp);
        int y2 = midY - (int)(sample2 * amp);

        // Clamp to box
        if (y1 < y) y1 = y;
        if (y1 > y + h) y1 = y + h;
        if (y2 < y) y2 = y;
        if (y2 > y + h) y2 = y + h;

        // Green line, brighter at peaks
        unsigned char ga = (unsigned char)(150 + 105 * fabsf(sample1));
        Color lineCol = {30, ga, 30, 220};
        DrawLine(x + px, y1, x + px + step, y2, lineCol);
    }

    // Center baseline (dim)
    DrawLine(x, midY, x + w, midY, (Color){30, 60, 30, 60});
}

// ============================================================================
// Nixie Tube Ammo Display — rolling digits with low-ammo blink
// ============================================================================

void HudDrawNixie(HudState *state, int value, int x, int y, float s) {
    if (!state) return;
    int maxAmmo = value; // not used for rendering, just drawing the state digits
    (void)maxAmmo;

    int numDigits = 4;
    int digitW = (int)(18 * s);
    int digitH = (int)(26 * s);
    int gap = (int)(3 * s);

    // Low ammo blink
    bool blinkOff = false;
    if (state->lowAmmoFlash > 0) {
        blinkOff = sinf(state->lowAmmoFlash * 2.0f * PI) > 0.5f;
    }

    for (int i = 0; i < numDigits; i++) {
        int dx = x + i * (digitW + gap);
        if (blinkOff) {
            // Draw empty dark tube during blink-off phase
            DrawRectangle(dx, y, digitW, digitH, (Color){15, 10, 5, 220});
            DrawRectangleLines(dx, y, digitW, digitH, (Color){60, 40, 20, 180});
        } else {
            DrawNixieDigit(state->displayDigits[i], state->digitRoll[i], dx, y, digitW, digitH, s);
        }
    }
}

// ============================================================================
// Wave Splash Transition
// ============================================================================

void HudDrawWaveSplash(HudState *state, int sw, int sh) {
    if (!state) return;
    if (state->waveSplashTimer <= 0) return;

    int cx = sw / 2;
    float s = (float)sh / (float)WINDOW_H;
    float t = (float)GetTime();

    char waveText[32];
    snprintf(waveText, sizeof(waveText), "WELLE %d", state->waveSplashWave);

    int fullLen = (int)strlen(waveText);
    int showChars = state->waveTypewriterChars;
    if (showChars > fullLen) showChars = fullLen;

    char displayText[32];
    memcpy(displayText, waveText, showChars);
    displayText[showChars] = '\0';

    float remaining = state->waveSplashTimer;
    bool shrinking = remaining < HUD_WAVE_SPLASH_SHRINK;

    if (!shrinking) {
        int fs = sh / 5;
        int tw = MeasureText(displayText, fs);
        int ty = sh / 3;

        float flashAlpha = 0.3f * (remaining / (HUD_WAVE_SPLASH_HOLD + HUD_WAVE_SPLASH_SHRINK));
        DrawRectangle(0, ty - sh / 10, sw, sh / 5 + sh / 10,
            (Color){60, 20, 10, (unsigned char)(flashAlpha * 255)});

        // Brass rules
        DrawLine(cx - tw / 2 - (int)(30 * s), ty - (int)(8 * s),
            cx + tw / 2 + (int)(30 * s), ty - (int)(8 * s), (Color)COLOR_BRASS_DIM);
        DrawLine(cx - tw / 2 - (int)(30 * s), ty + fs + (int)(8 * s),
            cx + tw / 2 + (int)(30 * s), ty + fs + (int)(8 * s), (Color)COLOR_BRASS_DIM);

        int jit = (int)(sinf(t * 15.0f) * 2.0f);
        DrawText(displayText, cx - tw / 2 + jit, ty, fs, (Color)COLOR_GAUGE_NEEDLE);

        // Typewriter cursor blink
        if (showChars < fullLen) {
            int cursorX = cx - tw / 2 + MeasureText(displayText, fs);
            float blink = sinf(t * 12.0f);
            if (blink > 0) {
                DrawRectangle(cursorX + (int)(2 * s), ty, (int)(fs * 0.6f), fs,
                    (Color){200, 60, 30, 180});
            }
        }
    } else {
        // Shrink transition to corner ticker
        float prog = 1.0f - (remaining / HUD_WAVE_SPLASH_SHRINK);
        int largeFontSize = sh / 5;
        int smallFontSize = sh / 14 / 2;
        int fs = largeFontSize + (int)((smallFontSize - largeFontSize) * prog);

        int barInset = sw / 7;
        int topL = barInset;
        int topW = (sw - barInset) - topL;
        int topY = sh / 20;
        int topH = sh / 14;
        int targetX = topL + topW / 2;
        int targetY = topY + topH / 2 - smallFontSize / 2;
        int startY = sh / 3;
        int startX = cx;

        int tw = MeasureText(waveText, fs);
        int drawX = startX + (int)((targetX - startX) * prog) - tw / 2;
        int drawY = startY + (int)((targetY - startY) * prog);

        unsigned char a = (unsigned char)(255 * (1.0f - prog * 0.3f));
        DrawText(waveText, drawX, drawY, fs, (Color){220, 60, 30, a});
    }
}

// ============================================================================
// Kill Feed — 5-slot notification list, top-right
// ============================================================================

void HudDrawKillFeed(HudState *state, int sw, int sh) {
    if (!state) return;
    float s = (float)sh / (float)WINDOW_H;
    float t = (float)GetTime();

    int feedX = sw - (int)(140 * s);
    int feedY = sh / 20 + sh / 14 + (int)(10 * s);
    int entryH = (int)(14 * s);
    int fs = (int)(9 * s);
    if (fs < 6) fs = 6;

    int displayed = 0;
    for (int i = 0; i < HUD_KILL_FEED_SLOTS; i++) {
        int idx = (state->killFeedHead - 1 - i + HUD_KILL_FEED_SLOTS) % HUD_KILL_FEED_SLOTS;
        float timer = state->killFeedTimers[idx];
        if (timer <= 0) continue;

        float alpha = (timer > 0.3f) ? 1.0f : timer / 0.3f;
        unsigned char a = (unsigned char)(alpha * 220);

        int ey = feedY + displayed * (entryH + (int)(2 * s));
        int jit = FlickerJitter(t + i * 1.3f);

        // Dark backing
        int labelW = (int)(100 * s);
        DrawRectangle(feedX, ey, labelW, entryH, (Color){15, 12, 8, (unsigned char)(a * 0.6f)});

        // Kill marker
        const char *killTxt = "FEIND ELIMINIERT";
        DrawText(killTxt, feedX + (int)(4 * s) + jit, ey + (int)(2 * s), fs,
            (Color){220, 60, 30, a});

        // Brass side accent
        DrawLine(feedX, ey, feedX, ey + entryH, (Color){180, 150, 60, a});

        displayed++;
    }

    // Streak counter
    if (state->streakDisplayTimer > 0 && state->streakLevel >= 3) {
        float alpha = (state->streakDisplayTimer > 0.5f) ? 1.0f : state->streakDisplayTimer / 0.5f;
        unsigned char a = (unsigned char)(alpha * 255);

        char streakBuf[32];
        snprintf(streakBuf, sizeof(streakBuf), "KILLSTREAK x%d", state->streakLevel);
        int streakFs = (int)(12 * s);
        int streakW = MeasureText(streakBuf, streakFs);
        int streakX = sw - streakW - (int)(20 * s);
        int streakY = feedY + displayed * (entryH + (int)(2 * s)) + (int)(8 * s);

        // Pulsing gold background
        float pulse = 0.5f + 0.5f * sinf(t * 10.0f);
        DrawRectangle(streakX - (int)(6 * s), streakY - (int)(2 * s),
            streakW + (int)(12 * s), streakFs + (int)(4 * s),
            (Color){80, 60, 20, (unsigned char)(a * pulse * 0.5f)});
        DrawRectangleLinesEx(
            (Rectangle){(float)(streakX - (int)(6 * s)), (float)(streakY - (int)(2 * s)),
                (float)(streakW + (int)(12 * s)), (float)(streakFs + (int)(4 * s))},
            s, (Color){220, 180, 50, a});

        DrawText(streakBuf, streakX, streakY, streakFs, (Color){255, 220, 80, a});
    }
}

// ============================================================================
// Scanline Interference — horizontal bands that drift vertically
// ============================================================================

void HudDrawInterference(HudState *state, int sw, int sh) {
    if (!state) return;
    if (!state->interferenceActive) return;

    float t = state->interferenceTimer;
    float intensity = state->interferenceIntensity;

    // Draw 8-15 scan lines at various positions
    int numLines = 8 + (int)(intensity * 7);
    for (int i = 0; i < numLines; i++) {
        // Each line drifts at slightly different speed
        float linePhase = (float)i / (float)numLines;
        float yPos = fmodf(state->interferenceY + linePhase + sinf(t * 0.7f + i * 0.5f) * 0.1f, 1.0f);
        int lineY = (int)(yPos * sh);

        // Transparency varies with position and intensity
        float distFromCenter = fabsf(yPos - 0.5f) * 2.0f;
        unsigned char a = (unsigned char)(intensity * 60 * (1.0f - distFromCenter * 0.5f));

        // Dark transparent overlay line
        DrawRectangle(0, lineY, sw, 2, (Color){20, 15, 10, a});
        // Subtle warm-tinted edge
        DrawRectangle(0, lineY + 2, sw, 1, (Color){180, 160, 120, (unsigned char)(a * 0.3f)});
    }

    // Wider dark band at the main interference position
    int bandY = (int)(state->interferenceY * sh);
    unsigned char bandA = (unsigned char)(intensity * 100 * (1.0f - fabsf(state->interferenceY - 0.5f) * 2.0f));
    int stripH = 8;
    DrawRectangle(0, bandY, sw, 3, (Color){200, 180, 140, bandA});
    DrawRectangle(0, bandY - stripH, sw, stripH, (Color){30, 15, 5, (unsigned char)(bandA * 0.15f)});
    DrawRectangle(0, bandY + 3, sw, stripH, (Color){30, 10, 5, (unsigned char)(bandA * 0.15f)});
}

// ============================================================================
// March Radio Indicator — small brass box below radar showing current song
// ============================================================================

void HudDrawMarchRadio(const char *marchName, int sw, int sh) {
    if (!marchName) return;
    (void)sw;
    if (marchName[0] == '\0') return;

    float s = (float)sh / (float)WINDOW_H;
    float t = (float)GetTime();

    // Position: bottom-left, inset from edge (opposite side from radar)
    int radioW = (int)(105 * s);
    int radioH = (int)(34 * s);
    int radioX = (int)(16 * s);
    int radioY = sh - (int)(sh / 30.0f) - sh / 14 - radioH - (int)(8 * s);

    // === Radio body: dark metal with double border ===
    DrawRectangle(radioX, radioY, radioW, radioH, (Color){18, 15, 12, 230});
    // Inner slightly lighter panel
    int in = (int)(2 * s);
    DrawRectangle(radioX + in, radioY + in, radioW - in * 2, radioH - in * 2,
        (Color)COLOR_GAUGE_BG);

    // Outer brass border
    float pulse = 0.7f + 0.3f * sinf(t * 2.0f);
    Color border = {180, 150, 70, (unsigned char)(220 * pulse)};
    DrawRectangleLinesEx((Rectangle){(float)radioX, (float)radioY,
        (float)radioW, (float)radioH}, s * 1.5f, border);
    // Inner border
    DrawRectangleLinesEx((Rectangle){(float)(radioX + in + 1), (float)(radioY + in + 1),
        (float)(radioW - in * 2 - 2), (float)(radioH - in * 2 - 2)},
        s * 0.8f, (Color){120, 100, 50, 100});

    // === Antenna: thin brass line sticking up from top-right ===
    int antX = radioX + radioW - (int)(12 * s);
    int antBase = radioY;
    int antTop = radioY - (int)(16 * s);
    DrawLineEx((Vector2){(float)antX, (float)antBase},
        (Vector2){(float)(antX - (int)(3 * s)), (float)antTop}, s * 1.2f,
        (Color){170, 140, 65, (unsigned char)(200 * pulse)});
    // Antenna tip ball
    DrawCircle(antX - (int)(3 * s), antTop, 2.0f * s, (Color){200, 170, 80, 220});

    // === Corner rivets ===
    int rv = (int)(4 * s);
    DrawRivet(radioX + rv, radioY + rv, s * 0.5f);
    DrawRivet(radioX + radioW - rv, radioY + rv, s * 0.5f);
    DrawRivet(radioX + rv, radioY + radioH - rv, s * 0.5f);
    DrawRivet(radioX + radioW - rv, radioY + radioH - rv, s * 0.5f);

    // === Speaker grille (left third) — vertical slot pattern ===
    int grX0 = radioX + (int)(6 * s);
    int grW = (int)(20 * s);
    int grY0 = radioY + (int)(6 * s);
    int grY1 = radioY + radioH - (int)(6 * s);
    // Background for speaker area
    DrawRectangle(grX0 - 1, grY0, grW + 2, grY1 - grY0, (Color){10, 8, 6, 200});
    // Vertical grille slots
    int slotGap = (int)(3 * s);
    if (slotGap < 2) slotGap = 2;
    for (int gx = grX0 + 1; gx < grX0 + grW; gx += slotGap) {
        DrawLine(gx, grY0 + 1, gx, grY1 - 1, (Color){80, 70, 45, 140});
    }

    // === "FUNK" label — top center, tiny brass ===
    int labelFs = (int)(7 * s);
    if (labelFs < 5) labelFs = 5;
    const char *label = "FUNK";
    int labelW = MeasureText(label, labelFs);
    int labelX = radioX + (int)(30 * s) + (radioW - (int)(30 * s)) / 2 - labelW / 2;
    DrawText(label, labelX, radioY + (int)(3 * s), labelFs, (Color){160, 130, 60, 180});

    // === Tuning dial — tiny circle right of speaker ===
    int dialX = grX0 + grW + (int)(6 * s);
    int dialY = radioY + radioH / 2 + (int)(4 * s);
    int dialR = (int)(3 * s);
    DrawCircle(dialX, dialY, (float)(dialR + 1), (Color){100, 85, 55, 200});
    DrawCircle(dialX, dialY, (float)dialR, (Color){60, 50, 35, 255});
    // Dial indicator line (rotates slowly)
    float dialAngle = t * 0.5f;
    DrawLine(dialX, dialY,
        dialX + (int)(dialR * cosf(dialAngle)),
        dialY + (int)(dialR * sinf(dialAngle)),
        (Color){200, 170, 80, 200});

    // === Song name display — right portion, warm amber nixie glow ===
    int nameFs = (int)(10 * s);
    if (nameFs < 7) nameFs = 7;
    float flicker = 0.85f + 0.15f * sinf(t * 25.0f + 3.1f);
    unsigned char nr = (unsigned char)(255 * flicker);
    unsigned char ng = (unsigned char)(200 * flicker);
    unsigned char nb = (unsigned char)(60 * flicker);

    // Display area: right of dial to right edge
    int dispX = dialX + dialR + (int)(5 * s);
    int dispW = radioX + radioW - (int)(6 * s) - dispX;
    int nameW = MeasureText(marchName, nameFs);
    int nameX = dispX;
    if (nameW < dispW) nameX += (dispW - nameW) / 2;
    int nameY = radioY + radioH / 2 - nameFs / 2 + (int)(1 * s);

    // Faint glow behind text
    DrawRectangle(dispX - 1, nameY - 1, dispW + 2, nameFs + 2, (Color){30, 20, 5, 100});
    DrawText(marchName, nameX, nameY, nameFs, (Color){nr, ng, nb, 240});
}
