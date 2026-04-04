#include "enemy_ai.h"
#include "enemy_components.h"
#include "ecs_world.h"
#include "world.h"
#include "structure/structure.h"
#include "config.h"
#include <math.h>
#include <stdlib.h>

// ============================================================================
// Spatial hash for O(N) collision avoidance
// ============================================================================
typedef struct {
    ecs_entity_t entities[SPATIAL_CELL_CAP];
    Vector3 positions[SPATIAL_CELL_CAP];
    EnemyRank ranks[SPATIAL_CELL_CAP];
    int count;
} SpatialCell;

static SpatialCell g_spatialGrid[SPATIAL_GRID_DIM][SPATIAL_GRID_DIM];
static float g_gridOriginX = 0, g_gridOriginZ = 0;

static void SpatialGridClear(void) {
    for (int x = 0; x < SPATIAL_GRID_DIM; x++)
        for (int z = 0; z < SPATIAL_GRID_DIM; z++)
            g_spatialGrid[x][z].count = 0;
}

static void SpatialGridInsert(ecs_entity_t e, Vector3 pos, EnemyRank rank) {
    int cx = (int)((pos.x - g_gridOriginX) / SPATIAL_CELL_SIZE) + SPATIAL_GRID_DIM / 2;
    int cz = (int)((pos.z - g_gridOriginZ) / SPATIAL_CELL_SIZE) + SPATIAL_GRID_DIM / 2;
    if (cx < 0 || cx >= SPATIAL_GRID_DIM || cz < 0 || cz >= SPATIAL_GRID_DIM) return;
    SpatialCell *cell = &g_spatialGrid[cx][cz];
    if (cell->count < SPATIAL_CELL_CAP) {
        cell->entities[cell->count] = e;
        cell->positions[cell->count] = pos;
        cell->ranks[cell->count] = rank;
        cell->count++;
    }
}

// Stored queries
static ecs_query_t *g_aiAliveQuery = NULL;
static ecs_query_t *g_squadQuery = NULL;

// ============================================================================
// SysAITargeting -- face toward player
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
// SysAIBehavior -- Soviet rush / American tactical
// ============================================================================
static void SysAIBehavior(ecs_iter_t *it) {
    EcTransform *tr = ecs_field(it, EcTransform, 0);
    EcAIState *ai = ecs_field(it, EcAIState, 1);
    EcCombatStats *cs = ecs_field(it, EcCombatStats, 2);
    EcFaction *fac = ecs_field(it, EcFaction, 3);
    EcVelocity *vel = ecs_field(it, EcVelocity, 4);
    EcSteering *steerArr = ecs_field(it, EcSteering, 5);
    (void)vel; // velocity now set via steering

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

        // AI_FLEE: run directly away from player
        if (ai[i].behavior == AI_FLEE) {
            moveDir = Vector3Scale(fwd, -1.0f); // away from player
            moving = true;
            // Write to steering and skip normal behavior logic
            float fleeSpd = cs[i].speed * MORALE_SPEED_PENALTY;
            steerArr[i].desiredVelocity = (Vector3){ -fwd.x * fleeSpd, 0, -fwd.z * fleeSpd };
            continue;
        }

        // Read rank for behavior modifications
        const EcRank *rk = ecs_get(it->world, entity, EcRank);
        EnemyRank rank = rk ? rk->rank : RANK_TROOPER;

        if (!doFullAI) {
            moveDir = fwd;
            moving = true;
            ai[i].behavior = AI_ADVANCE;
        } else if (fac[i].type == ENEMY_SOVIET) {
            // SOVIET: Charge as a spread -- run at player with wide strafe
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
                // Rush wide around -- heavy strafe to split around the base
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
            // Soviet NCO: lead the charge closer
            if (rank == RANK_NCO && dist > cs[i].preferredDist * 0.6f) {
                moveDir = Vector3Add(fwd, Vector3Scale(strafe, ai[i].strafeDir * 0.3f));
                moving = true;
            }
            // Soviet Officer: hold at preferred distance, dodge more
            if (rank == RANK_OFFICER && dist < cs[i].preferredDist * 0.8f && dist > 4.0f) {
                moveDir = Vector3Scale(strafe, ai[i].strafeDir * 1.2f);
                moving = true; ai[i].behavior = AI_STRAFE;
            }
        } else {
            // AMERICAN: Tactical -- cover behind rocks, flank around structures
            World *w = WorldGetActive();
            StructureManager *amStructs = StructureGetActive();
            bool foundCover = false;

            // Check if a structure blocks the path to player -- if so, flank
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
                // Flank around the structure -- rush wide to the side then re-engage
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
            // American NCO: tighter strafe, suppressive fire position
            if (rank == RANK_NCO) {
                if (dist > cs[i].preferredDist * 0.7f && dist < cs[i].preferredDist * 1.2f) {
                    moveDir = Vector3Scale(strafe, ai[i].strafeDir * 0.6f);
                    moving = true; ai[i].behavior = AI_STRAFE;
                }
            }
            // American Officer: always seek cover, extended range engagement
            if (rank == RANK_OFFICER && !foundCover) {
                if (dist < cs[i].preferredDist * 1.5f) {
                    moveDir = Vector3Add(Vector3Scale(fwd, -0.5f), Vector3Scale(strafe, ai[i].strafeDir));
                    moving = true; ai[i].behavior = AI_RETREAT;
                }
            }
        }

        // Squad cohesion: troopers bias toward squad centroid
        const EcSquad *squad = ecs_get(it->world, entity, EcSquad);
        if (squad && squad->squadId >= 0 && rank == RANK_TROOPER && g_squadQuery) {
            Vector3 centroid = {0, 0, 0};
            int squadCount = 0;
            ecs_iter_t sqit = ecs_query_iter(it->world, g_squadQuery);
            while (ecs_query_next(&sqit)) {
                EcTransform *sqtr = ecs_field(&sqit, EcTransform, 0);
                EcSquad *sqsq = ecs_field(&sqit, EcSquad, 1);
                for (int si = 0; si < sqit.count; si++) {
                    if (sqsq[si].squadId == squad->squadId && sqit.entities[si] != entity) {
                        centroid = Vector3Add(centroid, sqtr[si].position);
                        squadCount++;
                    }
                }
            }
            if (squadCount > 0) {
                centroid = Vector3Scale(centroid, 1.0f / (float)squadCount);
                Vector3 toCentroid = Vector3Subtract(centroid, tr[i].position);
                toCentroid.y = 0;
                float centDist = Vector3Length(toCentroid);
                if (centDist > 2.0f && centDist < SQUAD_COHESION_RADIUS) {
                    Vector3 cohesionPull = Vector3Scale(Vector3Normalize(toCentroid), SQUAD_COHESION_BIAS);
                    moveDir = Vector3Add(moveDir, cohesionPull);
                }
            }
        }

        // Store desired movement in steering component (physics will lerp)
        if (moving && Vector3Length(moveDir) > 0.01f) {
            moveDir = Vector3Normalize(moveDir);
            float spd = cs[i].speed * (ai[i].behavior == AI_DODGE ? 2.0f : 1.0f);
            const EcMorale *mor = ecs_get(it->world, entity, EcMorale);
            if (mor && mor->morale < MORALE_FLEE_THRESHOLD) {
                spd *= MORALE_SPEED_PENALTY;
            }
            steerArr[i].desiredVelocity = (Vector3){ moveDir.x * spd, 0, moveDir.z * spd };
        } else {
            steerArr[i].desiredVelocity = (Vector3){0, 0, 0};
        }
    }
}

// ============================================================================
// SysSpatialHashBuild -- populate spatial grid each frame (O(N))
// ============================================================================
// Flag to clear grid once per frame before collision avoidance runs
static bool g_gridBuiltThisFrame = false;

static void SysSpatialHashBuild(ecs_iter_t *it) {
    EcTransform *tr = ecs_field(it, EcTransform, 0);

    // Center grid on player and clear on first batch
    if (!g_gridBuiltThisFrame) {
        const EcGameContext *ctx = ecs_singleton_get(it->world, EcGameContext);
        if (ctx) { g_gridOriginX = ctx->playerPos.x; g_gridOriginZ = ctx->playerPos.z; }
        SpatialGridClear();
        g_gridBuiltThisFrame = true;
    }

    for (int i = 0; i < it->count; i++) {
        const EcRank *rk = ecs_get(it->world, it->entities[i], EcRank);
        EnemyRank rank = rk ? rk->rank : RANK_TROOPER;
        SpatialGridInsert(it->entities[i], tr[i].position, rank);
    }
}

// ============================================================================
// SysCollisionAvoidance -- spatial hash O(N) push apart
// ============================================================================
static void SysCollisionAvoidance(ecs_iter_t *it) {
    EcTransform *tr = ecs_field(it, EcTransform, 0);
    EcVelocity *vel = ecs_field(it, EcVelocity, 1);

    // Reset build flag for next frame
    g_gridBuiltThisFrame = false;

    float separationRadius = 3.5f;

    for (int i = 0; i < it->count; i++) {
        Vector3 pos = tr[i].position;
        int cx = (int)((pos.x - g_gridOriginX) / SPATIAL_CELL_SIZE) + SPATIAL_GRID_DIM / 2;
        int cz = (int)((pos.z - g_gridOriginZ) / SPATIAL_CELL_SIZE) + SPATIAL_GRID_DIM / 2;

        Vector3 pushAccum = {0, 0, 0};
        int neighbors = 0;

        // Check 3x3 neighborhood
        for (int dx = -1; dx <= 1; dx++) {
            for (int dz = -1; dz <= 1; dz++) {
                int nx = cx + dx, nz = cz + dz;
                if (nx < 0 || nx >= SPATIAL_GRID_DIM || nz < 0 || nz >= SPATIAL_GRID_DIM) continue;
                SpatialCell *cell = &g_spatialGrid[nx][nz];
                for (int j = 0; j < cell->count; j++) {
                    if (cell->entities[j] == it->entities[i]) continue;
                    Vector3 away = Vector3Subtract(pos, cell->positions[j]);
                    away.y = 0;
                    float len = Vector3Length(away);
                    if (len < separationRadius && len > 0.05f) {
                        float strength = (separationRadius - len) / separationRadius;
                        // Officers push harder — others give way
                        float pushMult = 4.0f;
                        const EcRank *myRank = ecs_get(it->world, it->entities[i], EcRank);
                        if (myRank && myRank->rank == RANK_OFFICER) pushMult = 6.0f;
                        else if (cell->ranks[j] == RANK_OFFICER) pushMult = 2.0f; // yield to officer
                        Vector3 push = Vector3Scale(Vector3Normalize(away), strength * pushMult);
                        pushAccum = Vector3Add(pushAccum, push);
                        neighbors++;
                    }
                }
            }
        }

        if (neighbors > 0) {
            vel[i].velocity = Vector3Add(vel[i].velocity, pushAccum);
        }
    }
}

// ============================================================================
// Registration
// ============================================================================

void EcsEnemyAISystemsRegister(ecs_world_t *world) {
    // Create stored queries
    g_aiAliveQuery = ecs_query(world, {
        .terms = {
            { .id = ecs_id(EcTransform) },
            { .id = ecs_id(EcAlive) }
        }
    });
    g_squadQuery = ecs_query(world, {
        .terms = {
            { .id = ecs_id(EcTransform) },
            { .id = ecs_id(EcSquad) },
            { .id = ecs_id(EcAlive) }
        }
    });

    // SysAITargeting -- EcsOnUpdate
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

    // SysAIBehavior -- EcsOnUpdate
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
            { .id = ecs_id(EcSteering) },
            { .id = ecs_id(EcAlive) }
        },
        .callback = SysAIBehavior
    });

    // SysSpatialHashBuild -- EcsPostUpdate (before collision avoidance)
    ecs_system_init(world, &(ecs_system_desc_t){
        .entity = ecs_entity(world, {
            .name = "SysSpatialHashBuild",
            .add = ecs_ids(ecs_dependson(EcsPostUpdate))
        }),
        .query.terms = {
            { .id = ecs_id(EcTransform) },
            { .id = ecs_id(EcAlive) }
        },
        .callback = SysSpatialHashBuild
    });

    // SysCollisionAvoidance -- EcsPostUpdate
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
}

void EcsEnemyAISystemsCleanup(void) {
    if (g_aiAliveQuery) {
        ecs_query_fini(g_aiAliveQuery);
        g_aiAliveQuery = NULL;
    }
    if (g_squadQuery) {
        ecs_query_fini(g_squadQuery);
        g_squadQuery = NULL;
    }
}
