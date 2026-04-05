#include "hud.h"
#include "structure/structure.h"
#include "rlgl.h"
#include <stdio.h>
#include <math.h>

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

// ---- Helper: analog gauge dial ----
static void DrawGauge(int cx, int cy, int radius, float value, float s, const char *label) {
    // value: 0.0 to 1.0
    // Arc from -30 deg (empty) to 210 deg (full), so 240 deg sweep
    float startAngle = -30.0f;
    float sweepAngle = 240.0f;

    // Gauge background arc
    DrawCircleSector((Vector2){(float)cx, (float)cy}, (float)radius, startAngle, startAngle + sweepAngle, 24, (Color)COLOR_GAUGE_BG);

    // Color zone arcs: red (0-25%), yellow (25-50%), green (50-100%)
    float redEnd = startAngle + sweepAngle * 0.25f;
    float yellowEnd = startAngle + sweepAngle * 0.50f;
    float greenEnd = startAngle + sweepAngle;
    int innerR = radius - (int)(4 * s);
    if (innerR < 2) innerR = 2;

    DrawCircleSector((Vector2){(float)cx, (float)cy}, (float)innerR, startAngle, redEnd, 8, (Color){180, 40, 40, 100});
    DrawCircleSector((Vector2){(float)cx, (float)cy}, (float)innerR, redEnd, yellowEnd, 8, (Color){180, 180, 40, 100});
    DrawCircleSector((Vector2){(float)cx, (float)cy}, (float)innerR, yellowEnd, greenEnd, 8, (Color){40, 180, 40, 100});

    // Tick marks (every 10%)
    for (int i = 0; i <= 10; i++) {
        float pct = (float)i / 10.0f;
        float angle = (startAngle + sweepAngle * pct) * DEG2RAD;
        float outerDist = (float)radius;
        float innerDist = (float)(radius - (int)(6 * s));
        float cosA = cosf(angle), sinA = sinf(angle);
        DrawLineEx(
            (Vector2){cx + cosA * innerDist, cy + sinA * innerDist},
            (Vector2){cx + cosA * outerDist, cy + sinA * outerDist},
            (i % 5 == 0) ? 2.0f * s : 1.0f * s,
            (Color)COLOR_GAUGE_MARK
        );
    }

    // Needle
    float needleAngle = (startAngle + sweepAngle * value) * DEG2RAD;
    float needleLen = (float)(radius - (int)(3 * s));
    DrawLineEx(
        (Vector2){(float)cx, (float)cy},
        (Vector2){cx + cosf(needleAngle) * needleLen, cy + sinf(needleAngle) * needleLen},
        2.0f * s,
        (Color)COLOR_GAUGE_NEEDLE
    );

    // Pivot rivet
    DrawRivet(cx, cy, s);

    // Label below
    if (label) {
        int fs = (int)(8 * s);
        if (fs < 6) fs = 6;
        int tw = MeasureText(label, fs);
        DrawText(label, cx - tw / 2, cy + radius + (int)(2 * s), fs, (Color)COLOR_HUD_TEXT);
    }
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
    float s = (float)sh / (float)RENDER_H;

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
    float s = (float)sh / (float)RENDER_H;

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

void HudDraw(Player *player, Weapon *weapon, Game *game, int sw, int sh) {
    int cx = sw / 2, cy = sh / 2;
    float s = (float)sh / (float)RENDER_H;
    int barInset = sw / 7;
    int topL = barInset, topR = sw - barInset;
    int topW = topR - topL;

    // ======== CROSSHAIR — hidden when Fuehrerauge is active ========
    if (player->fuehreraugeAnim <= 0.0f) {
        Color cross = {0, 255, 200, 230};
        int chOuter = (int)(15 * s), chInner = (int)(5 * s), chRad = (int)(3 * s);
        DrawLine(cx - chOuter, cy, cx - chInner, cy, cross);
        DrawLine(cx + chInner, cy, cx + chOuter, cy, cross);
        DrawLine(cx, cy - chOuter, cx, cy - chInner, cross);
        DrawLine(cx, cy + chInner, cx, cy + chOuter, cross);
        DrawCircleLines(cx, cy, (float)chRad, cross);
    }

    // ======== TOP BAR — Art Deco frame ========
    int topY = sh / 20;
    int topH = sh / 14;
    int pad = (int)(12 * s);
    DrawDecoBar(topL, topY, topW, topH, s);

    // ---- Health gauge (left) — bigger, extends below bar ----
    {
        int gaugeR = topH + (int)(4 * s);
        int gaugeCX = topL + pad + gaugeR + (int)(6 * s);
        int gaugeCY = topY + topH / 2 + (int)(4 * s);
        float hpPct = player->health / player->maxHealth;
        DrawGauge(gaugeCX, gaugeCY, gaugeR, hpPct, s, "HP");
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
        int tw = (int)(40 * s), th = topH - (int)(6 * s);
        int tx = topL + topW / 2 - tw / 2;
        int ty = topY + topH / 2 - th / 2;
        DrawTicker(tx, ty, tw, th, "WAVE", waveText, s);
    }

    // ---- Kills ticker (right) ----
    {
        char killText[16];
        snprintf(killText, sizeof(killText), "%d", game->killCount);
        int tw = (int)(48 * s), th = topH - (int)(6 * s);
        int tx = topR - tw - pad;
        int ty = topY + topH / 2 - th / 2;
        DrawTicker(tx, ty, tw, th, "KILLS", killText, s);
    }

    // ======== BOTTOM BAR — Art Deco frame ========
    int barH = sh / 14;
    int barY = sh - sh / 30 - barH;
    DrawDecoBar(topL, barY, topW, barH, s);

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

        // Dark inset backing behind the numbers
        int inPad = (int)(4 * s);
        DrawRectangle(ax - inPad, ay - inPad / 2, atw + inPad * 2, afs + inPad, (Color)COLOR_GAUGE_BG);
        DrawRectangleLinesEx(
            (Rectangle){(float)(ax - inPad), (float)(ay - inPad / 2),
                         (float)(atw + inPad * 2), (float)(afs + inPad)},
            s * 0.5f, (Color)COLOR_BRASS_DIM);

        DrawText(ammoText, ax, ay, afs, (Color)COLOR_BRASS_BRIGHT);
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
        int reloadR = (int)(18 * s);
        int reloadCX = cx;
        int reloadCY = cy + (int)(35 * s);
        DrawGauge(reloadCX, reloadCY, reloadR, prog, s, NULL);

        int rfs = (int)(8 * s);
        if (rfs < 5) rfs = 5;
        const char *rl = "RELOADING";
        int rlw = MeasureText(rl, rfs);
        DrawText(rl, reloadCX - rlw / 2, reloadCY - reloadR - rfs - (int)(3 * s), rfs, (Color)COLOR_HUD_TEXT);
    }

    // ======== Beam cooldown ========
    if (weapon->raketenTimer > 0 && !weapon->raketenFiring && weapon->current == WEAPON_RAKETENFAUST) {
        float cd = weapon->raketenTimer / weapon->raketenCooldown;
        int cdW = sw / 8, cdH = sh / 60;
        int cdX = cx - cdW / 2, cdY = cy + (int)(20 * s);
        DrawRectangle(cdX, cdY, cdW, cdH, (Color){40, 30, 15, 180});
        DrawRectangle(cdX, cdY, (int)(cdW * (1.0f - cd)), cdH, (Color)COLOR_BRASS);
    }

    // ======== Damage flash — amber-tinted red ========
    if (player->damageFlashTimer > 0) {
        unsigned char a = (unsigned char)(player->damageFlashTimer / DAMAGE_FLASH_DURATION * 120);
        DrawRectangle(0, 0, sw, sh, (Color){200, 30, 10, a});
    }
}

void HudDrawRadioTransmission(float timer, int sw, int sh) {
    if (timer <= 0) return;
    float s = (float)sh / (float)RENDER_H;

    float alpha = (timer > 0.5f) ? 1.0f : timer / 0.5f;
    unsigned char a = (unsigned char)(alpha * 220);

    int boxW = sw / 5;
    int boxH = sh / 8;
    int boxX = sw - boxW - sw / 20;
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
    float s = (float)sh / (float)RENDER_H;
    float alpha = (timer > 0.5f) ? 1.0f : timer / 0.5f;
    unsigned char a = (unsigned char)(alpha * 255);

    const char *text = (rankType == 2) ? "OFFIZIER ELIMINIERT!" : "UNTEROFFIZIER ELIMINIERT!";
    int fs = sh / 14;
    int tw = MeasureText(text, fs);
    int ty = sh / 3;

    // Dark metal background with brass border
    int boxPad = sh / 30;
    DrawRectangle(cx - tw / 2 - boxPad, ty - sh / 60, tw + boxPad * 2, fs + sh / 30, (Color){20, 18, 15, (unsigned char)(alpha * 200)});
    DrawRectangleLinesEx(
        (Rectangle){(float)(cx - tw / 2 - boxPad), (float)(ty - sh / 60),
                     (float)(tw + boxPad * 2), (float)(fs + sh / 30)},
        s, (Color){205, 170, 80, a});

    Color col = (rankType == 2) ? (Color){240, 200, 100, a} : (Color){205, 170, 80, a};
    DrawText(text, cx - tw / 2, ty, fs, col);
}

void HudDrawStructurePrompt(StructurePrompt prompt, int resuppliesLeft, float emptyTimer, int sw, int sh) {
    int cx = sw / 2;
    int cy = sh / 2;
    float s = (float)sh / (float)RENDER_H;

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
// Swings in on a metal arm from the upper-right, like an optometrist's phoropter
void HudDrawFuehrerauge(Texture2D zoomTex, Shader fishbowlShader, float animProgress, int sw, int sh) {
    if (animProgress <= 0.0f) return;
    float s = (float)sh / (float)RENDER_H;

    // Smoothstep ease with slight overshoot for mechanical feel
    float t = animProgress;
    float ease = t * t * (3.0f - 2.0f * t);

    // Pivot point: upper-right area of helmet (where arm is bolted)
    float pivotX = (float)sw * 0.85f;
    float pivotY = (float)sh * 0.05f;

    // Target: lens center at screen center when fully deployed
    float targetCX = (float)sw * 0.5f;
    float targetCY = (float)sh * 0.42f;

    // Compute arm length from pivot to screen center
    float dx = pivotX - targetCX;
    float dy = targetCY - pivotY;
    float armLen = sqrtf(dx * dx + dy * dy);

    // Deployed angle: derived from pivot→center vector
    float deployedAngle = atan2f(dx, dy); // angle from vertical (down)

    // Stowed: swing 80 degrees further clockwise (toward upper-right, off screen)
    float stowedAngle = deployedAngle - 80.0f * DEG2RAD;
    float angle = stowedAngle + ease * (deployedAngle - stowedAngle);

    // Lens center position on the arc
    float lensCX = pivotX - sinf(angle) * armLen;
    float lensCY = pivotY + cosf(angle) * armLen;

    // Viewport rect (texture still draws as a rectangle, trapezoid is the frame around it)
    int vpW = (int)(sw * FUEHRERAUGE_WIDTH_FRAC);
    int vpH = (int)(sh * FUEHRERAUGE_HEIGHT_FRAC);
    int vpX = (int)(lensCX) - vpW / 2;
    int vpY = (int)(lensCY) - vpH / 2;

    int frame = (int)(FUEHRERAUGE_FRAME_THICK * s);
    if (frame < 2) frame = 2;

    // Trapezoid inset: top is wider, bottom is narrower (upside-down trapezoid)
    int trapInset = (int)(vpW * 0.12f); // how much narrower the bottom is on each side

    // Trapezoid corner points (outer frame edge)
    float tl_x = (float)(vpX - frame);
    float tl_y = (float)(vpY - frame);
    float tr_x = (float)(vpX + vpW + frame);
    float tr_y = (float)(vpY - frame);
    float bl_x = (float)(vpX - frame + trapInset);
    float bl_y = (float)(vpY + vpH + frame);
    float br_x = (float)(vpX + vpW + frame - trapInset);
    float br_y = (float)(vpY + vpH + frame);

    // ---- Metal arm from pivot to lens ----
    {
        float armThick = 4.0f * s;
        if (armThick < 2.0f) armThick = 2.0f;
        Color armColor = (Color){55, 50, 42, 240};
        Color armEdge = (Color){90, 80, 65, 200};

        // Main arm bar
        DrawLineEx((Vector2){pivotX, pivotY}, (Vector2){lensCX, lensCY}, armThick + 2.0f * s, armEdge);
        DrawLineEx((Vector2){pivotX, pivotY}, (Vector2){lensCX, lensCY}, armThick, armColor);

        // Pivot bolt (big rivet at mount point)
        DrawCircle((int)pivotX, (int)pivotY, 5.0f * s, (Color){60, 55, 45, 240});
        DrawCircle((int)pivotX, (int)pivotY, 3.5f * s, (Color){100, 90, 70, 255});
        DrawCircle((int)pivotX, (int)pivotY, 1.5f * s, (Color){70, 65, 55, 255});

        // Small bracket near the lens end
        float perpXv = cosf(angle) * 6.0f * s;
        float perpYv = sinf(angle) * 6.0f * s;
        float nearLensX = lensCX + sinf(angle) * vpH * 0.35f;
        float nearLensY = lensCY - cosf(angle) * vpH * 0.35f;
        DrawLineEx(
            (Vector2){nearLensX - perpXv, nearLensY - perpYv},
            (Vector2){nearLensX + perpXv, nearLensY + perpYv},
            armThick * 0.7f, armColor);
        DrawRivet((int)nearLensX, (int)nearLensY, s * 0.6f);
    }

    // ---- Trapezoid frame (dark metal housing) ----
    // Fill with two triangles
    Color frameBg = (Color){25, 23, 20, 240};
    // Left dark triangle (outside viewport, left side taper)
    DrawTriangle((Vector2){tl_x, tl_y}, (Vector2){bl_x, bl_y}, (Vector2){(float)(vpX), (float)(vpY + vpH)}, frameBg);
    DrawTriangle((Vector2){tl_x, tl_y}, (Vector2){(float)(vpX), (float)(vpY + vpH)}, (Vector2){(float)(vpX), (float)(vpY)}, frameBg);
    // Right dark triangle
    DrawTriangle((Vector2){tr_x, tr_y}, (Vector2){(float)(vpX + vpW), (float)(vpY)}, (Vector2){(float)(vpX + vpW), (float)(vpY + vpH)}, frameBg);
    DrawTriangle((Vector2){tr_x, tr_y}, (Vector2){(float)(vpX + vpW), (float)(vpY + vpH)}, (Vector2){br_x, br_y}, frameBg);
    // Top bar
    DrawTriangle((Vector2){tl_x, tl_y}, (Vector2){(float)(vpX), (float)(vpY)}, (Vector2){(float)(vpX + vpW), (float)(vpY)}, frameBg);
    DrawTriangle((Vector2){tl_x, tl_y}, (Vector2){(float)(vpX + vpW), (float)(vpY)}, (Vector2){tr_x, tr_y}, frameBg);
    // Bottom bar
    DrawTriangle((Vector2){bl_x, bl_y}, (Vector2){br_x, br_y}, (Vector2){(float)(vpX + vpW), (float)(vpY + vpH)}, frameBg);
    DrawTriangle((Vector2){bl_x, bl_y}, (Vector2){(float)(vpX + vpW), (float)(vpY + vpH)}, (Vector2){(float)(vpX), (float)(vpY + vpH)}, frameBg);

    // ---- Trapezoid brass border lines ----
    float bthick = 2.5f * s;
    Color brassCol = (Color)COLOR_BRASS;
    DrawLineEx((Vector2){tl_x, tl_y}, (Vector2){tr_x, tr_y}, bthick, brassCol);       // top
    DrawLineEx((Vector2){bl_x, bl_y}, (Vector2){br_x, br_y}, bthick, brassCol);       // bottom
    DrawLineEx((Vector2){tl_x, tl_y}, (Vector2){bl_x, bl_y}, bthick, brassCol);       // left
    DrawLineEx((Vector2){tr_x, tr_y}, (Vector2){br_x, br_y}, bthick, brassCol);       // right
    // Inner border (at viewport edge, also tapered to suggest depth)
    Color brassD = (Color)COLOR_BRASS_DIM;
    float ithick = s;
    DrawLineEx((Vector2){(float)vpX, (float)vpY}, (Vector2){(float)(vpX + vpW), (float)vpY}, ithick, brassD);
    DrawLineEx((Vector2){(float)vpX, (float)(vpY + vpH)}, (Vector2){(float)(vpX + vpW), (float)(vpY + vpH)}, ithick, brassD);
    DrawLineEx((Vector2){(float)vpX, (float)vpY}, (Vector2){(float)vpX, (float)(vpY + vpH)}, ithick, brassD);
    DrawLineEx((Vector2){(float)(vpX + vpW), (float)vpY}, (Vector2){(float)(vpX + vpW), (float)(vpY + vpH)}, ithick, brassD);

    // ---- Corner rivets on trapezoid ----
    DrawRivet((int)tl_x, (int)tl_y, s);
    DrawRivet((int)tr_x, (int)tr_y, s);
    DrawRivet((int)bl_x, (int)bl_y, s);
    DrawRivet((int)br_x, (int)br_y, s);
    // Midpoint rivets on top and bottom edges
    DrawRivet((int)((tl_x + tr_x) * 0.5f), (int)tl_y, s);
    DrawRivet((int)((bl_x + br_x) * 0.5f), (int)bl_y, s);

    // ---- Copper accent at top corners ----
    int deco = (int)(10 * s);
    Color decoCol = (Color)COLOR_COPPER;
    DrawLineEx((Vector2){tl_x, tl_y}, (Vector2){tl_x + deco, tl_y}, 2.0f * s, decoCol);
    DrawLineEx((Vector2){tr_x, tr_y}, (Vector2){tr_x - deco, tr_y}, 2.0f * s, decoCol);

    // ---- Zoomed view with old-TV shader ----
    BeginShaderMode(fishbowlShader);
    DrawTexturePro(zoomTex,
        (Rectangle){0, 0, (float)zoomTex.width, -(float)zoomTex.height},
        (Rectangle){(float)vpX, (float)vpY, (float)vpW, (float)vpH},
        (Vector2){0, 0}, 0.0f, WHITE);
    EndShaderMode();

    // ---- Targeting reticle over zoomed view ----
    {
        int rcx = vpX + vpW / 2;
        int rcy = vpY + vpH / 2;
        Color reticle = (Color){240, 200, 100, 180};
        int rLen = (int)(20 * s);
        int rGap = (int)(6 * s);
        float thick = 1.0f * s;
        if (thick < 1.0f) thick = 1.0f;

        DrawLineEx((Vector2){(float)(rcx - rLen), (float)rcy}, (Vector2){(float)(rcx - rGap), (float)rcy}, thick, reticle);
        DrawLineEx((Vector2){(float)(rcx + rGap), (float)rcy}, (Vector2){(float)(rcx + rLen), (float)rcy}, thick, reticle);
        DrawLineEx((Vector2){(float)rcx, (float)(rcy - rLen)}, (Vector2){(float)rcx, (float)(rcy - rGap)}, thick, reticle);
        DrawLineEx((Vector2){(float)rcx, (float)(rcy + rGap)}, (Vector2){(float)rcx, (float)(rcy + rLen)}, thick, reticle);

        DrawCircle(rcx, rcy, 1.5f * s, reticle);
        DrawCircleLines(rcx, rcy, (float)rLen, (Color){200, 170, 80, 100});
    }

    // ---- "FUEHRERAUGE" label on top edge ----
    {
        int lfs = (int)(7 * s);
        if (lfs < 5) lfs = 5;
        const char *label = "FUEHRERAUGE";
        int lw = MeasureText(label, lfs);
        int lx = (int)((tl_x + tr_x) * 0.5f) - lw / 2;
        int ly = (int)tl_y + (int)(2 * s);
        DrawRectangle(lx - (int)(3 * s), ly - (int)(1 * s),
                      lw + (int)(6 * s), lfs + (int)(2 * s), (Color){25, 23, 20, 220});
        DrawText(label, lx, ly, lfs, (Color)COLOR_BRASS);
    }
}
