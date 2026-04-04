#ifndef GAME_H
#define GAME_H

#include "raylib.h"

typedef enum {
    STATE_MENU,
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
} Game;

void GameInit(Game *game);
void GameUpdate(Game *game);
void GameDrawMenu(Game *game);
void GameDrawPaused(void);
void GameDrawGameOver(Game *game);

#endif
