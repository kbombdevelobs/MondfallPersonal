#include "raylib.h"
#include "raymath.h"
#include "game.h"
#include "player.h"
#include "world.h"
#include "weapon.h"
#include "enemy.h"
#include "hud.h"
#include "audio.h"
#include "lander.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#define RENDER_W 640
#define RENDER_H 360

int main(void) {
    srand((unsigned int)time(NULL));

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(960, 540, "MONDFALL - DO IT FOR BORMANN");
    InitAudioDevice();
    SetTargetFPS(60);

    // Low-res render target for pixely look
    RenderTexture2D target = LoadRenderTexture(RENDER_W, RENDER_H);
    SetTextureFilter(target.texture, TEXTURE_FILTER_POINT); // nearest neighbor

    // CRT post-processing shader
    Shader crtShader = LoadShader(0, "assets/crt.fs");
    int timeLoc = GetShaderLocation(crtShader, "time");

    // HUD render texture + visor curve shader
    RenderTexture2D hudTarget = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());
    Shader hudShader = LoadShader(0, "assets/hud.fs");

    Game game;
    Player player;
    World world;
    Weapon weapon;
    EnemyManager enemies;
    GameAudio audio;
    memset(&audio, 0, sizeof(audio));
    LanderManager landers;
    memset(&landers, 0, sizeof(landers));

    GameInit(&game);
    PlayerInit(&player);

    bool assetsLoaded = false;
    bool cursorLocked = false;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        // Load assets after the window is visible (not before first frame)
        if (!assetsLoaded) {
            // Draw a loading screen first
            BeginDrawing();
            ClearBackground(BLACK);
            const char *msg = "LOADING...";
            DrawText(msg, (GetScreenWidth() - MeasureText(msg, 40)) / 2, GetScreenHeight() / 2 - 20, 40, WHITE);
            EndDrawing();

            WorldInit(&world);
            WeaponInit(&weapon);
            EnemyManagerInit(&enemies);
            GameAudioInit(&audio);
            LanderManagerInit(&landers);
            assetsLoaded = true;
            continue;
        }

        // Update music stream
        GameAudioUpdate(&audio);

        // ---- STATE MACHINE ----
        switch (game.state) {
            case STATE_MENU: {
                if (IsKeyPressed(KEY_ENTER)) {
                    game.state = STATE_PLAYING;
                    game.wave = 0;
                    game.killCount = 0;
                    game.waveActive = false;
                    game.enemiesRemaining = 0;
                    game.waveTimer = 0;
                    PlayerInit(&player);
                    WeaponInit(&weapon);
                    EnemyManagerInit(&enemies);
                    if (!cursorLocked) { DisableCursor(); cursorLocked = true; }
                }
                break;
            }

            case STATE_PLAYING: {
                // Pause
                if (IsKeyPressed(KEY_ESCAPE)) {
                    game.state = STATE_PAUSED;
                    EnableCursor();
                    cursorLocked = false;
                    break;
                }

                // Update systems
                PlayerUpdate(&player, dt);
                GameUpdate(&game);
                WeaponUpdate(&weapon, dt);
                EnemyManagerUpdate(&enemies, player.position, dt);
                WorldUpdate(&world, player.position);
                LanderManagerUpdate(&landers, &enemies, dt);

                // Heal over time — slow regen
                if (player.health < player.maxHealth) {
                    player.health += 2.0f * dt;
                    if (player.health > player.maxHealth) player.health = player.maxHealth;
                }

                // Screen shake from landers
                float shake = LanderGetScreenShake(&landers);
                if (shake > 0.05f) {
                    player.camera.position.x += ((float)rand()/RAND_MAX - 0.5f) * shake * 0.15f;
                    player.camera.position.y += ((float)rand()/RAND_MAX - 0.5f) * shake * 0.15f;
                }

                // Weapon bob sync
                weapon.weaponBobTimer = player.headBobTimer;

                // Wave spawning via landers
                if (game.waveActive && game.enemiesSpawned == 0) {
                    // Launch landers for this wave
                    LanderSpawnWave(&landers, player.position, game.enemiesPerWave, game.wave);
                    game.enemiesSpawned = game.enemiesPerWave; // mark as handled
                }

                // Check if wave is cleared (all landers done + all enemies dead)
                if (game.waveActive && !LanderWaveActive(&landers) && EnemyCountAlive(&enemies) == 0 && game.enemiesSpawned > 0) {
                    game.waveActive = false;
                    game.enemiesRemaining = 0;
                }

                // Shooting
                if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) || (weapon.current == WEAPON_JACKHAMMER && IsKeyDown(KEY_V))) {
                    Vector3 shootDir = PlayerGetForward(&player);
                    Vector3 barrelPos = WeaponGetBarrelWorldPos(&weapon, player.camera);
                    // Fire uses camera pos for accurate aim, but beam starts at barrel
                    Vector3 shootOrigin = player.camera.position;

                    if (WeaponFire(&weapon, barrelPos, shootDir)) {
                        // Recoil push in air (moon physics!)
                        float recoilForce = (weapon.current == WEAPON_RAKETENFAUST) ? 4.0f :
                                           (weapon.current == WEAPON_MOND_MP40) ? 0.5f : 0;
                        if (recoilForce > 0) PlayerApplyRecoil(&player, shootDir, recoilForce);

                        if (weapon.current == WEAPON_MOND_MP40) {
                            Ray shootRay = {shootOrigin, shootDir};
                            float hitDist = 0;
                            int hit = EnemyCheckHit(&enemies, shootRay, 100.0f, &hitDist);
                            if (hit >= 0) {
                                EnemyDamage(&enemies, hit, weapon.mp40Damage);
                                if (enemies.enemies[hit].state == ENEMY_DYING) game.killCount++;
                            }
                        } else if (weapon.current == WEAPON_JACKHAMMER) {
                            Vector3 meleePos = Vector3Add(shootOrigin, Vector3Scale(shootDir, weapon.jackhammerRange * 0.5f));
                            int hit = EnemyCheckSphereHit(&enemies, meleePos, weapon.jackhammerRange);
                            if (hit >= 0) {
                                EnemyDamage(&enemies, hit, weapon.jackhammerDamage);
                                if (enemies.enemies[hit].state == ENEMY_DYING) game.killCount++;
                            }
                        }
                    }
                }

                // Raketenfaust projectile hits
                for (int i = 0; i < MAX_PROJECTILES; i++) {
                    if (!weapon.projectiles[i].active) continue;
                    int hit = EnemyCheckSphereHit(&enemies, weapon.projectiles[i].position, weapon.projectiles[i].radius);
                    if (hit >= 0) {
                        for (int j = 0; j < MAX_ENEMIES; j++) {
                            if (!enemies.enemies[j].active || enemies.enemies[j].state != ENEMY_ALIVE) continue;
                            float d = Vector3Distance(weapon.projectiles[i].position, enemies.enemies[j].position);
                            if (d < weapon.projectiles[i].radius) {
                                float dmg = weapon.projectiles[i].damage * (1.0f - d / weapon.projectiles[i].radius);
                                EnemyDamage(&enemies, j, dmg);
                                if (enemies.enemies[j].state == ENEMY_DYING) game.killCount++;
                            }
                        }
                        WeaponSpawnExplosion(&weapon, weapon.projectiles[i].position, weapon.projectiles[i].radius);
                        weapon.projectiles[i].active = false;
                    }
                    // Ground hit
                    float groundH = WorldGetHeight(weapon.projectiles[i].position.x, weapon.projectiles[i].position.z);
                    if (weapon.projectiles[i].position.y <= groundH) {
                        WeaponSpawnExplosion(&weapon, weapon.projectiles[i].position, weapon.projectiles[i].radius);
                        weapon.projectiles[i].active = false;
                    }
                }

                // MEGA BEAM — update position each frame + kill everything in path
                if (weapon.raketenFiring) {
                    Vector3 shootDir2 = PlayerGetForward(&player);
                    Vector3 barrel2 = WeaponGetBarrelWorldPos(&weapon, player.camera);
                    weapon.raketenBeamStart = barrel2;
                    weapon.raketenBeamEnd = Vector3Add(player.camera.position, Vector3Scale(shootDir2, 100.0f));
                    // MASSIVE knockback — beam pushes you hard opposite
                    player.velocity.x -= shootDir2.x * 25.0f * dt;
                    player.velocity.z -= shootDir2.z * 25.0f * dt;
                    player.velocity.y += 5.0f * dt; // lifts you off ground
                    player.onGround = false;
                    // Kill everything in the beam path
                    Ray beamRay = {player.camera.position, shootDir2};
                    for (int ei = 0; ei < MAX_ENEMIES; ei++) {
                        Enemy *en = &enemies.enemies[ei];
                        if (!en->active || en->state != ENEMY_ALIVE) continue;
                        BoundingBox box = {
                            {en->position.x-1.0f, en->position.y-2.0f, en->position.z-1.0f},
                            {en->position.x+1.0f, en->position.y+2.0f, en->position.z+1.0f}};
                        RayCollision col = GetRayCollisionBox(beamRay, box);
                        if (col.hit && col.distance < 100.0f) {
                            EnemyDamage(&enemies, ei, 999.0f); // instant kill
                            if (enemies.enemies[ei].state == ENEMY_DYING) game.killCount++;
                            WeaponSpawnExplosion(&weapon, en->position, 4.0f);
                        }
                    }
                }

                // Enemy damage to player
                float dmg = EnemyCheckPlayerDamage(&enemies, player.position, dt);
                if (dmg > 0) {
                    PlayerTakeDamage(&player, dmg);
                }

                // Death check
                if (PlayerIsDead(&player)) {
                    game.state = STATE_GAME_OVER;
                    EnableCursor();
                    cursorLocked = false;
                }
                break;
            }

            case STATE_PAUSED: {
                if (IsKeyPressed(KEY_ESCAPE)) {
                    game.state = STATE_PLAYING;
                    DisableCursor();
                    cursorLocked = true;
                }
                break;
            }

            case STATE_GAME_OVER: {
                if (IsKeyPressed(KEY_ENTER)) {
                    game.state = STATE_PLAYING;
                    game.wave = 0;
                    game.killCount = 0;
                    game.waveActive = false;
                    game.enemiesRemaining = 0;
                    game.waveTimer = 0;
                    PlayerInit(&player);
                    WeaponInit(&weapon);
                    EnemyManagerInit(&enemies);
                    DisableCursor();
                    cursorLocked = true;
                }
                break;
            }
        }

        // ---- RENDER TO LOW-RES TARGET (gameplay only) ----
        bool renderGameplay = (game.state == STATE_PLAYING || game.state == STATE_PAUSED || game.state == STATE_GAME_OVER);

        if (renderGameplay) {
            BeginTextureMode(target);
            ClearBackground(BLACK);

            BeginMode3D(player.camera);
                WorldDrawSky(&world, player.camera);
                WorldDraw(&world, player.position);
                EnemyManagerDraw(&enemies);
                LanderManagerDraw(&landers);
                WeaponDrawWorld(&weapon);
                if (game.state == STATE_PLAYING || game.state == STATE_PAUSED) {
                    WeaponDrawFirst(&weapon, player.camera);
                }
            EndMode3D();
            // HUD drawn AFTER shader pass — see below
            EndTextureMode();
        }

        // ---- DRAW TO WINDOW ----
        BeginDrawing();
        ClearBackground(BLACK);

        // Update shader time uniform
        float timeVal = (float)GetTime();
        SetShaderValue(crtShader, timeLoc, &timeVal, SHADER_UNIFORM_FLOAT);

        if (renderGameplay) {
            float scale = fminf(
                (float)GetScreenWidth() / RENDER_W,
                (float)GetScreenHeight() / RENDER_H
            );
            int drawW = (int)(RENDER_W * scale);
            int drawH = (int)(RENDER_H * scale);
            int offsetX = (GetScreenWidth() - drawW) / 2;
            int offsetY = (GetScreenHeight() - drawH) / 2;

            BeginShaderMode(crtShader);
            DrawTexturePro(
                target.texture,
                (Rectangle){0, 0, RENDER_W, -RENDER_H},
                (Rectangle){offsetX, offsetY, drawW, drawH},
                (Vector2){0, 0}, 0.0f, WHITE
            );
            EndShaderMode();

            // HUD — render to texture, then draw with visor curve
            if (game.state == STATE_PLAYING || game.state == STATE_PAUSED) {
                BeginTextureMode(hudTarget);
                ClearBackground(BLANK);
                HudDraw(&player, &weapon, &game, hudTarget.texture.width, hudTarget.texture.height);
                EndTextureMode();

                BeginShaderMode(hudShader);
                DrawTexturePro(hudTarget.texture,
                    (Rectangle){0, 0, (float)hudTarget.texture.width, -(float)hudTarget.texture.height},
                    (Rectangle){0, 0, (float)GetScreenWidth(), (float)GetScreenHeight()},
                    (Vector2){0, 0}, 0, WHITE);
                EndShaderMode();
            }
        }

        // Overlays drawn at full window resolution
        if (game.state == STATE_MENU) {
            GameDrawMenu(&game);
        } else if (game.state == STATE_PAUSED) {
            GameDrawPaused();
        } else if (game.state == STATE_GAME_OVER) {
            GameDrawGameOver(&game);
        }

        DrawFPS(10, GetScreenHeight() - 20);
        EndDrawing();
    }

    WeaponUnload(&weapon);
    EnemyManagerUnload(&enemies);
    WorldUnload(&world);
    GameAudioUnload(&audio);
    LanderManagerUnload(&landers);
    UnloadShader(crtShader);
    UnloadShader(hudShader);
    UnloadRenderTexture(target);
    UnloadRenderTexture(hudTarget);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}
