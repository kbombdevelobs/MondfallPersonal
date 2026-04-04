#include "enemy.h"
#include "world.h"
#include "rlgl.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>

void EnemyManagerInit(EnemyManager *em) {
    memset(em, 0, sizeof(EnemyManager));
    em->spawnRate = 1.0f;

    em->mdlVisor = LoadModelFromMesh(GenMeshSphere(0.28f, 6, 6));
    em->mdlArm = LoadModelFromMesh(GenMeshCube(0.22f, 0.8f, 0.22f));
    em->mdlBoot = LoadModelFromMesh(GenMeshCube(0.28f, 0.3f, 0.35f));

    // Enemy shooting sounds
    {
        int sr = 44100, samples = (int)(0.1f * sr);
        Wave w = {0}; w.frameCount = samples; w.sampleRate = sr; w.sampleSize = 16; w.channels = 1;
        w.data = RL_CALLOC(samples, sizeof(short));
        short *d = (short *)w.data;
        for (int i = 0; i < samples; i++) {
            float t = (float)i / sr;
            d[i] = (short)(((float)rand()/RAND_MAX*2-1) * expf(-t*35) * 18000 + sinf(t*150*2*3.14159f)*expf(-t*20)*10000);
        }
        em->sndSovietFire = LoadSoundFromWave(w);
        UnloadWave(w);
    }
    {
        int sr = 44100, samples = (int)(0.08f * sr);
        Wave w = {0}; w.frameCount = samples; w.sampleRate = sr; w.sampleSize = 16; w.channels = 1;
        w.data = RL_CALLOC(samples, sizeof(short));
        short *d = (short *)w.data;
        for (int i = 0; i < samples; i++) {
            float t = (float)i / sr;
            d[i] = (short)(sinf(t*800*2*3.14159f) * expf(-t*40) * 16000);
        }
        em->sndAmericanFire = LoadSoundFromWave(w);
        UnloadWave(w);
    }

    em->modelsLoaded = true;
}

void EnemyManagerUnload(EnemyManager *em) {
    if (!em->modelsLoaded) return;
    UnloadModel(em->mdlVisor);
    UnloadModel(em->mdlArm);
    UnloadModel(em->mdlBoot);
    UnloadSound(em->sndSovietFire);
    UnloadSound(em->sndAmericanFire);
}

static void EnemyInit(Enemy *e, EnemyType type, Vector3 pos) {
    memset(e, 0, sizeof(Enemy));
    e->position = pos;
    e->type = type;
    e->state = ENEMY_ALIVE;
    e->animState = ANIM_IDLE;
    e->active = true;
    e->strafeDir = (rand() % 2) ? 1.0f : -1.0f;
    e->strafeTimer = 1.5f + (float)rand() / RAND_MAX * 2.0f;
    e->dodgeCooldown = 3.0f;

    if (type == ENEMY_SOVIET) {
        e->health = 80; e->maxHealth = 80;
        e->speed = 5.5f; e->damage = 7;
        e->attackRange = 22; e->attackRate = 0.15f;
        e->preferredDist = 8; e->behavior = AI_ADVANCE;
    } else {
        e->health = 55; e->maxHealth = 55;
        e->speed = 5; e->damage = 10;
        e->attackRange = 30; e->attackRate = 0.4f;
        e->preferredDist = 18; e->behavior = AI_STRAFE;
    }
}

void EnemySpawnAroundPlayer(EnemyManager *em, EnemyType type, Vector3 playerPos, float spawnRadius) {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (em->enemies[i].active) continue;
        float angle = ((float)rand() / RAND_MAX) * 2.0f * PI;
        float dist = spawnRadius + ((float)rand() / RAND_MAX) * 20.0f;
        Vector3 pos = { playerPos.x + cosf(angle) * dist, 0, playerPos.z + sinf(angle) * dist };
        pos.y = WorldGetHeight(pos.x, pos.z) + 1.2f;
        EnemyInit(&em->enemies[i], type, pos);
        em->count++;
        return;
    }
}

void EnemySpawnAt(EnemyManager *em, EnemyType type, Vector3 pos) {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (em->enemies[i].active) continue;
        EnemyInit(&em->enemies[i], type, pos);
        em->count++;
        return;
    }
}

static bool TooCloseToOthers(EnemyManager *em, int idx, float minDist) {
    Enemy *me = &em->enemies[idx];
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (i == idx || !em->enemies[i].active || em->enemies[i].state != ENEMY_ALIVE) continue;
        if (Vector3Distance(me->position, em->enemies[i].position) < minDist) return true;
    }
    return false;
}

void EnemyManagerUpdate(EnemyManager *em, Vector3 playerPos, float dt) {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        Enemy *e = &em->enemies[i];
        if (!e->active) continue;

        if (e->hitFlash > 0) e->hitFlash -= dt * 4;
        if (e->muzzleFlash > 0) e->muzzleFlash -= dt * 8;
        if (e->dodgeTimer > 0) e->dodgeTimer -= dt;
        if (e->burstCooldown > 0) e->burstCooldown -= dt;

        if (e->state == ENEMY_DYING) {
            e->animState = ANIM_DEATH;
            e->deathTimer -= dt;
            e->deathAngle += dt * 150;
            if (e->deathAngle > 90) e->deathAngle = 90;
            e->position.y -= dt * 1.5f;
            float gH = WorldGetHeight(e->position.x, e->position.z) + 0.2f;
            if (e->position.y < gH) e->position.y = gH;
            if (e->deathTimer <= 0) { e->state = ENEMY_DEAD; e->active = false; em->count--; }
            continue;
        }
        if (e->state != ENEMY_ALIVE) continue;

        Vector3 toPlayer = Vector3Subtract(playerPos, e->position);
        toPlayer.y = 0;
        float dist = Vector3Length(toPlayer);
        if (dist > 0.1f) {
            float tgt = atan2f(toPlayer.x, toPlayer.z);
            float diff = tgt - e->facingAngle;
            while (diff > PI) diff -= 2*PI;
            while (diff < -PI) diff += 2*PI;
            e->facingAngle += diff * dt * 6;
        }

        Vector3 fwd = {sinf(e->facingAngle), 0, cosf(e->facingAngle)};
        Vector3 strafe = {cosf(e->facingAngle), 0, -sinf(e->facingAngle)};

        e->behaviorTimer += dt;
        e->strafeTimer -= dt;
        if (e->strafeTimer <= 0) {
            e->strafeDir *= -1;
            e->strafeTimer = 1.0f + (float)rand() / RAND_MAX * 2.5f;
        }

        Vector3 moveDir = {0};
        bool moving = false;

        if (e->type == ENEMY_SOVIET) {
            // SOVIET: Charge as a spread — run at player with wide strafe to form a line
            if (dist > 4.0f) {
                // Sprint toward player with wide spread
                moveDir = Vector3Add(fwd, Vector3Scale(strafe, e->strafeDir * 0.6f));
                moving = true; e->behavior = AI_ADVANCE;
            } else {
                // In melee range — circle strafe fast
                moveDir = Vector3Add(Vector3Scale(strafe, e->strafeDir * 1.5f), Vector3Scale(fwd, 0.2f));
                moving = true; e->behavior = AI_STRAFE;
            }
            // Occasional dodge when close
            if (e->dodgeTimer <= 0 && dist < 12 && (rand() % 80) < 2) {
                moveDir = Vector3Scale(strafe, e->strafeDir * 3.5f);
                e->dodgeTimer = e->dodgeCooldown; e->behavior = AI_DODGE;
            }
        } else {
            // AMERICAN: Tactical — try to find cover behind nearby rocks
            // Check for rocks to hide behind
            World *w = WorldGetActive();
            bool foundCover = false;
            if (w && dist < e->attackRange * 1.2f && dist > 5.0f) {
                // Search nearby chunks for a rock between us and player
                for (int ci = 0; ci < w->chunkCount && !foundCover; ci++) {
                    if (!w->chunks[ci].generated) continue;
                    for (int ri = 0; ri < w->chunks[ci].rockCount && !foundCover; ri++) {
                        Rock *rock = &w->chunks[ci].rocks[ri];
                        float rockDist = Vector3Distance(e->position, rock->position);
                        if (rockDist > 3.0f && rockDist < 15.0f) {
                            // Is the rock between us and the player?
                            Vector3 toRock = Vector3Subtract(rock->position, e->position);
                            toRock.y = 0;
                            Vector3 toP = Vector3Subtract(playerPos, e->position);
                            toP.y = 0;
                            float dot = toRock.x * toP.x + toRock.z * toP.z;
                            if (dot > 0) {
                                // Move toward cover position (behind rock relative to player)
                                Vector3 coverDir = Vector3Normalize(Vector3Subtract(rock->position, playerPos));
                                Vector3 coverPos = Vector3Add(rock->position, Vector3Scale(coverDir, 2.5f));
                                Vector3 toCover = Vector3Subtract(coverPos, e->position);
                                toCover.y = 0;
                                if (Vector3Length(toCover) > 1.5f) {
                                    moveDir = Vector3Normalize(toCover);
                                    moving = true; e->behavior = AI_RETREAT;
                                    foundCover = true;
                                }
                            }
                        }
                    }
                }
            }

            if (!foundCover) {
                // No cover — default tactical behavior
                if (dist < e->preferredDist * 0.5f) {
                    // Too close — retreat while strafing
                    moveDir = Vector3Add(Vector3Scale(fwd, -1), Vector3Scale(strafe, e->strafeDir * 0.7f));
                    moving = true; e->behavior = AI_RETREAT;
                } else if (dist > e->preferredDist * 1.4f) {
                    // Too far — advance cautiously with strafe
                    moveDir = Vector3Add(fwd, Vector3Scale(strafe, e->strafeDir * 0.4f));
                    moving = true; e->behavior = AI_ADVANCE;
                } else {
                    // Good range — strafe and peek
                    moveDir = Vector3Scale(strafe, e->strafeDir);
                    moving = true; e->behavior = AI_STRAFE;
                }
            }

            // Dodge when taking damage
            if (e->health < e->maxHealth * 0.4f && e->dodgeTimer <= 0 && (rand() % 60) < 3) {
                moveDir = Vector3Scale(strafe, e->strafeDir * 3.0f);
                e->dodgeTimer = e->dodgeCooldown * 0.4f; e->behavior = AI_DODGE;
            }
        }

        if (TooCloseToOthers(em, i, 2.5f)) {
            for (int j = 0; j < MAX_ENEMIES; j++) {
                if (j == i || !em->enemies[j].active) continue;
                Vector3 away = Vector3Subtract(e->position, em->enemies[j].position);
                away.y = 0;
                if (Vector3Length(away) < 2.5f && Vector3Length(away) > 0.1f) {
                    moveDir = Vector3Add(moveDir, Vector3Scale(Vector3Normalize(away), 0.5f));
                    break;
                }
            }
        }

        if (moving && Vector3Length(moveDir) > 0.01f) {
            moveDir = Vector3Normalize(moveDir);
            float spd = e->speed * (e->behavior == AI_DODGE ? 2.0f : 1.0f);
            e->position.x += moveDir.x * spd * dt;
            e->position.z += moveDir.z * spd * dt;
            e->walkCycle += dt * spd * 1.5f;
            e->animState = ANIM_WALK;
        } else {
            e->animState = ANIM_IDLE;
            e->walkCycle *= 0.9f;
        }

        // Moon jumping — actual physics
        float groundH = WorldGetHeight(e->position.x, e->position.z) + 1.2f;
        e->jumpTimer -= dt;
        if (e->position.y <= groundH + 0.05f && e->jumpTimer <= 0 && moving) {
            e->vertVel = 2.5f + (float)(rand() % 100) * 0.015f; // random jump force
            e->jumpTimer = 1.0f + (float)(rand() % 200) * 0.01f; // random cooldown
        }
        e->vertVel -= 1.62f * dt; // moon gravity
        e->position.y += e->vertVel * dt;
        if (e->position.y < groundH) {
            e->position.y = groundH;
            e->vertVel = 0;
        }

        if (dist < e->attackRange) {
            if (e->shootAnim < 1) e->shootAnim += dt * 3;
            if (e->shootAnim > 1) e->shootAnim = 1;
            e->attackTimer += dt;
        } else {
            if (e->shootAnim > 0) e->shootAnim -= dt * 2;
            if (e->shootAnim < 0) e->shootAnim = 0;
        }
        if (e->hitFlash > 0) e->animState = ANIM_HIT;
    }
}

float EnemyCheckPlayerDamage(EnemyManager *em, Vector3 playerPos, float dt) {
    (void)dt;
    float total = 0;
    for (int i = 0; i < MAX_ENEMIES; i++) {
        Enemy *e = &em->enemies[i];
        if (!e->active || e->state != ENEMY_ALIVE) continue;
        float dist = Vector3Length(Vector3Subtract(playerPos, e->position));
        if (dist >= e->attackRange || e->burstCooldown > 0) continue;
        if (e->attackTimer >= e->attackRate) {
            e->attackTimer = 0; e->muzzleFlash = 1;
            e->burstShots++;
            if (e->type == ENEMY_SOVIET) PlaySound(em->sndSovietFire);
            else PlaySound(em->sndAmericanFire);
            float hitChance = 0.5f - (dist / e->attackRange) * 0.3f;
            if (e->behavior == AI_STRAFE) hitChance -= 0.1f;
            if ((float)rand() / RAND_MAX < hitChance) total += e->damage;
            int burst = (e->type == ENEMY_SOVIET) ? 5 : 2;
            if (e->burstShots >= burst) {
                e->burstShots = 0;
                e->burstCooldown = (e->type == ENEMY_SOVIET) ? 0.8f : 1.5f;
            }
        }
    }
    return total;
}

static void DrawAstronautModel(EnemyManager *em, Enemy *e) {
    Vector3 pos = e->position;
    Color tint = WHITE;
    if (e->hitFlash > 0) {
        unsigned char v = (unsigned char)(150 + e->hitFlash * 105);
        tint = (Color){255, v, v, 255};
    }

    Color visorColor, accentColor, suitBase, suitDark, bootColor, beltColor, buckleColor, helmetColor, gloveColor, backpackColor;

    if (e->type == ENEMY_SOVIET) {
        // Soviet: deep red uniform (#CD0000), gold helmet (#FFD700), black boots, brown belt + gold buckle
        suitBase     = (Color){205, 0, 0, 255};      // USSR red
        suitDark     = (Color){160, 0, 0, 255};      // darker red panels
        accentColor  = (Color){255, 215, 0, 255};    // gold accent (#FFD700)
        visorColor   = (Color){255, 200, 50, 230};   // gold visor
        helmetColor  = (Color){220, 185, 40, 255};   // gold/yellow helmet
        bootColor    = (Color){25, 25, 22, 255};     // matte black boots
        beltColor    = (Color){120, 75, 35, 255};    // medium brown leather
        buckleColor  = (Color){255, 215, 0, 255};    // polished gold buckle
        gloveColor   = (Color){30, 30, 28, 255};     // black gloves
        backpackColor= (Color){140, 10, 10, 255};    // dark red backpack
    } else {
        // American: navy blue uniform, silver/chrome helmet, tan boots, olive belt + silver buckle
        suitBase     = (Color){25, 40, 90, 255};     // navy blue
        suitDark     = (Color){18, 28, 65, 255};     // darker navy panels
        accentColor  = (Color){220, 220, 230, 255};  // silver/white accent
        visorColor   = (Color){100, 170, 255, 230};  // blue visor
        helmetColor  = (Color){195, 200, 210, 255};  // silver/chrome helmet
        bootColor    = (Color){140, 115, 75, 255};   // tan boots
        beltColor    = (Color){80, 85, 60, 255};     // olive drab belt
        buckleColor  = (Color){200, 205, 215, 255};  // silver buckle
        gloveColor   = (Color){160, 155, 140, 255};  // light tan gloves
        backpackColor= (Color){30, 45, 80, 255};     // dark navy backpack
    }

    rlPushMatrix();
    rlTranslatef(pos.x, pos.y, pos.z);
    rlRotatef(e->facingAngle * RAD2DEG, 0, 1, 0);
    if (e->deathAngle > 0) rlRotatef(e->deathAngle, 1, 0, 0);

    // === TORSO — layered for depth ===
    DrawCube((Vector3){0, 0, 0}, 0.9f, 1.5f, 0.55f, suitBase);               // main body
    DrawCube((Vector3){0, 0.2f, 0.02f}, 0.7f, 0.6f, 0.5f, suitDark);         // chest plate
    DrawCubeWires((Vector3){0, 0, 0}, 0.91f, 1.51f, 0.56f, suitDark); // seams
    // Collar ring
    DrawCube((Vector3){0, 0.72f, 0}, 0.55f, 0.1f, 0.45f, suitDark);
    // Waist belt — leather with buckle
    DrawCube((Vector3){0, -0.55f, 0}, 0.92f, 0.1f, 0.57f, beltColor);
    DrawCubeWires((Vector3){0, -0.55f, 0}, 0.93f, 0.11f, 0.58f, (Color){beltColor.r/2, beltColor.g/2, beltColor.b/2, 180});
    // Belt buckle — centered front
    DrawCube((Vector3){0, -0.55f, 0.29f}, 0.12f, 0.09f, 0.02f, buckleColor);
    DrawCubeWires((Vector3){0, -0.55f, 0.29f}, 0.13f, 0.1f, 0.03f, (Color){buckleColor.r/2, buckleColor.g/2, buckleColor.b/2, 200});
    // Shoulder pads
    DrawCube((Vector3){0.5f, 0.6f, 0}, 0.18f, 0.2f, 0.4f, suitDark);
    DrawCube((Vector3){-0.5f, 0.6f, 0}, 0.18f, 0.2f, 0.4f, suitDark);
    // Faction patches
    DrawCube((Vector3){0.5f, 0.62f, 0.15f}, 0.15f, 0.12f, 0.02f, accentColor);
    DrawCube((Vector3){-0.5f, 0.62f, 0.15f}, 0.15f, 0.12f, 0.02f, accentColor);
    // Chest badge
    DrawCube((Vector3){0.2f, 0.35f, 0.28f}, 0.12f, 0.12f, 0.02f, accentColor);

    // === HELMET — faction colored ===
    DrawSphere((Vector3){0, 1.1f, 0}, 0.48f, helmetColor);
    DrawCube((Vector3){0, 0.8f, 0}, 0.65f, 0.08f, 0.55f, suitDark); // neck ring
    DrawModel(em->mdlVisor, (Vector3){0, 1.12f, 0.32f}, 1.0f, visorColor);
    // Visor frame
    DrawCubeWires((Vector3){0, 1.12f, 0.34f}, 0.42f, 0.32f, 0.04f, (Color){160,158,152,200});
    // Antenna nub on top
    DrawCube((Vector3){0.12f, 1.55f, 0}, 0.03f, 0.12f, 0.03f, accentColor);

    // === BACKPACK — faction colored ===
    DrawCube((Vector3){0, 0.1f, -0.42f}, 0.62f, 0.9f, 0.28f, backpackColor);
    DrawCubeWires((Vector3){0, 0.1f, -0.42f}, 0.63f, 0.91f, 0.29f, suitDark);
    // Vent grilles
    for (int v = 0; v < 3; v++) {
        float vy = -0.1f + v * 0.3f;
        DrawCube((Vector3){0, vy, -0.57f}, 0.45f, 0.05f, 0.01f, suitDark);
    }
    // Hoses to helmet
    Color hoseColor = (e->type == ENEMY_SOVIET) ? (Color){100,30,30,255} : (Color){60,65,90,255};
    DrawCube((Vector3){0.18f, 0.65f, -0.25f}, 0.05f, 0.5f, 0.05f, hoseColor);
    DrawCube((Vector3){-0.18f, 0.65f, -0.25f}, 0.05f, 0.5f, 0.05f, hoseColor);

    // === GUN — two-handed hold, centered in front of body ===
    float gunAim = e->shootAnim * -25.0f; // tilts up when shooting
    rlPushMatrix();
    rlTranslatef(0.2f, 0.05f, 0.55f); // slightly right of center
    rlRotatef(gunAim, 1, 0, 0);

    if (e->type == ENEMY_SOVIET) {
        Color GM = {40,42,50,255};
        Color BK = {25,27,32,255};
        Color RD = {255,40,20,230};
        // Big chunky receiver
        DrawCube((Vector3){0,0,0}, 0.16f, 0.14f, 1.0f, GM);
        DrawCubeWires((Vector3){0,0,0}, 0.161f, 0.141f, 1.01f, BK);
        // Barrel — thick
        DrawCube((Vector3){0,0.02f,0.6f}, 0.09f, 0.09f, 0.35f, BK);
        // Muzzle brake — wide
        DrawCube((Vector3){0,0.02f,0.82f}, 0.13f, 0.13f, 0.07f, GM);
        DrawCubeWires((Vector3){0,0.02f,0.82f}, 0.131f, 0.131f, 0.071f, BK);
        // Drum magazine — big and round underneath
        DrawSphere((Vector3){0,-0.16f,0.0f}, 0.13f, GM);
        DrawSphere((Vector3){0,-0.16f,0.12f}, 0.11f, GM);
        // Red glow strips on sides
        DrawCube((Vector3){0.085f,0,0}, 0.008f, 0.04f, 0.7f, RD);
        DrawCube((Vector3){-0.085f,0,0}, 0.008f, 0.04f, 0.7f, RD);
        // Stock
        DrawCube((Vector3){0,0,-0.4f}, 0.06f, 0.12f, 0.3f, BK);
        DrawCube((Vector3){0,-0.02f,-0.55f}, 0.09f, 0.08f, 0.06f, GM);
    } else {
        Color GM = {30,35,50,255};
        Color BK = {20,23,35,255};
        Color BL = {50,130,255,230};
        // Sleek futuristic body
        DrawCube((Vector3){0,0,0}, 0.14f, 0.14f, 0.8f, GM);
        DrawCubeWires((Vector3){0,0,0}, 0.141f, 0.141f, 0.801f, BK);
        // Dish barrel — wide antenna
        DrawCube((Vector3){0,0,0.48f}, 0.18f, 0.18f, 0.12f, GM);
        DrawCube((Vector3){0,0,0.58f}, 0.26f, 0.26f, 0.05f, BK);
        DrawCubeWires((Vector3){0,0,0.58f}, 0.261f, 0.261f, 0.051f, (Color){40,80,180,150});
        // Blue energy core on top
        DrawCube((Vector3){0,0.12f,-0.05f}, 0.08f, 0.08f, 0.08f, BL);
        DrawSphere((Vector3){0,0.12f,-0.05f}, 0.05f, (Color){120,200,255,200});
        // Blue accent strips
        DrawCube((Vector3){0.075f,0,0}, 0.008f, 0.03f, 0.6f, BL);
        DrawCube((Vector3){-0.075f,0,0}, 0.008f, 0.03f, 0.6f, BL);
        // Stock
        DrawCube((Vector3){0,0,-0.35f}, 0.08f, 0.1f, 0.15f, BK);
    }
    // Muzzle flash
    if (e->muzzleFlash > 0) {
        float mz = (e->type == ENEMY_SOVIET) ? 0.88f : 0.64f;
        Color fc = (e->type == ENEMY_SOVIET) ?
            (Color){255,180,50,(unsigned char)(e->muzzleFlash*230)} :
            (Color){80,180,255,(unsigned char)(e->muzzleFlash*230)};
        DrawSphere((Vector3){0,0,mz}, e->muzzleFlash*0.18f, fc);
    }
    rlPopMatrix(); // gun

    // === ARMS — big swinging motion + gun hold ===
    float armSwing = sinf(e->walkCycle) * 25.0f; // degrees
    // Right arm — swings opposite to left leg
    rlPushMatrix();
    rlTranslatef(0.52f, 0.35f, 0.1f);
    rlRotatef(-25 - armSwing + gunAim * 0.4f, 1, 0, 0);
    rlRotatef(-12, 0, 0, 1);
    DrawModel(em->mdlArm, (Vector3){0, 0, 0}, 1.0f, tint);
    DrawCube((Vector3){0, -0.42f, 0}, 0.24f, 0.16f, 0.24f, gloveColor);
    rlPopMatrix();
    // Left arm — swings opposite to right leg
    rlPushMatrix();
    rlTranslatef(-0.52f, 0.35f, 0.1f);
    rlRotatef(-25 + armSwing + gunAim * 0.4f, 1, 0, 0);
    rlRotatef(12, 0, 0, 1);
    DrawModel(em->mdlArm, (Vector3){0, 0, 0}, 1.0f, tint);
    DrawCube((Vector3){0, -0.42f, 0}, 0.24f, 0.16f, 0.24f, gloveColor);
    rlPopMatrix();

    // === LEGS — big dramatic strides ===
    float legSwing = sinf(e->walkCycle) * 35.0f; // degrees — big stride
    // Right leg
    rlPushMatrix();
    rlTranslatef(0.22f, -0.85f, 0);
    rlRotatef(legSwing, 1, 0, 0);
    // Upper leg
    DrawCube((Vector3){0, -0.05f, 0}, 0.3f, 0.45f, 0.3f, suitBase);
    DrawCubeWires((Vector3){0, -0.05f, 0}, 0.31f, 0.46f, 0.31f, suitDark);
    // Knee joint
    DrawCube((Vector3){0, -0.3f, 0}, 0.22f, 0.08f, 0.22f, suitDark);
    // Lower leg (slight bend opposite to upper)
    rlPushMatrix();
    rlTranslatef(0, -0.3f, 0);
    rlRotatef(-legSwing * 0.4f, 1, 0, 0); // knee bends
    DrawCube((Vector3){0, -0.2f, 0}, 0.28f, 0.4f, 0.28f, suitBase);
    DrawModel(em->mdlBoot, (Vector3){0, -0.44f, 0.04f}, 1.0f, bootColor);
    rlPopMatrix();
    rlPopMatrix();
    // Left leg
    rlPushMatrix();
    rlTranslatef(-0.22f, -0.85f, 0);
    rlRotatef(-legSwing, 1, 0, 0);
    DrawCube((Vector3){0, -0.05f, 0}, 0.3f, 0.45f, 0.3f, suitBase);
    DrawCubeWires((Vector3){0, -0.05f, 0}, 0.31f, 0.46f, 0.31f, suitDark);
    DrawCube((Vector3){0, -0.3f, 0}, 0.22f, 0.08f, 0.22f, suitDark);
    rlPushMatrix();
    rlTranslatef(0, -0.3f, 0);
    rlRotatef(legSwing * 0.4f, 1, 0, 0);
    DrawCube((Vector3){0, -0.2f, 0}, 0.28f, 0.4f, 0.28f, suitBase);
    DrawModel(em->mdlBoot, (Vector3){0, -0.44f, 0.04f}, 1.0f, bootColor);
    rlPopMatrix();
    rlPopMatrix();

    rlPopMatrix(); // root

    // Health bar
    if (e->state == ENEMY_ALIVE && e->health < e->maxHealth) {
        float hp = e->health / e->maxHealth;
        Vector3 bp = {pos.x, pos.y + 1.9f, pos.z};
        DrawCube(bp, 0.9f, 0.07f, 0.02f, (Color){30, 30, 30, 220});
        DrawCube((Vector3){bp.x - (0.9f*(1-hp))*0.5f, bp.y, bp.z+0.01f},
            0.9f*hp, 0.06f, 0.02f,
            (Color){(unsigned char)(255*(1-hp)), (unsigned char)(255*hp), 0, 255});
    }
}

void EnemyManagerDraw(EnemyManager *em) {
    for (int i = 0; i < MAX_ENEMIES; i++)
        if (em->enemies[i].active) DrawAstronautModel(em, &em->enemies[i]);
}

int EnemyCheckHit(EnemyManager *em, Ray ray, float maxDist, float *hitDist) {
    int closest = -1; float cd = maxDist;
    for (int i = 0; i < MAX_ENEMIES; i++) {
        Enemy *e = &em->enemies[i];
        if (!e->active || e->state != ENEMY_ALIVE) continue;
        BoundingBox box = {{e->position.x-0.5f, e->position.y-1.2f, e->position.z-0.5f},
                           {e->position.x+0.5f, e->position.y+1.3f, e->position.z+0.5f}};
        RayCollision col = GetRayCollisionBox(ray, box);
        if (col.hit && col.distance < cd) { cd = col.distance; closest = i; }
    }
    if (hitDist) *hitDist = cd;
    return closest;
}

int EnemyCheckSphereHit(EnemyManager *em, Vector3 center, float radius) {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        Enemy *e = &em->enemies[i];
        if (!e->active || e->state != ENEMY_ALIVE) continue;
        if (Vector3Distance(center, e->position) < radius + 0.8f) return i;
    }
    return -1;
}

void EnemyDamage(EnemyManager *em, int index, float damage) {
    if (index < 0 || index >= MAX_ENEMIES) return;
    Enemy *e = &em->enemies[index];
    e->health -= damage; e->hitFlash = 1;
    if (e->dodgeTimer <= 0) { e->strafeDir *= -1; e->dodgeTimer = 1; }
    if (e->health <= 0) { e->state = ENEMY_DYING; e->deathTimer = 2; e->deathAngle = 0; }
}

int EnemyCountAlive(EnemyManager *em) {
    int c = 0;
    for (int i = 0; i < MAX_ENEMIES; i++)
        if (em->enemies[i].active && em->enemies[i].state == ENEMY_ALIVE) c++;
    return c;
}
