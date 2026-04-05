#ifndef CONFIG_H
#define CONFIG_H

// ============================================================================
// MONDFALL — Centralized Game Configuration
// All tunable constants in one place. Tweak gameplay here, not in .c files.
// ============================================================================

// --- Rendering ---
#define RENDER_W 640
#define RENDER_H 360
#define WINDOW_W 960
#define WINDOW_H 540
#define CAMERA_FOV 75.0f

// --- Player Physics ---
#define MOON_GRAVITY         1.62f
#define PLAYER_SPEED         8.0f
#define PLAYER_JUMP_FORCE    6.0f
#define MOUSE_SENSITIVITY    0.003f
#define PLAYER_HEIGHT        1.8f
#define PLAYER_MAX_HEALTH    100.0f
#define SPRINT_MULTIPLIER    1.5f
#define GROUND_POUND_VELOCITY (-25.0f)
#define GROUND_POUND_HOP     3.0f
#define PITCH_LIMIT          1.4f
#define HEAD_BOB_FREQ        10.0f
#define HEAD_BOB_AMP         0.08f
#define DECEL_FRICTION       5.0f
#define COLLISION_RADIUS     0.4f
#define DAMAGE_FLASH_DURATION 0.3f
#define HEALTH_REGEN_RATE    2.0f
#define DIFFICULTY_VALKYRIE_SCALE 0.0f
#define DIFFICULTY_EASY_SCALE   0.5f
#define DIFFICULTY_NORMAL_SCALE 1.0f
#define DIFFICULTY_HARD_SCALE   1.5f
#define RECOIL_Y_FACTOR      0.3f

// --- Weapon: Mond-MP40 ---
#define MP40_MAG_SIZE        32
#define MP40_RESERVE         128
#define MP40_FIRE_RATE       0.08f
#define MP40_DAMAGE          25.0f
#define MP40_RANGE           100.0f
#define MP40_RECOIL          0.5f
#define MP40_SHAKE           0.02f

// --- Weapon: Raketenfaust ---
#define RAKETEN_MAG_SIZE     5
#define RAKETEN_RESERVE      10
#define RAKETEN_FIRE_RATE    0.1f
#define RAKETEN_DAMAGE       500.0f
#define RAKETEN_BLAST_RADIUS 3.0f
#define RAKETEN_BEAM_DURATION 2.0f
#define RAKETEN_COOLDOWN     2.0f
#define RAKETEN_RECOIL       4.0f
#define RAKETEN_SHAKE        0.08f
#define RAKETEN_BEAM_RANGE   100.0f
#define RAKETEN_BEAM_PUSH    25.0f
#define RAKETEN_BEAM_LIFT    5.0f

// --- Weapon: Jackhammer ---
#define JACKHAMMER_DAMAGE    9999.0f
#define JACKHAMMER_RANGE     4.5f
#define JACKHAMMER_FIRE_RATE 0.5f
#define JACKHAMMER_SHAKE     0.06f
#define JACKHAMMER_LUNGE_SPEED  30.0f
#define JACKHAMMER_LUNGE_DURATION 0.3f

// --- Weapon Pools ---
#define MAX_BEAM_TRAILS  20
#define MAX_PROJECTILES  30
#define MAX_EXPLOSIONS   10

// --- Weapon Effects ---
#define MUZZLE_FLASH_DECAY   8.0f
#define RECOIL_DECAY         10.0f
#define JACKHAMMER_ANIM_DECAY 6.0f
#define RELOAD_DURATION      1.5f
#define BEAM_TRAIL_LIFE      0.1f

// --- Enemy: Limits ---
#define MAX_ENEMIES 50
#define TEST_MAX_ENEMIES    200

// --- Test Mode / LOD ---
#define LOD1_DISTANCE       30.0f
#define LOD2_DISTANCE       60.0f
#define CULL_DISTANCE       120.0f
#define AI_STAGGER_DIVISOR  4
#define COLLISION_CAP       8

// --- Enemy: Soviet ---
#define SOVIET_HEALTH        80.0f
#define SOVIET_SPEED         5.5f
#define SOVIET_DAMAGE        7.0f
#define SOVIET_ATTACK_RANGE  22.0f
#define SOVIET_ATTACK_RATE   0.15f
#define SOVIET_PREFERRED_DIST 8.0f

// --- Enemy: American ---
#define AMERICAN_HEALTH      55.0f
#define AMERICAN_SPEED       5.0f
#define AMERICAN_DAMAGE      10.0f
#define AMERICAN_ATTACK_RANGE 30.0f
#define AMERICAN_ATTACK_RATE 0.4f
#define AMERICAN_PREFERRED_DIST 18.0f

// --- Enemy AI ---
#define AI_STRAFE_TIMER_BASE  1.5f
#define AI_STRAFE_TIMER_RAND  2.0f
#define AI_DODGE_COOLDOWN     3.0f
#define AI_TURN_SPEED         6.0f
#define ENEMY_GROUND_OFFSET   1.2f
#define ENEMY_HIT_FLASH_DECAY 4.0f
#define ENEMY_MUZZLE_DECAY    8.0f

// --- World ---
#define CHUNK_SIZE           60.0f
#define RENDER_CHUNKS        5
#define MESH_RES             12
#define MAX_CHUNK_CRATERS    5
#define MAX_CHUNK_ROCKS      10
#define MAX_CHUNKS_PER_FRAME 3
#define MAX_STARS            300
#define MAX_CACHED_CHUNKS    (RENDER_CHUNKS * RENDER_CHUNKS)

// --- Lander ---
#define MAX_LANDERS          4
#define LANDER_SPAWN_RADIUS_BASE 30.0f
#define LANDER_SPAWN_RADIUS_RAND 25.0f
#define LANDER_INITIAL_HEIGHT_BASE 80.0f
#define LANDER_INITIAL_HEIGHT_RAND 40.0f
#define LANDER_DESCENT_SPEED_BASE 15.0f
#define LANDER_IMPACT_SHAKE  2.0f
#define LANDER_RETRO_SHAKE   0.3f
#define LANDER_DEPLOY_INTERVAL 0.4f
#define LANDER_KLAXON_WAIT   3.5f
#define LANDER_EXPLOSION_DURATION 1.5f

// --- Pickup ---
#define MAX_PICKUPS          30
#define PICKUP_LIFESPAN      15.0f
#define PICKUP_GRAB_RANGE    4.0f
#define PICKUP_BOB_FREQ      3.0f
#define PICKUP_DESPAWN_BLINK 2.0f
#define PICKUP_BLINK_SPEED   12.0f

// --- Pickup Weapon Stats: Soviet (KOSMOS-7 SMG) ---
#define PICKUP_SOVIET_AMMO       80
#define PICKUP_SOVIET_FIRE_RATE  0.05f   // fastest fire rate in game
#define PICKUP_SOVIET_DAMAGE     35.0f   // hits harder than MP40
#define PICKUP_SOVIET_RANGE      80.0f
#define PICKUP_SOVIET_RECOIL     2.0f    // snappy kick

// --- Pickup Weapon Stats: American (LIBERTY BLASTER) ---
#define PICKUP_AMERICAN_AMMO     8
#define PICKUP_AMERICAN_FIRE_RATE 0.5f   // slow but devastating
#define PICKUP_AMERICAN_DAMAGE   9999.0f // one-shot kill
#define PICKUP_AMERICAN_RANGE    80.0f
#define PICKUP_AMERICAN_RECOIL   2.5f    // massive kick

// --- Pickup Weapon Stats: KS-23 Molot (Soviet NCO Shotgun) ---
#define PICKUP_MOLOT_AMMO        12
#define PICKUP_MOLOT_FIRE_RATE   0.35f   // pump action
#define PICKUP_MOLOT_DAMAGE      60.0f   // per pellet
#define PICKUP_MOLOT_PELLETS     5       // pellets per shot
#define PICKUP_MOLOT_SPREAD      0.08f   // radians spread cone
#define PICKUP_MOLOT_RANGE       25.0f   // short but deadly
#define PICKUP_MOLOT_RECOIL      1.8f

// --- Pickup Weapon Stats: M8A1 Starhawk (American NCO Burst Rifle) ---
#define PICKUP_STARHAWK_AMMO      36
#define PICKUP_STARHAWK_FIRE_RATE 0.08f   // between burst rounds
#define PICKUP_STARHAWK_BURST_CD  0.5f    // cooldown between bursts
#define PICKUP_STARHAWK_BURST_SIZE 3      // rounds per burst
#define PICKUP_STARHAWK_DAMAGE    45.0f
#define PICKUP_STARHAWK_SPREAD    0.015f  // tight grouping
#define PICKUP_STARHAWK_RANGE     60.0f
#define PICKUP_STARHAWK_RECOIL    1.2f

// --- Pickup Weapon Stats: Zarya TK-4 (Soviet Officer Charged Pistol) ---
#define PICKUP_ZARYA_AMMO         6
#define PICKUP_ZARYA_CHARGE_TIME  1.5f    // seconds to full charge
#define PICKUP_ZARYA_DMG_MIN      150.0f  // uncharged damage
#define PICKUP_ZARYA_DMG_MAX      500.0f  // fully charged damage
#define PICKUP_ZARYA_RANGE        50.0f
#define PICKUP_ZARYA_RECOIL       2.2f

// --- Pickup Weapon Stats: ARC-9 Longbow (American Officer Piercing Beam) ---
#define PICKUP_LONGBOW_AMMO       4
#define PICKUP_LONGBOW_FIRE_RATE  1.0f
#define PICKUP_LONGBOW_DAMAGE     200.0f  // per enemy hit
#define PICKUP_LONGBOW_PIERCE     5       // max enemies pierced
#define PICKUP_LONGBOW_RANGE      70.0f
#define PICKUP_LONGBOW_RECOIL     1.5f

// --- AI Movement ---
#define AI_ACCEL_TROOPER     5.0f   // movement acceleration
#define AI_ACCEL_NCO         4.0f
#define AI_ACCEL_OFFICER     3.0f
#define AI_SLOPE_THRESHOLD   0.8f   // gradient above which speed is penalized
#define AI_SLOPE_MIN_FACTOR  0.2f   // minimum speed multiplier on steep terrain
#define SPATIAL_CELL_SIZE    4.0f   // spatial hash grid cell size
#define SPATIAL_GRID_DIM     32     // spatial hash grid dimension
#define SPATIAL_CELL_CAP     6      // max entities per cell
#define SQUAD_COHESION_BIAS  0.3f   // strength of pull toward squad centroid
#define SQUAD_COHESION_RADIUS 20.0f // max distance for cohesion pull

// --- Pickup Weapon Effects ---
#define PICKUP_RECOIL_FORCE  0.3f
#define PICKUP_SHAKE         0.03f

// --- Wave System ---
#define WAVE_DELAY           6.0f
#define WAVE_ENEMIES_BASE    3
#define WAVE_ENEMIES_PER_WAVE 2

// --- Audio ---
#define AUDIO_SAMPLE_RATE    44100
#define AUDIO_RADIO_SAMPLE_RATE 8000
#define AUDIO_RADIO_OVERDRIVE 2.5f
#define AUDIO_RADIO_CLIP     0.7f
#define AUDIO_RADIO_CRACKLE  0.08f
#define AUDIO_RADIO_POP_CHANCE 800
#define AUDIO_RADIO_POP_AMP  0.5f
#define AUDIO_DEATH_VOLUME   0.4f
#define AUDIO_MARCH_VOLUME   0.3f

// --- Colors (as initializer lists, use with (Color){...}) ---
#define COLOR_CROSSHAIR      {0, 255, 200, 230}
#define COLOR_SOVIET_RED     {255, 50, 30, 220}
#define COLOR_AMERICAN_BLUE  {60, 140, 255, 220}
#define COLOR_METAL_GRAY     {45, 48, 55, 255}
#define COLOR_METAL_DARK     {30, 32, 38, 255}
#define COLOR_HUD_ACCENT     {0, 200, 180, 100}
#define COLOR_HUD_CYAN       {0, 255, 220, 255}
#define COLOR_HUD_RELOAD     {0, 220, 255, 240}
#define COLOR_ALERT_RED      {255, 50, 30, 255}
#define COLOR_ALERT_YELLOW   {255, 200, 50, 255}
#define COLOR_HEALTH_GREEN   {50, 220, 50, 255}
#define COLOR_HEALTH_YELLOW  {220, 220, 50, 255}
#define COLOR_HEALTH_RED     {220, 50, 50, 255}
#define COLOR_DAMAGE_FLASH   {220, 0, 0}
#define COLOR_BEAM_SOVIET    {255, 80, 40, 180}
#define COLOR_BEAM_AMERICAN  {80, 160, 255, 180}

// --- Screen Shake ---
#define SHAKE_THRESHOLD      0.05f
#define SHAKE_AMPLITUDE      0.15f

// --- Structures ---
#define MAX_STRUCTURES           8
#define MAX_STRUCTURE_DOORS      3
#define STRUCTURE_INTERACT_RANGE 3.5f
#define STRUCTURE_INTERIOR_Y     500.0f
#define MOONBASE_EXTERIOR_RADIUS 4.5f
#define MOONBASE_DOOR_COUNT      3
#define MOONBASE_INTERIOR_W      24.0f
#define MOONBASE_INTERIOR_D      20.0f
#define MOONBASE_INTERIOR_H      5.0f
#define MOONBASE_GEODESIC_SEGS   12
#define STRUCTURE_SPAWN_CHANCE   15   // 1-in-N chunks gets a base (lower = more common)
#define MOONBASE_RESUPPLIES      3   // number of resupply uses per base
#define MOONBASE_COLLISION_HEIGHT 6.0f  // dome top height above terrain for collision

// --- Rank System ---
#define RANK_NCO_WAVE_START       1     // NCOs appear from wave 1
#define RANK_OFFICER_WAVE_START   1     // Officers appear from wave 1
#define RANK_NCO_CHANCE           20    // 1-in-N chance per spawn (~5%)
#define RANK_OFFICER_CHANCE       50    // 1-in-N chance per spawn (~2%)
#define RANK_MAX_OFFICERS_PER_WAVE 2    // cap officers per wave

// --- Rank Stat Multipliers ---
#define NCO_HEALTH_MULT          1.8f    // NCOs are tough frontline leaders
#define NCO_DAMAGE_MULT          1.3f
#define NCO_SPEED_MULT           1.1f
#define OFFICER_HEALTH_MULT      0.7f    // Officers are frailer — command from rear
#define OFFICER_DAMAGE_MULT      1.5f    // Deadly accurate
#define OFFICER_SPEED_MULT       0.85f   // Measured, not rushing
#define OFFICER_RANGE_MULT       1.5f    // Engages from further back
#define OFFICER_RATE_MULT        0.6f    // Fires deliberately
#define OFFICER_DIST_MULT        1.6f    // Holds back further than troops

// --- Leadership / Morale ---
#define LEADERSHIP_RADIUS        15.0f
#define MORALE_LEADER_BONUS      0.3f
#define MORALE_NCO_RALLY_RATE    0.15f
#define MORALE_DECAY_RATE        0.05f
#define MORALE_OFFICER_DEATH_HIT 0.6f
#define MORALE_NCO_DEATH_HIT     0.3f
#define MORALE_FLEE_THRESHOLD    0.25f
#define MORALE_FLEE_CHANCE       40
#define MORALE_ACCURACY_BONUS    0.15f
#define MORALE_SPEED_PENALTY     0.7f
#define MORALE_NATURAL_RECOVERY  0.08f
#define MORALE_RALLY_THRESHOLD   0.5f
#define MORALE_FLEE_DURATION_MIN 3.0f
#define MORALE_FLEE_DURATION_MAX 6.0f

#endif
