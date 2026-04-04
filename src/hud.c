#include "hud.h"
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
        DrawText(sub, cx - subW / 2, sh / 8 + sh / 30 + alertFs + 4, subFs, (Color){255, 200, 50, pa});
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
    DrawRectangleLines(px, py, pw, ph,
        (pm->pickupType == ENEMY_SOVIET) ? (Color){255, 80, 40, 200} : (Color){80, 160, 255, 200});

    const char *name = (pm->pickupType == ENEMY_SOVIET) ? "KOSMOS-7 SMG" : "LIBERTY BLASTER";
    char txt[64];
    snprintf(txt, sizeof(txt), "%s  [%d]", name, pm->pickupAmmo);
    int fs = ph * 2 / 3;
    int tw = MeasureText(txt, fs);
    Color tc = (pm->pickupType == ENEMY_SOVIET) ? (Color){255, 120, 60, 255} : (Color){100, 180, 255, 255};
    DrawText(txt, cx - tw / 2, py + ph / 2 - fs / 2, fs, tc);
    (void)barInset;
}

void HudDraw(Player *player, Weapon *weapon, Game *game, int sw, int sh) {
    int cx = sw / 2, cy = sh / 2;
    int barInset = sw / 7;
    int topL = barInset, topR = sw - barInset;
    int topW = topR - topL;

    // Crosshair
    Color cross = {0, 255, 200, 230};
    DrawLine(cx - 15, cy, cx - 5, cy, cross);
    DrawLine(cx + 5, cy, cx + 15, cy, cross);
    DrawLine(cx, cy - 15, cx, cy - 5, cross);
    DrawLine(cx, cy + 5, cx, cy + 15, cross);
    DrawCircleLines(cx, cy, 3, cross);

    // === TOP BAR ===
    int topY = sh / 20;
    int topH = sh / 14;
    DrawRectangle(topL, topY, topW, topH, (Color){0, 0, 0, 190});
    DrawLine(topL, topY + topH, topR, topY + topH, (Color){0, 200, 180, 100});

    // Health bar — left third
    int hbX = topL + 12;
    int hbY = topY + topH / 2 - topH / 4;
    int hbW = topW / 3 - 20;
    int hbH = topH / 2;
    float hpPct = player->health / player->maxHealth;
    DrawRectangle(hbX, hbY, hbW, hbH, (Color){40, 40, 40, 200});
    DrawRectangle(hbX, hbY, (int)(hbW * hpPct), hbH,
        hpPct > 0.5f ? (Color){50, 220, 50, 255} :
        hpPct > 0.25f ? (Color){220, 220, 50, 255} : (Color){220, 50, 50, 255});
    DrawRectangleLines(hbX, hbY, hbW, hbH, (Color){180, 180, 180, 180});
    char hpText[32];
    snprintf(hpText, sizeof(hpText), "%.0f HP", player->health);
    DrawText(hpText, hbX + 8, hbY + 2, topH / 3, WHITE);

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
    DrawText(killText, topR - kw - 12, topY + topH / 2 - kfs / 2, kfs, WHITE);

    // === BOTTOM BAR ===
    int barH = sh / 14;
    int barY = sh - sh / 30 - barH;
    DrawRectangle(topL, barY, topW, barH, (Color){0, 0, 0, 190});
    DrawLine(topL, barY, topR, barY, (Color){0, 200, 180, 100});

    // Weapon name — left
    const char *weapName = WeaponGetName(weapon);
    int wns = barH * 2 / 3;
    DrawText(weapName, topL + 14, barY + barH / 2 - wns / 2, wns, (Color){0, 255, 220, 255});

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
    DrawText(hints, topR - hw - 12, barY + barH / 2 - hfs / 2, hfs, (Color){160, 160, 160, 200});

    // Reload bar
    if (WeaponIsReloading(weapon)) {
        float prog = WeaponReloadProgress(weapon);
        int rbW = sw / 5, rbH = sh / 50;
        int rbX = cx - rbW / 2, rbY = cy + sh / 15;
        DrawRectangle(rbX - 2, rbY - 2, rbW + 4, rbH + 4, (Color){0, 0, 0, 200});
        DrawRectangle(rbX, rbY, (int)(rbW * prog), rbH, (Color){0, 220, 255, 240});
        int rfs = sh / 30;
        const char *rl = "RELOADING";
        DrawText(rl, cx - MeasureText(rl, rfs) / 2, rbY - rfs - 4, rfs, (Color){0, 255, 255, 240});
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

    float alpha = (timer > 0.5f) ? 1.0f : timer / 0.5f; // fade out last 0.5s
    unsigned char a = (unsigned char)(alpha * 220);

    // Box on the right side of screen
    int boxW = sw / 5;
    int boxH = sh / 8;
    int boxX = sw - boxW - sw / 20;
    int boxY = sh * 2 / 3;

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
    DrawText(hdr, boxX + boxW / 2 - hw / 2, boxY + 4, hfs, hdrCol);

    // Sound waveform visualization — fake oscilloscope
    int waveY = boxY + boxH / 3 + 4;
    int waveH = boxH / 3;
    int waveL = boxX + 8;
    int waveR = boxX + boxW - 8;
    DrawLine(waveL, waveY + waveH / 2, waveR, waveY + waveH / 2, (Color){80, 80, 70, (unsigned char)(a * 0.4f)});
    // Animated waveform
    for (int x = waveL; x < waveR; x += 2) {
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
    DrawText(ftr, boxX + boxW / 2 - fw / 2, boxY + boxH - ffs - 4, ffs, (Color){180, 180, 170, a});
}
