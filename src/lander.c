#include "lander.h"
#include "world.h"
#include "rlgl.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>

void LanderManagerInit(LanderManager *lm) {
    memset(lm, 0, sizeof(LanderManager));
    // Impact sound — deep thud
    {
        int sr = 44100, n = (int)(0.8f * sr);
        Wave w = {0}; w.frameCount = n; w.sampleRate = sr; w.sampleSize = 16; w.channels = 1;
        w.data = RL_CALLOC(n, sizeof(short));
        short *d = (short *)w.data;
        for (int i = 0; i < n; i++) {
            float t = (float)i / sr;
            d[i] = (short)(sinf(t * 40.0f * 6.283f) * expf(-t * 4.0f) * 28000.0f
                          + ((float)rand()/RAND_MAX*2-1) * expf(-t * 6.0f) * 12000.0f);
        }
        lm->sndImpact = LoadSoundFromWave(w); UnloadWave(w);
    }
    // Explosion sound
    {
        int sr = 44100, n = (int)(1.0f * sr);
        Wave w = {0}; w.frameCount = n; w.sampleRate = sr; w.sampleSize = 16; w.channels = 1;
        w.data = RL_CALLOC(n, sizeof(short));
        short *d = (short *)w.data;
        for (int i = 0; i < n; i++) {
            float t = (float)i / sr;
            d[i] = (short)((sinf(t*55*6.283f)*0.4f + ((float)rand()/RAND_MAX*2-1)*0.5f)
                          * expf(-t * 3.0f) * 30000.0f);
        }
        lm->sndExplode = LoadSoundFromWave(w); UnloadWave(w);
    }
    // Air raid klaxon — WW2 style siren, 3 rising wails
    {
        int sr = 44100, n = (int)(3.0f * sr);
        Wave w = {0}; w.frameCount = n; w.sampleRate = sr; w.sampleSize = 16; w.channels = 1;
        w.data = RL_CALLOC(n, sizeof(short));
        short *d = (short *)w.data;
        for (int i = 0; i < n; i++) {
            float t = (float)i / sr;
            // Three wail cycles over 3 seconds
            float cycle = fmodf(t, 1.0f);
            // Deep, menacing air raid siren
            float freq = 100.0f + cycle * cycle * 200.0f; // 100-300Hz — very deep
            // Main tone — sawtooth for that harsh mechanical siren sound
            float saw = fmodf(t * freq, 1.0f) * 2.0f - 1.0f;
            // Add second harmonic
            float harm = sinf(t * freq * 2.0f * 6.283f) * 0.3f;
            // Amplitude envelope per wail — loud then brief dip at cycle boundary
            float env = (cycle < 0.9f) ? 1.0f : (1.0f - cycle) / 0.1f;
            // Overall fade in/out
            float master = 1.0f;
            if (t < 0.1f) master = t / 0.1f;
            if (t > 2.7f) master = (3.0f - t) / 0.3f;

            d[i] = (short)((saw * 0.5f + harm) * env * master * 24000.0f);
        }
        lm->sndKlaxon = LoadSoundFromWave(w); UnloadWave(w);
    }
    lm->soundLoaded = true;
}

void LanderManagerUnload(LanderManager *lm) {
    if (lm->soundLoaded) {
        UnloadSound(lm->sndImpact);
        UnloadSound(lm->sndExplode);
        UnloadSound(lm->sndKlaxon);
    }
}

void LanderSpawnWave(LanderManager *lm, Vector3 playerPos, int enemyCount, int wave) {
    // Spawn 1-3 landers around the player
    int landerCount = 1 + (wave / 3);
    if (wave >= 5) landerCount = 2 + (wave / 2);
    if (landerCount > MAX_LANDERS) landerCount = MAX_LANDERS;
    int perLander = enemyCount / landerCount;

    PlaySound(lm->sndKlaxon);
    lm->klaxonPlayed = true;
    for (int i = 0; i < landerCount; i++) {
        Lander *l = &lm->landers[i];
        float angle = ((float)rand() / RAND_MAX) * 2.0f * PI;
        float dist = 30.0f + ((float)rand() / RAND_MAX) * 25.0f;

        l->targetPos = (Vector3){
            playerPos.x + cosf(angle) * dist,
            0,
            playerPos.z + sinf(angle) * dist
        };
        l->targetPos.y = WorldGetHeight(l->targetPos.x, l->targetPos.z);

        // Away direction = from player toward lander (enemies exit away from player)
        float dx = l->targetPos.x - playerPos.x;
        float dz = l->targetPos.z - playerPos.z;
        float len = sqrtf(dx*dx + dz*dz);
        if (len > 0.1f) { l->awayDirX = dx/len; l->awayDirZ = dz/len; }
        else { l->awayDirX = 1; l->awayDirZ = 0; }

        // Start high in the sky
        l->position = l->targetPos;
        l->position.y += 80.0f + ((float)rand() / RAND_MAX) * 40.0f;

        l->state = LANDER_WAITING;
        l->timer = 0;
        l->enemiesDeployed = 0;
        l->enemiesTotal = (i < landerCount - 1) ? perLander : enemyCount - perLander * i;
        l->factionType = (rand() % 2) ? ENEMY_SOVIET : ENEMY_AMERICAN;
        l->shakeAmount = 0;
    }
}

void LanderManagerUpdate(LanderManager *lm, EnemyManager *em, float dt) {
    for (int i = 0; i < MAX_LANDERS; i++) {
        Lander *l = &lm->landers[i];
        if (l->state == LANDER_INACTIVE || l->state == LANDER_DONE) continue;

        l->timer += dt;

        switch (l->state) {
            case LANDER_WAITING: {
                // Wait for klaxon to finish (3 seconds) then start descent
                if (l->timer > 3.5f) {
                    l->state = LANDER_DESCENDING;
                    l->timer = 0;
                }
                break;
            }
            case LANDER_DESCENDING: {
                // Fall toward target — decelerate near ground
                float distToGround = l->position.y - l->targetPos.y;
                float speed = 15.0f + distToGround * 0.5f;
                l->position.y -= speed * dt;
                // Retro rockets shake
                l->shakeAmount = 0.3f;

                if (l->position.y <= l->targetPos.y + 0.5f) {
                    l->position.y = l->targetPos.y + 0.5f;
                    l->state = LANDER_LANDED;
                    l->timer = 0;
                    l->shakeAmount = 2.0f; // big impact shake
                    PlaySound(lm->sndImpact);
                }
                break;
            }
            case LANDER_LANDED: {
                // Deploy enemies one at a time
                l->shakeAmount *= 0.95f;
                if (l->enemiesDeployed < l->enemiesTotal && l->timer > 0.5f) {
                    if (fmodf(l->timer, 0.4f) < dt) {
                        // Spawn at ramp on the side AWAY from player
                        // (lander.awayDir calculated at landing)
                        Vector3 hatchPos = l->position;
                        hatchPos.x += l->awayDirX * (3.0f + ((float)rand()/RAND_MAX) * 2.0f);
                        hatchPos.z += l->awayDirZ * (3.0f + ((float)rand()/RAND_MAX) * 2.0f);
                        hatchPos.x += ((float)rand()/RAND_MAX - 0.5f) * 1.0f;
                        hatchPos.z += ((float)rand()/RAND_MAX - 0.5f) * 1.0f;
                        hatchPos.y = WorldGetHeight(hatchPos.x, hatchPos.z) + 1.2f;
                        EnemySpawnAt(em, l->factionType, hatchPos);
                        l->enemiesDeployed++;
                    }
                }
                // All deployed — wait a beat then explode
                if (l->enemiesDeployed >= l->enemiesTotal && l->timer > l->enemiesTotal * 0.4f + 1.5f) {
                    l->state = LANDER_EXPLODING;
                    l->timer = 0;
                    l->shakeAmount = 3.0f;
                    PlaySound(lm->sndExplode);
                }
                break;
            }
            case LANDER_EXPLODING: {
                l->shakeAmount *= 0.9f;
                if (l->timer > 1.5f) {
                    l->state = LANDER_DONE;
                }
                break;
            }
            default: break;
        }
    }
}

void LanderManagerDraw(LanderManager *lm, Vector3 playerPos) {
    for (int i = 0; i < MAX_LANDERS; i++) {
        Lander *l = &lm->landers[i];
        if (l->state == LANDER_INACTIVE || l->state == LANDER_DONE || l->state == LANDER_WAITING) continue;

        Vector3 p = l->position;

        if (l->state == LANDER_EXPLODING) {
            // Explosion fireball
            float t = l->timer / 1.5f;
            float r = 4.0f + t * 8.0f;
            unsigned char a = (unsigned char)((1.0f - t) * 220);
            DrawSphereEx(p, r * 0.5f, 6, 6, (Color){255, 240, 180, a});
            DrawSphereEx(p, r * 0.3f, 5, 5, (Color){255, 180, 50, a});
            DrawSphereEx(p, r, 6, 6, (Color){255, 80, 0, (unsigned char)(a * 0.4f)});
            // Debris chunks
            for (int d = 0; d < 10; d++) {
                float da = (float)d * 0.628f + t * 3.0f;
                float dr = r * 0.8f * t;
                float dy = t * 6.0f - t * t * 8.0f;
                Vector3 dp = {p.x + cosf(da)*dr, p.y + dy + (d%3)*0.5f, p.z + sinf(da)*dr};
                DrawCube(dp, 0.4f, 0.3f, 0.4f, (Color){120, 120, 115, a});
            }
            return;
        }

        // === LANDER BODY — Apollo-style descent module ===
        Color body = {200, 195, 185, 255};
        Color dark = {140, 138, 132, 255};
        Color gold = {220, 200, 80, 255};

        rlPushMatrix();
        rlTranslatef(p.x, p.y, p.z);

        // Main descent stage — octagonal body (approximated with rotated cubes)
        DrawCube((Vector3){0, 1.5f, 0}, 3.0f, 2.5f, 3.0f, body);
        rlPushMatrix();
        rlRotatef(45, 0, 1, 0);
        DrawCube((Vector3){0, 1.5f, 0}, 3.0f, 2.5f, 3.0f, body);
        rlPopMatrix();
        DrawCubeWires((Vector3){0, 1.5f, 0}, 3.01f, 2.51f, 3.01f, dark);

        // Gold foil panels
        DrawCube((Vector3){1.52f, 1.5f, 0}, 0.02f, 2.0f, 2.0f, gold);
        DrawCube((Vector3){-1.52f, 1.5f, 0}, 0.02f, 2.0f, 2.0f, gold);
        DrawCube((Vector3){0, 1.5f, 1.52f}, 2.0f, 2.0f, 0.02f, gold);
        DrawCube((Vector3){0, 1.5f, -1.52f}, 2.0f, 2.0f, 0.02f, gold);

        // Landing legs (4)
        for (int leg = 0; leg < 4; leg++) {
            float la = leg * 1.5708f;
            float lx = cosf(la) * 2.5f;
            float lz = sinf(la) * 2.5f;
            // Strut
            DrawCube((Vector3){lx * 0.6f, 0.5f, lz * 0.6f}, 0.12f, 2.0f, 0.12f, dark);
            // Foot pad
            DrawCube((Vector3){lx, -0.3f, lz}, 0.8f, 0.1f, 0.8f, dark);
        }

        // Rocket nozzle underneath
        DrawCube((Vector3){0, -0.2f, 0}, 0.8f, 0.4f, 0.8f, dark);

        // Retro rockets firing during descent
        if (l->state == LANDER_DESCENDING) {
            float flame = 2.0f + sinf(l->timer * 20.0f) * 0.5f;
            DrawSphereEx((Vector3){0, -0.5f, 0}, 0.6f, 5, 5, (Color){255, 200, 50, 200});
            DrawSphereEx((Vector3){0, -0.5f - flame, 0}, 1.0f, 5, 5, (Color){255, 120, 20, 120});
            DrawSphereEx((Vector3){0, -0.5f - flame * 1.5f, 0}, 0.7f, 4, 4, (Color){255, 80, 0, 60});
        }

        // Hatch (front) — open when deploying
        if (l->state == LANDER_LANDED) {
            DrawCube((Vector3){0, 0.8f, 1.6f}, 1.0f, 1.2f, 0.05f, (Color){100, 100, 95, 255});
            // Ramp
            DrawCube((Vector3){0, 0.0f, 2.5f}, 1.0f, 0.05f, 1.5f, dark);
        }

        // Faction flag on side — pole + flag
        // Flag pole
        DrawCube((Vector3){1.55f, 2.8f, 0.5f}, 0.04f, 1.2f, 0.04f, (Color){160,160,155,255});
        if (l->factionType == ENEMY_SOVIET) {
            // Soviet flag — red background
            DrawCube((Vector3){1.57f, 3.1f, 0.0f}, 0.02f, 0.6f, 0.9f, (Color){200, 35, 25, 255});
            // Yellow star
            DrawCube((Vector3){1.59f, 3.2f, 0.15f}, 0.02f, 0.15f, 0.15f, (Color){255, 220, 50, 255});
            // Hammer shape
            DrawCube((Vector3){1.59f, 3.05f, -0.05f}, 0.02f, 0.08f, 0.2f, (Color){255, 220, 50, 255});
            DrawCube((Vector3){1.59f, 3.0f, -0.15f}, 0.02f, 0.15f, 0.04f, (Color){255, 220, 50, 255});
        } else {
            // American flag — blue canton + red/white stripes
            DrawCube((Vector3){1.57f, 3.1f, 0.0f}, 0.02f, 0.6f, 0.9f, (Color){220, 220, 220, 255});
            // Red stripes
            for (int s = 0; s < 4; s++) {
                float sy = 2.85f + s * 0.15f;
                DrawCube((Vector3){1.59f, sy, 0.0f}, 0.02f, 0.06f, 0.9f, (Color){200, 40, 40, 255});
            }
            // Blue canton (top-left)
            DrawCube((Vector3){1.59f, 3.25f, 0.3f}, 0.02f, 0.3f, 0.35f, (Color){30, 50, 150, 255});
            // White star dot
            DrawCube((Vector3){1.61f, 3.25f, 0.3f}, 0.02f, 0.08f, 0.08f, WHITE);
        }
        // Same flag on other side
        if (l->factionType == ENEMY_SOVIET) {
            DrawCube((Vector3){-1.55f, 2.8f, -0.5f}, 0.04f, 1.2f, 0.04f, (Color){160,160,155,255});
            DrawCube((Vector3){-1.57f, 3.1f, 0.0f}, 0.02f, 0.6f, 0.9f, (Color){200, 35, 25, 255});
        } else {
            DrawCube((Vector3){-1.55f, 2.8f, -0.5f}, 0.04f, 1.2f, 0.04f, (Color){160,160,155,255});
            DrawCube((Vector3){-1.57f, 3.1f, 0.0f}, 0.02f, 0.6f, 0.9f, (Color){220, 220, 220, 255});
            for (int s = 0; s < 4; s++)
                DrawCube((Vector3){-1.59f, 2.85f+s*0.15f, 0.0f}, 0.02f, 0.06f, 0.9f, (Color){200,40,40,255});
        }

        rlPopMatrix();
    }

    // (compass drawn on HUD layer instead)
    (void)playerPos;
}

float LanderGetScreenShake(LanderManager *lm) {
    float shake = 0;
    for (int i = 0; i < MAX_LANDERS; i++) {
        if (lm->landers[i].shakeAmount > shake)
            shake = lm->landers[i].shakeAmount;
    }
    return shake;
}

bool LanderWaveActive(LanderManager *lm) {
    for (int i = 0; i < MAX_LANDERS; i++) {
        LanderState s = lm->landers[i].state;
        if (s == LANDER_WAITING || s == LANDER_DESCENDING || s == LANDER_LANDED || s == LANDER_EXPLODING)
            return true;
    }
    return false;
}

int LanderEnemiesRemaining(LanderManager *lm) {
    int total = 0;
    for (int i = 0; i < MAX_LANDERS; i++) {
        if (lm->landers[i].state == LANDER_LANDED)
            total += lm->landers[i].enemiesTotal - lm->landers[i].enemiesDeployed;
    }
    return total;
}
