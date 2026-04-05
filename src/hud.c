#include "hud.h"
#include "structure/structure.h"
#include <stdio.h>
#include <math.h>

void HudDrawLanderArrows(LanderManager *lm, Camera3D camera, int sw, int sh) {
    int cx = sw / 2;

    // ACHTUNG only during klaxon wait — stops once landers appear
    bool incoming = false;
    for (int i = 0; i < MAX_LANDERS; i++)
        if (lm->landers[i].state == LANDER_WAITING)
            incoming = true;

    // BIG BLARING ALERT across top of screen
    if (incoming) {
        float pulse = (sinf(GetTime() * 6.0f) + 1.0f) * 0.5f;
        unsigned char pa = (unsigned char)(pulse * 255);

        // Red flashing bar
        DrawRectangle(0, sh / 8, sw, sh / 6, (Color){180, 20, 10, (unsigned char)(pulse * 80)});

        // Main text
        const char *alert = "ACHTUNG SOLDATEN";
        int alertFs = sh / 8;
        int alertW = MeasureText(alert, alertFs);
        DrawText(alert, cx - alertW / 2, sh / 8 + sh / 30, alertFs, (Color){255, 50, 30, pa});

        // Sub text
        const char *sub = "FEINDLICHE LANDUNG ERKANNT";
        int subFs = sh / 18;
        int subW = MeasureText(sub, subFs);
        float sc = (float)sh / (float)RENDER_H;
        DrawText(sub, cx - subW / 2, sh / 8 + sh / 30 + alertFs + (int)(4 * sc), subFs, (Color){255, 200, 50, pa});
    }
    // (compass removed)
    (void)camera;
}

void HudDrawPickup(PickupManager *pm, int sw, int sh) {
    if (!pm->hasPickup) return;
    int cx = sw / 2;
    int barInset = sw / 7;

    // Pickup weapon bar — just above bottom bar
    int py = sh - sh / 30 - sh / 14 - sh / 18;
    int pw = sw / 4;
    int px = cx - pw / 2;
    int ph = sh / 22;

    DrawRectangle(px, py, pw, ph, (Color){0, 0, 0, 180});
    // Soviet-faction weapons are red-tinted, American are blue-tinted
    bool isSoviet = (pm->pickupType == PICKUP_KOSMOS7 || pm->pickupType == PICKUP_KS23_MOLOT || pm->pickupType == PICKUP_ZARYA_TK4);
    DrawRectangleLines(px, py, pw, ph,
        isSoviet ? (Color){255, 80, 40, 200} : (Color){80, 160, 255, 200});

    const char *name = PickupGetName(pm->pickupType);
    char txt[64];
    snprintf(txt, sizeof(txt), "%s  [%d]", name, pm->pickupAmmo);
    int fs = ph * 2 / 3;
    int tw = MeasureText(txt, fs);
    Color tc = isSoviet ? (Color){255, 120, 60, 255} : (Color){100, 180, 255, 255};
    // Show charge indicator for Zarya TK-4
    if (pm->pickupType == PICKUP_ZARYA_TK4 && pm->charging) {
        float ratio = pm->chargeTime / PICKUP_ZARYA_CHARGE_TIME;
        if (ratio > 1.0f) ratio = 1.0f;
        DrawRectangle(px + 2, py + ph - 4, (int)((pw - 4) * ratio), 3, (Color){255, 80, 30, 255});
    }
    DrawText(txt, cx - tw / 2, py + ph / 2 - fs / 2, fs, tc);
    (void)barInset;
}

void HudDraw(Player *player, Weapon *weapon, Game *game, int sw, int sh) {
    int cx = sw / 2, cy = sh / 2;
    float s = (float)sh / (float)RENDER_H; // pixel scale factor
    int barInset = sw / 7;
    int topL = barInset, topR = sw - barInset;
    int topW = topR - topL;

    // Crosshair
    Color cross = {0, 255, 200, 230};
    int chOuter = (int)(15 * s), chInner = (int)(5 * s), chRad = (int)(3 * s);
    DrawLine(cx - chOuter, cy, cx - chInner, cy, cross);
    DrawLine(cx + chInner, cy, cx + chOuter, cy, cross);
    DrawLine(cx, cy - chOuter, cx, cy - chInner, cross);
    DrawLine(cx, cy + chInner, cx, cy + chOuter, cross);
    DrawCircleLines(cx, cy, (float)chRad, cross);

    // === TOP BAR ===
    int topY = sh / 20;
    int topH = sh / 14;
    int pad = (int)(12 * s);
    DrawRectangle(topL, topY, topW, topH, (Color){0, 0, 0, 190});
    DrawLine(topL, topY + topH, topR, topY + topH, (Color){0, 200, 180, 100});

    // Health bar — left third
    int hbX = topL + pad;
    int hbY = topY + topH / 2 - topH / 4;
    int hbW = topW / 3 - (int)(20 * s);
    int hbH = topH / 2;
    float hpPct = player->health / player->maxHealth;
    DrawRectangle(hbX, hbY, hbW, hbH, (Color){40, 40, 40, 200});
    DrawRectangle(hbX, hbY, (int)(hbW * hpPct), hbH,
        hpPct > 0.5f ? (Color){50, 220, 50, 255} :
        hpPct > 0.25f ? (Color){220, 220, 50, 255} : (Color){220, 50, 50, 255});
    DrawRectangleLines(hbX, hbY, hbW, hbH, (Color){180, 180, 180, 180});
    char hpText[32];
    snprintf(hpText, sizeof(hpText), "%.0f HP", player->health);
    DrawText(hpText, hbX + (int)(8 * s), hbY + (int)(2 * s), topH / 3, WHITE);

    // Wave — center
    char waveText[32];
    snprintf(waveText, sizeof(waveText), "WAVE %d", game->wave);
    int wfs = topH / 2;
    int ww = MeasureText(waveText, wfs);
    DrawText(waveText, topL + topW / 2 - ww / 2, topY + topH / 2 - wfs / 2, wfs, (Color){255, 60, 60, 255});

    // Kills — right
    char killText[32];
    snprintf(killText, sizeof(killText), "KILLS %d", game->killCount);
    int kfs = topH * 2 / 5;
    int kw = MeasureText(killText, kfs);
    DrawText(killText, topR - kw - pad, topY + topH / 2 - kfs / 2, kfs, WHITE);

    // === BOTTOM BAR ===
    int barH = sh / 14;
    int barY = sh - sh / 30 - barH;
    DrawRectangle(topL, barY, topW, barH, (Color){0, 0, 0, 190});
    DrawLine(topL, barY, topR, barY, (Color){0, 200, 180, 100});

    // Weapon name — left
    const char *weapName = WeaponGetName(weapon);
    int wns = barH * 2 / 3;
    DrawText(weapName, topL + (int)(14 * s), barY + barH / 2 - wns / 2, wns, (Color){0, 255, 220, 255});

    // Ammo — center
    int ammo = WeaponGetAmmo(weapon);
    int maxAmmo = WeaponGetMaxAmmo(weapon);
    int reserve = WeaponGetReserve(weapon);
    char ammoText[64];
    if (ammo < 0) snprintf(ammoText, sizeof(ammoText), "MELEE");
    else snprintf(ammoText, sizeof(ammoText), "%d / %d  [ %d ]", ammo, maxAmmo, reserve);
    int afs = barH / 2;
    int ammoW = MeasureText(ammoText, afs);
    DrawText(ammoText, topL + topW / 2 - ammoW / 2, barY + barH / 2 - afs / 2, afs, WHITE);

    // Hints — right
    const char *hints = "[1][2][3] [R] [X]";
    int hfs = barH / 3;
    int hw = MeasureText(hints, hfs);
    DrawText(hints, topR - hw - pad, barY + barH / 2 - hfs / 2, hfs, (Color){160, 160, 160, 200});

    // Reload bar
    if (WeaponIsReloading(weapon)) {
        float prog = WeaponReloadProgress(weapon);
        int rbW = sw / 5, rbH = sh / 50;
        int rbX = cx - rbW / 2, rbY = cy + sh / 15;
        int rbPad = (int)(2 * s);
        DrawRectangle(rbX - rbPad, rbY - rbPad, rbW + rbPad * 2, rbH + rbPad * 2, (Color){0, 0, 0, 200});
        DrawRectangle(rbX, rbY, (int)(rbW * prog), rbH, (Color){0, 220, 255, 240});
        int rfs = sh / 30;
        const char *rl = "RELOADING";
        DrawText(rl, cx - MeasureText(rl, rfs) / 2, rbY - rfs - (int)(4 * s), rfs, (Color){0, 255, 255, 240});
    }

    // Beam cooldown
    if (weapon->raketenTimer > 0 && !weapon->raketenFiring && weapon->current == WEAPON_RAKETENFAUST) {
        float cd = weapon->raketenTimer / weapon->raketenCooldown;
        int cdW = sw / 8, cdH = sh / 60;
        int cdX = cx - cdW / 2, cdY = cy + sh / 20;
        DrawRectangle(cdX, cdY, cdW, cdH, (Color){40, 20, 0, 180});
        DrawRectangle(cdX, cdY, (int)(cdW * (1.0f - cd)), cdH, (Color){255, 140, 30, 200});
    }

    // (wave incoming alert replaced by ACHTUNG SOLDATEN system)

    // Pickup weapon indicator (above bottom bar)
    // (drawn separately via HudDrawPickup)

    // Damage flash
    if (player->damageFlashTimer > 0) {
        unsigned char a = (unsigned char)(player->damageFlashTimer / 0.3f * 120);
        DrawRectangle(0, 0, sw, sh, (Color){220, 0, 0, a});
    }
}

void HudDrawRadioTransmission(float timer, int sw, int sh) {
    if (timer <= 0) return;
    float s = (float)sh / (float)RENDER_H;

    float alpha = (timer > 0.5f) ? 1.0f : timer / 0.5f; // fade out last 0.5s
    unsigned char a = (unsigned char)(alpha * 220);

    // Box on the right side of screen
    int boxW = sw / 5;
    int boxH = sh / 8;
    int boxX = sw - boxW - sw / 20;
    int boxY = sh * 2 / 3;
    int inPad = (int)(8 * s);
    int smPad = (int)(4 * s);

    // Dark background with red border
    DrawRectangle(boxX, boxY, boxW, boxH, (Color){10, 10, 8, (unsigned char)(a * 0.8f)});
    DrawRectangleLines(boxX, boxY, boxW, boxH, (Color){200, 50, 30, a});
    DrawRectangleLines(boxX + 1, boxY + 1, boxW - 2, boxH - 2, (Color){200, 50, 30, (unsigned char)(a * 0.5f)});

    // "TRANSMISSION" header
    int hfs = boxH / 4;
    const char *hdr = "TRANSMISSION";
    int hw = MeasureText(hdr, hfs);
    float blink = sinf(GetTime() * 8.0f);
    Color hdrCol = (blink > 0) ? (Color){255, 60, 40, a} : (Color){200, 40, 30, a};
    DrawText(hdr, boxX + boxW / 2 - hw / 2, boxY + smPad, hfs, hdrCol);

    // Sound waveform visualization — fake oscilloscope
    int waveY = boxY + boxH / 3 + smPad;
    int waveH = boxH / 3;
    int waveL = boxX + inPad;
    int waveR = boxX + boxW - inPad;
    int waveStep = (int)(2 * s); if (waveStep < 1) waveStep = 1;
    DrawLine(waveL, waveY + waveH / 2, waveR, waveY + waveH / 2, (Color){80, 80, 70, (unsigned char)(a * 0.4f)});
    // Animated waveform
    for (int x = waveL; x < waveR; x += waveStep) {
        float t = (float)(x - waveL) / (float)(waveR - waveL);
        float wave = sinf(t * 20.0f + GetTime() * 15.0f) * 0.6f;
        wave += sinf(t * 50.0f + GetTime() * 25.0f) * 0.3f;
        wave *= alpha;
        int y1 = waveY + waveH / 2;
        int y2 = y1 + (int)(wave * waveH * 0.45f);
        DrawLine(x, y1, x, y2, (Color){255, 80, 40, a});
    }

    // "RECEIVED" footer
    int ffs = boxH / 5;
    const char *ftr = "RECEIVED";
    int fw = MeasureText(ftr, ffs);
    DrawText(ftr, boxX + boxW / 2 - fw / 2, boxY + boxH - ffs - smPad, ffs, (Color){180, 180, 170, a});
}

void HudDrawRankKill(float timer, int rankType, int sw, int sh) {
    if (timer <= 0 || rankType <= 0) return;
    int cx = sw / 2;
    float alpha = (timer > 0.5f) ? 1.0f : timer / 0.5f;
    unsigned char a = (unsigned char)(alpha * 255);

    const char *text = (rankType == 2) ? "OFFIZIER ELIMINIERT!" : "UNTEROFFIZIER ELIMINIERT!";
    int fs = sh / 14;
    int tw = MeasureText(text, fs);
    int ty = sh / 3;

    DrawRectangle(cx - tw / 2 - sh / 30, ty - sh / 60, tw + sh / 15, fs + sh / 30, (Color){0, 0, 0, (unsigned char)(alpha * 180)});
    Color col = (rankType == 2) ? (Color){255, 215, 0, a} : (Color){255, 200, 80, a};
    DrawText(text, cx - tw / 2, ty, fs, col);
}

void HudDrawStructurePrompt(StructurePrompt prompt, int resuppliesLeft, float emptyTimer, int sw, int sh) {
    int cx = sw / 2;
    int cy = sh / 2;

    // "MEIN GOTT!" message — drawn on top of everything when triggered
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

        DrawRectangle(boxX, boxY, boxW, boxH, (Color){60, 10, 5, (unsigned char)(fade * 200)});
        DrawRectangleLines(boxX, boxY, boxW, boxH, (Color){200, 50, 30, a});

        float shake = sinf((float)GetTime() * 25.0f) * 2.0f * fade;
        DrawText(line1, cx - tw1 / 2 + (int)shake, boxY + sh / 30, fs1, (Color){255, 60, 30, a});
        DrawText(line2, cx - tw2 / 2, boxY + sh / 30 + fs1 + sh / 40, fs2, (Color){255, 200, 50, a});
    }

    if (prompt == PROMPT_NONE) return;

    const char *text = NULL;
    Color col = {0, 220, 120, 220};

    switch (prompt) {
        case PROMPT_ENTER:    text = "PRESS [E] TO ENTER BASE"; break;
        case PROMPT_EXIT:     text = "PRESS [E] TO EXIT BASE"; break;
        case PROMPT_RESUPPLY: {
            // Show remaining count
            static char resupplyBuf[64];
            snprintf(resupplyBuf, sizeof(resupplyBuf), "PRESS [E] TO RESUPPLY [%d]", resuppliesLeft);
            text = resupplyBuf;
            col = (Color){80, 255, 120, 240};
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

    int pad = sh / 40;
    DrawRectangle(cx - tw / 2 - pad, cy + sh / 6 - pad / 2, tw + pad * 2, fs + pad, (Color){0, 0, 0, 160});

    float pulse = 0.7f + 0.3f * sinf((float)GetTime() * 4.0f);
    Color drawCol = {(unsigned char)(col.r * pulse), (unsigned char)(col.g * pulse), col.b, col.a};
    DrawText(text, cx - tw / 2, cy + sh / 6, fs, drawCol);
}
