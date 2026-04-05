#include "raylib.h"
#include "raymath.h"
#include "config.h"
#include "game.h"
#include "player.h"
#include "world.h"
#include "world/world_draw.h"
#include "weapon.h"
#include "enemy/enemy_components.h"
#include "enemy/enemy_systems.h"
#include "enemy/enemy_spawn.h"
#include "enemy/enemy_draw_ecs.h"
#include "ecs_world.h"
#include "combat_ecs.h"
#include "hud.h"
#include "audio.h"
#include "lander.h"
#include "pickup.h"
#include "structure/structure.h"
#include "structure/structure_draw.h"
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
    Shader crtShader = LoadShader(0, "resources/crt.fs");
    int timeLoc = GetShaderLocation(crtShader, "time");

    // HUD render texture + visor curve shader
    RenderTexture2D hudTarget = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());
    Shader hudShader = LoadShader(0, "resources/hud.fs");

    // Fuehrerauge (Leader Eye) zoom render target + fishbowl shader
    RenderTexture2D feTarget = LoadRenderTexture(FUEHRERAUGE_RENDER_W, FUEHRERAUGE_RENDER_H);
    SetTextureFilter(feTarget.texture, TEXTURE_FILTER_POINT);
    Shader feShader = LoadShader(0, "resources/fuehrerauge.fs");
    int feTimeLoc = GetShaderLocation(feShader, "time");

    Game game;
    Player player;
    World world;
    Weapon weapon;
    memset(&weapon, 0, sizeof(weapon));
    GameAudio audio;
    memset(&audio, 0, sizeof(audio));
    LanderManager landers;
    memset(&landers, 0, sizeof(landers));
    PickupManager pickups;
    PickupManagerInit(&pickups);
    StructureManager structures;
    memset(&structures, 0, sizeof(structures));
    float rankKillTimer = 0;
    int rankKillType = 0;

    GameInit(&game);
    PlayerInit(&player);

    // ECS combat context (replaces old CombatContext)
    ecs_world_t *ecsWorld = NULL;
    EcsCombatContext ecsCombat = {&player, &weapon, &pickups, &game, NULL};

    IntroScript introScript;
    IntroScriptLoad(&introScript, "resources/intro.txt");

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

            // Initialize Flecs ECS world
            GameEcsInit();
            ecsWorld = GameEcsGetWorld();
            EcsEnemyComponentsRegister(ecsWorld);
            EcsEnemySystemsRegister(ecsWorld);
            EcsEnemyResourcesInit(ecsWorld);
            ecsCombat.ecsWorld = ecsWorld;

            // Set initial game context singleton
            ecs_singleton_set(ecsWorld, EcGameContext, {
                .playerPos = player.position,
                .camera = player.camera,
                .testMode = game.testMode,
                .aiFrameCounter = 0,
                .playerDamageAccum = 0,
                .killCount = 0,
                .rankKillType = 0
            });

            GameAudioInit(&audio);
            LanderManagerInit(&landers);
            StructureManagerInit(&structures);
            assetsLoaded = true;
            continue;
        }

        // Update music stream
        GameAudioUpdate(&audio);

        // Sync settings to systems
        player.mouseSensitivity = game.mouseSensitivity;
        GameAudioSetMusicVolume(&audio, game.musicVolume);
        WeaponSetSFXVolume(&weapon, game.sfxVolume);
        // SFX volume for ECS enemy resources
        {
            const EcEnemyResources *res = ecs_singleton_get(ecsWorld, EcEnemyResources);
            if (res) {
                SetSoundVolume(res->sndSovietFire, game.sfxVolume);
                SetSoundVolume(res->sndAmericanFire, game.sfxVolume);
                for (int i = 0; i < res->sovietDeathCount; i++)
                    SetSoundVolume(res->sndSovietDeath[i], game.sfxVolume * AUDIO_DEATH_VOLUME);
                for (int i = 0; i < res->americanDeathCount; i++)
                    SetSoundVolume(res->sndAmericanDeath[i], game.sfxVolume * AUDIO_DEATH_VOLUME);
            }
        }
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
                            game.testMode = false;
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
                                // Reset ECS world for new game
                                GameEcsInit();
                                ecsWorld = GameEcsGetWorld();
                                EcsEnemyComponentsRegister(ecsWorld);
                                EcsEnemySystemsRegister(ecsWorld);
                                EcsEnemyResourcesInit(ecsWorld);
                                ecsCombat.ecsWorld = ecsWorld;
                                LanderManagerInit(&landers);
                                PickupManagerInit(&pickups);
                                StructureManagerInit(&structures);
                                if (!cursorLocked) { DisableCursor(); cursorLocked = true; }
                            }
                            break;
                        case 1: // Test Mode
                            game.testMode = true;
                            game.state = STATE_PLAYING;
                            GameReset(&game);
                            PlayerInit(&player);
                            player.mouseSensitivity = game.mouseSensitivity;
                            WeaponInit(&weapon);
                            // Reset ECS world for test mode
                            GameEcsInit();
                            ecsWorld = GameEcsGetWorld();
                            EcsEnemyComponentsRegister(ecsWorld);
                            EcsEnemySystemsRegister(ecsWorld);
                            EcsEnemyResourcesInit(ecsWorld);
                            ecsCombat.ecsWorld = ecsWorld;
                            LanderManagerInit(&landers);
                            PickupManagerInit(&pickups);
                            // Spawn 200 enemies in concentric rings
                            for (int ei = 0; ei < TEST_MAX_ENEMIES; ei++) {
                                EnemyType etype = (ei % 2 == 0) ? ENEMY_SOVIET : ENEMY_AMERICAN;
                                float spawnR = 15.0f + (float)(ei / 10) * 5.0f;
                                EcsEnemySpawnAroundPlayer(ecsWorld, etype, player.position, spawnR);
                            }
                            if (!cursorLocked) { DisableCursor(); cursorLocked = true; }
                            break;
                        case 2: // Settings
                            game.settingsReturnState = STATE_MENU;
                            game.settingsSelection = 0;
                            game.state = STATE_SETTINGS;
                            break;
                        case 3: // Quit
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
                    GameEcsInit();
                    ecsWorld = GameEcsGetWorld();
                    EcsEnemyComponentsRegister(ecsWorld);
                    EcsEnemySystemsRegister(ecsWorld);
                    EcsEnemyResourcesInit(ecsWorld);
                    ecsCombat.ecsWorld = ecsWorld;
                    LanderManagerInit(&landers);
                    PickupManagerInit(&pickups);
                    StructureManagerInit(&structures);
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

                bool insideStructure = StructureIsPlayerInside(&structures);

                // Update game context singleton for ECS systems
                {
                    EcGameContext *ctx = ecs_singleton_ensure(ecsWorld, EcGameContext);
                    ctx->playerPos = player.position;
                    ctx->camera = player.camera;
                    ctx->testMode = game.testMode;
                    ctx->playerDamageAccum = 0;
                    ctx->killCount = 0;
                    ctx->rankKillType = 0;
                    ecs_singleton_modified(ecsWorld, EcGameContext);
                }

                // Player always updates (movement works inside and outside)
                PlayerUpdate(&player, dt);

                // Structure interaction (enter/exit/resupply) — always active
                StructureManagerUpdate(&structures, &player, &weapon, &pickups);

                // Re-sync camera after structure teleport/clamping may have moved player
                {
                    Vector3 fwd = PlayerGetForward(&player);
                    player.camera.position = player.position;
                    player.camera.position.y += player.headBob;
                    player.camera.target = Vector3Add(player.camera.position, fwd);
                }

                // World chunks always update (for terrain height)
                WorldUpdate(&world, player.position);

                // Discover new structures in nearby chunks
                StructureManagerCheckSpawns(&structures, player.position);

                // --- SIMULATION FREEZE when inside structure ---
                if (!insideStructure) {
                    GameUpdate(&game);
                    WeaponUpdate(&weapon, dt);

                    // ECS progress — runs all AI, physics, attack, death systems
                    ecs_progress(ecsWorld, dt);

                    LanderManagerUpdate(&landers, ecsWorld, dt);
                    PickupManagerUpdate(&pickups, player.position, dt);

                    // E to pick up dropped weapon (only outside)
                    if (IsKeyPressed(KEY_E)) {
                        PickupTryGrab(&pickups, player.position);
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

                    // Wave spawning via landers
                    if (game.waveActive && game.enemiesSpawned == 0) {
                        LanderSpawnWave(&landers, player.position, game.enemiesPerWave, game.wave);
                        game.enemiesSpawned = game.enemiesPerWave;
                    }

                    // Check if wave is cleared (all landers done + all enemies dead)
                    if (game.waveActive && !LanderWaveActive(&landers) && EcsEnemyCountAlive(ecsWorld) == 0 && game.enemiesSpawned > 0) {
                        game.waveActive = false;
                        game.enemiesRemaining = 0;
                    }

                    // Combat resolution (ECS-based)
                    EcsCombatProcessPickupFire(&ecsCombat);
                    EcsCombatProcessWeaponFire(&ecsCombat);
                    EcsCombatProcessProjectiles(&ecsCombat);
                    EcsCombatProcessBeam(&ecsCombat, dt);

                    // Collect player damage from ECS attack system
                    if (!game.testMode) {
                        const EcGameContext *ctx = ecs_singleton_get(ecsWorld, EcGameContext);
                        if (ctx && ctx->playerDamageAccum > 0) {
                            PlayerTakeDamage(&player, ctx->playerDamageAccum * game.damageScaler);
                        }
                    }

                    // Collect kill count and rank kill flash from ECS
                    {
                        const EcGameContext *ctx = ecs_singleton_get(ecsWorld, EcGameContext);
                        if (ctx) {
                            game.killCount += ctx->killCount;
                            if (ctx->rankKillType > 0) {
                                rankKillType = ctx->rankKillType;
                                rankKillTimer = 2.0f;
                            }
                        }
                    }

                    // Decay rank kill timer
                    if (rankKillTimer > 0) rankKillTimer -= dt;

                    // Death check
                    if (!game.testMode && PlayerIsDead(&player)) {
                        game.state = STATE_GAME_OVER;
                        game.menuSelection = 0;
                        EnableCursor();
                        cursorLocked = false;
                    }
                }

                // Heal over time — works inside and outside
                if (player.health < player.maxHealth) {
                    player.health += HEALTH_REGEN_RATE * dt;
                    if (player.health > player.maxHealth) player.health = player.maxHealth;
                }

                // Weapon bob sync
                weapon.weaponBobTimer = player.headBobTimer;

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
                    GameEcsInit();
                    ecsWorld = GameEcsGetWorld();
                    EcsEnemyComponentsRegister(ecsWorld);
                    EcsEnemySystemsRegister(ecsWorld);
                    EcsEnemyResourcesInit(ecsWorld);
                    ecsCombat.ecsWorld = ecsWorld;
                    LanderManagerInit(&landers);
                    PickupManagerInit(&pickups);
                    StructureManagerInit(&structures);
                    // If restarting test mode, re-spawn enemies
                    if (game.testMode) {
                        for (int ei = 0; ei < TEST_MAX_ENEMIES; ei++) {
                            EnemyType etype = (ei % 2 == 0) ? ENEMY_SOVIET : ENEMY_AMERICAN;
                            float spawnR = 15.0f + (float)(ei / 10) * 5.0f;
                            EcsEnemySpawnAroundPlayer(ecsWorld, etype, player.position, spawnR);
                        }
                    }
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
                if (StructureIsPlayerInside(&structures)) {
                    // Interior rendering — only the room
                    StructureManagerDrawInterior(&structures);
                } else {
                    // Normal world rendering
                    WorldDrawSky(&world, player.camera);
                    WorldDraw(&world, player.position, player.camera);
                    StructureManagerDraw(&structures, player.position);
                    // ECS enemy rendering
                    EcsEnemyManagerDraw(ecsWorld, player.camera, game.testMode);
                    LanderManagerDraw(&landers, player.position);
                    PickupManagerDraw(&pickups);
                    WeaponDrawWorld(&weapon);
                }
                if (game.state == STATE_PLAYING || game.state == STATE_PAUSED ||
                    (game.state == STATE_SETTINGS && game.settingsReturnState == STATE_PAUSED)) {
                    if (pickups.hasPickup)
                        PickupDrawFirstPerson(&pickups, player.camera, weapon.weaponBobTimer);
                    else
                        WeaponDrawFirst(&weapon, player.camera);
                }
            EndMode3D();
            EndTextureMode();

            // Fuehrerauge zoom render pass (second 3D render at narrow FOV)
            if (player.fuehreraugeAnim > 0.0f) {
                Camera3D zoomCam = player.camera;
                zoomCam.fovy = FUEHRERAUGE_FOV;

                BeginTextureMode(feTarget);
                ClearBackground(BLACK);
                BeginMode3D(zoomCam);
                    if (StructureIsPlayerInside(&structures)) {
                        StructureManagerDrawInterior(&structures);
                    } else {
                        WorldDrawSky(&world, zoomCam);
                        WorldDraw(&world, player.position, zoomCam);
                        StructureManagerDraw(&structures, player.position);
                        EcsEnemyManagerDraw(ecsWorld, zoomCam, game.testMode);
                        LanderManagerDraw(&landers, player.position);
                        PickupManagerDraw(&pickups);
                        WeaponDrawWorld(&weapon);
                    }
                EndMode3D();
                EndTextureMode();
            }
        }

        // ---- DRAW TO WINDOW ----
        BeginDrawing();
        ClearBackground(BLACK);

        // Update shader time uniform
        float timeVal = (float)GetTime();
        SetShaderValue(crtShader, timeLoc, &timeVal, SHADER_UNIFORM_FLOAT);
        SetShaderValue(feShader, feTimeLoc, &timeVal, SHADER_UNIFORM_FLOAT);

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
                // Get radio transmission timer from ECS
                float radioTimer = 0;
                const EcEnemyResources *res = ecs_singleton_get(ecsWorld, EcEnemyResources);
                if (res) radioTimer = res->radioTransmissionTimer;

                BeginTextureMode(hudTarget);
                ClearBackground(BLANK);
                // Fuehrerauge drawn first so announcements layer on top
                if (player.fuehreraugeAnim > 0.0f) {
                    HudDrawFuehrerauge(feTarget.texture, feShader,
                        player.fuehreraugeAnim,
                        hudTarget.texture.width, hudTarget.texture.height);
                }
                HudDraw(&player, &weapon, &game, hudTarget.texture.width, hudTarget.texture.height);
                HudDrawPickup(&pickups, hudTarget.texture.width, hudTarget.texture.height);
                HudDrawLanderArrows(&landers, player.camera, hudTarget.texture.width, hudTarget.texture.height);
                HudDrawRadioTransmission(radioTimer, hudTarget.texture.width, hudTarget.texture.height);
                HudDrawRankKill(rankKillTimer, rankKillType, hudTarget.texture.width, hudTarget.texture.height);
                {
                    int resLeft = 0;
                    if (structures.insideIndex >= 0)
                        resLeft = structures.structures[structures.insideIndex].resuppliesLeft;
                    HudDrawStructurePrompt(StructureGetPrompt(&structures), resLeft,
                        StructureGetEmptyMessageTimer(&structures),
                        hudTarget.texture.width, hudTarget.texture.height);
                }
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
    EcsEnemySystemsCleanup();
    EcsEnemyResourcesUnload(ecsWorld);
    GameEcsFini();
    WorldUnload(&world);
    GameAudioUnload(&audio);
    LanderManagerUnload(&landers);
    StructureManagerUnload(&structures);
    UnloadShader(crtShader);
    UnloadShader(hudShader);
    UnloadShader(feShader);
    UnloadRenderTexture(target);
    UnloadRenderTexture(hudTarget);
    UnloadRenderTexture(feTarget);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}
