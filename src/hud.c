#include "hud.h"
#include <stdio.h>
#include <math.h>

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
    int barY = sh - sh / 20 - barH;
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

    // Wave incoming
    if (!game->waveActive && game->enemiesRemaining <= 0 && game->wave > 0) {
        const char *nw = "NEXT WAVE INCOMING...";
        int nfs = sh / 20;
        int nwW = MeasureText(nw, nfs);
        float alpha = (sinf(GetTime() * 4.0f) + 1.0f) * 0.5f;
        DrawText(nw, (sw - nwW) / 2, sh / 3, nfs,
            (Color){255, 220, 50, (unsigned char)(alpha * 255)});
    }

    // Damage flash
    if (player->damageFlashTimer > 0) {
        unsigned char a = (unsigned char)(player->damageFlashTimer / 0.3f * 120);
        DrawRectangle(0, 0, sw, sh, (Color){220, 0, 0, a});
    }
}
