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
