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

    // Load Soviet death sounds — degrade to scratchy radio quality
    em->sovietDeathCount = 0;
    const char *deathFiles[] = {
        "assets/soviet_death_sounds/Echoes of the Frozen Front.mp3",
        "assets/soviet_death_sounds/Last Echo of the Soviet Front The Final Net.mp3",
    };
    for (int df = 0; df < 2; df++) {
        if (!FileExists(deathFiles[df])) continue;
        Wave deathWav = LoadWave(deathFiles[df]);
        if (deathWav.frameCount <= 0) { UnloadWave(deathWav); continue; }
        // Downsample to 8000Hz mono for radio quality
        WaveFormat(&deathWav, 8000, 16, 1);
        // Degrade: overdrive, crackle, pops
        short *d = (short *)deathWav.data;
        int n = deathWav.frameCount;
        float avg = 0;
        for (int i = 0; i < n; i++) {
            float s = (float)d[i] / 32767.0f;
            avg = avg * 0.95f + s * 0.05f;
            s -= avg; // high-pass (kill bass)
            s *= 2.5f; // overdrive
            if (s > 0.7f) s = 0.7f;
            if (s < -0.7f) s = -0.7f;
            s += ((float)rand() / RAND_MAX * 2.0f - 1.0f) * 0.08f; // crackle
            if (rand() % 800 == 0) s += ((float)rand() / RAND_MAX - 0.5f) * 0.5f; // pop
            d[i] = (short)(s * 20000.0f);
        }
        em->sndSovietDeath[em->sovietDeathCount] = LoadSoundFromWave(deathWav);
        SetSoundVolume(em->sndSovietDeath[em->sovietDeathCount], 0.4f);
        UnloadWave(deathWav);
        em->sovietDeathCount++;
    }

    // Load American death sounds — same radio degradation
    em->americanDeathCount = 0;
    const char *amDeathFiles[] = {
        "assets/american_death_sounds/Darn Bastard's Final Stand.mp3",
        "assets/american_death_sounds/The Last Yeehaw of the Lone Star Son (1).mp3",
    };
    for (int df = 0; df < 2; df++) {
        if (!FileExists(amDeathFiles[df])) continue;
        Wave dw = LoadWave(amDeathFiles[df]);
        if (dw.frameCount <= 0) { UnloadWave(dw); continue; }
        WaveFormat(&dw, 8000, 16, 1);
        short *d = (short *)dw.data;
        int n = dw.frameCount;
        float avg2 = 0;
        for (int i = 0; i < n; i++) {
            float s = (float)d[i] / 32767.0f;
            avg2 = avg2 * 0.95f + s * 0.05f;
            s -= avg2;
            s *= 2.5f;
            if (s > 0.7f) s = 0.7f;
            if (s < -0.7f) s = -0.7f;
            s += ((float)rand() / RAND_MAX * 2.0f - 1.0f) * 0.08f;
            if (rand() % 800 == 0) s += ((float)rand() / RAND_MAX - 0.5f) * 0.5f;
            d[i] = (short)(s * 20000.0f);
        }
        em->sndAmericanDeath[em->americanDeathCount] = LoadSoundFromWave(dw);
        SetSoundVolume(em->sndAmericanDeath[em->americanDeathCount], 0.4f);
        UnloadWave(dw);
        em->americanDeathCount++;
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
    for (int i = 0; i < em->sovietDeathCount; i++) UnloadSound(em->sndSovietDeath[i]);
    for (int i = 0; i < em->americanDeathCount; i++) UnloadSound(em->sndAmericanDeath[i]);
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
    // Decay radio transmission timer
    if (em->radioTransmissionTimer > 0) em->radioTransmissionTimer -= dt;
    // Reset death play counter when no enemies alive (wave cleared)
    if (EnemyCountAlive(em) == 0) { em->sovietDeathPlays = 0; em->americanDeathPlays = 0; }

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

            if (e->deathStyle == 0) {
                // RAGDOLL — pressurized suit blowout spin
                e->deathAngle += e->spinX * dt;
                e->facingAngle += e->spinY * dt * 0.02f;
                e->position.x += e->ragdollVelX * dt;
                e->position.z += e->ragdollVelZ * dt;
                e->position.y += e->ragdollVelY * dt;
                e->ragdollVelY -= 1.62f * dt;
                e->spinX *= (1.0f - 0.3f * dt);
                e->spinY *= (1.0f - 0.3f * dt);
                e->ragdollVelX *= (1.0f - 0.2f * dt);
                e->ragdollVelZ *= (1.0f - 0.2f * dt);
                float gH = WorldGetHeight(e->position.x, e->position.z) + 0.3f;
                if (e->position.y < gH) {
                    e->position.y = gH;
                    e->ragdollVelY = fabsf(e->ragdollVelY) * 0.4f;
                    e->spinX *= 0.7f;
                }
            } else {
                // CRUMPLE — just fall forward, sink to ground
                e->deathAngle += e->spinX * dt;
                if (e->deathAngle > 90.0f) {
                    e->deathAngle = 90.0f;
                    e->spinX = 0;
                }
                // Sink toward ground
                float gH = WorldGetHeight(e->position.x, e->position.z) + 0.3f;
                if (e->position.y > gH) e->position.y -= dt * 1.0f;
                else e->position.y = gH;
            }

            if (e->deathTimer <= 0) { e->state = ENEMY_DEAD; e->active = false; em->count--; }
            continue;
        }
        if (e->state == ENEMY_VAPORIZING) {
            e->deathTimer -= dt;
            e->vaporizeTimer += dt;
            float t = e->vaporizeTimer;

            if (t < 0.8f) {
                // Phase 1: JERK — limbs spasm, slowing down
                float intensity = 1.0f - t / 0.8f; // starts violent, fades
                e->walkCycle += dt * 60.0f * intensity;
                e->shootAnim = sinf(t * 40.0f) * 0.5f * intensity + 0.5f;
                e->deathAngle = sinf(t * 30.0f) * 20.0f * intensity;
                e->vaporizeScale = 1.0f;
            } else if (t < 2.0f) {
                // Phase 2: FREEZE
                e->shootAnim = 0.5f;
                e->deathAngle = 3.0f;
                e->vaporizeScale = 1.0f;
                e->position.y += dt * 0.15f;
            } else if (e->deathStyle == 1 && t < 2.5f) {
                // Phase 2b: SWELL (15% only)
                float swellT = (t - 2.0f) / 0.5f;
                e->vaporizeScale = 1.0f + swellT * 1.0f;
                e->position.y += dt * 0.1f;
            } else if (e->deathStyle == 1 && t < 2.6f) {
                // Phase 2c: POP (15% only)
                e->vaporizeScale = 1.0f;
            } else {
                // Phase 3: DISINTEGRATE
                float startDis = (e->deathStyle == 1) ? 2.6f : 2.0f;
                float disDur = (e->deathStyle == 1) ? 3.2f : 2.3f;
                float disT = (t - startDis) / disDur;
                if (disT > 1.0f) disT = 1.0f;
                e->vaporizeScale = 1.0f - disT;
                e->position.y += dt * 0.3f;
            }

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

    // === VAPORIZE — 3 phases: jerk, freeze, disintegrate ===
    if (e->state == ENEMY_VAPORIZING) {
        float t = e->vaporizeTimer;
        float sc = e->vaporizeScale;

        rlPushMatrix();
        rlTranslatef(pos.x, pos.y, pos.z);
        rlRotatef(e->facingAngle * RAD2DEG, 0, 1, 0);

        if (t < 0.8f) {
            // PHASE 1: JERK — violent spasms fading to stillness
            float intensity = 1.0f - t / 0.8f;
            float jerk = sinf(t * 40.0f) * intensity;
            rlRotatef(jerk * 20.0f, 1, 0, 0);
            rlRotatef(sinf(t * 30.0f) * 12.0f * intensity, 0, 0, 1);
            unsigned char flash = (unsigned char)(200 + sinf(t * 50.0f) * 55 * intensity);
            Color jCol = {flash, flash, flash, 255};
            DrawCube((Vector3){0, 0, 0}, 0.9f, 1.5f, 0.55f, jCol);
            DrawSphere((Vector3){0, 1.1f, 0}, 0.48f, jCol);
            // Arms flailing, slowing
            float armJerk = sinf(t * 35.0f) * 70.0f * intensity;
            rlPushMatrix(); rlTranslatef(0.52f, 0.3f, 0); rlRotatef(armJerk, 1, 0, 0);
            DrawCube((Vector3){0, -0.2f, 0}, 0.22f, 0.8f, 0.22f, jCol); rlPopMatrix();
            rlPushMatrix(); rlTranslatef(-0.52f, 0.3f, 0); rlRotatef(-armJerk*0.7f, 1, 0, 0);
            DrawCube((Vector3){0, -0.2f, 0}, 0.22f, 0.8f, 0.22f, jCol); rlPopMatrix();
            float legJerk = sinf(t * 45.0f) * 55.0f * intensity;
            rlPushMatrix(); rlTranslatef(0.22f, -0.85f, 0); rlRotatef(legJerk, 1, 0, 0);
            DrawCube((Vector3){0, -0.2f, 0}, 0.3f, 0.75f, 0.3f, jCol); rlPopMatrix();
            rlPushMatrix(); rlTranslatef(-0.22f, -0.85f, 0); rlRotatef(-legJerk*0.6f, 1, 0, 0);
            DrawCube((Vector3){0, -0.2f, 0}, 0.3f, 0.75f, 0.3f, jCol); rlPopMatrix();
        } else if ((e->deathStyle == 0 && t < 2.0f) || (e->deathStyle == 1 && t < 2.0f)) {
            // PHASE 2: FREEZE
            float glow = (t - 0.8f) / 1.2f;
            unsigned char gr = 255;
            unsigned char gg = (unsigned char)(240 - glow * 100);
            unsigned char gb = (unsigned char)(180 - glow * 130);
            Color fCol = {gr, gg, gb, 255};
            rlRotatef(4.0f, 1, 0, 0);
            rlRotatef(2.0f, 0, 0, 1);
            DrawCube((Vector3){0, 0, 0}, 0.9f, 1.5f, 0.55f, fCol);
            DrawSphere((Vector3){0, 1.1f, 0}, 0.48f, fCol);
            DrawCube((Vector3){0.52f, 0.1f, 0}, 0.22f, 0.8f, 0.22f, fCol);
            DrawCube((Vector3){-0.52f, 0.12f, 0.08f}, 0.22f, 0.8f, 0.22f, fCol);
            DrawCube((Vector3){0.22f, -0.9f, 0.08f}, 0.3f, 0.75f, 0.3f, fCol);
            DrawCube((Vector3){-0.22f, -0.88f, -0.04f}, 0.3f, 0.75f, 0.3f, fCol);
            // Energy crackle — slow, eerie
            for (int c = 0; c < 6; c++) {
                float ca = sinf(GetTime() * 8.0f + c * 2.5f);
                Vector3 c1 = {ca * 0.5f, -0.6f + c * 0.35f, ca * 0.25f};
                Vector3 c2 = {-ca * 0.35f, -0.4f + c * 0.35f, -ca * 0.3f};
                unsigned char la = (unsigned char)(180 - glow * 60);
                DrawLine3D(Vector3Add(pos, c1), Vector3Add(pos, c2), (Color){255, 255, 200, la});
            }
            // Faint hum glow around body
            DrawSphereWires(pos, 1.2f + sinf(GetTime() * 3.0f) * 0.1f, 6, 6,
                (Color){255, gg, gb, (unsigned char)(40 + glow * 30)});
        } else if (e->deathStyle == 1 && t < 2.5f) {
            // PHASE 2b: SWELL (15% only)
            float swellT = (t - 2.0f) / 0.5f;
            float swSc = 1.0f + swellT * 1.0f; // swell to 2x
            rlScalef(swSc, swSc * 0.85f, swSc); // thinner vertically — stretched look
            rlRotatef(4.0f, 1, 0, 0);
            // Getting hotter — bright white/yellow
            Color swCol = {255, (unsigned char)(255 - swellT * 60), (unsigned char)(200 - swellT * 120), 255};
            DrawCube((Vector3){0, 0, 0}, 0.9f, 1.5f, 0.55f, swCol);
            DrawSphere((Vector3){0, 1.1f, 0}, 0.48f, swCol);
            DrawCube((Vector3){0.52f, 0.1f, 0}, 0.22f, 0.8f, 0.22f, swCol);
            DrawCube((Vector3){-0.52f, 0.1f, 0}, 0.22f, 0.8f, 0.22f, swCol);
            DrawCube((Vector3){0.22f, -0.9f, 0}, 0.3f, 0.75f, 0.3f, swCol);
            DrawCube((Vector3){-0.22f, -0.9f, 0}, 0.3f, 0.75f, 0.3f, swCol);
        } else if (e->deathStyle == 1 && t < 2.6f) {
            // PHASE 2c: POP (15% only)
            float popT = (t - 2.5f) / 0.1f;
            DrawSphereEx(pos, 2.0f * (1.0f - popT), 5, 5,
                (Color){255, 255, 220, (unsigned char)((1.0f - popT) * 250)});
            // Draw body briefly at normal scale, charred
            rlRotatef(4.0f, 1, 0, 0);
            Color popCol = {200, 100, 50, 255};
            DrawCube((Vector3){0, 0, 0}, 0.9f, 1.5f, 0.55f, popCol);
            DrawSphere((Vector3){0, 1.1f, 0}, 0.48f, popCol);
        } else {
            // PHASE 3: DISINTEGRATE
            float startDis = (e->deathStyle == 1) ? 2.6f : 2.0f;
            float disDur = (e->deathStyle == 1) ? 3.2f : 2.3f;
            float disT = (t - startDis) / disDur;
            if (disT > 1.0f) disT = 1.0f;
            unsigned char da = (unsigned char)(255 * (1.0f - disT));
            Color dCol = {255, (unsigned char)(160 * (1.0f - disT)),
                         (unsigned char)(60 * (1.0f - disT)), da};
            rlScalef(sc, sc, sc);
            rlRotatef(4.0f, 1, 0, 0);
            // Head — first to go
            if (disT < 0.2f)
                DrawSphere((Vector3){0, 1.1f, 0}, 0.48f * (1.0f - disT/0.2f), dCol);
            // Arms — dissolve early
            if (disT < 0.35f) {
                float as = 1.0f - disT / 0.35f;
                DrawCube((Vector3){0.52f, 0.1f, 0}, 0.22f*as, 0.8f*as, 0.22f*as, dCol);
                DrawCube((Vector3){-0.52f, 0.1f, 0}, 0.22f*as, 0.8f*as, 0.22f*as, dCol);
            }
            // Legs — mid
            if (disT < 0.55f) {
                float ls = 1.0f - disT / 0.55f;
                DrawCube((Vector3){0.22f, -0.9f, 0}, 0.3f*ls, 0.75f*ls, 0.3f*ls, dCol);
                DrawCube((Vector3){-0.22f, -0.9f, 0}, 0.3f*ls, 0.75f*ls, 0.3f*ls, dCol);
            }
            // Torso — last to go
            float ts = 1.0f - disT;
            if (ts > 0) DrawCube((Vector3){0, 0, 0}, 0.9f*ts, 1.5f*ts, 0.55f*ts, dCol);
        }
        rlPopMatrix();

        // Ash/ember particles throughout all phases
        float particleT = t / 2.4f;
        for (int p = 0; p < 10; p++) {
            float pa = (float)p * 0.628f + t * 3.0f;
            float pr = particleT * 3.5f + (float)p * 0.2f;
            float py = particleT * 1.5f + sinf(pa * 2.0f) * 0.5f;
            Vector3 pp = {pos.x + cosf(pa) * pr, pos.y + py, pos.z + sinf(pa) * pr};
            float ps = 0.06f + (1.0f - particleT) * 0.08f;
            DrawSphereEx(pp, ps, 3, 3, (Color){255, 200, 120, (unsigned char)((1.0f - particleT) * 160)});
        }
        // Blood mist
        for (int b = 0; b < 6; b++) {
            float ba = (float)b * 1.05f + t * 2.5f;
            float br = particleT * 2.0f + (float)b * 0.15f;
            float by = particleT * 0.8f + sinf(ba) * 0.3f - 0.3f;
            DrawSphereEx((Vector3){pos.x + cosf(ba)*br, pos.y + by, pos.z + sinf(ba)*br},
                0.05f, 3, 3, (Color){150, 12, 8, (unsigned char)((1.0f - particleT) * 140)});
        }
        // Initial flash
        if (t < 0.15f) {
            DrawSphereEx(pos, 2.0f * (1.0f - t / 0.15f), 4, 4,
                (Color){255, 255, 240, (unsigned char)((1.0f - t / 0.15f) * 220)});
        }
        return;
    }

    rlPushMatrix();
    rlTranslatef(pos.x, pos.y, pos.z);
    rlRotatef(e->facingAngle * RAD2DEG, 0, 1, 0);
    // Ragdoll multi-axis tumble
    if (e->state == ENEMY_DYING) {
        rlRotatef(e->deathAngle, 1, 0, 0);
        rlRotatef(e->deathAngle * 0.7f, 0, 0, 1);
    }

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

    // Death particles
    if (e->state == ENEMY_DYING) {
        if (e->deathStyle == 0) {
            // RAGDOLL: gas leak + blood spray from a limb
            int limbSeed = (int)((size_t)e % 4);
            Vector3 leakOff = {0, 0, 0};
            switch (limbSeed) {
                case 0: leakOff = (Vector3){0, 1.1f, 0}; break;
                case 1: leakOff = (Vector3){0.5f, 0.3f, 0.1f}; break;
                case 2: leakOff = (Vector3){-0.2f, -0.9f, 0}; break;
                case 3: leakOff = (Vector3){0, 0.1f, -0.4f}; break;
            }
            for (int p = 0; p < 10; p++) {
                float pt = GetTime() * 8.0f + (float)p * 1.2f + e->deathAngle * 0.1f;
                float spread = (float)p * 0.18f + 0.1f;
                float px = pos.x + leakOff.x + cosf(pt) * spread;
                float py = pos.y + leakOff.y + sinf(pt * 0.7f) * 0.3f + (float)p * 0.15f;
                float pz = pos.z + leakOff.z + sinf(pt) * spread;
                DrawSphereEx((Vector3){px, py, pz}, 0.05f + (float)p * 0.025f, 3, 3,
                    (Color){220, 220, 220, (unsigned char)(180 - p * 15)});
            }
            for (int b = 0; b < 8; b++) {
                float bt = GetTime() * 6.0f + (float)b * 2.0f + e->deathAngle * 0.15f;
                float bspread = (float)b * 0.12f + 0.05f;
                DrawSphereEx((Vector3){
                    pos.x + leakOff.x + cosf(bt+1)*bspread,
                    pos.y + leakOff.y - (float)b * 0.1f,
                    pos.z + leakOff.z + sinf(bt+1)*bspread},
                    0.04f, 3, 3, (Color){180, 20, 20, (unsigned char)(200 - b * 20)});
            }
        } else {
            // CRUMPLE: blood pool spreading on ground
            float gH = WorldGetHeight(pos.x, pos.z) + 0.05f;
            float poolTime = 4.0f - e->deathTimer; // time since death
            float poolR = 0.3f + poolTime * 0.4f; // grows over time
            if (poolR > 2.5f) poolR = 2.5f;
            // Dark blood pool — flat cylinder on ground
            DrawCylinder((Vector3){pos.x, gH, pos.z}, poolR, poolR * 1.1f, 0.02f, 8,
                (Color){120, 8, 5, 200});
            DrawCylinder((Vector3){pos.x, gH + 0.01f, pos.z}, poolR * 0.6f, poolR * 0.7f, 0.02f, 6,
                (Color){160, 15, 10, 220});
            // A few floating blood droplets
            for (int b = 0; b < 3; b++) {
                float bt = GetTime() * 3.0f + (float)b * 2.1f;
                float bx = pos.x + cosf(bt) * poolR * 0.3f;
                float bz = pos.z + sinf(bt) * poolR * 0.3f;
                DrawSphereEx((Vector3){bx, gH + 0.1f + sinf(bt*2)*0.05f, bz},
                    0.03f, 3, 3, (Color){160, 15, 10, 160});
            }
        }
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
    if (e->health <= 0) {
        e->state = ENEMY_DYING;
        e->deathAngle = 0;
        e->deathStyle = (rand() % 100 < 40) ? 1 : 0; // 40% crumple, 60% ragdoll
        if (e->deathStyle == 0) {
            // Pressurized suit blowout — wild spin + launch
            e->deathTimer = 4.0f;
            e->spinX = 120.0f + (float)(rand() % 300);
            e->spinY = (float)(rand() % 200) - 100.0f;
            float launchAngle = ((float)rand() / RAND_MAX) * 2.0f * PI;
            float launchForce = 3.0f + ((float)rand() / RAND_MAX) * 5.0f;
            e->ragdollVelX = cosf(launchAngle) * launchForce;
            e->ragdollVelZ = sinf(launchAngle) * launchForce;
            e->ragdollVelY = 2.0f + ((float)rand() / RAND_MAX) * 4.0f;
        } else {
            // Simple crumple — fall over, bleed
            e->deathTimer = 5.0f;
            e->spinX = 80.0f + (float)(rand() % 60); // slower fall
            e->spinY = 0;
            e->ragdollVelX = 0; e->ragdollVelZ = 0; e->ragdollVelY = 0;
        }
        // 50% chance — scratchy radio death sound (max 3 per wave per faction, no overlap)
        bool anyPlaying = false;
        for (int si = 0; si < em->sovietDeathCount; si++)
            if (IsSoundPlaying(em->sndSovietDeath[si])) anyPlaying = true;
        for (int ai = 0; ai < em->americanDeathCount; ai++)
            if (IsSoundPlaying(em->sndAmericanDeath[ai])) anyPlaying = true;

        if (!anyPlaying && (rand() % 100 < 50)) {
            if (e->type == ENEMY_SOVIET && em->sovietDeathCount > 0 && em->sovietDeathPlays < 3) {
                int pick = GetRandomValue(0, em->sovietDeathCount - 1);
                PlaySound(em->sndSovietDeath[pick]);
                em->sovietDeathPlays++;
                em->radioTransmissionTimer = 3.0f;
            } else if (e->type == ENEMY_AMERICAN && em->americanDeathCount > 0 && em->americanDeathPlays < 3) {
                int pick = GetRandomValue(0, em->americanDeathCount - 1);
                PlaySound(em->sndAmericanDeath[pick]);
                em->americanDeathPlays++;
                em->radioTransmissionTimer = 3.0f;
            }
        }
    }
}

void EnemyVaporize(EnemyManager *em, int index) {
    if (index < 0 || index >= MAX_ENEMIES) return;
    Enemy *e = &em->enemies[index];
    e->state = ENEMY_VAPORIZING;
    e->vaporizeTimer = 0;
    e->vaporizeScale = 1.0f;
    e->health = 0;
    e->deathStyle = (rand() % 100 < 15) ? 1 : 0;
    e->deathTimer = (e->deathStyle == 1) ? 6.0f : 4.5f;

    // Death radio sound — same logic as bullet deaths
    bool anyPlaying = false;
    for (int si = 0; si < em->sovietDeathCount; si++)
        if (IsSoundPlaying(em->sndSovietDeath[si])) anyPlaying = true;
    for (int ai = 0; ai < em->americanDeathCount; ai++)
        if (IsSoundPlaying(em->sndAmericanDeath[ai])) anyPlaying = true;

    if (!anyPlaying && (rand() % 100 < 50)) {
        if (e->type == ENEMY_SOVIET && em->sovietDeathCount > 0 && em->sovietDeathPlays < 3) {
            int pick = GetRandomValue(0, em->sovietDeathCount - 1);
            PlaySound(em->sndSovietDeath[pick]);
            em->sovietDeathPlays++;
            em->radioTransmissionTimer = 3.0f;
        } else if (e->type == ENEMY_AMERICAN && em->americanDeathCount > 0 && em->americanDeathPlays < 3) {
            int pick = GetRandomValue(0, em->americanDeathCount - 1);
            PlaySound(em->sndAmericanDeath[pick]);
            em->americanDeathPlays++;
            em->radioTransmissionTimer = 3.0f;
        }
    }
}

int EnemyCountAlive(EnemyManager *em) {
    int c = 0;
    for (int i = 0; i < MAX_ENEMIES; i++)
        if (em->enemies[i].active && em->enemies[i].state == ENEMY_ALIVE) c++;
    return c;
}
