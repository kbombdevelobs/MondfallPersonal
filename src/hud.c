// Main HUD drawing — dieselpunk retro style.
// Primitives (rivets, gauges, tickers, etc.) in hud/hud_primitives.c
// Fuehrerauge zoom optic in hud/hud_fuehrerauge.c

#include "hud.h"
#include "hud/hud_primitives.h"
#include "structure/structure.h"
#include <stdio.h>
#include <math.h>

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

    HudDrawRivet(px + (int)(4 * s), py + ph / 2, s * 0.7f);
    HudDrawRivet(px + pw - (int)(4 * s), py + ph / 2, s * 0.7f);

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

    // ======== TOP BAR ========
    int topY = sh / 20;
    int topH = sh / 14;
    int pad = (int)(12 * s);
    HudDrawDecoBar(topL, topY, topW, topH, s);

    // Health gauge (left)
    {
        int gaugeR = topH + (int)(4 * s);
        int gaugeCX = topL + pad + gaugeR + (int)(6 * s);
        int gaugeCY = topY + topH / 2 + (int)(4 * s);
        float hpPct = player->health / player->maxHealth;
        HudDrawGauge(gaugeCX, gaugeCY, gaugeR, hpPct, s, "HP");
    }

    // Iron crosses between sections
    {
        int icSize = (int)(8 * s);
        HudDrawIronCross(topL + topW / 4, topY + topH / 2, icSize, (Color){160, 140, 80, 120});
        HudDrawIronCross(topL + topW * 3 / 4, topY + topH / 2, icSize, (Color){160, 140, 80, 120});
    }

    // Wave ticker (center)
    {
        char waveText[16];
        snprintf(waveText, sizeof(waveText), "%d", game->wave);
        int tw = (int)(40 * s), th = topH - (int)(6 * s);
        int tx = topL + topW / 2 - tw / 2;
        int ty = topY + topH / 2 - th / 2;
        HudDrawTicker(tx, ty, tw, th, "WAVE", waveText, s);
    }

    // Kills ticker (right)
    {
        char killText[16];
        snprintf(killText, sizeof(killText), "%d", game->killCount);
        int tw = (int)(48 * s), th = topH - (int)(6 * s);
        int tx = topR - tw - pad;
        int ty = topY + topH / 2 - th / 2;
        HudDrawTicker(tx, ty, tw, th, "KILLS", killText, s);
    }

    // ======== BOTTOM BAR ========
    int barH = sh / 14;
    int barY = sh - sh / 30 - barH;
    HudDrawDecoBar(topL, barY, topW, barH, s);

    // Weapon name (left)
    {
        const char *weapName = WeaponGetName(weapon);
        int wns = barH * 2 / 3;
        int wnx = topL + (int)(14 * s);
        int nameW = MeasureText(weapName, wns) + (int)(10 * s);
        DrawRectangle(wnx - (int)(5 * s), barY + barH / 2 - wns / 2 - (int)(2 * s),
                      nameW, wns + (int)(4 * s), (Color){35, 32, 28, 160});
        DrawText(weapName, wnx, barY + barH / 2 - wns / 2, wns, (Color)COLOR_BRASS);
    }

    // Ammo readout (center)
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

        int inPad = (int)(4 * s);
        DrawRectangle(ax - inPad, ay - inPad / 2, atw + inPad * 2, afs + inPad, (Color)COLOR_GAUGE_BG);
        DrawRectangleLinesEx(
            (Rectangle){(float)(ax - inPad), (float)(ay - inPad / 2),
                         (float)(atw + inPad * 2), (float)(afs + inPad)},
            s * 0.5f, (Color)COLOR_BRASS_DIM);
        DrawText(ammoText, ax, ay, afs, (Color)COLOR_BRASS_BRIGHT);
    }

    // Hints (right)
    {
        const char *hints = "[1][2][3] [R] [X]";
        int hfs = barH / 3;
        int hw = MeasureText(hints, hfs);
        DrawText(hints, topR - hw - pad, barY + barH / 2 - hfs / 2, hfs, (Color)COLOR_BRASS_DIM);
    }

    // Iron crosses flanking bottom bar
    {
        int icSize = (int)(6 * s);
        HudDrawIronCross(topL + (int)(8 * s), barY + barH / 2, icSize, (Color){140, 120, 70, 80});
        HudDrawIronCross(topR - (int)(8 * s), barY + barH / 2, icSize, (Color){140, 120, 70, 80});
    }

    // Eagle emblem just above bottom bar
    {
        int eagleSize = barH + (int)(4 * s);
        int eagleX = topL + topW / 2;
        int eagleY = barY - eagleSize / 4;
        HudDrawEagle(eagleX, eagleY, eagleSize, (Color)COLOR_BRASS);
    }

    // ======== RELOAD — circular gauge near crosshair ========
    if (WeaponIsReloading(weapon)) {
        float prog = WeaponReloadProgress(weapon);
        int reloadR = (int)(18 * s);
        int reloadCX = cx;
        int reloadCY = cy + (int)(35 * s);
        HudDrawGauge(reloadCX, reloadCY, reloadR, prog, s, NULL);

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

    // ======== Damage flash ========
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

    DrawRectangle(boxX, boxY, boxW, boxH, (Color){10, 10, 8, (unsigned char)(a * 0.8f)});
    DrawRectangleLinesEx((Rectangle){(float)boxX, (float)boxY, (float)boxW, (float)boxH},
                         s, (Color){180, 150, 70, a});
    DrawRectangleLinesEx((Rectangle){(float)(boxX + 1), (float)(boxY + 1), (float)(boxW - 2), (float)(boxH - 2)},
                         s * 0.5f, (Color){140, 120, 60, (unsigned char)(a * 0.5f)});

    HudDrawRivet(boxX + smPad, boxY + smPad, s * 0.6f);
    HudDrawRivet(boxX + boxW - smPad, boxY + smPad, s * 0.6f);

    int hfs = boxH / 4;
    const char *hdr = "TRANSMISSION";
    int hw = MeasureText(hdr, hfs);
    float blink = sinf(GetTime() * 8.0f);
    Color hdrCol = (blink > 0) ? (Color){240, 200, 100, a} : (Color){180, 150, 70, a};
    DrawText(hdr, boxX + boxW / 2 - hw / 2, boxY + smPad, hfs, hdrCol);

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

    DrawRectangle(cx - tw / 2 - promptPad, cy + sh / 6 - promptPad / 2, tw + promptPad * 2, fs + promptPad, (Color)COLOR_DARK_METAL);
    DrawRectangleLinesEx(
        (Rectangle){(float)(cx - tw / 2 - promptPad), (float)(cy + sh / 6 - promptPad / 2),
                     (float)(tw + promptPad * 2), (float)(fs + promptPad)},
        s * 0.5f, (Color)COLOR_BRASS_DIM);

    float pulse = 0.7f + 0.3f * sinf((float)GetTime() * 4.0f);
    Color drawCol = {(unsigned char)(col.r * pulse), (unsigned char)(col.g * pulse), col.b, col.a};
    DrawText(text, cx - tw / 2, cy + sh / 6, fs, drawCol);
}
