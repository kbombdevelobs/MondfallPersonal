#include "game.h"
#include <string.h>
#include <stdio.h>
#include <math.h>

void GameInit(Game *game) {
    memset(game, 0, sizeof(Game));
    game->state = STATE_MENU;
    game->waveDelay = WAVE_DELAY;
    game->enemiesPerWave = WAVE_ENEMIES_BASE + 1;
}

void GameReset(Game *game) {
    game->wave = 0;
    game->killCount = 0;
    game->waveActive = false;
    game->enemiesRemaining = 0;
    game->enemiesSpawned = 0;
    game->waveTimer = 0;
}

void GameUpdate(Game *game) {
    if (game->state == STATE_PLAYING) {
        if (!game->waveActive && game->enemiesRemaining <= 0) {
            game->waveTimer += GetFrameTime();
            if (game->waveTimer >= game->waveDelay) {
                game->wave++;
                game->enemiesPerWave = WAVE_ENEMIES_BASE + game->wave * WAVE_ENEMIES_PER_WAVE;
                game->enemiesSpawned = 0;
                game->enemiesRemaining = game->enemiesPerWave;
                game->waveActive = true;
                game->waveTimer = 0.0f;
            }
        }
    }
}

void GameDrawMenu(Game *game) {
    (void)game;
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    DrawRectangle(0, 0, sw, sh, BLACK);

    const char *title = "MONDFALL";
    int titleSize = 80;
    int titleW = MeasureText(title, titleSize);
    DrawText(title, (sw - titleW) / 2, sh / 4, titleSize, (Color){200, 50, 50, 255});

    const char *subtitle = "NAZIS ON THE MOON";
    int subSize = 30;
    int subW = MeasureText(subtitle, subSize);
    DrawText(subtitle, (sw - subW) / 2, sh / 4 + 90, subSize, GRAY);

    const char *prompt = "PRESS ENTER TO BEGIN";
    int promptSize = 24;
    int promptW = MeasureText(prompt, promptSize);
    float alpha = (sinf(GetTime() * 3.0f) + 1.0f) * 0.5f;
    DrawText(prompt, (sw - promptW) / 2, sh * 3 / 4, promptSize,
             (Color){255, 255, 255, (unsigned char)(alpha * 255)});
}

void GameDrawPaused(void) {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    DrawRectangle(0, 0, sw, sh, (Color){0, 0, 0, 150});
    const char *text = "PAUSED";
    int size = 60;
    int w = MeasureText(text, size);
    DrawText(text, (sw - w) / 2, sh / 2 - 30, size, WHITE);

    const char *hint = "PRESS ESC TO RESUME";
    int hs = 20;
    int hw = MeasureText(hint, hs);
    DrawText(hint, (sw - hw) / 2, sh / 2 + 40, hs, GRAY);
}

void GameDrawGameOver(Game *game) {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    DrawRectangle(0, 0, sw, sh, (Color){0, 0, 0, 200});

    const char *text = "GEFALLEN";
    int size = 60;
    int w = MeasureText(text, size);
    DrawText(text, (sw - w) / 2, sh / 3, size, RED);

    char stats[128];
    snprintf(stats, sizeof(stats), "WAVES SURVIVED: %d   KILLS: %d", game->wave, game->killCount);
    int ss = 24;
    int sw2 = MeasureText(stats, ss);
    DrawText(stats, (sw - sw2) / 2, sh / 2, ss, WHITE);

    const char *prompt = "PRESS ENTER TO RESTART";
    int ps = 20;
    int pw = MeasureText(prompt, ps);
    float alpha = (sinf(GetTime() * 3.0f) + 1.0f) * 0.5f;
    DrawText(prompt, (sw - pw) / 2, sh * 2 / 3, ps,
             (Color){255, 255, 255, (unsigned char)(alpha * 255)});
}
