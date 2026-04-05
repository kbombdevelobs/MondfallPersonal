---
title: Retro Dieselpunk HUD & Zoom Optic — Implementation Guide
purpose: Step-by-step reference for an AI agent to replicate this HUD style and zoom mechanic in a similar raylib/C FPS project
last_updated: 2026-04-05
---

# Retro Dieselpunk HUD & Zoom Optic — Implementation Guide

This document explains every technique used to transform a modern HUD into a 1930s dieselpunk aesthetic and add a swing-arm zoom optic ("Führerauge") to a raylib C FPS. Written so another AI agent can reproduce it in a similar project.

---

## 1. Design Theme

**Dieselpunk / 1930s retro-futurism.** Think brass instruments, analog gauges, riveted metal, art deco geometry, old vacuum-tube televisions. The HUD should feel like it's bolted inside a metal helmet, not projected onto glass.

### Color Palette

Replace any modern cyan/neon/digital colors with these warm industrial tones:

```c
#define COLOR_BRASS          {205, 170, 80, 255}   // primary accent
#define COLOR_BRASS_DIM      {160, 130, 60, 200}   // borders, secondary
#define COLOR_BRASS_BRIGHT   {240, 200, 100, 255}  // highlights, active values
#define COLOR_COPPER         {180, 100, 50, 255}    // accent corners
#define COLOR_DARK_METAL     {35, 32, 28, 230}      // backgrounds, housing
#define COLOR_RIVET          {120, 110, 90, 255}    // rivet fill
#define COLOR_GAUGE_BG       {20, 18, 15, 220}      // gauge/ticker backgrounds
#define COLOR_GAUGE_NEEDLE   {220, 60, 30, 255}     // needles, danger indicators
#define COLOR_GAUGE_MARK     {180, 160, 120, 180}   // tick marks
#define COLOR_HUD_TEXT       {200, 180, 130, 255}   // labels
```

**Rule:** No pure white, no cyan, no clean digital colors anywhere in the HUD. Everything is tinted warm.

---

## 2. Resolution-Independent Scaling

All HUD drawing uses a single scale factor derived from screen height:

```c
float s = (float)screenHeight / (float)RENDER_H;  // RENDER_H = 360
```

Every pixel measurement is multiplied by `s`. This ensures the HUD looks identical at 540p, 720p, 1080p, etc. Always clamp minimums (`if (val < 1) val = 1`) to avoid zero-size draws at low resolution.

---

## 3. Reusable Primitives

### 3.1 Rivets

Two concentric circles — lighter outer ring, darker inner fill. Gives a 3D bolt appearance.

```c
static void DrawRivet(int x, int y, float s) {
    int r = (int)(2.5f * s);
    if (r < 1) r = 1;
    DrawCircle(x, y, (float)r, (Color){140, 125, 95, 255});       // outer
    DrawCircle(x, y, (float)(r - 1 > 0 ? r - 1 : 1), (Color){100, 90, 70, 255}); // inner
}
```

Place rivets at: bar corners, midpoints of edges, gauge pivots, frame corners, near any mechanical joint. They sell the "bolted metal" look. Use `s * 0.6f` or `s * 0.7f` for smaller rivets on secondary elements.

### 3.2 Art Deco Bar Frame

The signature element. A dark rectangle with brass border lines and **45-degree corner cuts** instead of sharp right angles. This is what makes it "deco" rather than just "retro."

```c
static void DrawDecoBar(int x, int y, int w, int h, float s) {
    int cut = (int)(8 * s);
    // Dark background
    DrawRectangle(x, y, w, h, (Color){0, 0, 0, 190});
    // Brass border with corner cuts — lines between cut points, not full rectangle
    DrawLine(x + cut, y, x + w - cut, y, COLOR_BRASS_DIM);         // top
    DrawLine(x + cut, y + h, x + w - cut, y + h, COLOR_BRASS_DIM); // bottom
    DrawLine(x, y + cut, x, y + h - cut, COLOR_BRASS_DIM);         // left
    DrawLine(x + w, y + cut, x + w, y + h - cut, COLOR_BRASS_DIM); // right
    // 45-degree diagonal cuts at each corner
    DrawLine(x, y + cut, x + cut, y, COLOR_BRASS_DIM);             // top-left
    DrawLine(x + w - cut, y, x + w, y + cut, COLOR_BRASS_DIM);     // top-right
    DrawLine(x, y + h - cut, x + cut, y + h, COLOR_BRASS_DIM);     // bottom-left
    DrawLine(x + w - cut, y + h, x + w, y + h - cut, COLOR_BRASS_DIM); // bottom-right
    // Rivets at edge midpoints and near corners
    DrawRivet(x + w / 2, y, s);
    DrawRivet(x + w / 2, y + h, s);
    // ... etc
}
```

### 3.3 Iron Cross

Four flared arms drawn as triangle pairs. Each arm is narrower at the center (`armW = size/3`) and wider at the tip (`tipW = size/2`). A small center square fills the junction.

Each arm = 2 triangles forming a trapezoid from center to tip. **Important:** raylib's `DrawTriangle` requires counter-clockwise vertex winding. If a triangle is invisible, reverse the vertex order.

Use at low opacity (`alpha 80-120`) as decorative elements between HUD sections. They're background flavor, not primary UI.

### 3.4 Eagle Wings

After iterating on complex multi-triangle eagles (body, head, tail, talons — all misaligned at low resolution), the solution that actually works is **just the wings** drawn as thick lines:

```c
static void DrawEagle(int cx, int cy, int size, Color col) {
    Color dark = {col.r * 0.5f, col.g * 0.5f, col.b * 0.5f, col.a};
    float S = (float)size;
    float wingSpan = S * 1.8f;
    float wingRise = S * 0.5f;
    float thick = S * 0.14f;
    // Main wing strokes
    DrawLineEx({cx, cy}, {cx - wingSpan, cy - wingRise}, thick, col);
    DrawLineEx({cx, cy}, {cx + wingSpan, cy - wingRise}, thick, col);
    // Trailing edge (thinner, darker) for depth
    DrawLineEx({cx, cy + thick*0.4f}, {cx - wingSpan*0.85f, cy - wingRise + thick*1.2f}, thick*0.5f, dark);
    DrawLineEx({cx, cy + thick*0.4f}, {cx + wingSpan*0.85f, cy - wingRise + thick*1.2f}, thick*0.5f, dark);
    // Center dot
    DrawCircle(cx, cy, thick * 0.6f, col);
}
```

**Lesson learned:** Complex triangle silhouettes look terrible at low resolution (sub-50px). Thick lines read much better. Keep decorative emblems simple.

---

## 4. HUD Elements

### 4.1 Analog Gauge (Health)

Replaces a rectangular health bar with a semicircular dial. This is the centerpiece of the retro feel.

**How it works:**
1. `DrawCircleSector()` for the background arc and three color zones (red 0-25%, yellow 25-50%, green 50-100%)
2. Tick marks at 10% intervals using `DrawLineEx` with trig (`cosf`/`sinf`) to position inner→outer endpoints around the arc
3. A needle line from center to the current value angle
4. A rivet at the pivot point
5. A text label below

The arc sweeps from -30° to +210° (240° total). The needle angle is:
```c
float needleAngle = (-30.0f + 240.0f * value) * DEG2RAD;
```

**Sizing:** Make it bigger than you think. The gauge radius should be at least `topBarHeight + 4*s` — it's fine for it to extend below the bar it's in. Small gauges are unreadable.

### 4.2 Mechanical Tickers (Wave, Kills)

Dark inset rectangle with brass border and text inside — like an odometer or mechanical counter window.

```c
DrawRectangle(x, y, w, h, COLOR_GAUGE_BG);           // dark inset
DrawRectangleLinesEx({x, y, w, h}, s, COLOR_BRASS_DIM); // brass frame
DrawText(value, centered, centered, fontSize, COLOR_BRASS_BRIGHT);
// Label above in smaller text
// Corner rivets
```

### 4.3 Ammo Readout

After trying visual bullet grids (didn't fit in the bar height at low resolution), the solution is a **text readout in a dark inset panel:**

```c
snprintf(ammoText, sizeof(ammoText), "%d / %d  [ %d ]", mag, maxMag, reserve);
// Dark backing rectangle + brass border + brass bright text
```

This is readable at any resolution and fits in a `sh/14` height bar.

### 4.4 Reload Indicator

A circular gauge (same as health but smaller, near crosshair) with a sweeping needle showing reload progress. Label "RELOADING" above in `COLOR_HUD_TEXT`.

### 4.5 Crosshair Behavior

Keep whatever crosshair the game already has. **Hide it** when the zoom optic is active:

```c
if (player->zoomAnim <= 0.0f) {
    // draw crosshair
}
```

---

## 5. Führerauge (Zoom Optic)

### 5.1 Concept

A mechanical zoom lens mounted on a swing arm inside the helmet. Hold RMB → it swings in from the upper-right on a pivot, like an optometrist's phoropter. The zoomed view is rendered through an old-timey TV shader inside a trapezoid brass frame.

### 5.2 Player State

Add two fields to the player struct:

```c
bool fuehreraugeActive;   // is RMB held
float fuehreraugeAnim;    // 0.0 = retracted, 1.0 = fully deployed
```

In the player update, animate toward target:

```c
player->fuehreraugeActive = IsMouseButtonDown(MOUSE_BUTTON_RIGHT);
if (player->fuehreraugeActive) {
    player->fuehreraugeAnim += ANIM_SPEED * dt;  // ANIM_SPEED = 3.0
    if (player->fuehreraugeAnim > 1.0f) player->fuehreraugeAnim = 1.0f;
} else {
    player->fuehreraugeAnim -= ANIM_SPEED * dt;
    if (player->fuehreraugeAnim < 0.0f) player->fuehreraugeAnim = 0.0f;
}
```

### 5.3 Second Render Pass

This is the key architectural piece. When the optic is deploying or deployed (`anim > 0`), render the 3D scene a second time to a smaller RenderTexture at a narrower FOV:

```c
// In main loop, after the primary 3D render:
if (player.fuehreraugeAnim > 0.0f) {
    Camera3D zoomCam = player.camera;
    zoomCam.fovy = 15.0f;  // narrow FOV = high zoom

    BeginTextureMode(zoomTarget);  // 320x240 RenderTexture
    ClearBackground(BLACK);
    BeginMode3D(zoomCam);
        // Draw entire scene EXCEPT first-person weapon
        DrawWorld(...);
        DrawEnemies(...);
        DrawLanders(...);
        // NO first-person weapon model in the zoom view
    EndMode3D();
    EndTextureMode();
}
```

**Setup (once at init):**
```c
RenderTexture2D zoomTarget = LoadRenderTexture(320, 240);
SetTextureFilter(zoomTarget.texture, TEXTURE_FILTER_POINT); // nearest-neighbour
Shader zoomShader = LoadShader(0, "resources/fuehrerauge.fs");
int zoomTimeLoc = GetShaderLocation(zoomShader, "time");
```

**Each frame:** pass the time uniform:
```c
SetShaderValue(zoomShader, zoomTimeLoc, &timeVal, SHADER_UNIFORM_FLOAT);
```

**Performance:** The zoom texture is 320x240 (1/3 of the main 640x360), so the second pass costs roughly 1/3 of the main render. Only active when RMB is held. No first-person weapon drawing in the zoom pass saves additional cost.

### 5.4 Swing Arm Animation

The optic swings on an arc from a pivot point (upper-right of screen) to the deployed position (screen center). This is computed with trigonometry:

```c
// Pivot bolt location
float pivotX = screenW * 0.85f;
float pivotY = screenH * 0.05f;

// Deployed target = screen center (slightly above center for comfort)
float targetCX = screenW * 0.5f;
float targetCY = screenH * 0.42f;

// Arm length and deployed angle computed from pivot→target vector
float dx = pivotX - targetCX;
float dy = targetCY - pivotY;
float armLen = sqrtf(dx * dx + dy * dy);
float deployedAngle = atan2f(dx, dy);

// Stowed angle = 80 degrees further clockwise (off-screen right)
float stowedAngle = deployedAngle - 80.0f * DEG2RAD;

// Current angle interpolated by smoothstepped animation
float ease = t * t * (3.0f - 2.0f * t);  // smoothstep
float angle = stowedAngle + ease * (deployedAngle - stowedAngle);

// Lens center on the arc
float lensCX = pivotX - sinf(angle) * armLen;
float lensCY = pivotY + cosf(angle) * armLen;
```

**Key insight:** Compute the arm length and deployed angle FROM the desired endpoint (screen center), not the other way around. This guarantees the lens lands exactly where you want it.

### 5.5 Drawing the Swing Arm

```c
// Arm: two overlapping thick lines (edge highlight + main bar)
DrawLineEx(pivot, lensCenter, thickness + 2*s, edgeColor);
DrawLineEx(pivot, lensCenter, thickness, armColor);

// Pivot bolt: 3 concentric circles (outer dark, middle brass, inner dark)
DrawCircle(pivotX, pivotY, 5*s, darkMetal);
DrawCircle(pivotX, pivotY, 3.5*s, brass);
DrawCircle(pivotX, pivotY, 1.5*s, darkCenter);

// Small perpendicular bracket near lens end
// Use cosf/sinf of the arm angle to orient perpendicular
```

### 5.6 Trapezoid Frame

The viewport frame is wider at top, narrower at bottom (upside-down trapezoid). The actual texture still draws as a rectangle — the trapezoid is the decorative frame around it, filled with dark metal triangles.

```c
int trapInset = vpW * 0.12f;  // how much narrower bottom is per side

// Four trapezoid corners
tl = (vpX - frame, vpY - frame)               // top-left
tr = (vpX + vpW + frame, vpY - frame)         // top-right
bl = (vpX - frame + trapInset, vpY + vpH + frame)   // bottom-left (inset)
br = (vpX + vpW + frame - trapInset, vpY + vpH + frame) // bottom-right (inset)
```

Fill the frame area with triangles between the trapezoid edge and the rectangular viewport. Draw brass border lines along the four trapezoid edges. Add rivets at corners and midpoints.

### 5.7 Old-Timey TV Shader

The zoom viewport is drawn through a GLSL fragment shader that simulates a 1940s television. Effects in order:

1. **Heavy barrel distortion:** `uv = 0.5 + center * (1.0 + dist2 * 0.8 + dist2^2 * 0.6)` — aggressive fishbowl
2. **Nearest-neighbour pixelation:** `uv = floor(uv * vec2(120, 90) + 0.5) / vec2(120, 90)` — chunky pixel grid
3. **Rolling interference band:** A horizontal noise stripe that scrolls vertically over time (`fract(time * 0.15)`)
4. **Chromatic aberration:** Subtle (`dist2 * 0.002`) — offset R channel outward, B inward
5. **Interlace:** Every other scanline darkened (`mod(floor(y * 360), 2)`)
6. **Scanline beam profile:** Gaussian bright center per scanline (`exp(-dist^2 * 12)`)
7. **Phosphor stripes:** Every 3rd pixel column tinted slightly toward R, G, or B
8. **Static grain:** Hash-based per-pixel noise, animated by time
9. **Warm tint:** Multiply by `vec3(1.03, 0.99, 0.93)` — very mild amber
10. **Heavy vignette:** `1.0 - dist2 * 4.0` — dark edges like a tube TV
11. **Brightness reduction:** `* 0.9` — dim old-TV feel
12. **Flicker:** `0.97 + 0.03 * sin(time * 8)` — subtle brightness oscillation

The shader requires a `uniform float time` updated each frame from `GetTime()`.

### 5.8 Targeting Reticle

Drawn OVER the shaded zoom texture in the HUD layer (not in the shader):

```c
// Thin brass cross with gap at center
DrawLineEx(left, centerGap, thickness, brassColor);
DrawLineEx(centerGap, right, thickness, brassColor);
// ... vertical lines same pattern
DrawCircle(center, 1.5*s, brassColor);         // center dot
DrawCircleLines(center, 20*s, dimBrassColor);   // range circle
```

### 5.9 HUD Compositing Order

The zoom optic is drawn **first** in the HUD texture, before all other HUD elements. This means announcements (rank kills, lander alerts, prompts) naturally layer on top of it:

```c
BeginTextureMode(hudTarget);
ClearBackground(BLANK);
// 1. Zoom optic (behind everything)
if (player.fuehreraugeAnim > 0.0f)
    HudDrawFuehrerauge(zoomTexture, zoomShader, anim, sw, sh);
// 2. Main HUD (crosshair, bars, gauges)
HudDraw(...);
// 3. Overlays (pickups, alerts, radio, rank kills, prompts)
HudDrawPickup(...);
HudDrawLanderArrows(...);
// ... etc
EndTextureMode();
```

---

## 6. Constants Reference

```c
// Zoom optic
#define FUEHRERAUGE_FOV          15.0f   // very high zoom (normal = 75)
#define FUEHRERAUGE_ANIM_SPEED   3.0f    // ~0.33s deploy/retract
#define FUEHRERAUGE_WIDTH_FRAC   0.35f   // viewport = 35% of screen width
#define FUEHRERAUGE_HEIGHT_FRAC  0.40f   // viewport = 40% of screen height
#define FUEHRERAUGE_RENDER_W     320     // zoom render texture width
#define FUEHRERAUGE_RENDER_H     240     // zoom render texture height
#define FUEHRERAUGE_FRAME_THICK  6.0f    // brass frame thickness
```

---

## 7. Files Modified / Created

| File | Role |
|------|------|
| `src/config.h` | Color palette defines + zoom optic constants |
| `src/player.h` | Two new fields: `fuehreraugeActive`, `fuehreraugeAnim` |
| `src/player.c` | RMB input detection + animation interpolation in PlayerUpdate |
| `src/hud.h` | New function signature: `HudDrawFuehrerauge()` |
| `src/hud.c` | All HUD drawing: helpers (rivet, deco bar, gauge, ticker, iron cross, eagle), main HUD layout, zoom optic viewport |
| `src/main.c` | Zoom RenderTexture + shader init, second 3D render pass, time uniform, compositing order, cleanup |
| `resources/fuehrerauge.fs` | Old-TV fragment shader (GLSL 330) |

---

## 8. Lessons Learned

1. **Bullet grids don't fit in thin bars.** At `sh/14` bar height (~38px at 540p), even 2-row bullet layouts are cramped. Use text readouts with dark inset backing instead.

2. **Complex triangle silhouettes fail at low res.** An eagle with body/head/tail/talons looked like a mess of misaligned triangles at 40px. Thick `DrawLineEx` strokes read much better for small decorative emblems.

3. **Compute swing-arm geometry from the endpoint.** Don't guess the arm length and deployed angle — derive them from `pivot → desired_center` using `atan2f` and `sqrtf`. The lens lands exactly at screen center every time.

4. **Draw the zoom optic FIRST in the HUD layer.** Announcements (rank kills, lander alerts) must appear on top. Drawing order = z-order in 2D.

5. **Hide the main crosshair during zoom.** Having both a crosshair and a zoom reticle is confusing. Check `anim <= 0` before drawing the crosshair.

6. **Keep shader color distortion subtle on the zoom view.** Heavy chromatic aberration and phosphor stripes make it hard to actually use for aiming. Pixelation + scanlines + fishbowl are the main visual effects; color should stay mostly faithful.

7. **The second render pass is cheap if the texture is small.** 320x240 is ~1/3 the pixels of 640x360. Skip the first-person weapon in the zoom pass for additional savings. Only render when `anim > 0`.

8. **Smoothstep easing** (`t*t*(3-2t)`) gives mechanical animation a natural feel — fast in the middle, slow at start and end. Perfect for a metal arm swinging into position.
