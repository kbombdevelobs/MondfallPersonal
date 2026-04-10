#include "enemy_draw.h"
#include "enemy_draw_death.h"
#include "enemy_bodydef.h"
#include "enemy_model_loader.h"
#include "enemy_model_bones.h"
#include "config.h"
#include "world.h"
#include "rlgl.h"
#include <math.h>
#include <stddef.h>

static Color FlashColor(Color c, float hf) {
    if (hf <= 0) return c;
    int r = c.r + (int)(hf * 140); if (r > 255) r = 255;
    int g = c.g + (int)(hf * 70);  if (g > 255) g = 255;
    int b = c.b + (int)(hf * 56);  if (b > 255) b = 255;
    return (Color){(unsigned char)r, (unsigned char)g, (unsigned char)b, c.a};
}


void DrawAstronautModel(EnemyManager *em, Enemy *e) {
    /* Vaporize always uses procedural path */
    if (e->state == ENEMY_VAPORIZING) { DrawAstronautVaporize(e, em); return; }

    /* === TRY SKELETAL MODEL PATH === */
    if (em->astroModels) {
        int fi = (int)e->type;
        int ri = (int)e->rank;
        if (fi >= 0 && fi < 2 && ri >= 0 && ri < 3) {
            /* --- EVISCERATE: use dismemberment models if available --- */
            if (e->state == ENEMY_EVISCERATING) {
                AstroModelSet *ms = em->astroModels;
                if (ms->dismember[fi][DISMEMBER_HEAD].loaded) {
                    DrawAstronautEviscerateSkeletal(e, em);
                    return;
                }
                /* fallback to procedural */
                DrawAstronautEviscerate(e, em);
                return;
            }

            /* --- DECAPITATE: use headshot model if available --- */
            if (e->state == ENEMY_DECAPITATING) {
                AstroModelSet *ms = em->astroModels;
                if (ms->headshot[fi][ri].loaded && ms->characters[fi][ri].loaded) {
                    DrawAstronautDecapitateSkeletal(e, em);
                    return;
                }
                /* Fallback to procedural decapitate */
                DrawAstronautDecapitate(e);
                return;
            }
            AstroModel *am = &em->astroModels->characters[fi][ri];
            if (am->loaded && e->hasLimbState) {
                const AnimProfile *ap = AnimProfileGet(e->type);
                float hf = (e->state == ENEMY_ALIVE && e->hitFlash > 0) ? e->hitFlash : 0;

                /* Apply spring-damper state to skeleton bones */
                {
                    bool dying = (e->state == ENEMY_DYING);
                    float dTime = dying ? (10.0f - e->deathTimer) : 0;
                    AstroModelApplySpringState(am, &e->limbState, ap, hf,
                                               e->knockdownAngle > 0 ? 1.0f : 0,
                                               e->knockdownAngle,
                                               e->isCowering, dying, dTime);
                }

                /* Position and orient the model (flat world) */
                rlPushMatrix();
                rlTranslatef(e->position.x, e->position.y, e->position.z);
                rlRotatef(e->facingAngle * RAD2DEG, 0, 1, 0);
                rlRotatef(180.0f, 0, 1, 0); /* Blender face fix */

                /* Hit stagger jerk */
                if (e->state == ENEMY_ALIVE && e->hitFlash > 0) {
                    float f = e->hitFlash;
                    float s = e->facingAngle * 5.17f;
                    rlRotatef(-f * 20.0f, 1, 0, 0);
                    rlRotatef(sinf(s) * f * 15.0f, 0, 0, 1);
                    rlRotatef(cosf(s * 1.3f) * f * 10.0f, 0, 1, 0);
                }

                /* Dying: apply death rotation with per-enemy random topple */
                if (e->state == ENEMY_DYING) {
                    rlRotatef(e->deathAngle, 1, 0, 0);
                    rlRotatef(e->spinX * 0.02f + e->spinZ * 0.5f, 0, 0, 1);
                    rlRotatef(e->spinZ * 0.5f, 0, 1, 0);
                }

                Color tint = (hf > 0) ? FlashColor(WHITE, hf) : WHITE;
                DrawModel(am->model, (Vector3){0, 0, 0}, 1.0f, tint);

                /* Draw gun attached to hand_R (alive only) */
                if (e->state == ENEMY_ALIVE) {
                    AstroModel *gun = AstroModelGetGun(em->astroModels, fi, ri);
                    if (gun) {
                        Vector3 handPos = AstroModelGetBonePosition(am, BONE_HAND_R);
                        rlPushMatrix();
                        rlTranslatef(handPos.x, handPos.y, handPos.z);
                        rlRotatef(-90.0f, 1, 0, 0);
                        DrawModel(gun->model, (Vector3){0, 0, 0}, 1.0f, WHITE);
                        rlPopMatrix();
                    }
                }

                rlPopMatrix();

                /* Death particles for dying state */
                if (e->state == ENEMY_DYING)
                    DrawAstronautRagdoll(e);

                return;
            }
        }
    }

    /* Eviscerate fallback when no skeletal models available */
    if (e->state == ENEMY_EVISCERATING) { DrawAstronautEviscerate(e, em); return; }

    /* === PROCEDURAL FALLBACK PATH === */

    /* Body definitions */
    const BodyDef    *bd  = BodyDefGetAstronaut();
    const AnimProfile *ap = AnimProfileGet(e->type);
    const BodyColors *pal = BodyColorsGet(e->type);
    const GunDef     *gun = GunDefGet(e->type);
    const RankOverlayDef *rkOv = RankOverlayGet(e->type, e->rank);
    Model models[MDL_COUNT] = { em->mdlVisor, em->mdlArm, em->mdlBoot };
    int fac = (int)e->type;
    const EcLimbState *ls = e->hasLimbState ? &e->limbState : NULL;
    bool alive = (e->state == ENEMY_ALIVE);
    bool useSprings = alive;

    /* Hit flash amount */
    float hf = (alive && e->hitFlash > 0) ? e->hitFlash : 0;

    /* === ROOT BASIS === */
    rlPushMatrix();
    rlTranslatef(e->position.x, e->position.y, e->position.z);
    rlRotatef(e->facingAngle * RAD2DEG, 0, 1, 0);

    /* Hit stagger jerk */
    if (alive && e->hitFlash > 0) {
        float f = e->hitFlash;
        float s = e->facingAngle * 5.17f;
        rlRotatef(-f * 20.0f, 1, 0, 0);
        rlRotatef(sinf(s) * f * 15.0f, 0, 0, 1);
        rlRotatef(cosf(s * 1.3f) * f * 10.0f, 0, 1, 0);
    }

    /* Knockdown sprawl or cowering crouch */
    if (alive && e->knockdownAngle > 1.0f) {
        if (e->isCowering) {
            /* Cowering: pitch FORWARD (negative X rotation = face-down) */
            rlRotatef(-e->knockdownAngle, 1, 0, 0);
            /* Visible fear trembling — whole body shakes */
            float ks = e->facingAngle * 3.71f;
            float t  = (float)GetTime();
            rlRotatef(sinf(t * 8.0f + ks) * 5.0f, 0, 0, 1);       // lateral shudder
            rlRotatef(cosf(t * 6.0f + ks * 1.7f) * 4.0f, 0, 1, 0); // yaw tremble
            rlRotatef(sinf(t * 10.0f + ks * 2.3f) * 3.0f, 1, 0, 0); // pitch quiver
        } else {
            rlRotatef(-e->knockdownAngle, 1, 0, 0);
            float ks = e->facingAngle * 3.71f;
            float t  = (float)GetTime();
            float sp = e->knockdownAngle / 80.0f;
            float chaos = sp * sp;
            float rollAmp = 18.0f + 22.0f * chaos;
            float yawAmp  = 12.0f + 18.0f * chaos;
            float rollFreq = 1.8f + 4.0f * chaos;
            float yawFreq  = 1.3f + 3.0f * chaos;
            rlRotatef(sinf(t * (rollFreq + sinf(ks) * 0.6f)) * rollAmp * sp, 0, 0, 1);
            rlRotatef(cosf(t * (yawFreq + cosf(ks * 1.7f) * 0.4f)) * yawAmp * sp, 0, 1, 0);
            rlRotatef(sinf(t * 7.5f + ks * 2.1f) * 8.0f * chaos, 1, 0, 0);
        }
    }

    /* Dying rotation with per-enemy random topple */
    if (e->state == ENEMY_DYING) {
        rlRotatef(e->deathAngle, 1, 0, 0);
        rlRotatef(e->spinX * 0.02f + e->spinZ * 0.5f, 0, 0, 1);
        rlRotatef(e->spinZ * 0.5f, 0, 1, 0);
    }

    /* Spring-driven secondary motion (alive + springs) */
    if (useSprings && ls) {
        rlRotatef(-ls->torsoLean.angle, 1, 0, 0);
        rlRotatef(ls->torsoTwist.angle, 0, 1, 0);
        rlTranslatef(ls->hipSway.angle, 0, 0);
        /* Idle breathing */
        float bsc = 1.0f + sinf(ls->breathPhase) * ap->breathScale;
        rlScalef(1.0f, bsc, 1.0f);
    }

    /* === TORSO === */
    DrawPartGroup(&bd->torso, pal, models, hf, fac);

    /* === HEAD (with bob) === */
    rlPushMatrix();
    rlTranslatef(0, bd->headPivotY, 0);
    if (useSprings && ls) {
        rlRotatef(ls->headPitch.angle, 1, 0, 0);
        rlRotatef(ls->headYaw.angle, 0, 1, 0);
    }
    DrawPartGroup(&bd->head, pal, models, hf, fac);
    rlPopMatrix();

    /* === BACKPACK === */
    DrawPartGroup(&bd->backpack, pal, models, hf, fac);

    /* === RIGHT ARM + GUN IN HAND === */
    {
        float armSwingR = 0;  // track arm swing for elbow bend
        rlPushMatrix();
        rlTranslatef(bd->armPivot.x, bd->armPivot.y, bd->armPivot.z);
        if (useSprings && ls) {
            armSwingR = ls->armSwingR.angle;
            rlRotatef(ls->armSwingR.angle, 1, 0, 0);
            rlRotatef(ls->armSpreadR.angle, 0, 0, 1);
        } else if (e->state == ENEMY_DYING) {
            float elapsed = DEATH_BODY_PERSIST_TIME - e->deathTimer;
            if (elapsed < 0) elapsed = 0;
            float t_ = (float)GetTime();
            float ks_ = e->facingAngle * 2.31f;
            float seed1 = e->facingAngle * 17.3f - floorf(e->facingAngle * 17.3f);
            float flailFade = (elapsed < 1.0f) ? 1.0f : (elapsed < 3.0f ? (3.0f - elapsed) / 2.0f : 0.0f);
            float flailSwing = sinf(t_ * 4.0f + ks_) * 30.0f * flailFade;
            float flailSpread = cosf(t_ * 3.0f + ks_) * 20.0f * flailFade;
            float limpSwing = 5.0f + seed1 * 10.0f;
            float limpSpread = 25.0f + seed1 * 20.0f;
            armSwingR = limpSwing + flailSwing;
            rlRotatef(limpSwing + flailSwing, 1, 0, 0);
            rlRotatef(limpSpread + flailSpread, 0, 0, 1);
        } else {
            float kd_ = (e->knockdownAngle > 1.0f) ? e->knockdownAngle / 80.0f : 0.0f;
            float nA = sinf(e->walkCycle) * ap->armSwingDeg;
            float t_ = (float)GetTime();
            float ks_ = e->facingAngle * 2.31f;
            float rA = sinf(t_*0.7f+ks_)*40.0f-60.0f;
            float rS = cosf(t_*0.5f+ks_)*20.0f-25.0f;
            float wd = 1.0f - e->shootAnim * 0.7f;
            float bPitch = ap->armBasePitch + gun->restPitchOffset;
            float sw = (bPitch - nA*wd + e->shootAnim*22.0f)*(1.0f-kd_) + rA*kd_;
            float sp_ = ap->armBaseSpread*(1.0f-kd_) + rS*kd_;
            armSwingR = sw;
            rlRotatef(sw, 1, 0, 0);
            rlRotatef(sp_, 0, 0, 1);
        }

        /* Upper arm — draw arm model only (part 0) */
        {
            Color armCol = BodyColorResolve(pal, bd->armParts.parts[0].color,
                bd->armParts.parts[0].fixedColor, bd->armParts.parts[0].dim);
            if (hf > 0) armCol = FlashColor(armCol, hf);
            if (models && bd->armParts.parts[0].modelId >= 0)
                DrawModel(models[bd->armParts.parts[0].modelId],
                    bd->armParts.parts[0].pos, 1.0f, armCol);
        }

        /* Forearm with elbow bend */
        {
            Color suitCol = BodyColorResolve(pal, COL_SUIT, (Color){0,0,0,0}, false);
            if (hf > 0) suitCol = FlashColor(suitCol, hf);
            Color gloveCol = BodyColorResolve(pal, COL_GLOVE, (Color){0,0,0,0}, false);
            if (hf > 0) gloveCol = FlashColor(gloveCol, hf);

            rlPushMatrix();
            rlTranslatef(0, -0.4f, 0);  // elbow position
            float elbowBend = 0;
            if (alive) {
                // Elbow bends more when arm swings back, straightens when forward
                elbowBend = (armSwingR < 0) ? armSwingR * 0.6f : armSwingR * 0.2f;
                elbowBend -= e->shootAnim * 15.0f;  // forearm raises when shooting
            }
            rlRotatef(elbowBend, 1, 0, 0);
            DrawCube((Vector3){0, -0.15f, 0}, 0.20f, 0.3f, 0.20f, suitCol);  // forearm
            DrawCube((Vector3){0, -0.32f, 0}, 0.24f, 0.16f, 0.24f, gloveCol); // glove
            rlPopMatrix();
        }

        /* Gun held in right hand */
        {
            float localAim = e->shootAnim * -8.0f;
            rlPushMatrix();
            rlTranslatef(bd->gunHandOffset.x, bd->gunHandOffset.y, bd->gunHandOffset.z);
            rlRotatef(localAim, 1, 0, 0);
            DrawPartGroup(&gun->parts, pal, models, hf, fac);
            if (e->muzzleFlash > 0 && alive) {
                float mz = gun->muzzleZ;
                Color fc = (e->type == ENEMY_SOVIET)
                    ? (Color){255,180,50,(unsigned char)(e->muzzleFlash*230)}
                    : (Color){80,180,255,(unsigned char)(e->muzzleFlash*230)};
                DrawSphere((Vector3){0,0,mz}, e->muzzleFlash*0.35f, fc);
                Color glow = fc; glow.a = (unsigned char)(e->muzzleFlash * 120);
                DrawSphere((Vector3){0,0,mz+0.1f}, e->muzzleFlash*0.55f, glow);
            }
            rlPopMatrix();
        }
        rlPopMatrix();
    }

    /* === LEFT ARM (converges toward gun when shooting) === */
    {
        float armSwingL = 0;  // track arm swing for elbow bend
        rlPushMatrix();
        rlTranslatef(-bd->armPivot.x, bd->armPivot.y, bd->armPivot.z);
        if (useSprings && ls) {
            armSwingL = ls->armSwingL.angle;
            rlRotatef(ls->armSwingL.angle, 1, 0, 0);
            rlRotatef(ls->armSpreadL.angle, 0, 0, 1);
        } else if (e->state == ENEMY_DYING) {
            float elapsed = DEATH_BODY_PERSIST_TIME - e->deathTimer;
            if (elapsed < 0) elapsed = 0;
            float t_ = (float)GetTime();
            float ks_ = e->facingAngle * 2.31f;
            float seed1 = e->facingAngle * 13.7f - floorf(e->facingAngle * 13.7f);
            float flailFade = (elapsed < 1.0f) ? 1.0f : (elapsed < 3.0f ? (3.0f - elapsed) / 2.0f : 0.0f);
            float flailSwing = sinf(t_ * 3.5f + ks_ * 1.3f) * 30.0f * flailFade;
            float flailSpread = cosf(t_ * 2.8f + ks_ * 1.7f) * 20.0f * flailFade;
            float limpSwing = 8.0f + seed1 * 10.0f;
            float limpSpread = -(25.0f + seed1 * 20.0f);
            armSwingL = limpSwing + flailSwing;
            rlRotatef(limpSwing + flailSwing, 1, 0, 0);
            rlRotatef(limpSpread + flailSpread, 0, 0, 1);
        } else {
            float kd_ = (e->knockdownAngle > 1.0f) ? e->knockdownAngle / 80.0f : 0.0f;
            float nA = sinf(e->walkCycle) * ap->armSwingDeg;
            float t_ = (float)GetTime();
            float ks_ = e->facingAngle * 2.31f;
            float rA = sinf(t_*0.9f+ks_*1.3f)*35.0f-55.0f;
            float rS = cosf(t_*0.6f+ks_*1.7f)*20.0f+25.0f;
            float wd = 1.0f - e->shootAnim * 0.7f;
            float bPitch = ap->armBasePitch + gun->restPitchOffset;
            float sw = (bPitch + nA*wd + e->shootAnim*18.0f)*(1.0f-kd_) + rA*kd_;
            float sp_ = (-ap->armBaseSpread*(1.0f-e->shootAnim*0.5f) + e->shootAnim*(-5.0f))*(1.0f-kd_) + rS*kd_;
            armSwingL = sw;
            rlRotatef(sw, 1, 0, 0);
            rlRotatef(sp_, 0, 0, 1);
        }

        /* Upper arm — draw arm model only (part 0) */
        {
            Color armCol = BodyColorResolve(pal, bd->armParts.parts[0].color,
                bd->armParts.parts[0].fixedColor, bd->armParts.parts[0].dim);
            if (hf > 0) armCol = FlashColor(armCol, hf);
            if (models && bd->armParts.parts[0].modelId >= 0)
                DrawModel(models[bd->armParts.parts[0].modelId],
                    bd->armParts.parts[0].pos, 1.0f, armCol);
        }

        /* Forearm with elbow bend */
        {
            Color suitCol = BodyColorResolve(pal, COL_SUIT, (Color){0,0,0,0}, false);
            if (hf > 0) suitCol = FlashColor(suitCol, hf);
            Color gloveCol = BodyColorResolve(pal, COL_GLOVE, (Color){0,0,0,0}, false);
            if (hf > 0) gloveCol = FlashColor(gloveCol, hf);

            rlPushMatrix();
            rlTranslatef(0, -0.4f, 0);  // elbow position
            float elbowBend = 0;
            if (alive) {
                elbowBend = (armSwingL < 0) ? armSwingL * 0.6f : armSwingL * 0.2f;
                elbowBend -= e->shootAnim * 15.0f;
            }
            rlRotatef(elbowBend, 1, 0, 0);
            DrawCube((Vector3){0, -0.15f, 0}, 0.20f, 0.3f, 0.20f, suitCol);  // forearm
            DrawCube((Vector3){0, -0.32f, 0}, 0.24f, 0.16f, 0.24f, gloveCol); // glove
            rlPopMatrix();
        }

        rlPopMatrix();
    }

    /* === LEGS === */
    {
        float kd = (e->knockdownAngle > 1.0f) ? e->knockdownAngle / 80.0f : 0.0f;
        float nL = sinf(e->walkCycle) * ap->legSwingDeg;
        float t_ = (float)GetTime();
        float ks_ = e->facingAngle * 2.31f;

        /* Right leg */
        float legR, legSprR, knR;
        if (useSprings && ls) {
            legR = ls->legSwingR.angle;
            legSprR = ls->legSpreadR.angle;
            knR = ls->kneeR.angle;
        } else {
            float rL = sinf(t_*0.6f+ks_*0.7f)*25.0f+15.0f;
            float rS = cosf(t_*0.4f+ks_)*10.0f+8.0f;
            legR = nL*(1.0f-kd) + rL*kd;
            legSprR = rS*kd;
            float fxR = (legR < 0) ? fabsf(legR)*ap->kneeBendFactor : fabsf(legR)*ap->kneeBendFactor*0.3f;
            knR = (fxR - ap->kneeFlexBase) * (1.0f-kd);
            if (e->state == ENEMY_DYING && e->deathTimer < 5.0f) {
                float lb = 1.0f - (e->deathTimer > 0 ? e->deathTimer / 5.0f : 0.0f);
                float limpLeg = 5.0f + (e->facingAngle * 11.3f - floorf(e->facingAngle * 11.3f)) * 15.0f;
                float limpSpr = 8.0f + (e->facingAngle * 5.7f - floorf(e->facingAngle * 5.7f)) * 12.0f;
                float limpKnee = -(15.0f + (e->facingAngle * 3.1f - floorf(e->facingAngle * 3.1f)) * 20.0f);
                legR = legR * (1.0f - lb) + limpLeg * lb;
                legSprR = legSprR * (1.0f - lb) + limpSpr * lb;
                knR = knR * (1.0f - lb) + limpKnee * lb;
            }
        }
        rlPushMatrix();
        rlTranslatef(bd->legPivot.x, bd->legPivot.y, bd->legPivot.z);
        rlRotatef(legR, 1, 0, 0);
        rlRotatef(legSprR, 0, 0, 1);
        DrawPartGroup(&bd->legUpper, pal, models, hf, fac);
        rlPushMatrix();
        rlTranslatef(bd->kneePivot.x, bd->kneePivot.y, bd->kneePivot.z);
        rlRotatef(-knR, 1, 0, 0);  // negate: positive knee = flex backward
        DrawPartGroup(&bd->legLower, pal, models, hf, fac);
        rlPopMatrix();
        rlPopMatrix();

        /* Left leg */
        float legL, legSprL, knL;
        if (useSprings && ls) {
            legL = ls->legSwingL.angle;
            legSprL = ls->legSpreadL.angle;
            knL = ls->kneeL.angle;
        } else {
            float rL = sinf(t_*0.8f+ks_*1.1f)*25.0f-15.0f;
            float rS = cosf(t_*0.5f+ks_*1.4f)*10.0f-8.0f;
            legL = (-nL)*(1.0f-kd) + rL*kd;
            legSprL = rS*kd;
            float fxL = (legL < 0) ? fabsf(legL)*ap->kneeBendFactor : fabsf(legL)*ap->kneeBendFactor*0.3f;
            knL = (fxL - ap->kneeFlexBase) * (1.0f-kd);
            if (e->state == ENEMY_DYING && e->deathTimer < 5.0f) {
                float lb = 1.0f - (e->deathTimer > 0 ? e->deathTimer / 5.0f : 0.0f);
                float limpLeg = 5.0f + (e->facingAngle * 14.1f - floorf(e->facingAngle * 14.1f)) * 15.0f;
                float limpSpr = -(8.0f + (e->facingAngle * 8.9f - floorf(e->facingAngle * 8.9f)) * 12.0f);
                float limpKnee = -(15.0f + (e->facingAngle * 6.3f - floorf(e->facingAngle * 6.3f)) * 20.0f);
                legL = legL * (1.0f - lb) + limpLeg * lb;
                legSprL = legSprL * (1.0f - lb) + limpSpr * lb;
                knL = knL * (1.0f - lb) + limpKnee * lb;
            }
        }
        rlPushMatrix();
        rlTranslatef(-bd->legPivot.x, bd->legPivot.y, bd->legPivot.z);
        rlRotatef(legL, 1, 0, 0);
        rlRotatef(legSprL, 0, 0, 1);
        DrawPartGroup(&bd->legUpper, pal, models, hf, fac);
        rlPushMatrix();
        rlTranslatef(bd->kneePivot.x, bd->kneePivot.y, bd->kneePivot.z);
        rlRotatef(-knL, 1, 0, 0);  // negate: positive knee = flex backward
        DrawPartGroup(&bd->legLower, pal, models, hf, fac);
        rlPopMatrix();
        rlPopMatrix();
    }

    /* === RANK OVERLAY === */
    if (rkOv) {
        if (rkOv->parts.count > 0)
            DrawPartGroup(&rkOv->parts, pal, models, hf, fac);

        if (rkOv->hasSamBrowne) {
            Color sc = FlashColor(rkOv->strapColor, hf);
            rlPushMatrix(); rlRotatef(35.0f, 0, 0, 1);
            DrawCube((Vector3){0.15f, 0.2f, 0.28f}, 0.08f, 1.0f, 0.04f, sc);
            rlPopMatrix();
            rlPushMatrix(); rlRotatef(-35.0f, 0, 0, 1);
            DrawCube((Vector3){-0.15f, 0.2f, 0.28f}, 0.08f, 1.0f, 0.04f, sc);
            rlPopMatrix();
        }
        if (rkOv->hasCoatTail) {
            Color cc = FlashColor(rkOv->coatColor, hf);
            Color ct = FlashColor(rkOv->coatTrimColor, hf);
            float sway = sinf(e->walkCycle * 0.5f) * 5.0f;
            rlPushMatrix();
            rlTranslatef(0, -0.3f, -0.35f);
            rlRotatef(sway, 1, 0, 0);
            DrawCube((Vector3){0, -0.35f, 0}, 0.55f, 0.8f, 0.1f, cc);
            DrawCube((Vector3){0, -0.35f, 0.01f}, 0.45f, 0.7f, 0.01f, ct);
            rlPopMatrix();
        }
        if (rkOv->hasEpaulettes) {
            Color ec = FlashColor(rkOv->epauletteColor, hf);
            DrawCube((Vector3){0.52f, 0.72f, 0.1f}, 0.2f, 0.06f, 0.2f, ec);
            DrawCube((Vector3){-0.52f, 0.72f, 0.1f}, 0.2f, 0.06f, 0.2f, ec);
        }
    }

    rlPopMatrix(); /* root */

    /* Death particles */
    if (e->state == ENEMY_DYING)
        DrawAstronautRagdoll(e);
}

/* === LOD 1: Simplified astronaut with springs (~15 draw calls) === */
void DrawAstronautLOD1(EnemyManager *em, Enemy *e) {
    (void)em; /* reserved for future skeletal LOD1 path */
    const BodyDef    *bd  = BodyDefGetAstronaut();
    const AnimProfile *ap = AnimProfileGet(e->type);
    const BodyColors *pal = BodyColorsGet(e->type);
    int fac = (int)e->type;
    const EcLimbState *ls = e->hasLimbState ? &e->limbState : NULL;
    bool alive = (e->state == ENEMY_ALIVE);
    float hf = (alive && e->hitFlash > 0) ? e->hitFlash : 0;

    rlPushMatrix();
    rlTranslatef(e->position.x, e->position.y, e->position.z);
    rlRotatef(e->facingAngle * RAD2DEG, 0, 1, 0);
    if (e->state == ENEMY_DYING)
        rlRotatef(e->deathAngle, 1, 0, 0);

    /* Spring-driven secondary motion */
    if (alive && ls) {
        rlRotatef(-ls->torsoLean.angle, 1, 0, 0);
        rlRotatef(ls->torsoTwist.angle, 0, 1, 0);
        rlTranslatef(ls->hipSway.angle, 0, 0);
    }

    /* Torso (skip wireframes via DrawPartGroupLOD) */
    DrawPartGroupLOD(&bd->torso, pal, NULL, hf, fac);

    /* Head with bob */
    rlPushMatrix();
    rlTranslatef(0, bd->headPivotY, 0);
    if (alive && ls) {
        rlRotatef(ls->headPitch.angle, 1, 0, 0);
        rlRotatef(ls->headYaw.angle, 0, 1, 0);
    }
    DrawPartGroupLOD(&bd->head, pal, NULL, hf, fac);
    rlPopMatrix();

    /* Right arm */
    rlPushMatrix();
    rlTranslatef(bd->armPivot.x, bd->armPivot.y, bd->armPivot.z);
    if (alive && ls) {
        rlRotatef(ls->armSwingR.angle, 1, 0, 0);
        rlRotatef(ls->armSpreadR.angle, 0, 0, 1);
    } else {
        float bP = ap->armBasePitch;
        rlRotatef(bP - sinf(e->walkCycle)*ap->armSwingDeg + e->shootAnim*22.0f, 1, 0, 0);
        rlRotatef(ap->armBaseSpread, 0, 0, 1);
    }
    DrawPartGroupLOD(&bd->armParts, pal, NULL, hf, fac);
    rlPopMatrix();

    /* Left arm */
    rlPushMatrix();
    rlTranslatef(-bd->armPivot.x, bd->armPivot.y, bd->armPivot.z);
    if (alive && ls) {
        rlRotatef(ls->armSwingL.angle, 1, 0, 0);
        rlRotatef(ls->armSpreadL.angle, 0, 0, 1);
    } else {
        float bP = ap->armBasePitch;
        rlRotatef(bP + sinf(e->walkCycle)*ap->armSwingDeg + e->shootAnim*18.0f, 1, 0, 0);
        rlRotatef(-ap->armBaseSpread, 0, 0, 1);
    }
    DrawPartGroupLOD(&bd->armParts, pal, NULL, hf, fac);
    rlPopMatrix();

    /* Right leg */
    {
        float legR, knR;
        if (alive && ls) {
            legR = ls->legSwingR.angle;
            knR  = ls->kneeR.angle;
        } else {
            legR = sinf(e->walkCycle) * ap->legSwingDeg;
            knR  = fabsf(legR) * ap->kneeBendFactor;
        }
        rlPushMatrix();
        rlTranslatef(bd->legPivot.x, bd->legPivot.y, bd->legPivot.z);
        rlRotatef(legR, 1, 0, 0);
        DrawPartGroupLOD(&bd->legUpper, pal, NULL, hf, fac);
        rlPushMatrix();
        rlTranslatef(bd->kneePivot.x, bd->kneePivot.y, bd->kneePivot.z);
        rlRotatef(-knR, 1, 0, 0);  // negate: positive knee = flex backward
        DrawPartGroupLOD(&bd->legLower, pal, NULL, hf, fac);
        rlPopMatrix();
        rlPopMatrix();
    }

    /* Left leg */
    {
        float legL, knL;
        if (alive && ls) {
            legL = ls->legSwingL.angle;
            knL  = ls->kneeL.angle;
        } else {
            legL = -sinf(e->walkCycle) * ap->legSwingDeg;
            knL  = fabsf(legL) * ap->kneeBendFactor;
        }
        rlPushMatrix();
        rlTranslatef(-bd->legPivot.x, bd->legPivot.y, bd->legPivot.z);
        rlRotatef(legL, 1, 0, 0);
        DrawPartGroupLOD(&bd->legUpper, pal, NULL, hf, fac);
        rlPushMatrix();
        rlTranslatef(bd->kneePivot.x, bd->kneePivot.y, bd->kneePivot.z);
        rlRotatef(-knL, 1, 0, 0);  // negate: positive knee = flex backward
        DrawPartGroupLOD(&bd->legLower, pal, NULL, hf, fac);
        rlPopMatrix();
        rlPopMatrix();
    }

    rlPopMatrix();
}

/* === LOD 2: Single colored cube (1 draw call) === */
void DrawAstronautLOD2(Enemy *e) {
    Color col = BodyColorsGet(e->type)->suit;
    rlPushMatrix();
    rlTranslatef(e->position.x, e->position.y, e->position.z);
    rlRotatef(e->facingAngle * RAD2DEG, 0, 1, 0);
    DrawCube((Vector3){0, 0, 0}, 0.9f, 2.5f, 0.55f, col);
    rlPopMatrix();
}
