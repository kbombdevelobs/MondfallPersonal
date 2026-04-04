#ifndef LANDER_H
#define LANDER_H

#include "raylib.h"
#include "raymath.h"
#include "enemy.h"

#define MAX_LANDERS 4

typedef enum {
    LANDER_INACTIVE,
    LANDER_WAITING,     // klaxon playing, not yet visible
    LANDER_DESCENDING,  // falling from sky
    LANDER_LANDED,      // on ground, deploying enemies
    LANDER_EXPLODING,   // done deploying, self-destruct
    LANDER_DONE
} LanderState;

typedef struct {
    Vector3 position;
    Vector3 targetPos;
    LanderState state;
    float timer;
    float shakeAmount;
    int enemiesDeployed;
    int enemiesTotal;
    EnemyType factionType;
    float awayDirX, awayDirZ; // direction away from player (for hatch)
} Lander;

typedef struct {
    Lander landers[MAX_LANDERS];
    Sound sndImpact;
    Sound sndExplode;
    Sound sndKlaxon;
    bool klaxonPlayed;
    bool soundLoaded;
} LanderManager;

void LanderManagerInit(LanderManager *lm);
void LanderManagerUnload(LanderManager *lm);
void LanderSpawnWave(LanderManager *lm, Vector3 playerPos, int enemyCount, int wave);
void LanderManagerUpdate(LanderManager *lm, EnemyManager *em, float dt);
void LanderManagerDraw(LanderManager *lm, Vector3 playerPos);
float LanderGetScreenShake(LanderManager *lm);
bool LanderWaveActive(LanderManager *lm);
int LanderEnemiesRemaining(LanderManager *lm);

#endif
