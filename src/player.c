#include "player.h"
#include "world.h"
#include <math.h>

void PlayerInit(Player *player) {
    player->position = (Vector3){0.0f, PLAYER_HEIGHT, 0.0f};
    player->velocity = (Vector3){0.0f, 0.0f, 0.0f};
    player->yaw = 0.0f;
    player->pitch = 0.0f;
    player->health = PLAYER_MAX_HEALTH;
    player->maxHealth = PLAYER_MAX_HEALTH;
    player->onGround = true;
    player->damageFlashTimer = 0.0f;
    player->headBob = 0.0f;
    player->headBobTimer = 0.0f;
    player->mouseSensitivity = MOUSE_SENSITIVITY;

    player->camera.position = player->position;
    player->camera.target = (Vector3){0.0f, PLAYER_HEIGHT, -1.0f};
    player->camera.up = (Vector3){0.0f, 1.0f, 0.0f};
    player->camera.fovy = CAMERA_FOV;
    player->camera.projection = CAMERA_PERSPECTIVE;
}

Vector3 PlayerGetForward(Player *player) {
    Vector3 forward;
    forward.x = cosf(player->pitch) * sinf(player->yaw);
    forward.y = sinf(player->pitch);
    forward.z = cosf(player->pitch) * cosf(player->yaw);
    return Vector3Normalize(forward);
}

void PlayerUpdate(Player *player, float dt) {
    // Mouse look
    Vector2 mouseDelta = GetMouseDelta();
    player->yaw -= mouseDelta.x * player->mouseSensitivity;
    player->pitch -= mouseDelta.y * player->mouseSensitivity;

    if (player->pitch > PITCH_LIMIT) player->pitch = PITCH_LIMIT;
    if (player->pitch < -PITCH_LIMIT) player->pitch = -PITCH_LIMIT;

    // Lunge timer — blocks WASD while active
    if (player->lungeTimer > 0) {
        player->lungeTimer -= dt;
    }

    // Movement direction vectors (horizontal only)
    Vector3 forward = {sinf(player->yaw), 0.0f, cosf(player->yaw)};
    Vector3 right = {-cosf(player->yaw), 0.0f, sinf(player->yaw)};

    Vector3 moveDir = {0};
    bool moving = false;

    // During lunge, WASD is locked out — velocity is purely from the lunge impulse
    if (player->lungeTimer <= 0) {
        if (IsKeyDown(KEY_W)) { moveDir = Vector3Add(moveDir, forward); moving = true; }
        if (IsKeyDown(KEY_S)) { moveDir = Vector3Subtract(moveDir, forward); moving = true; }
        if (IsKeyDown(KEY_D)) { moveDir = Vector3Add(moveDir, right); moving = true; }
        if (IsKeyDown(KEY_A)) { moveDir = Vector3Subtract(moveDir, right); moving = true; }

        if (moving) {
            moveDir = Vector3Normalize(moveDir);
            float speed = PLAYER_SPEED;
            if (IsKeyDown(KEY_LEFT_SHIFT)) speed *= SPRINT_MULTIPLIER;
            player->velocity.x = moveDir.x * speed;
            player->velocity.z = moveDir.z * speed;
        } else {
            // Moon-like sliding deceleration
            player->velocity.x *= (1.0f - DECEL_FRICTION * dt);
            player->velocity.z *= (1.0f - DECEL_FRICTION * dt);
        }
    } else {
        // During lunge: mild drag so it doesn't feel floaty at the end
        player->velocity.x *= (1.0f - 2.0f * dt);
        player->velocity.z *= (1.0f - 2.0f * dt);
        moving = true; // keep head bob going
    }

    // Jump (floaty moon jump)
    if (IsKeyPressed(KEY_SPACE) && player->onGround) {
        player->velocity.y = PLAYER_JUMP_FORCE;
        player->onGround = false;
    }

    // Ground pound (X key) — slam down HARD
    if (IsKeyPressed(KEY_X) && !player->onGround) {
        player->velocity.y = GROUND_POUND_VELOCITY;
        player->velocity.x *= 0.2f;
        player->velocity.z *= 0.2f;
    }
    // Also allow ground pound if barely above ground (within jump threshold)
    if (IsKeyPressed(KEY_X) && player->onGround) {
        // Mini hop then slam
        player->velocity.y = GROUND_POUND_HOP;
        player->onGround = false;
    }

    // Moon gravity
    if (!player->onGround) {
        player->velocity.y -= MOON_GRAVITY * dt;
    }

    // Apply velocity
    float newX = player->position.x + player->velocity.x * dt;
    float newZ = player->position.z + player->velocity.z * dt;
    player->position.y += player->velocity.y * dt;

    // Boulder collision — push back if moving into a rock
    if (!WorldCheckCollision(WorldGetActive(), (Vector3){newX, player->position.y, newZ}, COLLISION_RADIUS)) {
        player->position.x = newX;
        player->position.z = newZ;
    } else {
        // Try sliding along one axis
        if (!WorldCheckCollision(WorldGetActive(), (Vector3){newX, player->position.y, player->position.z}, COLLISION_RADIUS)) {
            player->position.x = newX;
        } else {
            player->velocity.x = 0;
        }
        if (!WorldCheckCollision(WorldGetActive(), (Vector3){player->position.x, player->position.y, newZ}, COLLISION_RADIUS)) {
            player->position.z = newZ;
        } else {
            player->velocity.z = 0;
        }
    }

    // Ground collision (terrain height)
    float groundH = WorldGetHeight(player->position.x, player->position.z) + PLAYER_HEIGHT;
    if (player->position.y <= groundH) {
        player->position.y = groundH;
        player->velocity.y = 0.0f;
        player->onGround = true;
    }

    // Head bob
    if (moving && player->onGround) {
        player->headBobTimer += dt * HEAD_BOB_FREQ;
        player->headBob = sinf(player->headBobTimer) * HEAD_BOB_AMP;
    } else {
        player->headBob *= 0.9f;
        player->headBobTimer = 0.0f;
    }

    // Update camera
    Vector3 lookForward = PlayerGetForward(player);
    player->camera.position = player->position;
    player->camera.position.y += player->headBob;
    player->camera.target = Vector3Add(player->camera.position, lookForward);

    // Damage flash decay
    if (player->damageFlashTimer > 0.0f) {
        player->damageFlashTimer -= dt;
    }
}

void PlayerApplyRecoil(Player *player, Vector3 direction, float force) {
    // Only push when airborne (moon physics — recoil matters in low gravity!)
    if (!player->onGround) {
        player->velocity.x -= direction.x * force;
        player->velocity.y -= direction.y * force * RECOIL_Y_FACTOR;
        player->velocity.z -= direction.z * force;
    }
}

void PlayerTakeDamage(Player *player, float amount) {
    player->health -= amount;
    player->damageFlashTimer = DAMAGE_FLASH_DURATION;
    if (player->health < 0.0f) player->health = 0.0f;
}

bool PlayerIsDead(Player *player) {
    return player->health <= 0.0f;
}
