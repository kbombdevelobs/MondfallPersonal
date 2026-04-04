#ifndef GAME_H
#define GAME_H

#include "raylib.h"
#include "config.h"

typedef enum {
    STATE_MENU,
    STATE_INTRO,
    STATE_SETTINGS,
    STATE_PLAYING,
    STATE_PAUSED,
    STATE_GAME_OVER
} GameState;

typedef enum {
    DIFFICULTY_VALKYRIE,
    DIFFICULTY_EASY,
    DIFFICULTY_NORMAL,
    DIFFICULTY_HARD
} Difficulty;

typedef struct {
    GameState state;
    int wave;
    int killCount;
    float waveTimer;
    float waveDelay;
    int enemiesRemaining;
    int enemiesSpawned;
    int enemiesPerWave;
    bool waveActive;
    float spawnTimer;

    // Menu navigation
    int menuSelection;       // 0=Play, 1=Settings, 2=Quit
    int pauseSelection;      // 0=Resume, 1=Settings, 2=Main Menu
    int settingsSelection;   // 0=Mouse Sens, 1=Music Vol, 2=SFX Vol, 3=Back
    GameState settingsReturnState; // where to go back from settings

    // Settings values
    float mouseSensitivity;
    float musicVolume;
    float sfxVolume;
    int screenScale;        // 1-4 (1x=640x360, 2x=1280x720, etc.)
    Difficulty difficulty;
    float damageScaler;     // multiplier on incoming player damage
    bool quitRequested;

    // Intro lore screen
    bool introSeen;           // true after intro shown once (skip on restart)
    float introTimer;         // time elapsed in intro sequence
    float introLineTimer;     // time for current line typewriter
    int introLineIndex;       // which line we're revealing
    int introCharIndex;       // typewriter char position in current line
    bool introSkipHeld;       // debounce for skip
    bool introFadingOut;      // fade to black before gameplay
    float introFadeAlpha;     // 0..1 fade progress
} Game;

void GameInit(Game *game);
void GameReset(Game *game);
void GameUpdate(Game *game);
void GameDrawMenu(Game *game);
void GameDrawSettings(Game *game);
void GameDrawPaused(Game *game);
void GameDrawGameOver(Game *game);

// Intro lore screen
#define INTRO_MAX_LINES 64
#define INTRO_MAX_LINE_LEN 128

typedef struct {
    char lines[INTRO_MAX_LINES][INTRO_MAX_LINE_LEN];
    int lineCount;
    float pauseAfter[INTRO_MAX_LINES];   // extra pause after each line
    bool isClear[INTRO_MAX_LINES];       // @CLEAR directive
    int style[INTRO_MAX_LINES];          // 0=normal, 1=title, 2=dim
} IntroScript;

void IntroScriptLoad(IntroScript *script, const char *path);
void GameDrawIntro(Game *game, IntroScript *script);
bool GameUpdateIntro(Game *game, IntroScript *script);

#endif
