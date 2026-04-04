#include "raylib.h"
#include "raymath.h"
#include "config.h"
#include "game.h"
#include "player.h"
#include "world.h"
#include "world/world_draw.h"
#include "weapon.h"
#include "enemy/enemy.h"
#include "hud.h"
#include "audio.h"
#include "lander.h"
#include "pickup.h"
#include "combat.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

int main(void) {
    srand((unsigned int)time(NULL));

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(WINDOW_W, WINDOW_H, "MONDFALL - DO IT FOR BORMANN");
    SetExitKey(0); // Disable ESC closing the window
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
    memset(&weapon, 0, sizeof(weapon));
    EnemyManager enemies;
    memset(&enemies, 0, sizeof(enemies));
    GameAudio audio;
    memset(&audio, 0, sizeof(audio));
    LanderManager landers;
    memset(&landers, 0, sizeof(landers));
    PickupManager pickups;
    PickupManagerInit(&pickups);

    GameInit(&game);
    PlayerInit(&player);

    CombatContext combat = {&player, &weapon, &enemies, &pickups, &game};

    IntroScript introScript;
    IntroScriptLoad(&introScript, "assets/intro.txt");

    bool assetsLoaded = false;
    bool cursorLocked = false;
    int lastScreenScale = game.screenScale;

    while (!game.quitRequested) {
        // Handle window close button (X button)
        if (WindowShouldClose()) break;

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

        // Sync settings to systems
        player.mouseSensitivity = game.mouseSensitivity;
        GameAudioSetMusicVolume(&audio, game.musicVolume);
        WeaponSetSFXVolume(&weapon, game.sfxVolume);
        EnemyManagerSetSFXVolume(&enemies, game.sfxVolume);
        LanderManagerSetSFXVolume(&landers, game.sfxVolume);
        PickupManagerSetSFXVolume(&pickups, game.sfxVolume);

        // Apply screen scale only when setting changes
        if (game.screenScale != lastScreenScale) {
            SetWindowSize(RENDER_W * game.screenScale, RENDER_H * game.screenScale);
            lastScreenScale = game.screenScale;
        }

        // Recreate HUD render texture if window size changed
        if (hudTarget.texture.width != GetScreenWidth() || hudTarget.texture.height != GetScreenHeight()) {
            UnloadRenderTexture(hudTarget);
            hudTarget = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());
        }

        // ---- STATE MACHINE ----
        switch (game.state) {
            case STATE_MENU: {
                // Toggle intro skip with I key
                if (IsKeyPressed(KEY_I)) {
                    game.introSeen = !game.introSeen;
                }
                if (IsKeyPressed(KEY_ENTER)) {
                    switch (game.menuSelection) {
                        case 0: // Play
                            if (!game.introSeen) {
                                // Show intro first
                                game.state = STATE_INTRO;
                                game.introTimer = 0;
                                game.introLineTimer = 0;
                                game.introLineIndex = 0;
                                game.introCharIndex = 0;
                                game.introFadingOut = false;
                                game.introFadeAlpha = 0;
                                game.introSeen = true; // don't show again
                            } else {
                                // Skip intro, go straight to game
                                game.state = STATE_PLAYING;
                                GameReset(&game);
                                PlayerInit(&player);
                                player.mouseSensitivity = game.mouseSensitivity;
                                WeaponInit(&weapon);
                                EnemyManagerInit(&enemies);
                                LanderManagerInit(&landers);
                                PickupManagerInit(&pickups);
                                combat = (CombatContext){&player, &weapon, &enemies, &pickups, &game};
                                if (!cursorLocked) { DisableCursor(); cursorLocked = true; }
                            }
                            break;
                        case 1: // Settings
                            game.settingsReturnState = STATE_MENU;
                            game.settingsSelection = 0;
                            game.state = STATE_SETTINGS;
                            break;
                        case 2: // Quit
                            game.quitRequested = true;
                            break;
                    }
                }
                break;
            }

            case STATE_INTRO: {
                bool introDone = GameUpdateIntro(&game, &introScript);
                if (introDone) {
                    game.state = STATE_PLAYING;
                    GameReset(&game);
                    PlayerInit(&player);
                    player.mouseSensitivity = game.mouseSensitivity;
                    WeaponInit(&weapon);
                    EnemyManagerInit(&enemies);
                    LanderManagerInit(&landers);
                    PickupManagerInit(&pickups);
                    combat = (CombatContext){&player, &weapon, &enemies, &pickups, &game};
                    if (!cursorLocked) { DisableCursor(); cursorLocked = true; }
                }
                break;
            }

            case STATE_SETTINGS: {
                // Back via ESC or selecting Back option
                if (IsKeyPressed(KEY_ESCAPE) ||
                    (IsKeyPressed(KEY_ENTER) && game.settingsSelection == 5)) {
                    game.state = game.settingsReturnState;
                    if (game.settingsReturnState == STATE_PAUSED) {
                        game.pauseSelection = 1; // keep cursor on Settings
                    }
                }
                break;
            }

            case STATE_PLAYING: {
                // Pause
                if (IsKeyPressed(KEY_ESCAPE)) {
                    game.state = STATE_PAUSED;
                    game.pauseSelection = 0;
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
                PickupManagerUpdate(&pickups, player.position, dt);

                // E to pick up dropped weapon
                if (IsKeyPressed(KEY_E)) {
                    PickupTryGrab(&pickups, player.position);
                }

                // Heal over time — slow regen
                if (player.health < player.maxHealth) {
                    player.health += HEALTH_REGEN_RATE * dt;
                    if (player.health > player.maxHealth) player.health = player.maxHealth;
                }

                // Screen shake — landers + weapon recoil
                float shake = LanderGetScreenShake(&landers);
                if (weapon.recoil > 0.1f) {
                    float gunShake = 0;
                    if (weapon.current == WEAPON_MOND_MP40) gunShake = weapon.recoil * MP40_SHAKE;
                    else if (weapon.raketenFiring) gunShake = weapon.recoil * RAKETEN_SHAKE;
                    else if (weapon.current == WEAPON_JACKHAMMER) gunShake = weapon.recoil * JACKHAMMER_SHAKE;
                    shake += gunShake;
                }
                if (pickups.hasPickup && pickups.pickupRecoil > 0.1f) {
                    shake += pickups.pickupRecoil * PICKUP_SHAKE;
                }
                if (shake > SHAKE_THRESHOLD) {
                    player.camera.position.x += ((float)rand()/RAND_MAX - 0.5f) * shake * SHAKE_AMPLITUDE;
                    player.camera.position.y += ((float)rand()/RAND_MAX - 0.5f) * shake * SHAKE_AMPLITUDE;
                }

                // Weapon bob sync
                weapon.weaponBobTimer = player.headBobTimer;

                // Wave spawning via landers
                if (game.waveActive && game.enemiesSpawned == 0) {
                    LanderSpawnWave(&landers, player.position, game.enemiesPerWave, game.wave);
                    game.enemiesSpawned = game.enemiesPerWave;
                }

                // Check if wave is cleared (all landers done + all enemies dead)
                if (game.waveActive && !LanderWaveActive(&landers) && EnemyCountAlive(&enemies) == 0 && game.enemiesSpawned > 0) {
                    game.waveActive = false;
                    game.enemiesRemaining = 0;
                }

                // Combat resolution
                CombatProcessPickupFire(&combat);
                CombatProcessWeaponFire(&combat);
                CombatProcessProjectiles(&combat);
                CombatProcessBeam(&combat, dt);
                CombatProcessEnemyDamage(&combat, dt);

                // Death check
                if (PlayerIsDead(&player)) {
                    game.state = STATE_GAME_OVER;
                    game.menuSelection = 0; // reset game over selection
                    EnableCursor();
                    cursorLocked = false;
                }
                break;
            }

            case STATE_PAUSED: {
                if (IsKeyPressed(KEY_ESCAPE)) {
                    // ESC in pause = resume
                    game.state = STATE_PLAYING;
                    DisableCursor();
                    cursorLocked = true;
                }
                if (IsKeyPressed(KEY_ENTER)) {
                    switch (game.pauseSelection) {
                        case 0: // Resume
                            game.state = STATE_PLAYING;
                            DisableCursor();
                            cursorLocked = true;
                            break;
                        case 1: // Settings
                            game.settingsReturnState = STATE_PAUSED;
                            game.settingsSelection = 0;
                            game.state = STATE_SETTINGS;
                            break;
                        case 2: // Main Menu
                            game.state = STATE_MENU;
                            game.menuSelection = 0;
                            EnableCursor();
                            cursorLocked = false;
                            break;
                    }
                }
                break;
            }

            case STATE_GAME_OVER: {
                // Game over selection is handled inside GameDrawGameOver via menuSelection
                if (game.menuSelection == -1) {
                    // Restart
                    game.menuSelection = 0;
                    game.state = STATE_PLAYING;
                    GameReset(&game);
                    PlayerInit(&player);
                    player.mouseSensitivity = game.mouseSensitivity;
                    WeaponInit(&weapon);
                    EnemyManagerInit(&enemies);
                    LanderManagerInit(&landers);
                    PickupManagerInit(&pickups);
                    combat = (CombatContext){&player, &weapon, &enemies, &pickups, &game};
                    DisableCursor();
                    cursorLocked = true;
                } else if (game.menuSelection == -2) {
                    // Main menu
                    game.menuSelection = 0;
                    game.state = STATE_MENU;
                    EnableCursor();
                    cursorLocked = false;
                }
                break;
            }
        }

        // ---- RENDER TO LOW-RES TARGET (gameplay only) ----
        bool renderGameplay = (game.state == STATE_PLAYING || game.state == STATE_PAUSED ||
                               game.state == STATE_GAME_OVER ||
                               (game.state == STATE_SETTINGS && game.settingsReturnState == STATE_PAUSED));

        if (renderGameplay) {
            BeginTextureMode(target);
            ClearBackground(BLACK);

            BeginMode3D(player.camera);
                WorldDrawSky(&world, player.camera);
                WorldDraw(&world, player.position, player.camera);
                EnemyManagerDraw(&enemies);
                LanderManagerDraw(&landers, player.position);
                PickupManagerDraw(&pickups);
                WeaponDrawWorld(&weapon);
                if (game.state == STATE_PLAYING || game.state == STATE_PAUSED ||
                    (game.state == STATE_SETTINGS && game.settingsReturnState == STATE_PAUSED)) {
                    if (pickups.hasPickup)
                        PickupDrawFirstPerson(&pickups, player.camera, weapon.weaponBobTimer);
                    else
                        WeaponDrawFirst(&weapon, player.camera);
                }
            EndMode3D();
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
            if (game.state == STATE_PLAYING || game.state == STATE_PAUSED ||
                (game.state == STATE_SETTINGS && game.settingsReturnState == STATE_PAUSED)) {
                BeginTextureMode(hudTarget);
                ClearBackground(BLANK);
                HudDraw(&player, &weapon, &game, hudTarget.texture.width, hudTarget.texture.height);
                HudDrawPickup(&pickups, hudTarget.texture.width, hudTarget.texture.height);
                HudDrawLanderArrows(&landers, player.camera, hudTarget.texture.width, hudTarget.texture.height);
                HudDrawRadioTransmission(enemies.radioTransmissionTimer, hudTarget.texture.width, hudTarget.texture.height);
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
        if (game.state == STATE_INTRO) {
            GameDrawIntro(&game, &introScript);
        } else if (game.state == STATE_MENU) {
            GameDrawMenu(&game);
        } else if (game.state == STATE_SETTINGS) {
            GameDrawSettings(&game);
        } else if (game.state == STATE_PAUSED) {
            GameDrawPaused(&game);
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
