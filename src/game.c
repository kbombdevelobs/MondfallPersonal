#include "game.h"
#include "config.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// Menu color theme
#define MENU_BG         (Color){0, 0, 0, 255}
#define MENU_HIGHLIGHT  (Color){200, 50, 50, 255}
#define MENU_TEXT        (Color){200, 200, 200, 255}
#define MENU_DIM         (Color){100, 100, 100, 255}
#define MENU_OVERLAY    (Color){0, 0, 0, 180}
#define MENU_BAR_BG     (Color){40, 40, 40, 255}
#define MENU_BAR_FILL   (Color){200, 50, 50, 255}

// Scale helper — all hardcoded pixel values designed for WINDOW_H (540)
#define S(px) ((int)((px) * uiScale))

static void DrawMenuOption(const char *text, int x, int y, int size, bool selected) {
    Color col = selected ? MENU_HIGHLIGHT : MENU_TEXT;
    if (selected) {
        const char *cursor = "> ";
        int cw = MeasureText(cursor, size);
        DrawText(cursor, x - cw, y, size, MENU_HIGHLIGHT);
    }
    DrawText(text, x, y, size, col);
}

static void DrawSlider(const char *label, float value, float minVal, float maxVal,
                        int x, int y, int width, int size, bool selected, float uiScale) {
    Color labelCol = selected ? MENU_HIGHLIGHT : MENU_TEXT;
    DrawText(label, x, y, size, labelCol);

    int barX = x + S(260);
    int barY = y + size / 2 - S(6);
    int barW = width - S(260);
    int barH = S(12);

    DrawRectangle(barX, barY, barW, barH, MENU_BAR_BG);
    float frac = (value - minVal) / (maxVal - minVal);
    if (frac < 0) frac = 0;
    if (frac > 1) frac = 1;
    DrawRectangle(barX, barY, (int)(barW * frac), barH, selected ? MENU_BAR_FILL : MENU_DIM);

    char valText[32];
    snprintf(valText, sizeof(valText), "%.0f%%", frac * 100.0f);
    DrawText(valText, barX + barW + S(10), y, size, labelCol);
}

void GameInit(Game *game) {
    memset(game, 0, sizeof(Game));
    game->state = STATE_MENU;
    game->waveDelay = WAVE_DELAY;
    game->enemiesPerWave = WAVE_ENEMIES_BASE + 1;
    game->menuSelection = 0;
    game->pauseSelection = 0;
    game->settingsSelection = 0;
    game->mouseSensitivity = MOUSE_SENSITIVITY;
    game->musicVolume = AUDIO_MARCH_VOLUME;
    game->sfxVolume = 1.0f;
    game->screenScale = 2; // default 2x = 1280x720
    game->difficulty = DIFFICULTY_NORMAL;
    game->damageScaler = DIFFICULTY_NORMAL_SCALE;
    game->quitRequested = false;
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

// ==== MAIN MENU ====

void GameDrawMenu(Game *game) {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    float uiScale = (float)sh / (float)WINDOW_H;

    DrawRectangle(0, 0, sw, sh, MENU_BG);

    // Title
    int titleSize = S(80);
    const char *title = "MONDFALL";
    int titleW = MeasureText(title, titleSize);
    DrawText(title, (sw - titleW) / 2, sh / 5, titleSize, MENU_HIGHLIGHT);

    const char *subtitle = "NAZIS ON THE MOON";
    int subSize = S(30);
    int subW = MeasureText(subtitle, subSize);
    DrawText(subtitle, (sw - subW) / 2, sh / 5 + S(90), subSize, MENU_DIM);

    // Menu options
    const char *options[] = { "PLAY", "TEST MODE", "SETTINGS", "QUIT" };
    int optCount = 4;
    int optSize = S(32);
    int startY = sh / 2;
    int spacing = S(50);
    int optX = (sw - S(200)) / 2;

    // Input
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W))
        game->menuSelection = (game->menuSelection - 1 + optCount) % optCount;
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S))
        game->menuSelection = (game->menuSelection + 1) % optCount;

    for (int i = 0; i < optCount; i++) {
        DrawMenuOption(options[i], optX, startY + i * spacing, optSize, i == game->menuSelection);
    }

    // Footer
    const char *footer = "WASD/ARROWS: NAVIGATE   ENTER: SELECT";
    int fs = S(16);
    int fw = MeasureText(footer, fs);
    DrawText(footer, (sw - fw) / 2, sh - S(40), fs, MENU_DIM);

    // "Skip Intro" toggle in menu
    {
        const char *introLabel = game->introSeen ? "INTRO: SKIP" : "INTRO: SHOW";
        int introSize = S(14);
        int introW = MeasureText(introLabel, introSize);
        DrawText(introLabel, sw - introW - S(20), sh - S(35), introSize, MENU_DIM);
    }
}

// ==== SETTINGS SCREEN ====

void GameDrawSettings(Game *game) {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    float uiScale = (float)sh / (float)WINDOW_H;

    // If we came from pause, draw overlay on game; otherwise full black
    if (game->settingsReturnState == STATE_PAUSED) {
        DrawRectangle(0, 0, sw, sh, MENU_OVERLAY);
    } else {
        DrawRectangle(0, 0, sw, sh, MENU_BG);
    }

    const char *title = "SETTINGS";
    int titleSize = S(50);
    int titleW = MeasureText(title, titleSize);
    DrawText(title, (sw - titleW) / 2, sh / 6, titleSize, MENU_HIGHLIGHT);

    int optCount = 6;
    int optSize = S(24);
    int startY = sh / 4 + S(60);
    int spacing = S(46);
    int sliderW = S(400);
    int optX = (sw - sliderW) / 2;

    // Input
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W))
        game->settingsSelection = (game->settingsSelection - 1 + optCount) % optCount;
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S))
        game->settingsSelection = (game->settingsSelection + 1) % optCount;

    // Slider adjustment
    float step = 0;
    if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) step = -0.05f;
    if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) step = 0.05f;
    // Hold for continuous adjustment
    if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) {
        static float holdTimer = 0;
        holdTimer += GetFrameTime();
        if (holdTimer > 0.4f) step = -0.1f * GetFrameTime() * 5;
    } else if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) {
        static float holdTimer2 = 0;
        holdTimer2 += GetFrameTime();
        if (holdTimer2 > 0.4f) step = 0.1f * GetFrameTime() * 5;
    }

    switch (game->settingsSelection) {
        case 0: // Mouse sensitivity
            game->mouseSensitivity += step * 0.005f;
            if (game->mouseSensitivity < 0.0005f) game->mouseSensitivity = 0.0005f;
            if (game->mouseSensitivity > 0.01f) game->mouseSensitivity = 0.01f;
            break;
        case 1: // Music volume
            game->musicVolume += step;
            if (game->musicVolume < 0) game->musicVolume = 0;
            if (game->musicVolume > 1.0f) game->musicVolume = 1.0f;
            break;
        case 2: // SFX volume
            game->sfxVolume += step;
            if (game->sfxVolume < 0) game->sfxVolume = 0;
            if (game->sfxVolume > 1.0f) game->sfxVolume = 1.0f;
            break;
        case 3: // Screen scale
            if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) {
                if (game->screenScale > 1) game->screenScale--;
            }
            if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) {
                if (game->screenScale < 4) game->screenScale++;
            }
            break;
        case 4: // Difficulty
            if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) {
                if (game->difficulty > DIFFICULTY_VALKYRIE) game->difficulty--;
            }
            if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) {
                if (game->difficulty < DIFFICULTY_HARD) game->difficulty++;
            }
            // Sync scaler
            switch (game->difficulty) {
                case DIFFICULTY_VALKYRIE: game->damageScaler = DIFFICULTY_VALKYRIE_SCALE; break;
                case DIFFICULTY_EASY:     game->damageScaler = DIFFICULTY_EASY_SCALE;     break;
                case DIFFICULTY_NORMAL:   game->damageScaler = DIFFICULTY_NORMAL_SCALE;   break;
                case DIFFICULTY_HARD:     game->damageScaler = DIFFICULTY_HARD_SCALE;     break;
            }
            break;
    }

    DrawSlider("MOUSE SENS", game->mouseSensitivity, 0.0005f, 0.01f,
               optX, startY, sliderW, optSize, game->settingsSelection == 0, uiScale);
    DrawSlider("MUSIC", game->musicVolume, 0.0f, 1.0f,
               optX, startY + spacing, sliderW, optSize, game->settingsSelection == 1, uiScale);
    DrawSlider("SFX", game->sfxVolume, 0.0f, 1.0f,
               optX, startY + spacing * 2, sliderW, optSize, game->settingsSelection == 2, uiScale);

    // Screen scale option (discrete: 1x, 2x, 3x, 4x)
    {
        bool sel = (game->settingsSelection == 3);
        Color labelCol = sel ? MENU_HIGHLIGHT : MENU_TEXT;
        int sy = startY + spacing * 3;
        if (sel) {
            const char *cursor = "> ";
            int cw = MeasureText(cursor, optSize);
            DrawText(cursor, optX - cw, sy, optSize, MENU_HIGHLIGHT);
        }
        DrawText("SCREEN SCALE", optX, sy, optSize, labelCol);

        const char *scaleLabels[] = { "1x (640x360)", "2x (1280x720)", "3x (1920x1080)", "4x (2560x1440)" };
        int si = game->screenScale - 1;
        if (si < 0) si = 0;
        if (si > 3) si = 3;

        int labelX = optX + S(260);
        DrawText("< ", labelX, sy, optSize, sel ? MENU_DIM : (Color){60, 60, 60, 255});
        int arrowW = MeasureText("< ", optSize);
        DrawText(scaleLabels[si], labelX + arrowW, sy, optSize, labelCol);
        int valW = MeasureText(scaleLabels[si], optSize);
        DrawText(" >", labelX + arrowW + valW, sy, optSize, sel ? MENU_DIM : (Color){60, 60, 60, 255});
    }

    // Difficulty option (discrete: Easy, Normal, Hard)
    {
        bool sel = (game->settingsSelection == 4);
        Color labelCol = sel ? MENU_HIGHLIGHT : MENU_TEXT;
        int dy = startY + spacing * 4;
        if (sel) {
            const char *cursor = "> ";
            int cw = MeasureText(cursor, optSize);
            DrawText(cursor, optX - cw, dy, optSize, MENU_HIGHLIGHT);
        }
        DrawText("DIFFICULTY", optX, dy, optSize, labelCol);

        const char *diffLabels[] = { "VALKYRIE (0% DMG)", "WUNDERKIND (50% DMG)", "SOLDAT", "ENDKAMPF (150% DMG)" };
        int di = (int)game->difficulty;
        if (di < 0) di = 0;
        if (di > 3) di = 3;

        int labelX = optX + S(260);
        DrawText("< ", labelX, dy, optSize, sel ? MENU_DIM : (Color){60, 60, 60, 255});
        int arrowW = MeasureText("< ", optSize);
        DrawText(diffLabels[di], labelX + arrowW, dy, optSize, labelCol);
        int valW = MeasureText(diffLabels[di], optSize);
        DrawText(" >", labelX + arrowW + valW, dy, optSize, sel ? MENU_DIM : (Color){60, 60, 60, 255});
    }

    DrawMenuOption("BACK", optX, startY + spacing * 5, optSize, game->settingsSelection == 5);

    // Footer
    const char *footer = "LEFT/RIGHT: ADJUST   ENTER/ESC: BACK";
    int fs = S(16);
    int fw = MeasureText(footer, fs);
    DrawText(footer, (sw - fw) / 2, sh - S(40), fs, MENU_DIM);
}

// ==== PAUSE MENU ====

void GameDrawPaused(Game *game) {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    float uiScale = (float)sh / (float)WINDOW_H;
    DrawRectangle(0, 0, sw, sh, MENU_OVERLAY);

    int titleSize = S(60);
    const char *title = "PAUSED";
    int titleW = MeasureText(title, titleSize);
    DrawText(title, (sw - titleW) / 2, sh / 4, titleSize, WHITE);

    const char *options[] = { "RESUME", "SETTINGS", "MAIN MENU" };
    int optCount = 3;
    int optSize = S(28);
    int startY = sh / 2 - S(20);
    int spacing = S(45);
    int optX = (sw - S(200)) / 2;

    // Input
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W))
        game->pauseSelection = (game->pauseSelection - 1 + optCount) % optCount;
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S))
        game->pauseSelection = (game->pauseSelection + 1) % optCount;

    for (int i = 0; i < optCount; i++) {
        DrawMenuOption(options[i], optX, startY + i * spacing, optSize, i == game->pauseSelection);
    }

    const char *footer = "WASD/ARROWS: NAVIGATE   ENTER: SELECT   ESC: RESUME";
    int fs = S(16);
    int fw = MeasureText(footer, fs);
    DrawText(footer, (sw - fw) / 2, sh - S(40), fs, MENU_DIM);
}

// ==== GAME OVER ====

void GameDrawGameOver(Game *game) {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    float uiScale = (float)sh / (float)WINDOW_H;
    DrawRectangle(0, 0, sw, sh, (Color){0, 0, 0, 200});

    int size = S(60);
    const char *text = "GEFALLEN";
    int w = MeasureText(text, size);
    DrawText(text, (sw - w) / 2, sh / 4, size, RED);

    char stats[128];
    snprintf(stats, sizeof(stats), "WAVES SURVIVED: %d   KILLS: %d", game->wave, game->killCount);
    int ss = S(24);
    int sw2 = MeasureText(stats, ss);
    DrawText(stats, (sw - sw2) / 2, sh / 2 - S(30), ss, WHITE);

    const char *options[] = { "RESTART", "MAIN MENU" };
    int optCount = 2;
    int optSize = S(28);
    int startY = sh / 2 + S(30);
    int spacing = S(45);
    int optX = (sw - S(200)) / 2;

    // Reuse menuSelection for game over screen
    static int goSelection = 0;
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W))
        goSelection = (goSelection - 1 + optCount) % optCount;
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S))
        goSelection = (goSelection + 1) % optCount;

    for (int i = 0; i < optCount; i++) {
        DrawMenuOption(options[i], optX, startY + i * spacing, optSize, i == goSelection);
    }

    // Handle selection (checked in main.c, but we store it)
    if (IsKeyPressed(KEY_ENTER)) {
        if (goSelection == 0) {
            game->menuSelection = -1; // signal restart
        } else {
            game->menuSelection = -2; // signal main menu
        }
    }

    const char *footer = "WASD/ARROWS: NAVIGATE   ENTER: SELECT";
    int fs = S(16);
    int fw = MeasureText(footer, fs);
    DrawText(footer, (sw - fw) / 2, sh - S(40), fs, MENU_DIM);
}

// ==== INTRO LORE SCREEN ====

// Tile-reveal timing
#define INTRO_TILE_SPEED     40.0f    // tiles (chars) per second
#define INTRO_LINE_DELAY     0.6f     // pause between lines
#define INTRO_FADE_SPEED     1.5f     // fade-to-black speed
#define INTRO_TILE_FLASH     0.25f    // seconds a tile stays "hot" after reveal
#define INTRO_TILE_SCRAMBLE  3        // scramble cycles before settling
#define INTRO_TILE_GAP       0.35f    // extra gap between tiles as fraction of fontSize

// Pseudo-random scramble chars — military/cipher aesthetic
static const char SCRAMBLE_CHARS[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789#@$%&*+=<>/?";
#define SCRAMBLE_COUNT ((int)(sizeof(SCRAMBLE_CHARS) - 1))

static void TrimNewline(char *s) {
    int len = (int)strlen(s);
    while (len > 0 && (s[len-1] == '\n' || s[len-1] == '\r' || s[len-1] == ' '))
        s[--len] = '\0';
}

void IntroScriptLoad(IntroScript *script, const char *path) {
    memset(script, 0, sizeof(IntroScript));

    FILE *f = fopen(path, "r");
    if (!f) {
        strcpy(script->lines[0], "THE YEAR IS 2057. THEY HAVE COME.");
        script->lineCount = 1;
        script->pauseAfter[0] = 2.0f;
        return;
    }

    char buf[256];
    int pendingStyle = 0;
    float pendingPause = 0;
    bool pendingClear = false;

    while (fgets(buf, sizeof(buf), f) && script->lineCount < INTRO_MAX_LINES) {
        TrimNewline(buf);
        if (buf[0] == '#') continue;
        if (buf[0] == '@') {
            if (strncmp(buf, "@STYLE TITLE", 12) == 0) pendingStyle = 1;
            else if (strncmp(buf, "@STYLE DIM", 10) == 0) pendingStyle = 2;
            else if (strncmp(buf, "@PAUSE", 6) == 0) pendingPause = (float)atof(buf + 7);
            else if (strncmp(buf, "@CLEAR", 6) == 0) pendingClear = true;
            continue;
        }
        if (buf[0] == '\0') {
            if (script->lineCount > 0)
                script->pauseAfter[script->lineCount - 1] += 0.4f;
            continue;
        }
        int idx = script->lineCount;
        strncpy(script->lines[idx], buf, INTRO_MAX_LINE_LEN - 1);
        script->lines[idx][INTRO_MAX_LINE_LEN - 1] = '\0';
        script->style[idx] = pendingStyle;
        script->pauseAfter[idx] = INTRO_LINE_DELAY + pendingPause;
        script->isClear[idx] = pendingClear;
        pendingStyle = 0;
        pendingPause = 0;
        pendingClear = false;
        script->lineCount++;
    }
    fclose(f);
}

bool GameUpdateIntro(Game *game, IntroScript *script) {
    float dt = GetFrameTime();

    if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER)) {
        if (!game->introFadingOut) {
            game->introFadingOut = true;
            game->introFadeAlpha = 0.0f;
        }
        return false;
    }

    if (game->introFadingOut) {
        game->introFadeAlpha += dt * INTRO_FADE_SPEED;
        if (game->introFadeAlpha >= 1.0f) { game->introFadeAlpha = 1.0f; return true; }
        return false;
    }

    game->introTimer += dt;

    if (game->introLineIndex < script->lineCount) {
        int lineLen = (int)strlen(script->lines[game->introLineIndex]);
        if (game->introCharIndex < lineLen) {
            game->introLineTimer += dt;
            int newChars = (int)(game->introLineTimer * INTRO_TILE_SPEED);
            if (newChars > 0) {
                game->introCharIndex += newChars;
                if (game->introCharIndex > lineLen) game->introCharIndex = lineLen;
                game->introLineTimer -= (float)newChars / INTRO_TILE_SPEED;
            }
        } else {
            game->introLineTimer += dt;
            if (game->introLineTimer >= script->pauseAfter[game->introLineIndex]) {
                game->introLineIndex++;
                game->introCharIndex = 0;
                game->introLineTimer = 0;
            }
        }
    } else {
        game->introLineTimer += dt;
        if (game->introLineTimer > 2.0f) {
            game->introFadingOut = true;
            game->introFadeAlpha = 0.0f;
        }
    }
    return false;
}

// Draw a single character tile with background cell and optional glow
static void DrawCharTile(char ch, int x, int y, int fontSize, int cellW, int cellH,
                         Color textCol, float age, bool isSpace) {
    if (isSpace) return; // don't draw tiles for spaces

    // Tile background — brief bright flash that decays
    float flashT = 1.0f - (age / INTRO_TILE_FLASH);
    if (flashT < 0) flashT = 0;

    if (flashT > 0) {
        // Hot tile background glow
        unsigned char bgA = (unsigned char)(flashT * 60);
        Color bgCol = {(unsigned char)(textCol.r / 2), (unsigned char)(textCol.g / 2),
                       (unsigned char)(textCol.b / 2), bgA};
        DrawRectangle(x - 1, y - 1, cellW + 2, cellH + 1, bgCol);

        // Bright border on newest tiles
        if (flashT > 0.6f) {
            unsigned char borderA = (unsigned char)(flashT * 120);
            Color borderCol = {textCol.r, textCol.g, textCol.b, borderA};
            DrawRectangleLines(x - 1, y - 1, cellW + 2, cellH + 1, borderCol);
        }
    }

    // Scramble effect: recently revealed chars cycle through random glyphs
    char display = ch;
    if (age < INTRO_TILE_FLASH * 0.5f && ch != ' ') {
        // How many scramble frames to show based on age
        int scramblePhase = (int)((age / (INTRO_TILE_FLASH * 0.5f)) * INTRO_TILE_SCRAMBLE);
        if (scramblePhase < INTRO_TILE_SCRAMBLE) {
            // Deterministic scramble based on position + phase
            int seed = (x * 31 + y * 17 + scramblePhase * 7) & 0x7FFFFFFF;
            display = SCRAMBLE_CHARS[seed % SCRAMBLE_COUNT];
        }
    }

    // Text color — brighten during flash
    Color drawCol = textCol;
    if (flashT > 0) {
        // Lerp toward white during flash
        drawCol.r = (unsigned char)(textCol.r + (int)((255 - textCol.r) * flashT * 0.6f));
        drawCol.g = (unsigned char)(textCol.g + (int)((255 - textCol.g) * flashT * 0.6f));
        drawCol.b = (unsigned char)(textCol.b + (int)((255 - textCol.b) * flashT * 0.6f));
    }

    char buf[2] = {display, '\0'};
    DrawText(buf, x, y, fontSize, drawCol);
}

void GameDrawIntro(Game *game, IntroScript *script) {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    float uiScale = (float)sh / (float)WINDOW_H;

    DrawRectangle(0, 0, sw, sh, BLACK);

    // Subtle scanline overlay for CRT feel
    for (int row = 0; row < sh; row += 4) {
        DrawRectangle(0, row, sw, 1, (Color){0, 0, 0, 25});
    }

    // Find first visible line after last @CLEAR
    int firstVisible = 0;
    for (int i = 0; i <= game->introLineIndex && i < script->lineCount; i++) {
        if (script->isClear[i]) firstVisible = i;
    }

    int normalSize = S(20);
    int titleSize = S(28);
    int lineSpacing = S(36);
    int visibleCount = game->introLineIndex - firstVisible + 1;
    if (game->introLineIndex >= script->lineCount)
        visibleCount = script->lineCount - firstVisible;
    if (visibleCount < 0) visibleCount = 0;

    int totalHeight = visibleCount * lineSpacing;
    int startY = (sh - totalHeight) / 2;

    for (int i = firstVisible; i < firstVisible + visibleCount && i < script->lineCount; i++) {
        int lineIdx = i - firstVisible;
        int y = startY + lineIdx * lineSpacing;
        int lineLen = (int)strlen(script->lines[i]);

        int fontSize = (script->style[i] == 1) ? titleSize : normalSize;
        Color col;
        if (script->style[i] == 1) col = MENU_HIGHLIGHT;
        else if (script->style[i] == 2) col = MENU_DIM;
        else col = MENU_TEXT;

        int cellH = fontSize;
        int tileGap = (int)(fontSize * INTRO_TILE_GAP);

        // Center the line — account for tile gaps
        int fullW = 0;
        for (int c = 0; c < lineLen; c++) {
            char tmp[2] = {script->lines[i][c], '\0'};
            fullW += MeasureText(tmp, fontSize);
            if (c < lineLen - 1) fullW += tileGap;
        }
        int baseX = (sw - fullW) / 2;

        int revealedChars = lineLen; // fully revealed for past lines
        if (i == game->introLineIndex) {
            revealedChars = game->introCharIndex;
            if (revealedChars > lineLen) revealedChars = lineLen;
        } else if (i > game->introLineIndex) {
            revealedChars = 0;
        }

        // Draw each character as an individual tile
        int curX = baseX;
        for (int c = 0; c < revealedChars; c++) {
            char ch = script->lines[i][c];

            // Age: how long ago this char was revealed
            // For current line: based on char index difference
            // For past lines: old enough that flash is done
            float charAge;
            if (i < game->introLineIndex) {
                charAge = 999.0f; // fully settled
            } else {
                // Approximate: each char revealed at 1/TILE_SPEED intervals
                float charsAgo = (float)(game->introCharIndex - c);
                charAge = charsAgo / INTRO_TILE_SPEED + game->introLineTimer;
            }

            // Measure this specific character's width for proper spacing
            char chBuf[2] = {ch, '\0'};
            int charW = MeasureText(chBuf, fontSize);

            DrawCharTile(ch, curX, y, fontSize, charW, cellH, col, charAge, ch == ' ');
            curX += charW + tileGap;
        }

        // For the active line, draw dim placeholder cells for unrevealed chars
        if (i == game->introLineIndex && revealedChars < lineLen) {
            for (int c = revealedChars; c < lineLen; c++) {
                char ch = script->lines[i][c];
                char chBuf[2] = {ch, '\0'};
                int charW = MeasureText(chBuf, fontSize);
                if (ch != ' ') {
                    // Very faint placeholder dot
                    DrawRectangle(curX + charW/2 - 1, y + cellH/2 - 1, 2, 2, (Color){60, 60, 60, 40});
                }
                curX += charW + tileGap;
            }
        }
    }

    // Skip prompt
    const char *skip = "SPACE / ENTER  —  SKIP";
    int skipSize = S(14);
    int skipW = MeasureText(skip, skipSize);
    float pulse = (sinf(game->introTimer * 2.0f) + 1.0f) * 0.5f;
    unsigned char alpha = (unsigned char)(80 + (int)(pulse * 80));
    DrawText(skip, (sw - skipW) / 2, sh - S(35), skipSize, (Color){150, 150, 150, alpha});

    // Fade overlay
    if (game->introFadingOut) {
        unsigned char fadeA = (unsigned char)(game->introFadeAlpha * 255);
        DrawRectangle(0, 0, sw, sh, (Color){0, 0, 0, fadeA});
    }
}
