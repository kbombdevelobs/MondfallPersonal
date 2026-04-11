#ifndef PLAYER_H
#define PLAYER_H

#include "raylib.h"
#include "raymath.h"
#include "config.h"

typedef struct {
    Camera3D camera;
    Vector3 position;
    Vector3 velocity;
    float yaw;
    float pitch;
    float health;
    float maxHealth;
    bool onGround;
    float damageFlashTimer;
    float headBob;
    float headBobTimer;
    float mouseSensitivity;
    float lungeTimer;           // > 0 means lunge is active, WASD locked out
    // Ground pound
    bool isGroundPounding;      // true while accelerating downward
    float groundPoundImpactTimer; // > 0 means impact just happened (for shake/dust)
    Vector3 groundPoundImpactPos; // where the slam landed
    float groundPoundStartAlt;  // altitude when pound initiated (for min-height gate)
    bool groundPoundReady;      // true when high enough to initiate ground pound
    Sound sndGroundPound;       // procedural boom sound
    bool groundPoundSoundLoaded;
    // He-3 Jump System (VonBraun-Mondschwerkraftsprungstiefel)
    float he3Fuel;              // current He-3 fuel (0 to HE3_MAX_FUEL)
    float he3ChargeTimer;       // how long SPACE has been held on ground (0 = not charging)
    bool he3Charging;           // true while holding SPACE on ground for charged jump
    bool hasDoubleJumped;       // reset on landing, one air jump per airborne
    float doubleJumpThrust;     // > 0 means thrust is still being applied (gradual boost)
    bool he3JetActive;          // true while emitting trail (velocity.y > 0 after boost)
    Vector3 he3Trail[HE3_JET_TRAIL_MAX]; // ring buffer of past foot positions
    float he3TrailAge[HE3_JET_TRAIL_MAX]; // age of each trail point (seconds)
    int he3TrailHead;           // next write index
    int he3TrailCount;          // how many valid entries
    float he3TrailSpawnTimer;   // time until next trail point is recorded
    // Fuehrerauge zoom optic (hold RMB)
    bool fuehreraugeActive;     // RMB currently held
    float fuehreraugeAnim;      // 0.0 = retracted, 1.0 = fully deployed
} Player;

void PlayerInit(Player *player);
void PlayerUpdate(Player *player, float dt);
void PlayerTakeDamage(Player *player, float amount);
void PlayerApplyRecoil(Player *player, Vector3 direction, float force);
bool PlayerIsDead(Player *player);
Vector3 PlayerGetForward(Player *player);

#endif
