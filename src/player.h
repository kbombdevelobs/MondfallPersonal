#ifndef PLAYER_H
#define PLAYER_H

#include "raylib.h"
#include "raymath.h"

#define MOON_GRAVITY 1.62f
#define PLAYER_SPEED 8.0f
#define PLAYER_JUMP_FORCE 6.0f
#define MOUSE_SENSITIVITY 0.003f
#define PLAYER_HEIGHT 1.8f
#define PLAYER_MAX_HEALTH 100.0f

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
} Player;

void PlayerInit(Player *player);
void PlayerUpdate(Player *player, float dt);
void PlayerTakeDamage(Player *player, float amount);
void PlayerApplyRecoil(Player *player, Vector3 direction, float force);
bool PlayerIsDead(Player *player);
Vector3 PlayerGetForward(Player *player);

#endif
