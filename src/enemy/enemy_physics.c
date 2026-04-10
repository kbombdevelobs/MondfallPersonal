#include "enemy_physics.h"
#include "enemy_components.h"
#include "ecs_world.h"
#include "world.h"
#include "structure/structure.h"
#include "config.h"
#include <math.h>
#include <stdlib.h>

// ============================================================================
// SysPhysics -- apply velocity, gravity, jumping, animation
// ============================================================================
static void SysPhysics(ecs_iter_t *it) {
    EcTransform *tr = ecs_field(it, EcTransform, 0);
    EcVelocity *vel = ecs_field(it, EcVelocity, 1);
    EcAnimation *anim = ecs_field(it, EcAnimation, 2);

    float dt = it->delta_time;

    for (int i = 0; i < it->count; i++) {
        // Decay stagger timer
        if (anim[i].staggerTimer > 0) anim[i].staggerTimer -= dt;

        // Decay knockdown timer and animate getting back up
        if (anim[i].knockdownTimer > 0) {
            anim[i].knockdownTimer -= dt;
            if (anim[i].knockdownTimer <= 0) {
                anim[i].knockdownTimer = 0;
                anim[i].knockdownAngle = 0;
            } else if (anim[i].knockdownTimer < 1.8f) {
                // Slowly ease back to standing during final 1.8s
                float recoveryT = 1.0f - (anim[i].knockdownTimer / 1.8f);
                float ease = recoveryT * recoveryT * (3.0f - 2.0f * recoveryT); // smoothstep
                float targetAngle = anim[i].knockdownAngle * (1.0f - ease);
                anim[i].knockdownAngle += (targetAngle - anim[i].knockdownAngle) * 2.0f * dt;
            }
        }

        // Momentum: lerp velocity toward desired velocity from steering
        // Skip during stagger or knockdown — let impulse play out
        const EcSteering *steer = ecs_get(it->world, it->entities[i], EcSteering);
        if (steer && anim[i].staggerTimer <= 0 && anim[i].knockdownTimer <= 0) {
            float a = steer->acceleration * dt;
            if (a > 1.0f) a = 1.0f;
            vel[i].velocity.x += (steer->desiredVelocity.x - vel[i].velocity.x) * a;
            vel[i].velocity.z += (steer->desiredVelocity.z - vel[i].velocity.z) * a;
        }

        bool moving = Vector3Length(vel[i].velocity) > 0.01f;

        if (moving) {
            float spd = Vector3Length(vel[i].velocity);
            float newX = tr[i].position.x + vel[i].velocity.x * dt;
            float newZ = tr[i].position.z + vel[i].velocity.z * dt;

            // Slope awareness: penalize speed on steep terrain
            float currentH = WorldGetHeight(tr[i].position.x, tr[i].position.z);
            float targetH = WorldGetHeight(newX, newZ);
            float hDiff = fabsf(targetH - currentH);
            float moveLen = spd * dt;
            if (moveLen > 0.001f) {
                float slope = hDiff / moveLen;
                if (slope > AI_SLOPE_THRESHOLD) {
                    float penalty = 1.0f - (slope - AI_SLOPE_THRESHOLD) * 2.0f;
                    if (penalty < AI_SLOPE_MIN_FACTOR) penalty = AI_SLOPE_MIN_FACTOR;
                    newX = tr[i].position.x + vel[i].velocity.x * dt * penalty;
                    newZ = tr[i].position.z + vel[i].velocity.z * dt * penalty;
                }
            }

            // Structure collision -- slide around bases instead of walking through
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
// SysAttack -- fire at player, accumulate damage
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

        // Fleeing, knocked-down, or staggered units don't fire
        if (ai[i].behavior == AI_FLEE) continue;
        if (anim[i].knockdownTimer > 0) continue;
        if (anim[i].staggerTimer > 0) continue;
        if (dist >= cs[i].attackRange || ai[i].burstCooldown > 0) continue;

        if (ai[i].attackTimer >= cs[i].attackRate) {
            ai[i].attackTimer = 0;
            anim[i].muzzleFlash = 1;
            ai[i].burstShots++;

            if (res) {
                if (fac[i].type == ENEMY_SOVIET) PlaySound(res->sndSovietFire);
                else PlaySound(res->sndAmericanFire);
            }

            // Rank-based accuracy: troopers spray, NCOs steady, officers deadly
            float accBase, accRange;
            const EcRank *rkAcc = ecs_get(it->world, it->entities[i], EcRank);
            EnemyRank rkAccVal = rkAcc ? rkAcc->rank : RANK_TROOPER;
            if (rkAccVal == RANK_OFFICER) {
                accBase = OFFICER_ACCURACY_BASE; accRange = OFFICER_ACCURACY_RANGE;
            } else if (rkAccVal == RANK_NCO) {
                accBase = NCO_ACCURACY_BASE; accRange = NCO_ACCURACY_RANGE;
            } else {
                accBase = TROOPER_ACCURACY_BASE; accRange = TROOPER_ACCURACY_RANGE;
            }
            float hitChance = accBase - (dist / cs[i].attackRange) * accRange;
            if (ai[i].behavior == AI_STRAFE) hitChance -= 0.1f;
            // Morale accuracy bonus when near leader
            const EcMorale *mor = ecs_get(it->world, it->entities[i], EcMorale);
            if (mor && mor->leaderDist <= LEADERSHIP_RADIUS && mor->morale > 0.7f) {
                hitChance += MORALE_ACCURACY_BONUS;
            }
            if ((float)rand() / RAND_MAX < hitChance) {
                if (ctxMut) ctxMut->playerDamageAccum += cs[i].damage;
            }

            // Occasional visible bolt projectile
            if (ctxMut && ctxMut->boltCount < MAX_ENEMY_BOLTS && (rand() % 100) < ENEMY_BOLT_CHANCE) {
                Vector3 up = {0, 1, 0};
                float fa = tr[i].facingAngle;
                Vector3 fwd = {sinf(fa), 0, cosf(fa)};
                Vector3 right = Vector3CrossProduct(fwd, up);
                Vector3 muzzle = Vector3Add(tr[i].position,
                    Vector3Add(Vector3Scale(up, 0.65f),
                    Vector3Add(Vector3Scale(right, 0.2f),
                               Vector3Scale(fwd, 0.88f))));
                int bi = ctxMut->boltCount;
                ctxMut->boltStart[bi] = muzzle;
                ctxMut->boltEnd[bi] = playerPos;
                ctxMut->boltColor[bi] = (fac[i].type == ENEMY_SOVIET) ?
                    (Color){255, 150, 50, 230} : (Color){80, 180, 255, 230};
                ctxMut->boltProgress[bi] = 0;
                ctxMut->boltLife[bi] = ENEMY_BOLT_LIFETIME;
                ctxMut->boltCount++;
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
// Registration
// ============================================================================

void EcsEnemyPhysicsSystemsRegister(ecs_world_t *world) {
    // SysPhysics -- EcsPostUpdate
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

    // SysAttack -- EcsPostUpdate
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
}
