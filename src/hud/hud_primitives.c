// Reusable dieselpunk HUD drawing primitives:
// rivets, iron crosses, eagle wings, art deco bar frames, analog gauges, tickers.

#include "hud_primitives.h"
#include <math.h>

// ---- Rivet: two concentric circles for a 3D bolt look ----
void HudDrawRivet(int x, int y, float s) {
    int r = (int)(2.5f * s);
    if (r < 1) r = 1;
    DrawCircle(x, y, (float)r, (Color){140, 125, 95, 255});
    DrawCircle(x, y, (float)(r - 1 > 0 ? r - 1 : 1), (Color){100, 90, 70, 255});
}

// ---- Iron Cross: four flared arms drawn as triangle pairs ----
void HudDrawIronCross(int cx, int cy, int size, Color col) {
    int armW = size / 3;
    int tipW = size / 2;
    int armLen = size;
    if (armW < 1) armW = 1;
    if (tipW < 2) tipW = 2;

    // Right arm
    DrawTriangle(
        (Vector2){(float)(cx + armW / 2), (float)(cy - armW / 2)},
        (Vector2){(float)(cx + armLen), (float)(cy - tipW / 2)},
        (Vector2){(float)(cx + armLen), (float)(cy + tipW / 2)}, col);
    DrawTriangle(
        (Vector2){(float)(cx + armW / 2), (float)(cy - armW / 2)},
        (Vector2){(float)(cx + armLen), (float)(cy + tipW / 2)},
        (Vector2){(float)(cx + armW / 2), (float)(cy + armW / 2)}, col);
    // Left arm
    DrawTriangle(
        (Vector2){(float)(cx - armW / 2), (float)(cy + armW / 2)},
        (Vector2){(float)(cx - armLen), (float)(cy + tipW / 2)},
        (Vector2){(float)(cx - armLen), (float)(cy - tipW / 2)}, col);
    DrawTriangle(
        (Vector2){(float)(cx - armW / 2), (float)(cy + armW / 2)},
        (Vector2){(float)(cx - armLen), (float)(cy - tipW / 2)},
        (Vector2){(float)(cx - armW / 2), (float)(cy - armW / 2)}, col);
    // Top arm
    DrawTriangle(
        (Vector2){(float)(cx + armW / 2), (float)(cy - armW / 2)},
        (Vector2){(float)(cx - armW / 2), (float)(cy - armW / 2)},
        (Vector2){(float)(cx - tipW / 2), (float)(cy - armLen)}, col);
    DrawTriangle(
        (Vector2){(float)(cx + armW / 2), (float)(cy - armW / 2)},
        (Vector2){(float)(cx - tipW / 2), (float)(cy - armLen)},
        (Vector2){(float)(cx + tipW / 2), (float)(cy - armLen)}, col);
    // Bottom arm
    DrawTriangle(
        (Vector2){(float)(cx - armW / 2), (float)(cy + armW / 2)},
        (Vector2){(float)(cx + armW / 2), (float)(cy + armW / 2)},
        (Vector2){(float)(cx + tipW / 2), (float)(cy + armLen)}, col);
    DrawTriangle(
        (Vector2){(float)(cx - armW / 2), (float)(cy + armW / 2)},
        (Vector2){(float)(cx + tipW / 2), (float)(cy + armLen)},
        (Vector2){(float)(cx - tipW / 2), (float)(cy + armLen)}, col);
    // Center square
    DrawRectangle(cx - armW / 2, cy - armW / 2, armW, armW, col);
}

// ---- Eagle: clean swept wings as thick lines ----
void HudDrawEagle(int cx, int cy, int size, Color col) {
    Color dark = {(unsigned char)(col.r * 0.5f), (unsigned char)(col.g * 0.5f),
                  (unsigned char)(col.b * 0.5f), col.a};

    float S = (float)size;
    float X = (float)cx, Y = (float)cy;

    float wingSpan = S * 1.8f;
    float wingRise = S * 0.5f;
    float thick = S * 0.14f;
    if (thick < 2.0f) thick = 2.0f;

    // Main wing strokes
    DrawLineEx((Vector2){X, Y}, (Vector2){X - wingSpan, Y - wingRise}, thick, col);
    DrawLineEx((Vector2){X, Y}, (Vector2){X + wingSpan, Y - wingRise}, thick, col);
    // Trailing edge (thinner, darker) for depth
    DrawLineEx((Vector2){X, Y + thick * 0.4f},
               (Vector2){X - wingSpan * 0.85f, Y - wingRise + thick * 1.2f}, thick * 0.5f, dark);
    DrawLineEx((Vector2){X, Y + thick * 0.4f},
               (Vector2){X + wingSpan * 0.85f, Y - wingRise + thick * 1.2f}, thick * 0.5f, dark);
    // Center body dot
    DrawCircle(cx, cy, thick * 0.6f, col);
}

// ---- Art Deco Bar: dark rect with brass borders, 45-degree corner cuts, rivets ----
void HudDrawDecoBar(int x, int y, int w, int h, float s) {
    int cut = (int)(8 * s);
    Color bg = (Color){0, 0, 0, 190};
    Color border = (Color)COLOR_BRASS_DIM;

    DrawRectangle(x, y, w, h, bg);

    // Border lines with corner cuts
    DrawLine(x + cut, y, x + w - cut, y, border);
    DrawLine(x + cut, y + h, x + w - cut, y + h, border);
    DrawLine(x, y + cut, x, y + h - cut, border);
    DrawLine(x + w, y + cut, x + w, y + h - cut, border);

    // 45-degree corner diagonals
    DrawLine(x, y + cut, x + cut, y, border);
    DrawLine(x + w - cut, y, x + w, y + cut, border);
    DrawLine(x, y + h - cut, x + cut, y + h, border);
    DrawLine(x + w - cut, y + h, x + w, y + h - cut, border);

    // Edge midpoint rivets
    HudDrawRivet(x + w / 2, y, s);
    HudDrawRivet(x + w / 2, y + h, s);
    HudDrawRivet(x, y + h / 2, s);
    HudDrawRivet(x + w, y + h / 2, s);

    // Near-corner rivets
    HudDrawRivet(x + cut + (int)(3 * s), y + (int)(2 * s), s);
    HudDrawRivet(x + w - cut - (int)(3 * s), y + (int)(2 * s), s);
    HudDrawRivet(x + cut + (int)(3 * s), y + h - (int)(2 * s), s);
    HudDrawRivet(x + w - cut - (int)(3 * s), y + h - (int)(2 * s), s);
}

// ---- Analog Gauge: semicircular dial with needle ----
void HudDrawGauge(int cx, int cy, int radius, float value, float s, const char *label) {
    float startAngle = -30.0f;
    float sweepAngle = 240.0f;

    // Background arc
    DrawCircleSector((Vector2){(float)cx, (float)cy}, (float)radius,
                     startAngle, startAngle + sweepAngle, 24, (Color)COLOR_GAUGE_BG);

    // Color zone arcs
    float redEnd = startAngle + sweepAngle * 0.25f;
    float yellowEnd = startAngle + sweepAngle * 0.50f;
    float greenEnd = startAngle + sweepAngle;
    int innerR = radius - (int)(4 * s);
    if (innerR < 2) innerR = 2;

    DrawCircleSector((Vector2){(float)cx, (float)cy}, (float)innerR,
                     startAngle, redEnd, 8, (Color){180, 40, 40, 100});
    DrawCircleSector((Vector2){(float)cx, (float)cy}, (float)innerR,
                     redEnd, yellowEnd, 8, (Color){180, 180, 40, 100});
    DrawCircleSector((Vector2){(float)cx, (float)cy}, (float)innerR,
                     yellowEnd, greenEnd, 8, (Color){40, 180, 40, 100});

    // Tick marks
    for (int i = 0; i <= 10; i++) {
        float pct = (float)i / 10.0f;
        float angle = (startAngle + sweepAngle * pct) * DEG2RAD;
        float outerDist = (float)radius;
        float innerDist = (float)(radius - (int)(6 * s));
        float cosA = cosf(angle), sinA = sinf(angle);
        DrawLineEx(
            (Vector2){cx + cosA * innerDist, cy + sinA * innerDist},
            (Vector2){cx + cosA * outerDist, cy + sinA * outerDist},
            (i % 5 == 0) ? 2.0f * s : 1.0f * s, (Color)COLOR_GAUGE_MARK);
    }

    // Needle
    float needleAngle = (startAngle + sweepAngle * value) * DEG2RAD;
    float needleLen = (float)(radius - (int)(3 * s));
    DrawLineEx(
        (Vector2){(float)cx, (float)cy},
        (Vector2){cx + cosf(needleAngle) * needleLen, cy + sinf(needleAngle) * needleLen},
        2.0f * s, (Color)COLOR_GAUGE_NEEDLE);

    // Pivot rivet
    HudDrawRivet(cx, cy, s);

    // Label below
    if (label) {
        int fs = (int)(8 * s);
        if (fs < 6) fs = 6;
        int tw = MeasureText(label, fs);
        DrawText(label, cx - tw / 2, cy + radius + (int)(2 * s), fs, (Color)COLOR_HUD_TEXT);
    }
}

// ---- Mechanical Ticker: dark inset box with brass frame ----
void HudDrawTicker(int x, int y, int w, int h, const char *label, const char *value, float s) {
    DrawRectangle(x, y, w, h, (Color)COLOR_GAUGE_BG);
    DrawRectangleLinesEx((Rectangle){(float)x, (float)y, (float)w, (float)h}, s, (Color)COLOR_BRASS_DIM);

    int vfs = h * 2 / 3;
    if (vfs < 6) vfs = 6;
    int vw = MeasureText(value, vfs);
    DrawText(value, x + w / 2 - vw / 2, y + h / 2 - vfs / 2, vfs, (Color)COLOR_BRASS_BRIGHT);

    if (label) {
        int lfs = (int)(7 * s);
        if (lfs < 5) lfs = 5;
        int lw = MeasureText(label, lfs);
        DrawText(label, x + w / 2 - lw / 2, y - lfs - (int)(2 * s), lfs, (Color)COLOR_HUD_TEXT);
    }

    HudDrawRivet(x + (int)(3 * s), y + (int)(3 * s), s * 0.7f);
    HudDrawRivet(x + w - (int)(3 * s), y + (int)(3 * s), s * 0.7f);
}
