#ifndef GAME_H
#define GAME_H

#include "raylib.h"
#include "config.h"

typedef enum {
    STATE_MENU,
    STATE_SETTINGS,
    STATE_PLAYING,
    STATE_PAUSED,
    STATE_GAME_OVER
} GameState;

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
    bool quitRequested;
} Game;

void GameInit(Game *game);
void GameReset(Game *game);
void GameUpdate(Game *game);
void GameDrawMenu(Game *game);
void GameDrawSettings(Game *game);
void GameDrawPaused(Game *game);
void GameDrawGameOver(Game *game);

#endif
