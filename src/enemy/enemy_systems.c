#include "enemy_systems.h"
#include "enemy_components.h"
#include "ecs_world.h"
#include "world.h"
#include "structure/structure.h"
#include "config.h"
#include "sound_gen.h"
#include <math.h>
#include <stdlib.h>

// ============================================================================
// Stored queries for nested iteration
// ============================================================================
static ecs_query_t *g_aliveQuery = NULL;

// ============================================================================
// Helper: play death radio sound (shared by all death triggers)
// ============================================================================
static void PlayDeathRadioSound(ecs_world_t *world, EnemyType type, int chance) {
    const EcEnemyResources *res = ecs_singleton_get(world, EcEnemyResources);
    if (!res) return;

    // Check if any death sound is currently playing
    bool anyPlaying = false;
    for (int si = 0; si < res->sovietDeathCount; si++)
        if (IsSoundPlaying(res->sndSovietDeath[si])) anyPlaying = true;
    for (int ai = 0; ai < res->americanDeathCount; ai++)
        if (IsSoundPlaying(res->sndAmericanDeath[ai])) anyPlaying = true;

    if (anyPlaying || (rand() % 100 >= chance)) return;

    // Need mutable access for play counts and timer
    EcEnemyResources *resMut = ecs_singleton_ensure(world, EcEnemyResources);
    if (!resMut) return;

    if (type == ENEMY_SOVIET && resMut->sovietDeathCount > 0 && resMut->sovietDeathPlays < 3) {
        int pick = GetRandomValue(0, resMut->sovietDeathCount - 1);
        PlaySound(resMut->sndSovietDeath[pick]);
        resMut->sovietDeathPlays++;
        resMut->radioTransmissionTimer = 3.0f;
    } else if (type == ENEMY_AMERICAN && resMut->americanDeathCount > 0 && resMut->americanDeathPlays < 3) {
        int pick = GetRandomValue(0, resMut->americanDeathCount - 1);
        PlaySound(resMut->sndAmericanDeath[pick]);
        resMut->americanDeathPlays++;
        resMut->radioTransmissionTimer = 3.0f;
    }
}

// ============================================================================
// SysAITargeting — face toward player
// ============================================================================
static void SysAITargeting(ecs_iter_t *it) {
    EcTransform *tr = ecs_field(it, EcTransform, 0);
    (void)ecs_field(it, EcAIState, 1); // required by query, not used directly

    const EcGameContext *ctx = ecs_singleton_get(it->world, EcGameContext);
    if (!ctx) return;
    Vector3 playerPos = ctx->playerPos;
    float dt = it->delta_time;

    for (int i = 0; i < it->count; i++) {
        Vector3 toPlayer = Vector3Subtract(playerPos, tr[i].position);
        toPlayer.y = 0;
        float dist = Vector3Length(toPlayer);
        if (dist > 0.1f) {
            float tgt = atan2f(toPlayer.x, toPlayer.z);
            float diff = tgt - tr[i].facingAngle;
            while (diff > PI) diff -= 2 * PI;
            while (diff < -PI) diff += 2 * PI;
            tr[i].facingAngle += diff * dt * AI_TURN_SPEED;
        }
    }
}

// ============================================================================
// SysAIBehavior — Soviet rush / American tactical
// ============================================================================
static void SysAIBehavior(ecs_iter_t *it) {
    EcTransform *tr = ecs_field(it, EcTransform, 0);
    EcAIState *ai = ecs_field(it, EcAIState, 1);
    EcCombatStats *cs = ecs_field(it, EcCombatStats, 2);
    EcFaction *fac = ecs_field(it, EcFaction, 3);
    EcVelocity *vel = ecs_field(it, EcVelocity, 4);

    const EcGameContext *ctx = ecs_singleton_get(it->world, EcGameContext);
    if (!ctx) return;
    Vector3 playerPos = ctx->playerPos;
    float dt = it->delta_time;

    for (int i = 0; i < it->count; i++) {
        ecs_entity_t entity = it->entities[i];

        // Decay timers
        if (ai[i].dodgeTimer > 0) ai[i].dodgeTimer -= dt;
        if (ai[i].burstCooldown > 0) ai[i].burstCooldown -= dt;

        Vector3 toPlayer = Vector3Subtract(playerPos, tr[i].position);
        toPlayer.y = 0;
        float dist = Vector3Length(toPlayer);

        Vector3 fwd = {sinf(tr[i].facingAngle), 0, cosf(tr[i].facingAngle)};
        Vector3 strafe = {cosf(tr[i].facingAngle), 0, -sinf(tr[i].facingAngle)};

        ai[i].behaviorTimer += dt;
        ai[i].strafeTimer -= dt;
        if (ai[i].strafeTimer <= 0) {
            ai[i].strafeDir *= -1;
            ai[i].strafeTimer = 1.0f + (float)rand() / RAND_MAX * 2.5f;
        }

        Vector3 moveDir = {0, 0, 0};
        bool moving = false;

        // Staggered AI in test mode
        bool doFullAI = true;
        if (ctx->testMode) {
            int entityIndex = (int)(entity & 0xFFFF);
            doFullAI = (entityIndex % AI_STAGGER_DIVISOR) == (ctx->aiFrameCounter % AI_STAGGER_DIVISOR);
            if (dist > LOD1_DISTANCE) doFullAI = false;
        }

        if (!doFullAI) {
            moveDir = fwd;
            moving = true;
            ai[i].behavior = AI_ADVANCE;
        } else if (fac[i].type == ENEMY_SOVIET) {
            // SOVIET: Charge as a spread — run at player with wide strafe
            // Check if a structure blocks the direct charge path
            bool sovStructBlock = false;
            StructureManager *sovStructs = StructureGetActive();
            if (sovStructs && dist > 6.0f) {
                float collR = MOONBASE_EXTERIOR_RADIUS + 1.0f;
                for (int si = 0; si < sovStructs->count; si++) {
                    Structure *st = &sovStructs->structures[si];
                    if (!st->active) continue;
                    Vector3 toS = {st->worldPos.x - tr[i].position.x, 0, st->worldPos.z - tr[i].position.z};
                    Vector3 toP = {playerPos.x - tr[i].position.x, 0, playerPos.z - tr[i].position.z};
                    float tsLen = sqrtf(toS.x * toS.x + toS.z * toS.z);
                    float tpLen = sqrtf(toP.x * toP.x + toP.z * toP.z);
                    if (tsLen < tpLen && tsLen < collR * 3.0f) {
                        float dotVal = (toS.x * toP.x + toS.z * toP.z) / (tsLen * tpLen + 0.001f);
                        if (dotVal > 0.5f) { sovStructBlock = true; break; }
                    }
                }
            }

            if (sovStructBlock) {
                // Rush wide around — heavy strafe to split around the base
                moveDir = Vector3Add(fwd, Vector3Scale(strafe, ai[i].strafeDir * 2.0f));
                moving = true; ai[i].behavior = AI_ADVANCE;
            } else if (dist > 4.0f) {
                moveDir = Vector3Add(fwd, Vector3Scale(strafe, ai[i].strafeDir * 0.6f));
                moving = true; ai[i].behavior = AI_ADVANCE;
            } else {
                moveDir = Vector3Add(Vector3Scale(strafe, ai[i].strafeDir * 1.5f), Vector3Scale(fwd, 0.2f));
                moving = true; ai[i].behavior = AI_STRAFE;
            }
            // Occasional dodge when close
            if (ai[i].dodgeTimer <= 0 && dist < 12 && (rand() % 80) < 2) {
                moveDir = Vector3Scale(strafe, ai[i].strafeDir * 3.5f);
                ai[i].dodgeTimer = ai[i].dodgeCooldown; ai[i].behavior = AI_DODGE;
            }
        } else {
            // AMERICAN: Tactical — cover behind rocks, flank around structures
            World *w = WorldGetActive();
            StructureManager *amStructs = StructureGetActive();
            bool foundCover = false;

            // Check if a structure blocks the path to player — if so, flank
            bool structBlocking = false;
            int flankSign = ai[i].strafeDir > 0 ? 1 : -1;
            if (amStructs) {
                float collR = MOONBASE_EXTERIOR_RADIUS + 1.0f;
                for (int si = 0; si < amStructs->count; si++) {
                    Structure *st = &amStructs->structures[si];
                    if (!st->active) continue;
                    Vector3 toStruct = {st->worldPos.x - tr[i].position.x, 0, st->worldPos.z - tr[i].position.z};
                    Vector3 toP = {playerPos.x - tr[i].position.x, 0, playerPos.z - tr[i].position.z};
                    float tsPLen = Vector3Length(toStruct);
                    float tpLen = Vector3Length(toP);
                    if (tsPLen < tpLen && tsPLen < collR * 3.0f) {
                        float dotVal = (toStruct.x * toP.x + toStruct.z * toP.z) / (tsPLen * tpLen + 0.001f);
                        if (dotVal > 0.5f) {
                            structBlocking = true;
                            break;
                        }
                    }
                }
            }

            if (structBlocking) {
                // Flank around the structure — rush wide to the side then re-engage
                moveDir = Vector3Add(fwd, Vector3Scale(strafe, (float)flankSign * 1.8f));
                moving = true; ai[i].behavior = AI_ADVANCE;
            } else if (w && dist < cs[i].attackRange * 1.2f && dist > 5.0f) {
                // Search nearby chunks for a rock to hide behind
                for (int ci = 0; ci < w->chunkCount && !foundCover; ci++) {
                    if (!w->chunks[ci].generated) continue;
                    for (int ri = 0; ri < w->chunks[ci].rockCount && !foundCover; ri++) {
                        Rock *rock = &w->chunks[ci].rocks[ri];
                        float rockDist = Vector3Distance(tr[i].position, rock->position);
                        if (rockDist > 3.0f && rockDist < 15.0f) {
                            Vector3 toRock = Vector3Subtract(rock->position, tr[i].position);
                            toRock.y = 0;
                            Vector3 toP = Vector3Subtract(playerPos, tr[i].position);
                            toP.y = 0;
                            float dot = toRock.x * toP.x + toRock.z * toP.z;
                            if (dot > 0) {
                                Vector3 coverDir = Vector3Normalize(Vector3Subtract(rock->position, playerPos));
                                Vector3 coverPos = Vector3Add(rock->position, Vector3Scale(coverDir, 2.5f));
                                Vector3 toCover = Vector3Subtract(coverPos, tr[i].position);
                                toCover.y = 0;
                                if (Vector3Length(toCover) > 1.5f) {
                                    moveDir = Vector3Normalize(toCover);
                                    moving = true; ai[i].behavior = AI_RETREAT;
                                    foundCover = true;
                                }
                            }
                        }
                    }
                }
            }

            if (!foundCover && !structBlocking) {
                if (dist < cs[i].preferredDist * 0.5f) {
                    moveDir = Vector3Add(Vector3Scale(fwd, -1), Vector3Scale(strafe, ai[i].strafeDir * 0.7f));
                    moving = true; ai[i].behavior = AI_RETREAT;
                } else if (dist > cs[i].preferredDist * 1.4f) {
                    moveDir = Vector3Add(fwd, Vector3Scale(strafe, ai[i].strafeDir * 0.4f));
                    moving = true; ai[i].behavior = AI_ADVANCE;
                } else {
                    moveDir = Vector3Scale(strafe, ai[i].strafeDir);
                    moving = true; ai[i].behavior = AI_STRAFE;
                }
            }

            // Dodge when taking damage
            if (cs[i].health < cs[i].maxHealth * 0.4f && ai[i].dodgeTimer <= 0 && (rand() % 60) < 3) {
                moveDir = Vector3Scale(strafe, ai[i].strafeDir * 3.0f);
                ai[i].dodgeTimer = ai[i].dodgeCooldown * 0.4f; ai[i].behavior = AI_DODGE;
            }
        }

        // Store movement direction and speed in velocity for physics to apply
        if (moving && Vector3Length(moveDir) > 0.01f) {
            moveDir = Vector3Normalize(moveDir);
            float spd = cs[i].speed * (ai[i].behavior == AI_DODGE ? 2.0f : 1.0f);
            vel[i].velocity = (Vector3){ moveDir.x * spd, 0, moveDir.z * spd };
        } else {
            vel[i].velocity = (Vector3){0, 0, 0};
        }
    }
}

// ============================================================================
// SysCollisionAvoidance — push apart overlapping enemies
// ============================================================================
static void SysCollisionAvoidance(ecs_iter_t *it) {
    EcTransform *tr = ecs_field(it, EcTransform, 0);
    EcVelocity *vel = ecs_field(it, EcVelocity, 1);

    const EcGameContext *ctx = ecs_singleton_get(it->world, EcGameContext);
    int collisionCap = (ctx && ctx->testMode) ? COLLISION_CAP : 0;

    for (int i = 0; i < it->count; i++) {
        // Check against other alive entities using the stored query
        ecs_iter_t qit = ecs_query_iter(it->world, g_aliveQuery);
        int checked = 0;
        bool pushed = false;
        while (ecs_query_next(&qit)) {
            EcTransform *otr = ecs_field(&qit, EcTransform, 0);
            for (int j = 0; j < qit.count && !pushed; j++) {
                if (it->entities[i] == qit.entities[j]) continue;
                Vector3 away = Vector3Subtract(tr[i].position, otr[j].position);
                away.y = 0;
                float len = Vector3Length(away);
                if (len < 2.5f && len > 0.1f) {
                    Vector3 push = Vector3Scale(Vector3Normalize(away), 0.5f);
                    vel[i].velocity = Vector3Add(vel[i].velocity, push);
                    pushed = true;
                }
                if (collisionCap > 0 && ++checked >= collisionCap) { pushed = true; break; }
            }
            if (pushed) break;
        }
    }
}

// ============================================================================
// SysPhysics — apply velocity, gravity, jumping, animation
// ============================================================================
static void SysPhysics(ecs_iter_t *it) {
    EcTransform *tr = ecs_field(it, EcTransform, 0);
    EcVelocity *vel = ecs_field(it, EcVelocity, 1);
    EcAnimation *anim = ecs_field(it, EcAnimation, 2);

    float dt = it->delta_time;

    for (int i = 0; i < it->count; i++) {
        bool moving = Vector3Length(vel[i].velocity) > 0.01f;

        if (moving) {
            float spd = Vector3Length(vel[i].velocity);
            float newX = tr[i].position.x + vel[i].velocity.x * dt;
            float newZ = tr[i].position.z + vel[i].velocity.z * dt;

            // Structure collision — slide around bases instead of walking through
            StructureManager *structs = StructureGetActive();
            if (structs && StructureCheckCollision(structs, (Vector3){newX, tr[i].position.y, newZ}, 0.8f)) {
                float collR = MOONBASE_EXTERIOR_RADIUS + 0.5f + 0.8f;
                for (int si = 0; si < structs->count; si++) {
                    Structure *st = &structs->structures[si];
                    if (!st->active) continue;
                    float sdx = newX - st->worldPos.x;
                    float sdz = newZ - st->worldPos.z;
                    if (sdx * sdx + sdz * sdz < collR * collR) {
                        float radLen = sqrtf(sdx * sdx + sdz * sdz);
                        if (radLen > 0.1f) {
                            // Tangent slide direction
                            float tx = -sdz / radLen;
                            float tz = sdx / radLen;
                            float dotVal = tx * vel[i].velocity.x + tz * vel[i].velocity.z;
                            if (dotVal < 0) { tx = -tx; tz = -tz; }
                            float pushX = sdx / radLen * 0.3f;
                            float pushZ = sdz / radLen * 0.3f;
                            newX = tr[i].position.x + (tx * spd + pushX) * dt;
                            newZ = tr[i].position.z + (tz * spd + pushZ) * dt;
                        }
                        break;
                    }
                }
            }

            tr[i].position.x = newX;
            tr[i].position.z = newZ;
            anim[i].walkCycle += dt * spd * 1.5f;
            anim[i].animState = ANIM_WALK;
        } else {
            anim[i].animState = ANIM_IDLE;
            anim[i].walkCycle *= 0.9f;
        }

        // Moon jumping
        float groundH = WorldGetHeight(tr[i].position.x, tr[i].position.z) + ENEMY_GROUND_OFFSET;
        vel[i].jumpTimer -= dt;
        if (tr[i].position.y <= groundH + 0.05f && vel[i].jumpTimer <= 0 && moving) {
            vel[i].vertVel = 2.5f + (float)(rand() % 100) * 0.015f;
            vel[i].jumpTimer = 1.0f + (float)(rand() % 200) * 0.01f;
        }
        vel[i].vertVel -= MOON_GRAVITY * dt;
        tr[i].position.y += vel[i].vertVel * dt;
        if (tr[i].position.y < groundH) {
            tr[i].position.y = groundH;
            vel[i].vertVel = 0;
        }

        // Hit flash decay
        if (anim[i].hitFlash > 0) {
            anim[i].hitFlash -= dt * ENEMY_HIT_FLASH_DECAY;
            anim[i].animState = ANIM_HIT;
        }
        if (anim[i].muzzleFlash > 0) anim[i].muzzleFlash -= dt * ENEMY_MUZZLE_DECAY;
    }
}

// ============================================================================
// SysAttack — fire at player, accumulate damage
// ============================================================================
static void SysAttack(ecs_iter_t *it) {
    EcTransform *tr = ecs_field(it, EcTransform, 0);
    EcCombatStats *cs = ecs_field(it, EcCombatStats, 1);
    EcAIState *ai = ecs_field(it, EcAIState, 2);
    EcFaction *fac = ecs_field(it, EcFaction, 3);
    EcAnimation *anim = ecs_field(it, EcAnimation, 4);

    const EcGameContext *ctx = ecs_singleton_get(it->world, EcGameContext);
    if (!ctx) return;
    Vector3 playerPos = ctx->playerPos;

    const EcEnemyResources *res = ecs_singleton_get(it->world, EcEnemyResources);
    EcGameContext *ctxMut = ecs_singleton_ensure(it->world, EcGameContext);

    float dt = it->delta_time;

    for (int i = 0; i < it->count; i++) {
        float dist = Vector3Distance(tr[i].position, playerPos);

        if (dist < cs[i].attackRange) {
            if (anim[i].shootAnim < 1) anim[i].shootAnim += dt * 3;
            if (anim[i].shootAnim > 1) anim[i].shootAnim = 1;
            ai[i].attackTimer += dt;
        } else {
            if (anim[i].shootAnim > 0) anim[i].shootAnim -= dt * 2;
            if (anim[i].shootAnim < 0) anim[i].shootAnim = 0;
        }

        if (dist >= cs[i].attackRange || ai[i].burstCooldown > 0) continue;

        if (ai[i].attackTimer >= cs[i].attackRate) {
            ai[i].attackTimer = 0;
            anim[i].muzzleFlash = 1;
            ai[i].burstShots++;

            if (res) {
                if (fac[i].type == ENEMY_SOVIET) PlaySound(res->sndSovietFire);
                else PlaySound(res->sndAmericanFire);
            }

            float hitChance = 0.5f - (dist / cs[i].attackRange) * 0.3f;
            if (ai[i].behavior == AI_STRAFE) hitChance -= 0.1f;
            if ((float)rand() / RAND_MAX < hitChance) {
                if (ctxMut) ctxMut->playerDamageAccum += cs[i].damage;
            }

            int burst = (fac[i].type == ENEMY_SOVIET) ? 5 : 2;
            if (ai[i].burstShots >= burst) {
                ai[i].burstShots = 0;
                ai[i].burstCooldown = (fac[i].type == ENEMY_SOVIET) ? 0.8f : 1.5f;
            }
        }
    }
}

// ============================================================================
// SysRagdollDeath — ragdoll blowout + crumple
// ============================================================================
static void SysRagdollDeath(ecs_iter_t *it) {
    EcTransform *tr = ecs_field(it, EcTransform, 0);
    EcAnimation *anim = ecs_field(it, EcAnimation, 1);
    EcRagdollDeath *rd = ecs_field(it, EcRagdollDeath, 2);

    float dt = it->delta_time;

    for (int i = it->count - 1; i >= 0; i--) {
        anim[i].animState = ANIM_DEATH;
        rd[i].deathTimer -= dt;

        if (rd[i].deathStyle == 0) {
            // RAGDOLL — pressurized suit blowout spin
            anim[i].deathAngle += rd[i].spinX * dt;
            tr[i].facingAngle += rd[i].spinY * dt * 0.02f;
            tr[i].position.x += rd[i].ragdollVelX * dt;
            tr[i].position.z += rd[i].ragdollVelZ * dt;
            tr[i].position.y += rd[i].ragdollVelY * dt;
            rd[i].ragdollVelY -= MOON_GRAVITY * dt;
            rd[i].spinX *= (1.0f - 0.3f * dt);
            rd[i].spinY *= (1.0f - 0.3f * dt);
            rd[i].ragdollVelX *= (1.0f - 0.5f * dt);
            rd[i].ragdollVelZ *= (1.0f - 0.5f * dt);

            float gH = WorldGetHeight(tr[i].position.x, tr[i].position.z) + 0.6f;
            if (tr[i].position.y < gH) {
                tr[i].position.y = gH;
                if (fabsf(rd[i].ragdollVelY) < 0.5f) {
                    rd[i].ragdollVelY = 0;
                    rd[i].ragdollVelX *= 0.7f;
                    rd[i].ragdollVelZ *= 0.7f;
                    rd[i].spinX *= 0.85f;
                } else {
                    rd[i].ragdollVelY = fabsf(rd[i].ragdollVelY) * 0.3f;
                    rd[i].spinX *= 0.6f;
                }
            }
        } else {
            // CRUMPLE — fall forward
            anim[i].deathAngle += rd[i].spinX * dt;
            if (anim[i].deathAngle > 90.0f) {
                anim[i].deathAngle = 90.0f;
                rd[i].spinX = 0;
            }
            float gH = WorldGetHeight(tr[i].position.x, tr[i].position.z) + 0.5f;
            if (tr[i].position.y > gH + 0.1f) tr[i].position.y -= dt * 2.0f;
            else tr[i].position.y = gH;
        }

        if (rd[i].deathTimer <= 0) {
            ecs_delete(it->world, it->entities[i]);
        }
    }
}

// ============================================================================
// SysVaporizeDeath — jerk, freeze, swell, disintegrate
// ============================================================================
static void SysVaporizeDeath(ecs_iter_t *it) {
    EcTransform *tr = ecs_field(it, EcTransform, 0);
    EcAnimation *anim = ecs_field(it, EcAnimation, 1);
    EcVaporizeDeath *vd = ecs_field(it, EcVaporizeDeath, 2);

    float dt = it->delta_time;

    for (int i = it->count - 1; i >= 0; i--) {
        vd[i].deathTimer -= dt;
        vd[i].vaporizeTimer += dt;
        float t = vd[i].vaporizeTimer;

        if (t < 0.8f) {
            // Phase 1: JERK
            float intensity = 1.0f - t / 0.8f;
            anim[i].walkCycle += dt * 60.0f * intensity;
            anim[i].shootAnim = sinf(t * 40.0f) * 0.5f * intensity + 0.5f;
            anim[i].deathAngle = sinf(t * 30.0f) * 20.0f * intensity;
            vd[i].vaporizeScale = 1.0f;
        } else if (t < 2.0f) {
            // Phase 2: FREEZE
            anim[i].shootAnim = 0.5f;
            anim[i].deathAngle = 3.0f;
            vd[i].vaporizeScale = 1.0f;
            tr[i].position.y += dt * 0.15f;
        } else if (vd[i].deathStyle == 1 && t < 2.5f) {
            // Phase 2b: SWELL (15% only)
            float swellT = (t - 2.0f) / 0.5f;
            vd[i].vaporizeScale = 1.0f + swellT * 1.0f;
            tr[i].position.y += dt * 0.1f;
        } else if (vd[i].deathStyle == 1 && t < 2.6f) {
            // Phase 2c: POP (15% only)
            vd[i].vaporizeScale = 1.0f;
        } else {
            // Phase 3: DISINTEGRATE
            float startDis = (vd[i].deathStyle == 1) ? 2.6f : 2.0f;
            float disDur = (vd[i].deathStyle == 1) ? 3.2f : 2.3f;
            float disT = (t - startDis) / disDur;
            if (disT > 1.0f) disT = 1.0f;
            vd[i].vaporizeScale = 1.0f - disT;
            tr[i].position.y += dt * 0.3f;
        }

        if (vd[i].deathTimer <= 0) {
            ecs_delete(it->world, it->entities[i]);
        }
    }
}

// ============================================================================
// SysEviscerateDeath — limb physics
// ============================================================================
static void SysEviscerateDeath(ecs_iter_t *it) {
    EcTransform *tr = ecs_field(it, EcTransform, 0);
    EcEviscerateDeath *ed = ecs_field(it, EcEviscerateDeath, 1);

    float dt = it->delta_time;

    for (int i = it->count - 1; i >= 0; i--) {
        ed[i].deathTimer -= dt;
        ed[i].evisTimer += dt;
        float t = ed[i].evisTimer;

        for (int li = 0; li < 6; li++) {
            ed[i].evisLimbPos[li] = Vector3Add(ed[i].evisLimbPos[li],
                Vector3Scale(ed[i].evisLimbVel[li], dt));
            ed[i].evisLimbVel[li].y -= MOON_GRAVITY * dt;

            float gH = WorldGetHeight(
                tr[i].position.x + ed[i].evisLimbPos[li].x,
                tr[i].position.z + ed[i].evisLimbPos[li].z);
            float limbGround = gH - tr[i].position.y + 0.1f;
            if (ed[i].evisLimbPos[li].y < limbGround) {
                ed[i].evisLimbPos[li].y = limbGround;
                ed[i].evisLimbVel[li].y = fabsf(ed[i].evisLimbVel[li].y) * 0.2f;
                ed[i].evisLimbVel[li].x *= 0.6f;
                ed[i].evisLimbVel[li].z *= 0.6f;
            }
            if (t < 3.0f) ed[i].evisBloodTimer[li] += dt;
        }

        if (ed[i].deathTimer <= 0) {
            ecs_delete(it->world, it->entities[i]);
        }
    }
}

// ============================================================================
// Damage / Death trigger functions
// ============================================================================

void EcsEnemyDamage(ecs_world_t *world, ecs_entity_t entity, float damage) {
    if (!ecs_is_alive(world, entity)) return;
    if (!ecs_has(world, entity, EcAlive)) return;

    EcCombatStats *cs = ecs_ensure(world, entity, EcCombatStats);
    EcAnimation *anim = ecs_ensure(world, entity, EcAnimation);
    EcAIState *ai = ecs_ensure(world, entity, EcAIState);
    const EcFaction *fac = ecs_get(world, entity, EcFaction);

    cs->health -= damage;
    anim->hitFlash = 1;
    if (ai->dodgeTimer <= 0) { ai->strafeDir *= -1; ai->dodgeTimer = 1; }

    if (cs->health <= 0) {
        // Remove alive, add dying
        ecs_remove(world, entity, EcAlive);
        ecs_add(world, entity, EcDying);

        anim->deathAngle = 0;
        int deathStyle = (rand() % 100 < 40) ? 1 : 0;

        if (deathStyle == 0) {
            float launchAngle = ((float)rand() / RAND_MAX) * 2.0f * PI;
            float launchForce = 3.0f + ((float)rand() / RAND_MAX) * 5.0f;
            ecs_set(world, entity, EcRagdollDeath, {
                .spinX = 120.0f + (float)(rand() % 300),
                .spinY = (float)(rand() % 200) - 100.0f,
                .spinZ = 0,
                .ragdollVelX = cosf(launchAngle) * launchForce,
                .ragdollVelZ = sinf(launchAngle) * launchForce,
                .ragdollVelY = 2.0f + ((float)rand() / RAND_MAX) * 4.0f,
                .deathTimer = 10.0f,
                .deathStyle = 0
            });
        } else {
            ecs_set(world, entity, EcRagdollDeath, {
                .spinX = 80.0f + (float)(rand() % 60),
                .spinY = 0,
                .spinZ = 0,
                .ragdollVelX = 0, .ragdollVelZ = 0, .ragdollVelY = 0,
                .deathTimer = 12.0f,
                .deathStyle = 1
            });
        }

        if (fac) PlayDeathRadioSound(world, fac->type, 50);

        EcGameContext *ctx = ecs_singleton_ensure(world, EcGameContext);
        if (ctx) ctx->killCount++;
    }
}

void EcsEnemyVaporize(ecs_world_t *world, ecs_entity_t entity) {
    if (!ecs_is_alive(world, entity)) return;
    if (!ecs_has(world, entity, EcAlive)) return;

    const EcFaction *fac = ecs_get(world, entity, EcFaction);
    EcCombatStats *cs = ecs_ensure(world, entity, EcCombatStats);
    cs->health = 0;

    ecs_remove(world, entity, EcAlive);
    ecs_add(world, entity, EcVaporizing);

    int deathStyle = (rand() % 100 < 15) ? 1 : 0;
    ecs_set(world, entity, EcVaporizeDeath, {
        .vaporizeTimer = 0,
        .vaporizeScale = 1.0f,
        .deathTimer = (deathStyle == 1) ? 6.0f : 4.5f,
        .deathStyle = deathStyle
    });

    if (fac) PlayDeathRadioSound(world, fac->type, 50);

    EcGameContext *ctx = ecs_singleton_ensure(world, EcGameContext);
    if (ctx) ctx->killCount++;
}

void EcsEnemyEviscerate(ecs_world_t *world, ecs_entity_t entity, Vector3 hitDir) {
    if (!ecs_is_alive(world, entity)) return;
    if (!ecs_has(world, entity, EcAlive)) return;

    const EcFaction *fac = ecs_get(world, entity, EcFaction);
    EcCombatStats *cs = ecs_ensure(world, entity, EcCombatStats);
    cs->health = 0;

    ecs_remove(world, entity, EcAlive);
    ecs_add(world, entity, EcEviscerating);

    hitDir = Vector3Normalize(hitDir);

    // 0=head, 1=torso, 2=right arm, 3=left arm, 4=right leg, 5=left leg
    Vector3 origins[6] = {
        {0, 1.1f, 0}, {0, 0, 0}, {0.52f, 0.3f, 0},
        {-0.52f, 0.3f, 0}, {0.22f, -0.85f, 0}, {-0.22f, -0.85f, 0}
    };

    EcEviscerateDeath ed = {
        .evisTimer = 0,
        .evisDir = hitDir,
        .deathTimer = 10.0f
    };

    for (int i = 0; i < 6; i++) {
        ed.evisLimbPos[i] = origins[i];
        ed.evisBloodTimer[i] = 0;
        float rx = ((float)rand() / RAND_MAX - 0.5f) * 6.0f;
        float ry = 2.0f + ((float)rand() / RAND_MAX) * 5.0f;
        float rz = ((float)rand() / RAND_MAX - 0.5f) * 6.0f;
        ed.evisLimbVel[i] = (Vector3){
            hitDir.x * 4.0f + rx + origins[i].x * 2.0f, ry,
            hitDir.z * 4.0f + rz + origins[i].z * 2.0f
        };
    }
    ed.evisLimbVel[1].x += hitDir.x * 3.0f;
    ed.evisLimbVel[1].z += hitDir.z * 3.0f;
    ed.evisLimbVel[0].y += 4.0f;

    ecs_set_id(world, entity, ecs_id(EcEviscerateDeath), sizeof(EcEviscerateDeath), &ed);

    if (fac) PlayDeathRadioSound(world, fac->type, 65);

    EcGameContext *ctx = ecs_singleton_ensure(world, EcGameContext);
    if (ctx) ctx->killCount++;
}

int EcsEnemyCountAlive(ecs_world_t *world) {
    if (!g_aliveQuery) return 0;
    int count = 0;
    ecs_iter_t it = ecs_query_iter(world, g_aliveQuery);
    while (ecs_query_next(&it)) {
        count += it.count;
    }
    return count;
}

// ============================================================================
// System Registration
// ============================================================================

void EcsEnemySystemsRegister(ecs_world_t *world) {
    // Create the alive query for collision avoidance and count
    g_aliveQuery = ecs_query(world, {
        .terms = {
            { .id = ecs_id(EcTransform) },
            { .id = ecs_id(EcAlive) }
        }
    });

    // SysAITargeting — EcsOnUpdate
    ecs_system_init(world, &(ecs_system_desc_t){
        .entity = ecs_entity(world, {
            .name = "SysAITargeting",
            .add = ecs_ids(ecs_dependson(EcsOnUpdate))
        }),
        .query.terms = {
            { .id = ecs_id(EcTransform) },
            { .id = ecs_id(EcAIState) },
            { .id = ecs_id(EcAlive) }
        },
        .callback = SysAITargeting
    });

    // SysAIBehavior — EcsOnUpdate
    ecs_system_init(world, &(ecs_system_desc_t){
        .entity = ecs_entity(world, {
            .name = "SysAIBehavior",
            .add = ecs_ids(ecs_dependson(EcsOnUpdate))
        }),
        .query.terms = {
            { .id = ecs_id(EcTransform) },
            { .id = ecs_id(EcAIState) },
            { .id = ecs_id(EcCombatStats) },
            { .id = ecs_id(EcFaction) },
            { .id = ecs_id(EcVelocity) },
            { .id = ecs_id(EcAlive) }
        },
        .callback = SysAIBehavior
    });

    // SysCollisionAvoidance — EcsPostUpdate
    ecs_system_init(world, &(ecs_system_desc_t){
        .entity = ecs_entity(world, {
            .name = "SysCollisionAvoidance",
            .add = ecs_ids(ecs_dependson(EcsPostUpdate))
        }),
        .query.terms = {
            { .id = ecs_id(EcTransform) },
            { .id = ecs_id(EcVelocity) },
            { .id = ecs_id(EcAlive) }
        },
        .callback = SysCollisionAvoidance
    });

    // SysPhysics — EcsPostUpdate
    ecs_system_init(world, &(ecs_system_desc_t){
        .entity = ecs_entity(world, {
            .name = "SysPhysics",
            .add = ecs_ids(ecs_dependson(EcsPostUpdate))
        }),
        .query.terms = {
            { .id = ecs_id(EcTransform) },
            { .id = ecs_id(EcVelocity) },
            { .id = ecs_id(EcAnimation) },
            { .id = ecs_id(EcAlive) }
        },
        .callback = SysPhysics
    });

    // SysAttack — EcsPostUpdate
    ecs_system_init(world, &(ecs_system_desc_t){
        .entity = ecs_entity(world, {
            .name = "SysAttack",
            .add = ecs_ids(ecs_dependson(EcsPostUpdate))
        }),
        .query.terms = {
            { .id = ecs_id(EcTransform) },
            { .id = ecs_id(EcCombatStats) },
            { .id = ecs_id(EcAIState) },
            { .id = ecs_id(EcFaction) },
            { .id = ecs_id(EcAnimation) },
            { .id = ecs_id(EcAlive) }
        },
        .callback = SysAttack
    });

    // SysRagdollDeath — EcsOnUpdate
    ecs_system_init(world, &(ecs_system_desc_t){
        .entity = ecs_entity(world, {
            .name = "SysRagdollDeath",
            .add = ecs_ids(ecs_dependson(EcsOnUpdate))
        }),
        .query.terms = {
            { .id = ecs_id(EcTransform) },
            { .id = ecs_id(EcAnimation) },
            { .id = ecs_id(EcRagdollDeath) },
            { .id = ecs_id(EcDying) }
        },
        .callback = SysRagdollDeath
    });

    // SysVaporizeDeath — EcsOnUpdate
    ecs_system_init(world, &(ecs_system_desc_t){
        .entity = ecs_entity(world, {
            .name = "SysVaporizeDeath",
            .add = ecs_ids(ecs_dependson(EcsOnUpdate))
        }),
        .query.terms = {
            { .id = ecs_id(EcTransform) },
            { .id = ecs_id(EcAnimation) },
            { .id = ecs_id(EcVaporizeDeath) },
            { .id = ecs_id(EcVaporizing) }
        },
        .callback = SysVaporizeDeath
    });

    // SysEviscerateDeath — EcsOnUpdate
    ecs_system_init(world, &(ecs_system_desc_t){
        .entity = ecs_entity(world, {
            .name = "SysEviscerateDeath",
            .add = ecs_ids(ecs_dependson(EcsOnUpdate))
        }),
        .query.terms = {
            { .id = ecs_id(EcTransform) },
            { .id = ecs_id(EcEviscerateDeath) },
            { .id = ecs_id(EcEviscerating) }
        },
        .callback = SysEviscerateDeath
    });
}
