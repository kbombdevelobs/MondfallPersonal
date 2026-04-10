#include "player.h"
#include "world.h"
#include "structure/structure.h"
#include "sound_gen.h"
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
    player->isGroundPounding = false;
    player->groundPoundImpactTimer = 0.0f;
    player->groundPoundImpactPos = (Vector3){0};
    player->groundPoundStartAlt = 0.0f;
    player->groundPoundReady = false;
    player->he3Fuel = HE3_MAX_FUEL;
    player->he3ChargeTimer = 0.0f;
    player->he3Charging = false;
    player->hasDoubleJumped = false;
    player->doubleJumpThrust = 0.0f;
    player->he3JetActive = false;
    player->he3TrailHead = 0;
    player->he3TrailCount = 0;
    player->he3TrailSpawnTimer = 0.0f;
    if (!player->groundPoundSoundLoaded) {
        player->sndGroundPound = SoundGenNoiseTone(0.6f, 55.0f, 4.0f);
        player->groundPoundSoundLoaded = true;
    }
    player->fuehreraugeActive = false;
    player->fuehreraugeAnim = 0.0f;

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

    // Fuehrerauge (Leader Eye) activation
    player->fuehreraugeActive = IsMouseButtonDown(MOUSE_BUTTON_RIGHT);
    if (player->fuehreraugeActive) {
        player->fuehreraugeAnim += FUEHRERAUGE_ANIM_SPEED * dt;
        if (player->fuehreraugeAnim > 1.0f) player->fuehreraugeAnim = 1.0f;
    } else {
        player->fuehreraugeAnim -= FUEHRERAUGE_ANIM_SPEED * dt;
        if (player->fuehreraugeAnim < 0.0f) player->fuehreraugeAnim = 0.0f;
    }

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

    // Safety: clear stale ground pound state if somehow stuck on ground
    if (player->isGroundPounding && player->onGround) {
        player->isGroundPounding = false;
    }

    // Reset double jump on landing
    if (player->onGround) {
        player->hasDoubleJumped = false;
    }

    // === He-3 Charged Jump (VonBraun-Mondschwerkraftsprungstiefel) ===
    // Hold SPACE on ground to charge, release to launch. Tap = normal free jump.
    bool canJump = player->onGround;

    if (canJump && IsKeyDown(KEY_SPACE) && !player->he3Charging && IsKeyPressed(KEY_SPACE)) {
        // Start charging on the frame SPACE is first pressed while grounded
        player->he3Charging = true;
        player->he3ChargeTimer = 0.0f;
    }

    // Once charging, keep building as long as SPACE is held
    if (player->he3Charging && IsKeyDown(KEY_SPACE)) {
        player->he3ChargeTimer += dt;
        if (player->he3ChargeTimer > HE3_CHARGE_MAX_TIME) {
            player->he3ChargeTimer = HE3_CHARGE_MAX_TIME;
        }
    }

    // Release SPACE -> launch
    bool chargeRelease = player->he3Charging && !IsKeyDown(KEY_SPACE);
    if (chargeRelease) {
        float chargeRatio = player->he3ChargeTimer / HE3_CHARGE_MAX_TIME;

        if (player->he3ChargeTimer < HE3_TAP_THRESHOLD || player->he3Fuel <= 0.0f) {
            // Quick tap or empty tank: normal free jump
            player->velocity.y = PLAYER_JUMP_FORCE;
        } else {
            // Charged He-3 jump: scale force with charge, consume fuel
            float force = PLAYER_JUMP_FORCE + HE3_CHARGED_FORCE_MIN +
                          (HE3_CHARGED_FORCE_MAX - HE3_CHARGED_FORCE_MIN) * chargeRatio;
            float cost = HE3_CHARGE_COST * chargeRatio;
            if (cost > player->he3Fuel) {
                // Partial charge -- scale force to remaining fuel
                chargeRatio = player->he3Fuel / HE3_CHARGE_COST;
                force = PLAYER_JUMP_FORCE + HE3_CHARGED_FORCE_MIN +
                        (HE3_CHARGED_FORCE_MAX - HE3_CHARGED_FORCE_MIN) * chargeRatio;
                cost = player->he3Fuel;
            }
            player->he3Fuel -= cost;
            player->velocity.y = force;
            player->he3JetActive = true;
            player->he3TrailSpawnTimer = 0.0f;
        }
        player->onGround = false;
        player->he3Charging = false;
        player->he3ChargeTimer = 0.0f;
    }

    // === Double Jump (one per airborne, costs He-3) ===
    if (!player->onGround && !player->hasDoubleJumped && IsKeyPressed(KEY_SPACE)
        && player->he3Fuel >= HE3_DOUBLE_JUMP_COST && !player->he3Charging
        && !player->isGroundPounding) {
        // Kill downward velocity for a clean start
        if (player->velocity.y < 0) player->velocity.y = 0;
        player->he3Fuel -= HE3_DOUBLE_JUMP_COST;
        player->hasDoubleJumped = true;
        player->doubleJumpThrust = HE3_DOUBLE_JUMP_DURATION;
        player->velocity.x *= 0.7f;
        player->velocity.z *= 0.7f;
        player->pitch += 0.06f;
        player->he3JetActive = true;
        player->he3TrailSpawnTimer = 0.0f;
    }
    // Apply gradual double jump thrust each frame
    if (player->doubleJumpThrust > 0.0f) {
        float thrustPerSec = HE3_DOUBLE_JUMP_FORCE / HE3_DOUBLE_JUMP_DURATION;
        player->velocity.y += thrustPerSec * dt;
        player->doubleJumpThrust -= dt;
        if (player->doubleJumpThrust < 0.0f) player->doubleJumpThrust = 0.0f;
    }

    // Track whether player is high enough for ground pound (used by HUD)
    player->groundPoundReady = false;
    if (!player->onGround && !player->isGroundPounding) {
        float groundH = WorldGetHeight(player->position.x, player->position.z);
        float heightAboveGround = player->position.y - PLAYER_HEIGHT - groundH;
        if (heightAboveGround >= GROUND_POUND_MIN_HEIGHT) {
            player->groundPoundReady = true;
        }
    }

    // Ground pound (X key) -- slam down HARD, only when well above ground
    if (IsKeyPressed(KEY_X) && player->groundPoundReady) {
        float groundH = WorldGetHeight(player->position.x, player->position.z);
        float heightAboveGround = player->position.y - PLAYER_HEIGHT - groundH;
        player->velocity.y = GROUND_POUND_VELOCITY;
        player->velocity.x *= 0.2f;
        player->velocity.z *= 0.2f;
        player->isGroundPounding = true;
        player->groundPoundStartAlt = heightAboveGround;
    }
    // Also allow ground pound if barely above ground (mini hop then slam)
    if (IsKeyPressed(KEY_X) && player->onGround && !player->isGroundPounding) {
        player->velocity.y = GROUND_POUND_HOP;
        player->onGround = false;
    }

    // Ground pound continuous acceleration -- keep pulling harder each frame
    if (player->isGroundPounding && !player->onGround) {
        player->velocity.y += GROUND_POUND_ACCEL * dt;
        // Kill any lateral drift for a clean vertical slam
        player->velocity.x *= (1.0f - 8.0f * dt);
        player->velocity.z *= (1.0f - 8.0f * dt);
    }

    // Decay impact timer
    if (player->groundPoundImpactTimer > 0.0f) {
        player->groundPoundImpactTimer -= dt;
    }

    // He-3 jet trail -- record positions while boosting upward, age all points
    if (player->he3JetActive && player->velocity.y <= 0.0f && player->doubleJumpThrust <= 0.0f) {
        player->he3JetActive = false;
    }
    if (player->he3JetActive && !player->onGround) {
        player->he3TrailSpawnTimer -= dt;
        if (player->he3TrailSpawnTimer <= 0.0f) {
            player->he3TrailSpawnTimer = 0.04f;
            Vector3 foot = player->position;
            foot.y -= PLAYER_HEIGHT * 0.5f;
            player->he3Trail[player->he3TrailHead] = foot;
            player->he3TrailAge[player->he3TrailHead] = 0.0f;
            player->he3TrailHead = (player->he3TrailHead + 1) % HE3_JET_TRAIL_MAX;
            if (player->he3TrailCount < HE3_JET_TRAIL_MAX) player->he3TrailCount++;
        }
    }
    for (int ti = 0; ti < player->he3TrailCount; ti++) {
        player->he3TrailAge[ti] += dt;
    }
    // Expire old trail points
    while (player->he3TrailCount > 0) {
        int oldest = (player->he3TrailHead - player->he3TrailCount + HE3_JET_TRAIL_MAX) % HE3_JET_TRAIL_MAX;
        if (player->he3TrailAge[oldest] >= HE3_JET_TRAIL_LIFE) {
            player->he3TrailCount--;
        } else break;
    }

    // Moon gravity
    if (!player->onGround) {
        player->velocity.y -= MOON_GRAVITY * dt;
    }

    // Apply velocity
    float newX = player->position.x + player->velocity.x * dt;
    float newZ = player->position.z + player->velocity.z * dt;
    player->position.y += player->velocity.y * dt;

    // When inside a structure, skip exterior collision and terrain ground checks
    StructureManager *structs = StructureGetActive();
    bool insideStructure = structs && StructureIsPlayerInside(structs);

    if (!insideStructure) {
        // Boulder + structure collision — push back if moving into a rock or base
        bool rockHit = WorldCheckCollision(WorldGetActive(), (Vector3){newX, player->position.y, newZ}, COLLISION_RADIUS);
        bool structHit = StructureCheckCollision(structs, (Vector3){newX, player->position.y, newZ}, COLLISION_RADIUS);
        if (!rockHit && !structHit) {
            player->position.x = newX;
            player->position.z = newZ;
        } else {
            // Try sliding along one axis
            bool xRock = WorldCheckCollision(WorldGetActive(), (Vector3){newX, player->position.y, player->position.z}, COLLISION_RADIUS);
            bool xStruct = StructureCheckCollision(structs, (Vector3){newX, player->position.y, player->position.z}, COLLISION_RADIUS);
            if (!xRock && !xStruct) {
                player->position.x = newX;
            } else {
                player->velocity.x = 0;
            }
            bool zRock = WorldCheckCollision(WorldGetActive(), (Vector3){player->position.x, player->position.y, newZ}, COLLISION_RADIUS);
            bool zStruct = StructureCheckCollision(structs, (Vector3){player->position.x, player->position.y, newZ}, COLLISION_RADIUS);
            if (!zRock && !zStruct) {
                player->position.z = newZ;
            } else {
                player->velocity.z = 0;
            }
        }

        // Ground collision (terrain height)
        float groundH = WorldGetHeight(player->position.x, player->position.z) + PLAYER_HEIGHT;
        if (player->position.y <= groundH) {
            // Ground pound impact detection
            if (player->isGroundPounding && !player->onGround) {
                if (player->groundPoundStartAlt >= GROUND_POUND_MIN_HEIGHT) {
                    player->groundPoundImpactTimer = GROUND_POUND_DUST_LIFE;
                    player->groundPoundImpactPos = (Vector3){player->position.x, groundH - PLAYER_HEIGHT, player->position.z};
                    if (player->groundPoundSoundLoaded) {
                        PlaySound(player->sndGroundPound);
                    }
                }
                player->isGroundPounding = false;
            }
            player->position.y = groundH;
            player->velocity.y = 0.0f;
            player->onGround = true;
        }
    } else {
        // Inside structure — free movement, no world collision
        player->position.x = newX;
        player->position.z = newZ;
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
