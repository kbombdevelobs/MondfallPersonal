#include "game.h"
#include "config.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// Menu color theme (shared with game.c)
#define MENU_HIGHLIGHT  (Color){200, 50, 50, 255}
#define MENU_TEXT        (Color){200, 200, 200, 255}
#define MENU_DIM         (Color){100, 100, 100, 255}

// Scale helper — all hardcoded pixel values designed for WINDOW_H (540)
#define S(px) ((int)((px) * uiScale))

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
            float charAge;
            if (i < game->introLineIndex) {
                charAge = 999.0f; // fully settled
            } else {
                float charsAgo = (float)(game->introCharIndex - c);
                charAge = charsAgo / INTRO_TILE_SPEED + game->introLineTimer;
            }

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
