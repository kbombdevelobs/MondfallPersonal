#include "enemy.h"
#include "enemy_draw.h"
#include "world.h"
#include "structure/structure.h"
#include "sound_gen.h"
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
        int sr = AUDIO_SAMPLE_RATE;
        Wave w = SoundGenCreateWave(sr, 0.1f);
        short *d = (short *)w.data;
        for (int i = 0; i < (int)w.frameCount; i++) {
            float t = (float)i / sr;
            d[i] = (short)(((float)rand()/RAND_MAX*2-1) * expf(-t*35) * 18000 + sinf(t*150*2*3.14159f)*expf(-t*20)*10000);
        }
        em->sndSovietFire = LoadSoundFromWave(w);
        UnloadWave(w);
    }
    {
        int sr = AUDIO_SAMPLE_RATE;
        Wave w = SoundGenCreateWave(sr, 0.08f);
        short *d = (short *)w.data;
        for (int i = 0; i < (int)w.frameCount; i++) {
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
        WaveFormat(&deathWav, AUDIO_RADIO_SAMPLE_RATE, 16, 1);
        SoundGenDegradeRadio(&deathWav);
        em->sndSovietDeath[em->sovietDeathCount] = LoadSoundFromWave(deathWav);
        SetSoundVolume(em->sndSovietDeath[em->sovietDeathCount], AUDIO_DEATH_VOLUME);
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
        WaveFormat(&dw, AUDIO_RADIO_SAMPLE_RATE, 16, 1);
        SoundGenDegradeRadio(&dw);
        em->sndAmericanDeath[em->americanDeathCount] = LoadSoundFromWave(dw);
        SetSoundVolume(em->sndAmericanDeath[em->americanDeathCount], AUDIO_DEATH_VOLUME);
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
    e->strafeTimer = AI_STRAFE_TIMER_BASE + (float)rand() / RAND_MAX * AI_STRAFE_TIMER_RAND;
    e->dodgeCooldown = AI_DODGE_COOLDOWN;

    if (type == ENEMY_SOVIET) {
        e->health = SOVIET_HEALTH; e->maxHealth = SOVIET_HEALTH;
        e->speed = SOVIET_SPEED; e->damage = SOVIET_DAMAGE;
        e->attackRange = SOVIET_ATTACK_RANGE; e->attackRate = SOVIET_ATTACK_RATE;
        e->preferredDist = SOVIET_PREFERRED_DIST; e->behavior = AI_ADVANCE;
    } else {
        e->health = AMERICAN_HEALTH; e->maxHealth = AMERICAN_HEALTH;
        e->speed = AMERICAN_SPEED; e->damage = AMERICAN_DAMAGE;
        e->attackRange = AMERICAN_ATTACK_RANGE; e->attackRate = AMERICAN_ATTACK_RATE;
        e->preferredDist = AMERICAN_PREFERRED_DIST; e->behavior = AI_STRAFE;
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
                e->ragdollVelY -= MOON_GRAVITY * dt;
                e->spinX *= (1.0f - 0.3f * dt);
                e->spinY *= (1.0f - 0.3f * dt);
                e->ragdollVelX *= (1.0f - 0.5f * dt);
                e->ragdollVelZ *= (1.0f - 0.5f * dt);
                // Keep body ON terrain surface, not sinking through
                float gH = WorldGetHeight(e->position.x, e->position.z) + 0.6f;
                if (e->position.y < gH) {
                    e->position.y = gH;
                    if (fabsf(e->ragdollVelY) < 0.5f) {
                        // Settled — stop bouncing, lock to terrain
                        e->ragdollVelY = 0;
                        e->ragdollVelX *= 0.7f;
                        e->ragdollVelZ *= 0.7f;
                        e->spinX *= 0.85f;
                    } else {
                        e->ragdollVelY = fabsf(e->ragdollVelY) * 0.3f;
                        e->spinX *= 0.6f;
                    }
                }
            } else {
                // CRUMPLE — fall forward onto terrain surface
                e->deathAngle += e->spinX * dt;
                if (e->deathAngle > 90.0f) {
                    e->deathAngle = 90.0f;
                    e->spinX = 0;
                }
                // Track terrain height continuously — never sink through
                float gH = WorldGetHeight(e->position.x, e->position.z) + 0.5f;
                if (e->position.y > gH + 0.1f) e->position.y -= dt * 2.0f;
                else e->position.y = gH;
            }

            if (e->deathTimer <= 0) { e->state = ENEMY_DEAD; e->active = false; em->count--; }
            continue;
        }
        if (e->state == ENEMY_EVISCERATING) {
            e->deathTimer -= dt;
            e->evisTimer += dt;
            float t = e->evisTimer;
            for (int li = 0; li < 6; li++) {
                e->evisLimbPos[li] = Vector3Add(e->evisLimbPos[li], Vector3Scale(e->evisLimbVel[li], dt));
                e->evisLimbVel[li].y -= MOON_GRAVITY * dt;
                float gH = WorldGetHeight(
                    e->position.x + e->evisLimbPos[li].x,
                    e->position.z + e->evisLimbPos[li].z);
                float limbGround = gH - e->position.y + 0.1f;
                if (e->evisLimbPos[li].y < limbGround) {
                    e->evisLimbPos[li].y = limbGround;
                    e->evisLimbVel[li].y = fabsf(e->evisLimbVel[li].y) * 0.2f;
                    e->evisLimbVel[li].x *= 0.6f;
                    e->evisLimbVel[li].z *= 0.6f;
                }
                if (t < 3.0f) e->evisBloodTimer[li] += dt;
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
            // SOVIET: Charge as a spread — run at player with wide strafe
            // Check if a structure blocks the direct charge path
            bool sovStructBlock = false;
            StructureManager *sovStructs = StructureGetActive();
            if (sovStructs && dist > 6.0f) {
                float collR = MOONBASE_EXTERIOR_RADIUS + 1.0f;
                for (int si = 0; si < sovStructs->count; si++) {
                    Structure *st = &sovStructs->structures[si];
                    if (!st->active) continue;
                    Vector3 toS = {st->worldPos.x - e->position.x, 0, st->worldPos.z - e->position.z};
                    Vector3 toP = {playerPos.x - e->position.x, 0, playerPos.z - e->position.z};
                    float tsLen = sqrtf(toS.x * toS.x + toS.z * toS.z);
                    float tpLen = sqrtf(toP.x * toP.x + toP.z * toP.z);
                    if (tsLen < tpLen && tsLen < collR * 3.0f) {
                        float dot = (toS.x * toP.x + toS.z * toP.z) / (tsLen * tpLen + 0.001f);
                        if (dot > 0.5f) { sovStructBlock = true; break; }
                    }
                }
            }

            if (sovStructBlock) {
                // Rush wide around — heavy strafe to split around the base
                moveDir = Vector3Add(fwd, Vector3Scale(strafe, e->strafeDir * 2.0f));
                moving = true; e->behavior = AI_ADVANCE;
            } else if (dist > 4.0f) {
                moveDir = Vector3Add(fwd, Vector3Scale(strafe, e->strafeDir * 0.6f));
                moving = true; e->behavior = AI_ADVANCE;
            } else {
                moveDir = Vector3Add(Vector3Scale(strafe, e->strafeDir * 1.5f), Vector3Scale(fwd, 0.2f));
                moving = true; e->behavior = AI_STRAFE;
            }
            // Occasional dodge when close
            if (e->dodgeTimer <= 0 && dist < 12 && (rand() % 80) < 2) {
                moveDir = Vector3Scale(strafe, e->strafeDir * 3.5f);
                e->dodgeTimer = e->dodgeCooldown; e->behavior = AI_DODGE;
            }
        } else {
            // AMERICAN: Tactical — cover behind rocks, flank around structures
            World *w = WorldGetActive();
            StructureManager *amStructs = StructureGetActive();
            bool foundCover = false;

            // Check if a structure blocks the path to player — if so, flank
            bool structBlocking = false;
            int flankSign = e->strafeDir > 0 ? 1 : -1; // each enemy flanks a different way
            if (amStructs) {
                float collR = MOONBASE_EXTERIOR_RADIUS + 1.0f;
                for (int si = 0; si < amStructs->count; si++) {
                    Structure *st = &amStructs->structures[si];
                    if (!st->active) continue;
                    // Check if structure center is roughly between enemy and player
                    Vector3 toStruct = {st->worldPos.x - e->position.x, 0, st->worldPos.z - e->position.z};
                    Vector3 toP = {playerPos.x - e->position.x, 0, playerPos.z - e->position.z};
                    float tsPLen = Vector3Length(toStruct);
                    float tpLen = Vector3Length(toP);
                    if (tsPLen < tpLen && tsPLen < collR * 3.0f) {
                        float dot = (toStruct.x * toP.x + toStruct.z * toP.z) / (tsPLen * tpLen + 0.001f);
                        if (dot > 0.5f) { // structure roughly in the way
                            structBlocking = true;
                            break;
                        }
                    }
                }
            }

            if (structBlocking) {
                // Flank around the structure — rush wide to the side then re-engage
                moveDir = Vector3Add(fwd, Vector3Scale(strafe, (float)flankSign * 1.8f));
                moving = true; e->behavior = AI_ADVANCE;
            } else if (w && dist < e->attackRange * 1.2f && dist > 5.0f) {
                // Search nearby chunks for a rock to hide behind
                for (int ci = 0; ci < w->chunkCount && !foundCover; ci++) {
                    if (!w->chunks[ci].generated) continue;
                    for (int ri = 0; ri < w->chunks[ci].rockCount && !foundCover; ri++) {
                        Rock *rock = &w->chunks[ci].rocks[ri];
                        float rockDist = Vector3Distance(e->position, rock->position);
                        if (rockDist > 3.0f && rockDist < 15.0f) {
                            Vector3 toRock = Vector3Subtract(rock->position, e->position);
                            toRock.y = 0;
                            Vector3 toP = Vector3Subtract(playerPos, e->position);
                            toP.y = 0;
                            float dot = toRock.x * toP.x + toRock.z * toP.z;
                            if (dot > 0) {
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

            if (!foundCover && !structBlocking) {
                if (dist < e->preferredDist * 0.5f) {
                    moveDir = Vector3Add(Vector3Scale(fwd, -1), Vector3Scale(strafe, e->strafeDir * 0.7f));
                    moving = true; e->behavior = AI_RETREAT;
                } else if (dist > e->preferredDist * 1.4f) {
                    moveDir = Vector3Add(fwd, Vector3Scale(strafe, e->strafeDir * 0.4f));
                    moving = true; e->behavior = AI_ADVANCE;
                } else {
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
            float newX = e->position.x + moveDir.x * spd * dt;
            float newZ = e->position.z + moveDir.z * spd * dt;
            // Structure collision — slide around bases instead of stopping
            StructureManager *structs = StructureGetActive();
            if (structs && StructureCheckCollision(structs, (Vector3){newX, e->position.y, newZ}, 0.8f)) {
                // Find which structure we're hitting and compute tangent slide
                float collR = MOONBASE_EXTERIOR_RADIUS + 0.5f + 0.8f;
                for (int si = 0; si < structs->count; si++) {
                    Structure *st = &structs->structures[si];
                    if (!st->active) continue;
                    float sdx = newX - st->worldPos.x;
                    float sdz = newZ - st->worldPos.z;
                    if (sdx * sdx + sdz * sdz < collR * collR) {
                        // Compute tangent: perpendicular to radial direction
                        float radLen = sqrtf(sdx * sdx + sdz * sdz);
                        if (radLen > 0.1f) {
                            // Pick tangent direction that aligns with movement intent
                            float tx = -sdz / radLen;  // tangent option 1
                            float tz = sdx / radLen;
                            float dot = tx * moveDir.x + tz * moveDir.z;
                            if (dot < 0) { tx = -tx; tz = -tz; } // pick the direction we're trying to go
                            // Slide along tangent + push outward slightly
                            float pushX = sdx / radLen * 0.3f;
                            float pushZ = sdz / radLen * 0.3f;
                            newX = e->position.x + (tx * spd + pushX) * dt;
                            newZ = e->position.z + (tz * spd + pushZ) * dt;
                        }
                        break;
                    }
                }
            }
            e->position.x = newX;
            e->position.z = newZ;
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
            e->deathTimer = 10.0f;
            e->spinX = 120.0f + (float)(rand() % 300);
            e->spinY = (float)(rand() % 200) - 100.0f;
            float launchAngle = ((float)rand() / RAND_MAX) * 2.0f * PI;
            float launchForce = 3.0f + ((float)rand() / RAND_MAX) * 5.0f;
            e->ragdollVelX = cosf(launchAngle) * launchForce;
            e->ragdollVelZ = sinf(launchAngle) * launchForce;
            e->ragdollVelY = 2.0f + ((float)rand() / RAND_MAX) * 4.0f;
        } else {
            // Simple crumple — fall over, bleed
            e->deathTimer = 12.0f;
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

void EnemyEviscerate(EnemyManager *em, int index, Vector3 hitDir) {
    if (index < 0 || index >= MAX_ENEMIES) return;
    Enemy *e = &em->enemies[index];
    e->state = ENEMY_EVISCERATING;
    e->evisTimer = 0;
    e->health = 0;
    e->deathTimer = 10.0f;
    e->evisDir = Vector3Normalize(hitDir);

    // 0=head, 1=torso, 2=right arm, 3=left arm, 4=right leg, 5=left leg
    Vector3 origins[6] = {
        {0, 1.1f, 0}, {0, 0, 0}, {0.52f, 0.3f, 0},
        {-0.52f, 0.3f, 0}, {0.22f, -0.85f, 0}, {-0.22f, -0.85f, 0}
    };
    for (int i = 0; i < 6; i++) {
        e->evisLimbPos[i] = origins[i];
        e->evisBloodTimer[i] = 0;
        float rx = ((float)rand() / RAND_MAX - 0.5f) * 6.0f;
        float ry = 2.0f + ((float)rand() / RAND_MAX) * 5.0f;
        float rz = ((float)rand() / RAND_MAX - 0.5f) * 6.0f;
        e->evisLimbVel[i] = (Vector3){
            hitDir.x * 4.0f + rx + origins[i].x * 2.0f, ry,
            hitDir.z * 4.0f + rz + origins[i].z * 2.0f
        };
    }
    e->evisLimbVel[1].x += hitDir.x * 3.0f;
    e->evisLimbVel[1].z += hitDir.z * 3.0f;
    e->evisLimbVel[0].y += 4.0f;

    // Death radio sound
    bool anyPlaying = false;
    for (int si = 0; si < em->sovietDeathCount; si++)
        if (IsSoundPlaying(em->sndSovietDeath[si])) anyPlaying = true;
    for (int ai = 0; ai < em->americanDeathCount; ai++)
        if (IsSoundPlaying(em->sndAmericanDeath[ai])) anyPlaying = true;
    if (!anyPlaying && (rand() % 100 < 65)) {
        if (e->type == ENEMY_SOVIET && em->sovietDeathCount > 0 && em->sovietDeathPlays < 3) {
            PlaySound(em->sndSovietDeath[GetRandomValue(0, em->sovietDeathCount - 1)]);
            em->sovietDeathPlays++;
            em->radioTransmissionTimer = 3.0f;
        } else if (e->type == ENEMY_AMERICAN && em->americanDeathCount > 0 && em->americanDeathPlays < 3) {
            PlaySound(em->sndAmericanDeath[GetRandomValue(0, em->americanDeathCount - 1)]);
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

void EnemyManagerSetSFXVolume(EnemyManager *em, float vol) {
    SetSoundVolume(em->sndSovietFire, vol);
    SetSoundVolume(em->sndAmericanFire, vol);
    for (int i = 0; i < em->sovietDeathCount; i++)
        SetSoundVolume(em->sndSovietDeath[i], vol * AUDIO_DEATH_VOLUME);
    for (int i = 0; i < em->americanDeathCount; i++)
        SetSoundVolume(em->sndAmericanDeath[i], vol * AUDIO_DEATH_VOLUME);
}
