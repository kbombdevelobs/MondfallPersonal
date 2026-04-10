#include "enemy_draw_ecs.h"
#include "enemy.h"
#include "enemy_draw.h"
#include "enemy_model_loader.h"
#include "enemy_components.h"
#include "ecs_world.h"
#include "world.h"
#include "sound_gen.h"
#include "config.h"
#include <string.h>
#include <stdlib.h>

/* Persistent skeletal model set — loaded once, shared across frames */
static AstroModelSet g_astroModels;
static bool g_astroModelsLoaded = false;

void EcsEnemyResourcesInit(ecs_world_t *world) {
    EcEnemyResources res;
    memset(&res, 0, sizeof(res));

    res.mdlVisor = LoadModelFromMesh(GenMeshSphere(0.28f, 6, 6));
    res.mdlArm = LoadModelFromMesh(GenMeshCube(0.22f, 0.8f, 0.22f));
    res.mdlBoot = LoadModelFromMesh(GenMeshCube(0.28f, 0.3f, 0.35f));

    // Enemy shooting sounds
    {
        int sr = AUDIO_SAMPLE_RATE;
        Wave w = SoundGenCreateWave(sr, 0.1f);
        short *d = (short *)w.data;
        for (int i = 0; i < (int)w.frameCount; i++) {
            float t = (float)i / sr;
            d[i] = (short)(((float)rand()/RAND_MAX*2-1) * expf(-t*35) * 18000 + sinf(t*150*2*3.14159f)*expf(-t*20)*10000);
        }
        res.sndSovietFire = LoadSoundFromWave(w);
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
        res.sndAmericanFire = LoadSoundFromWave(w);
        UnloadWave(w);
    }

    // Load Soviet death sounds
    res.sovietDeathCount = 0;
    const char *deathFiles[] = {
        "sounds/soviet_death_sounds/Echoes of the Frozen Front.mp3",
        "sounds/soviet_death_sounds/Last Echo of the Soviet Front The Final Net.mp3",
    };
    for (int df = 0; df < 2; df++) {
        if (!FileExists(deathFiles[df])) continue;
        Wave deathWav = LoadWave(deathFiles[df]);
        if (deathWav.frameCount <= 0) { UnloadWave(deathWav); continue; }
        WaveFormat(&deathWav, AUDIO_RADIO_SAMPLE_RATE, 16, 1);
        SoundGenDegradeRadio(&deathWav);
        res.sndSovietDeath[res.sovietDeathCount] = LoadSoundFromWave(deathWav);
        SetSoundVolume(res.sndSovietDeath[res.sovietDeathCount], AUDIO_DEATH_VOLUME);
        UnloadWave(deathWav);
        res.sovietDeathCount++;
    }

    // Load American death sounds
    res.americanDeathCount = 0;
    const char *amDeathFiles[] = {
        "sounds/american_death_sounds/Darn Bastard's Final Stand.mp3",
        "sounds/american_death_sounds/The Last Yeehaw of the Lone Star Son (1).mp3",
    };
    for (int df = 0; df < 2; df++) {
        if (!FileExists(amDeathFiles[df])) continue;
        Wave dw = LoadWave(amDeathFiles[df]);
        if (dw.frameCount <= 0) { UnloadWave(dw); continue; }
        WaveFormat(&dw, AUDIO_RADIO_SAMPLE_RATE, 16, 1);
        SoundGenDegradeRadio(&dw);
        res.sndAmericanDeath[res.americanDeathCount] = LoadSoundFromWave(dw);
        SetSoundVolume(res.sndAmericanDeath[res.americanDeathCount], AUDIO_DEATH_VOLUME);
        UnloadWave(dw);
        res.americanDeathCount++;
    }

    // Load ground pound scream transmissions
    res.groundPoundScreamCount = 0;
    const char *screamFiles[] = {
        "sounds/ground_pound_screams/scream_01.mp3",
        "sounds/ground_pound_screams/scream_02.mp3",
        "sounds/ground_pound_screams/scream_03.mp3",
        "sounds/ground_pound_screams/scream_04.mp3",
    };
    for (int sf = 0; sf < 4; sf++) {
        if (!FileExists(screamFiles[sf])) continue;
        Wave sw = LoadWave(screamFiles[sf]);
        if (sw.frameCount <= 0) { UnloadWave(sw); continue; }
        WaveFormat(&sw, AUDIO_RADIO_SAMPLE_RATE, 16, 1);
        SoundGenDegradeRadio(&sw);
        res.sndGroundPoundScream[res.groundPoundScreamCount] = LoadSoundFromWave(sw);
        SetSoundVolume(res.sndGroundPoundScream[res.groundPoundScreamCount], AUDIO_DEATH_VOLUME);
        UnloadWave(sw);
        res.groundPoundScreamCount++;
    }

    /* Load skeletal .glb character models */
    if (!g_astroModelsLoaded) {
        AstroModelsLoad(&g_astroModels);
        g_astroModelsLoaded = true;
    }

    res.modelsLoaded = true;
    ecs_singleton_set_ptr(world, EcEnemyResources, &res);
}

void EcsEnemyResourcesUnload(ecs_world_t *world) {
    const EcEnemyResources *rp = ecs_get(world, ecs_id(EcEnemyResources), EcEnemyResources);
    if (!rp || !rp->modelsLoaded) return;

    // Copy to local since we need mutable access
    EcEnemyResources res = *rp;
    UnloadModel(res.mdlVisor);
    UnloadModel(res.mdlArm);
    UnloadModel(res.mdlBoot);
    UnloadSound(res.sndSovietFire);
    UnloadSound(res.sndAmericanFire);
    for (int i = 0; i < res.sovietDeathCount; i++) UnloadSound(res.sndSovietDeath[i]);
    for (int i = 0; i < res.americanDeathCount; i++) UnloadSound(res.sndAmericanDeath[i]);
    for (int i = 0; i < res.groundPoundScreamCount; i++) UnloadSound(res.sndGroundPoundScream[i]);

    /* Unload skeletal models */
    if (g_astroModelsLoaded) {
        AstroModelsUnload(&g_astroModels);
        g_astroModelsLoaded = false;
    }
}

// Fill a temporary Enemy struct from ECS components for draw functions
static void FillTempEnemy(ecs_world_t *world, ecs_entity_t entity,
                          const EcTransform *tr, const EcFaction *fac, const EcAnimation *anim,
                          Enemy *out) {
    memset(out, 0, sizeof(Enemy));
    out->position = tr->position;
    out->facingAngle = tr->facingAngle;
    out->type = fac->type;
    out->active = true;
    out->animState = anim->animState;
    out->walkCycle = anim->walkCycle;
    out->shootAnim = anim->shootAnim;
    out->hitFlash = anim->hitFlash;
    out->muzzleFlash = anim->muzzleFlash;
    out->deathAngle = anim->deathAngle;

    // Default to alive
    out->state = ENEMY_ALIVE;

    // Check death states and fill death-specific data
    if (ecs_has(world, entity, EcEviscerating)) {
        out->state = ENEMY_EVISCERATING;
        const EcEviscerateDeath *ed = ecs_get(world, entity, EcEviscerateDeath);
        if (ed) {
            out->evisTimer = ed->evisTimer;
            out->evisDir = ed->evisDir;
            for (int i = 0; i < 6; i++) {
                out->evisLimbPos[i] = ed->evisLimbPos[i];
                out->evisLimbVel[i] = ed->evisLimbVel[i];
                out->evisBloodTimer[i] = ed->evisBloodTimer[i];
            }
            out->deathTimer = ed->deathTimer;
        }
    } else if (ecs_has(world, entity, EcVaporizing)) {
        out->state = ENEMY_VAPORIZING;
        const EcVaporizeDeath *vd = ecs_get(world, entity, EcVaporizeDeath);
        if (vd) {
            out->vaporizeTimer = vd->vaporizeTimer;
            out->vaporizeScale = vd->vaporizeScale;
            out->deathTimer = vd->deathTimer;
            out->deathStyle = vd->deathStyle;
        }
    } else if (ecs_has(world, entity, EcDecapitating)) {
        out->state = ENEMY_DECAPITATING;
        const EcDecapitateDeath *dd = ecs_get(world, entity, EcDecapitateDeath);
        if (dd) {
            out->decapTimer = dd->timer;
            out->decapBloodTimer = dd->bloodTimer;
            out->decapDriftVel = dd->driftVel;
            out->decapDriftVelY = dd->driftVelY;
            out->decapHitDir = dd->hitDir;
            out->deathTimer = dd->deathTimer;
            out->spinX = dd->spinX;
            out->spinY = dd->spinY;
        }
    } else if (ecs_has(world, entity, EcDying)) {
        out->state = ENEMY_DYING;
        const EcRagdollDeath *rd = ecs_get(world, entity, EcRagdollDeath);
        if (rd) {
            out->spinX = rd->spinX;
            out->spinY = rd->spinY;
            out->spinZ = rd->spinZ;
            out->ragdollVelX = rd->ragdollVelX;
            out->ragdollVelZ = rd->ragdollVelZ;
            out->ragdollVelY = rd->ragdollVelY;
            out->deathTimer = rd->deathTimer;
            out->deathStyle = rd->deathStyle;
        }
    }

    // Fill rank if present
    const EcRank *rk = ecs_get(world, entity, EcRank);
    out->rank = rk ? rk->rank : RANK_TROOPER;

    // Fill combat stats if present
    const EcCombatStats *cs = ecs_get(world, entity, EcCombatStats);
    if (cs) {
        out->health = cs->health;
        out->maxHealth = cs->maxHealth;
        out->speed = cs->speed;
        out->damage = cs->damage;
        out->attackRange = cs->attackRange;
        out->attackRate = cs->attackRate;
        out->preferredDist = cs->preferredDist;
    }

    // Copy limb state for spring-driven procedural animation
    const EcLimbState *ls = ecs_get(world, entity, EcLimbState);
    if (ls) {
        out->limbState = *ls;
        out->hasLimbState = true;
    }

    // Copy knockdown/cowering animation state
    out->knockdownAngle = anim->knockdownAngle;
    out->isCowering = anim->isCowering;
}

// ============================================================================
// Enemy Bolt Projectile Update + Rendering
// ============================================================================

void EcsEnemyUpdateBolts(ecs_world_t *world, float dt) {
    EcGameContext *ctx = ecs_singleton_ensure(world, EcGameContext);
    if (!ctx) return;
    if (ctx->boltCount < 0 || ctx->boltCount > MAX_ENEMY_BOLTS) ctx->boltCount = 0;
    for (int bi = 0; bi < ctx->boltCount; bi++) {
        float boltDist = Vector3Distance(ctx->boltStart[bi], ctx->boltEnd[bi]);
        if (boltDist > 0.1f)
            ctx->boltProgress[bi] += (ENEMY_BOLT_SPEED * dt) / boltDist;
        else
            ctx->boltProgress[bi] = 1.0f;
        ctx->boltLife[bi] -= dt;
        if (ctx->boltProgress[bi] >= 1.0f || ctx->boltLife[bi] <= 0) {
            ctx->boltCount--;
            if (bi < ctx->boltCount) {
                ctx->boltStart[bi] = ctx->boltStart[ctx->boltCount];
                ctx->boltEnd[bi] = ctx->boltEnd[ctx->boltCount];
                ctx->boltColor[bi] = ctx->boltColor[ctx->boltCount];
                ctx->boltProgress[bi] = ctx->boltProgress[ctx->boltCount];
                ctx->boltLife[bi] = ctx->boltLife[ctx->boltCount];
            }
            bi--;
        }
    }
}

void EcsEnemyDrawBolts(ecs_world_t *world) {
    const EcGameContext *bctx = ecs_singleton_get(world, EcGameContext);
    if (!bctx) return;
    for (int bi = 0; bi < bctx->boltCount; bi++) {
        float p = bctx->boltProgress[bi];
        if (p < 0) p = 0; if (p > 1) p = 1;
        Color bc = bctx->boltColor[bi];
        Vector3 boltPos = Vector3Lerp(bctx->boltStart[bi], bctx->boltEnd[bi], p);
        DrawSphere(boltPos, ENEMY_BOLT_SIZE, (Color){255, 255, 255, 240});
        DrawSphere(boltPos, ENEMY_BOLT_SIZE * 1.5f, (Color){bc.r, bc.g, bc.b, 160});
        float trailP = p - 0.08f;
        if (trailP < 0) trailP = 0;
        Vector3 trailPos = Vector3Lerp(bctx->boltStart[bi], bctx->boltEnd[bi], trailP);
        for (int tl = 0; tl < 3; tl++) {
            float tp = p - (float)(tl + 1) * 0.03f;
            if (tp < 0) tp = 0;
            Vector3 tp1 = Vector3Lerp(bctx->boltStart[bi], bctx->boltEnd[bi], tp);
            float tp2v = p - (float)(tl + 2) * 0.03f;
            if (tp2v < 0) tp2v = 0;
            Vector3 tp2 = Vector3Lerp(bctx->boltStart[bi], bctx->boltEnd[bi], tp2v);
            unsigned char ta = (unsigned char)(180 - tl * 50);
            DrawLine3D(tp1, tp2, (Color){bc.r, bc.g, bc.b, ta});
        }
        DrawLine3D(boltPos, trailPos, (Color){bc.r, bc.g, bc.b, 180});
    }
}

void EcsEnemyManagerDraw(ecs_world_t *world, Camera3D camera, bool testMode) {
    const EcEnemyResources *rp = ecs_get(world, ecs_id(EcEnemyResources), EcEnemyResources);
    if (!rp || !rp->modelsLoaded) return;

    // Build a temporary EnemyManager with just the resource fields
    // so DrawAstronautModel can access mdlVisor, mdlArm, mdlBoot
    EnemyManager tempMgr;
    memset(&tempMgr, 0, sizeof(tempMgr));
    tempMgr.mdlVisor = rp->mdlVisor;
    tempMgr.mdlArm = rp->mdlArm;
    tempMgr.mdlBoot = rp->mdlBoot;
    tempMgr.astroModels = g_astroModelsLoaded && g_astroModels.available ? &g_astroModels : NULL;
    tempMgr.modelsLoaded = true;

    // View-projection matrix for frustum culling (used in testMode)
    Matrix vp;
    if (testMode) {
        Matrix view = GetCameraMatrix(camera);
        Matrix proj = MatrixPerspective(camera.fovy * DEG2RAD,
            (float)GetScreenWidth() / (float)GetScreenHeight(), 0.1f, 1000.0f);
        vp = MatrixMultiply(view, proj);
    }

    // Query all entities with transform + faction + animation (covers alive and dying)
    ecs_query_t *q = ecs_query(world, {
        .terms = {
            { .id = ecs_id(EcTransform) },
            { .id = ecs_id(EcFaction) },
            { .id = ecs_id(EcAnimation) }
        }
    });
    ecs_iter_t it = ecs_query_iter(world, q);
    while (ecs_query_next(&it)) {
        EcTransform *t = ecs_field(&it, EcTransform, 0);
        EcFaction *f = ecs_field(&it, EcFaction, 1);
        EcAnimation *a = ecs_field(&it, EcAnimation, 2);

        for (int i = 0; i < it.count; i++) {
            Vector3 pos = t[i].position;

            if (testMode) {
                // Distance culling
                float dist = Vector3Distance(pos, camera.position);
                if (dist > CULL_DISTANCE) continue;

                // Frustum culling: transform to clip space
                float cx = vp.m0*pos.x + vp.m4*pos.y + vp.m8*pos.z + vp.m12;
                float cy = vp.m1*pos.x + vp.m5*pos.y + vp.m9*pos.z + vp.m13;
                float cw = vp.m3*pos.x + vp.m7*pos.y + vp.m11*pos.z + vp.m15;
                float cz = vp.m2*pos.x + vp.m6*pos.y + vp.m10*pos.z + vp.m14;

                if (cw > 0) {
                    float margin = 3.0f;
                    if (cx < -cw - margin || cx > cw + margin ||
                        cy < -cw - margin || cy > cw + margin ||
                        cz < 0) {
                        continue;
                    }
                }

                // LOD dispatch
                Enemy tempEnemy;
                FillTempEnemy(world, it.entities[i], &t[i], &f[i], &a[i], &tempEnemy);

                if (dist < LOD1_DISTANCE) {
                    DrawAstronautModel(&tempMgr, &tempEnemy);
                } else if (dist < LOD2_DISTANCE) {
                    DrawAstronautLOD1(&tempMgr, &tempEnemy);
                } else {
                    DrawAstronautLOD2(&tempEnemy);
                }
            } else {
                // Non-test mode: draw all at full detail
                Enemy tempEnemy;
                FillTempEnemy(world, it.entities[i], &t[i], &f[i], &a[i], &tempEnemy);
                DrawAstronautModel(&tempMgr, &tempEnemy);
            }
        }
    }
    ecs_query_fini(q);
}
