#ifndef HUD_H
#define HUD_H

#include "raylib.h"
#include "player.h"
#include "weapon.h"
#include "game.h"
#include "pickup.h"
#include "lander.h"
#include "structure/structure.h"

// ============================================================================
// HUD Animation Constants
// ============================================================================

#define HUD_CROSSHAIR_ROTATE_SPEED  15.0f
#define HUD_CROSSHAIR_SPREAD_DECAY  8.0f
#define HUD_CROSSHAIR_KILL_FLASH    0.15f
#define HUD_EKG_BASE_SPEED          1.2f
#define HUD_EKG_CRIT_SPEED          4.0f
#define HUD_HEALTH_SEGMENTS         10
#define HUD_NIXIE_ROLL_SPEED        12.0f
#define HUD_NIXIE_RELOAD_SPIN       30.0f
#define HUD_RADAR_RPM               12.0f
#define HUD_RADAR_RANGE             60.0f
#define HUD_RADAR_FADE              2.0f
#define HUD_WAVE_SPLASH_HOLD        1.5f
#define HUD_WAVE_SPLASH_SHRINK      0.5f
#define HUD_KILL_FEED_SLOTS         5
#define HUD_KILL_FEED_LIFE          1.2f
#define HUD_STREAK_WINDOW           3.0f
#define HUD_STREAK_DISPLAY          2.0f
#define HUD_INTERFERENCE_MIN        8.0f
#define HUD_INTERFERENCE_MAX        15.0f
#define HUD_INTERFERENCE_SPEED      1.5f
#define HUD_PANEL_GLOW_SPEED        0.5f
#define HUD_LOW_AMMO_THRESHOLD      0.2f
#define HUD_LOW_AMMO_BLINK_SPEED    8.0f
#define HUD_COMPASS_MARKS           36
#define HUD_EKG_BUFFER_SIZE         64
#define HUD_RADIO_TRANSMISSION_DUR  3.0f

// Dieselpunk deco bar corner cut size (scaled)
#define HUD_DECO_CUT                8

// ============================================================================
// Radar Blip
// ============================================================================

#define HUD_MAX_RADAR_BLIPS 50

typedef struct {
    Vector3 position;
    int faction;        // 0 = Soviet (red), 1 = American (blue)
    bool isLander;
} HudRadarBlip;

// ============================================================================
// HUD State
// ============================================================================

typedef struct {
    // Crosshair
    float crossRotation;
    float crossSpread;
    float killConfirmTimer;

    // Health EKG
    float ekgPhase;
    float ekgBuffer[HUD_EKG_BUFFER_SIZE];

    // Nixie ammo counter
    int prevAmmo;
    float digitRoll[4];
    int displayDigits[4];
    int targetDigits[4];
    bool reloadSpin;
    float nixieRollTimer;

    // Radar
    float radarAngle;
    float blipTimers[HUD_MAX_RADAR_BLIPS];
    Vector2 blipPositions[HUD_MAX_RADAR_BLIPS];
    int blipFactions[HUD_MAX_RADAR_BLIPS];
    bool blipIsLander[HUD_MAX_RADAR_BLIPS];
    int blipCount;

    // Compass
    float compassYaw;

    // Wave splash
    int lastWave;
    float waveSplashTimer;
    int waveSplashWave;
    int waveTypewriterChars;
    float waveTypewriterTimer;

    // Kill feed
    float killFeedTimers[HUD_KILL_FEED_SLOTS];
    int killFeedHead;
    int prevKillCount;

    // Kill streak
    int streakCount;
    float streakTimer;
    float streakDisplayTimer;
    int streakLevel;

    // Scanline interference
    float interferenceCountdown;
    float interferenceY;
    float interferenceIntensity;
    bool interferenceActive;
    float interferenceTimer;

    // Panel glow
    float panelGlowPhase;

    // Low ammo
    float lowAmmoFlash;

    // Enemy count
    int enemyCount;

    // Fuehrerauge zoom optic
    float fuehreraugeAnim;

    bool initialized;
} HudState;

// ============================================================================
// Public API
// ============================================================================

/// Zero-initialize all HUD animation state.
void HudInit(HudState *state);

/// Advance all HUD animations each frame.
void HudUpdate(HudState *state, Player *player, Weapon *weapon, Game *game, float dt);

/// Draw the main HUD overlay (crosshair, top bar, bottom bar, reload gauge, damage flash).
void HudDraw(HudState *state, Player *player, Weapon *weapon, Game *game, int sw, int sh);

/// Draw the Fuehrerauge zoom optic viewport.
void HudDrawFuehrerauge(Texture2D zoomTexture, Shader zoomShader, float anim, int sw, int sh);

/// Draw the pickup weapon indicator bar.
void HudDrawPickup(PickupManager *pm, int sw, int sh);

/// Draw pulsing "ACHTUNG SOLDATEN" alert when landers are inbound.
void HudDrawLanderArrows(LanderManager *lm, Camera3D camera, int sw, int sh);

/// Draw brass-framed radio overlay with oscilloscope waveform.
void HudDrawRadioTransmission(float timer, int sw, int sh);

/// Draw rank kill notification flash.
void HudDrawRankKill(float timer, int rankType, int sw, int sh);

/// Draw structure interaction prompts.
void HudDrawStructurePrompt(StructurePrompt prompt, int resuppliesLeft, float emptyTimer, int sw, int sh);

/// Draw the brass radar circle with blip projection, sweep line, and compass.
void HudDrawRadar(HudState *state, Player *player, HudRadarBlip *blips, int blipCount, int sw, int sh);

/// Draw the EKG heartbeat waveform behind the health gauge.
void HudDrawEKG(HudState *state, float health, float maxHealth, int x, int y, int w, int h);

/// Draw nixie tube rolling digit ammo display.
void HudDrawNixie(HudState *state, int value, int x, int y, float s);

/// Draw the wave splash transition with typewriter text reveal.
void HudDrawWaveSplash(HudState *state, int sw, int sh);

/// Draw the kill feed overlay (top-right notifications + streak counter).
void HudDrawKillFeed(HudState *state, int sw, int sh);

/// Draw horizontal scanline interference bands.
void HudDrawInterference(HudState *state, int sw, int sh);

/// Draw a small dieselpunk radio widget in the bottom-left showing the current march song name.
void HudDrawMarchRadio(const char *marchName, int sw, int sh);

#endif
