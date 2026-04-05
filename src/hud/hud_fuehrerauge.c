// Fuehrerauge (Leader Eye) targeting computer viewport.
// Swings in on a metal arm from upper-right, renders zoomed view
// through old-TV shader in a trapezoid brass frame.

#include "hud_fuehrerauge.h"
#include "hud_primitives.h"
#include "config.h"
#include <math.h>

void HudDrawFuehrerauge(Texture2D zoomTex, Shader fishbowlShader, float animProgress, int sw, int sh) {
    if (animProgress <= 0.0f) return;
    float s = (float)sh / (float)RENDER_H;

    // Smoothstep ease
    float t = animProgress;
    float ease = t * t * (3.0f - 2.0f * t);

    // Pivot point: upper-right area of helmet (where arm is bolted)
    float pivotX = (float)sw * 0.85f;
    float pivotY = (float)sh * 0.05f;

    // Target: lens center at screen center when fully deployed
    float targetCX = (float)sw * 0.5f;
    float targetCY = (float)sh * 0.42f;

    // Compute arm length from pivot to screen center
    float dx = pivotX - targetCX;
    float dy = targetCY - pivotY;
    float armLen = sqrtf(dx * dx + dy * dy);

    // Deployed angle: derived from pivot->center vector
    float deployedAngle = atan2f(dx, dy);

    // Stowed: swing 80 degrees further clockwise (toward upper-right, off screen)
    float stowedAngle = deployedAngle - 80.0f * DEG2RAD;
    float angle = stowedAngle + ease * (deployedAngle - stowedAngle);

    // Lens center position on the arc
    float lensCX = pivotX - sinf(angle) * armLen;
    float lensCY = pivotY + cosf(angle) * armLen;

    // Viewport rect
    int vpW = (int)(sw * FUEHRERAUGE_WIDTH_FRAC);
    int vpH = (int)(sh * FUEHRERAUGE_HEIGHT_FRAC);
    int vpX = (int)(lensCX) - vpW / 2;
    int vpY = (int)(lensCY) - vpH / 2;

    int frame = (int)(FUEHRERAUGE_FRAME_THICK * s);
    if (frame < 2) frame = 2;

    // Trapezoid inset: top is wider, bottom is narrower
    int trapInset = (int)(vpW * 0.12f);

    // Trapezoid corner points (outer frame edge)
    float tl_x = (float)(vpX - frame);
    float tl_y = (float)(vpY - frame);
    float tr_x = (float)(vpX + vpW + frame);
    float tr_y = (float)(vpY - frame);
    float bl_x = (float)(vpX - frame + trapInset);
    float bl_y = (float)(vpY + vpH + frame);
    float br_x = (float)(vpX + vpW + frame - trapInset);
    float br_y = (float)(vpY + vpH + frame);

    // ---- Metal arm from pivot to lens ----
    {
        float armThick = 4.0f * s;
        if (armThick < 2.0f) armThick = 2.0f;
        Color armColor = (Color){55, 50, 42, 240};
        Color armEdge = (Color){90, 80, 65, 200};

        DrawLineEx((Vector2){pivotX, pivotY}, (Vector2){lensCX, lensCY}, armThick + 2.0f * s, armEdge);
        DrawLineEx((Vector2){pivotX, pivotY}, (Vector2){lensCX, lensCY}, armThick, armColor);

        // Pivot bolt
        DrawCircle((int)pivotX, (int)pivotY, 5.0f * s, (Color){60, 55, 45, 240});
        DrawCircle((int)pivotX, (int)pivotY, 3.5f * s, (Color){100, 90, 70, 255});
        DrawCircle((int)pivotX, (int)pivotY, 1.5f * s, (Color){70, 65, 55, 255});

        // Bracket near lens end
        float perpXv = cosf(angle) * 6.0f * s;
        float perpYv = sinf(angle) * 6.0f * s;
        float nearLensX = lensCX + sinf(angle) * vpH * 0.35f;
        float nearLensY = lensCY - cosf(angle) * vpH * 0.35f;
        DrawLineEx(
            (Vector2){nearLensX - perpXv, nearLensY - perpYv},
            (Vector2){nearLensX + perpXv, nearLensY + perpYv},
            armThick * 0.7f, armColor);
        HudDrawRivet((int)nearLensX, (int)nearLensY, s * 0.6f);
    }

    // ---- Trapezoid frame (dark metal housing) ----
    Color frameBg = (Color){25, 23, 20, 240};
    // Left side
    DrawTriangle((Vector2){tl_x, tl_y}, (Vector2){bl_x, bl_y}, (Vector2){(float)(vpX), (float)(vpY + vpH)}, frameBg);
    DrawTriangle((Vector2){tl_x, tl_y}, (Vector2){(float)(vpX), (float)(vpY + vpH)}, (Vector2){(float)(vpX), (float)(vpY)}, frameBg);
    // Right side
    DrawTriangle((Vector2){tr_x, tr_y}, (Vector2){(float)(vpX + vpW), (float)(vpY)}, (Vector2){(float)(vpX + vpW), (float)(vpY + vpH)}, frameBg);
    DrawTriangle((Vector2){tr_x, tr_y}, (Vector2){(float)(vpX + vpW), (float)(vpY + vpH)}, (Vector2){br_x, br_y}, frameBg);
    // Top bar
    DrawTriangle((Vector2){tl_x, tl_y}, (Vector2){(float)(vpX), (float)(vpY)}, (Vector2){(float)(vpX + vpW), (float)(vpY)}, frameBg);
    DrawTriangle((Vector2){tl_x, tl_y}, (Vector2){(float)(vpX + vpW), (float)(vpY)}, (Vector2){tr_x, tr_y}, frameBg);
    // Bottom bar
    DrawTriangle((Vector2){bl_x, bl_y}, (Vector2){br_x, br_y}, (Vector2){(float)(vpX + vpW), (float)(vpY + vpH)}, frameBg);
    DrawTriangle((Vector2){bl_x, bl_y}, (Vector2){(float)(vpX + vpW), (float)(vpY + vpH)}, (Vector2){(float)(vpX), (float)(vpY + vpH)}, frameBg);

    // ---- Trapezoid brass border lines ----
    float bthick = 2.5f * s;
    Color brassCol = (Color)COLOR_BRASS;
    DrawLineEx((Vector2){tl_x, tl_y}, (Vector2){tr_x, tr_y}, bthick, brassCol);
    DrawLineEx((Vector2){bl_x, bl_y}, (Vector2){br_x, br_y}, bthick, brassCol);
    DrawLineEx((Vector2){tl_x, tl_y}, (Vector2){bl_x, bl_y}, bthick, brassCol);
    DrawLineEx((Vector2){tr_x, tr_y}, (Vector2){br_x, br_y}, bthick, brassCol);
    // Inner border
    Color brassD = (Color)COLOR_BRASS_DIM;
    float ithick = s;
    DrawLineEx((Vector2){(float)vpX, (float)vpY}, (Vector2){(float)(vpX + vpW), (float)vpY}, ithick, brassD);
    DrawLineEx((Vector2){(float)vpX, (float)(vpY + vpH)}, (Vector2){(float)(vpX + vpW), (float)(vpY + vpH)}, ithick, brassD);
    DrawLineEx((Vector2){(float)vpX, (float)vpY}, (Vector2){(float)vpX, (float)(vpY + vpH)}, ithick, brassD);
    DrawLineEx((Vector2){(float)(vpX + vpW), (float)vpY}, (Vector2){(float)(vpX + vpW), (float)(vpY + vpH)}, ithick, brassD);

    // ---- Corner rivets on trapezoid ----
    HudDrawRivet((int)tl_x, (int)tl_y, s);
    HudDrawRivet((int)tr_x, (int)tr_y, s);
    HudDrawRivet((int)bl_x, (int)bl_y, s);
    HudDrawRivet((int)br_x, (int)br_y, s);
    HudDrawRivet((int)((tl_x + tr_x) * 0.5f), (int)tl_y, s);
    HudDrawRivet((int)((bl_x + br_x) * 0.5f), (int)bl_y, s);

    // ---- Copper accent at top corners ----
    int deco = (int)(10 * s);
    Color decoCol = (Color)COLOR_COPPER;
    DrawLineEx((Vector2){tl_x, tl_y}, (Vector2){tl_x + deco, tl_y}, 2.0f * s, decoCol);
    DrawLineEx((Vector2){tr_x, tr_y}, (Vector2){tr_x - deco, tr_y}, 2.0f * s, decoCol);

    // ---- Zoomed view with old-TV shader ----
    BeginShaderMode(fishbowlShader);
    DrawTexturePro(zoomTex,
        (Rectangle){0, 0, (float)zoomTex.width, -(float)zoomTex.height},
        (Rectangle){(float)vpX, (float)vpY, (float)vpW, (float)vpH},
        (Vector2){0, 0}, 0.0f, WHITE);
    EndShaderMode();

    // ---- Targeting reticle ----
    {
        int rcx = vpX + vpW / 2;
        int rcy = vpY + vpH / 2;
        Color reticle = (Color){240, 200, 100, 180};
        int rLen = (int)(20 * s);
        int rGap = (int)(6 * s);
        float thick = 1.0f * s;
        if (thick < 1.0f) thick = 1.0f;

        DrawLineEx((Vector2){(float)(rcx - rLen), (float)rcy}, (Vector2){(float)(rcx - rGap), (float)rcy}, thick, reticle);
        DrawLineEx((Vector2){(float)(rcx + rGap), (float)rcy}, (Vector2){(float)(rcx + rLen), (float)rcy}, thick, reticle);
        DrawLineEx((Vector2){(float)rcx, (float)(rcy - rLen)}, (Vector2){(float)rcx, (float)(rcy - rGap)}, thick, reticle);
        DrawLineEx((Vector2){(float)rcx, (float)(rcy + rGap)}, (Vector2){(float)rcx, (float)(rcy + rLen)}, thick, reticle);

        DrawCircle(rcx, rcy, 1.5f * s, reticle);
        DrawCircleLines(rcx, rcy, (float)rLen, (Color){200, 170, 80, 100});
    }

    // ---- "FUEHRERAUGE" label ----
    {
        int lfs = (int)(7 * s);
        if (lfs < 5) lfs = 5;
        const char *label = "FUEHRERAUGE";
        int lw = MeasureText(label, lfs);
        int lx = (int)((tl_x + tr_x) * 0.5f) - lw / 2;
        int ly = (int)tl_y + (int)(2 * s);
        DrawRectangle(lx - (int)(3 * s), ly - (int)(1 * s),
                      lw + (int)(6 * s), lfs + (int)(2 * s), (Color){25, 23, 20, 220});
        DrawText(label, lx, ly, lfs, (Color)COLOR_BRASS);
    }
}
