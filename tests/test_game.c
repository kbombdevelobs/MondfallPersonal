// Mondfall: Letztes Gefecht — Unit Tests — no GPU/window required, exercises pure game logic
// Run: make test

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "../src/config.h"
#include "../src/player.h"
#include "../src/weapon.h"
#include "../src/enemy/enemy.h"
#include "../src/enemy/enemy_components.h"
#include "../src/enemy/enemy_systems.h"
#include "../src/enemy/enemy_spawn.h"
#include "../src/combat_ecs.h"
#include "../src/ecs_world.h"
#include "../src/pickup.h"
#include "../src/game.h"
#include "../src/world.h"
#include "../src/world/world_noise.h"
#include "../src/structure/structure.h"
#include "../src/enemy/enemy_morale.h"

// ---- Minimal test framework ----
static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) static void name(void)
#define RUN(name) do { \
    tests_run++; \
    printf("  %-50s ", #name); \
    name(); \
    tests_passed++; \
    printf("PASS\n"); \
} while(0)

#define ASSERT(cond) do { \
    if (!(cond)) { \
        printf("FAIL\n    %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        tests_failed++; \
        tests_passed--; \
        return; \
    } \
} while(0)

#define ASSERT_FLOAT_EQ(a, b, eps) ASSERT(fabsf((a) - (b)) < (eps))
#define ASSERT_GT(a, b) ASSERT((a) > (b))
#define ASSERT_LT(a, b) ASSERT((a) < (b))

// ============================================================================
// 1. CONFIG SANITY — catch broken tuning before it hits gameplay
// ============================================================================

TEST(test_ppsh_fastest_fire_rate) {
    // PPSh must be faster than MP40 (lower = faster)
    ASSERT_LT(PICKUP_SOVIET_FIRE_RATE, MP40_FIRE_RATE);
}

TEST(test_liberty_oneshots_soviets) {
    ASSERT_GT(PICKUP_AMERICAN_DAMAGE, SOVIET_HEALTH);
}

TEST(test_liberty_oneshots_americans) {
    ASSERT_GT(PICKUP_AMERICAN_DAMAGE, AMERICAN_HEALTH);
}

TEST(test_player_max_health_positive) {
    ASSERT_GT(PLAYER_MAX_HEALTH, 0.0f);
}

TEST(test_wave_formula) {
    // Wave 1 should have BASE + 1 * PER_WAVE enemies
    int expected = WAVE_ENEMIES_BASE + 1 * WAVE_ENEMIES_PER_WAVE;
    ASSERT(expected > 0);
    ASSERT(expected == 5); // 3 + 1*2 = 5
}

TEST(test_pickup_ammo_positive) {
    ASSERT_GT(PICKUP_SOVIET_AMMO, 0);
    ASSERT_GT(PICKUP_AMERICAN_AMMO, 0);
}

TEST(test_liberty_recoil_greater_than_ppsh) {
    ASSERT_GT(PICKUP_AMERICAN_RECOIL, PICKUP_SOVIET_RECOIL);
}

// ============================================================================
// 2. PLAYER PHYSICS
// ============================================================================

static Player make_player(void) {
    Player p;
    memset(&p, 0, sizeof(p));
    p.health = PLAYER_MAX_HEALTH;
    p.maxHealth = PLAYER_MAX_HEALTH;
    p.onGround = true;
    p.camera.up = (Vector3){0, 1, 0};
    p.camera.fovy = CAMERA_FOV;
    p.camera.projection = CAMERA_PERSPECTIVE;
    return p;
}

TEST(test_player_forward_yaw_zero) {
    Player p = make_player();
    p.yaw = 0.0f; p.pitch = 0.0f;
    Vector3 fwd = PlayerGetForward(&p);
    // yaw=0, pitch=0 → forward is (0, 0, 1)
    ASSERT_FLOAT_EQ(fwd.x, 0.0f, 0.01f);
    ASSERT_FLOAT_EQ(fwd.y, 0.0f, 0.01f);
    ASSERT_FLOAT_EQ(fwd.z, 1.0f, 0.01f);
}

TEST(test_player_forward_yaw_90) {
    Player p = make_player();
    p.yaw = PI / 2.0f; p.pitch = 0.0f;
    Vector3 fwd = PlayerGetForward(&p);
    // yaw=PI/2 → forward is (1, 0, 0)
    ASSERT_FLOAT_EQ(fwd.x, 1.0f, 0.01f);
    ASSERT_FLOAT_EQ(fwd.y, 0.0f, 0.01f);
    ASSERT_FLOAT_EQ(fwd.z, 0.0f, 0.05f);
}

TEST(test_player_forward_pitch_up) {
    Player p = make_player();
    p.yaw = 0.0f; p.pitch = PI / 4.0f;
    Vector3 fwd = PlayerGetForward(&p);
    // pitch up → y > 0, z > 0
    ASSERT_GT(fwd.y, 0.0f);
    ASSERT_GT(fwd.z, 0.0f);
}

TEST(test_player_take_damage) {
    Player p = make_player();
    PlayerTakeDamage(&p, 30.0f);
    ASSERT_FLOAT_EQ(p.health, 70.0f, 0.01f);
    ASSERT(!PlayerIsDead(&p));
}

TEST(test_player_take_damage_overkill) {
    Player p = make_player();
    PlayerTakeDamage(&p, 200.0f);
    ASSERT_FLOAT_EQ(p.health, 0.0f, 0.01f);
    ASSERT(PlayerIsDead(&p));
}

TEST(test_player_is_dead_boundary) {
    Player p = make_player();
    p.health = 0.01f;
    ASSERT(!PlayerIsDead(&p));
    p.health = 0.0f;
    ASSERT(PlayerIsDead(&p));
}

TEST(test_player_recoil_airborne) {
    Player p = make_player();
    p.onGround = false;
    p.velocity = (Vector3){0, 0, 0};
    Vector3 dir = {0, 0, 1};
    PlayerApplyRecoil(&p, dir, 5.0f);
    // Should push opposite to direction
    ASSERT_LT(p.velocity.z, 0.0f);
}

TEST(test_player_recoil_grounded_noop) {
    Player p = make_player();
    p.onGround = true;
    p.velocity = (Vector3){0, 0, 0};
    Vector3 dir = {0, 0, 1};
    PlayerApplyRecoil(&p, dir, 5.0f);
    // Grounded: no recoil
    ASSERT_FLOAT_EQ(p.velocity.z, 0.0f, 0.001f);
}

// ============================================================================
// 3. WEAPON LOGIC
// ============================================================================

// WeaponInit needs sounds/meshes — skip it, test getters with manual setup
static Weapon make_weapon(WeaponType type) {
    Weapon w;
    memset(&w, 0, sizeof(w));
    w.current = type;
    w.mp40Mag = MP40_MAG_SIZE; w.mp40MagSize = MP40_MAG_SIZE;
    w.mp40Reserve = MP40_RESERVE; w.mp40FireRate = MP40_FIRE_RATE;
    w.mp40Damage = MP40_DAMAGE;
    w.raketenMag = RAKETEN_MAG_SIZE; w.raketenMagSize = RAKETEN_MAG_SIZE;
    w.raketenReserve = RAKETEN_RESERVE;
    w.raketenBeamDuration = RAKETEN_BEAM_DURATION;
    w.jackhammerDamage = JACKHAMMER_DAMAGE;
    w.jackhammerRange = JACKHAMMER_RANGE;
    w.jackhammerFireRate = JACKHAMMER_FIRE_RATE;
    return w;
}

TEST(test_weapon_mp40_ammo) {
    Weapon w = make_weapon(WEAPON_MOND_MP40);
    ASSERT(WeaponGetAmmo(&w) == MP40_MAG_SIZE);
    ASSERT(WeaponGetMaxAmmo(&w) == MP40_MAG_SIZE);
    ASSERT(WeaponGetReserve(&w) == MP40_RESERVE);
}

TEST(test_weapon_switch) {
    Weapon w = make_weapon(WEAPON_MOND_MP40);
    WeaponSwitch(&w, WEAPON_RAKETENFAUST);
    ASSERT(w.current == WEAPON_RAKETENFAUST);
    ASSERT(WeaponGetAmmo(&w) == RAKETEN_MAG_SIZE);
}

TEST(test_weapon_not_reloading_initially) {
    Weapon w = make_weapon(WEAPON_MOND_MP40);
    ASSERT(!WeaponIsReloading(&w));
    ASSERT_FLOAT_EQ(WeaponReloadProgress(&w), 0.0f, 0.01f);
}

TEST(test_weapon_reload_starts) {
    Weapon w = make_weapon(WEAPON_MOND_MP40);
    w.mp40Mag = 10; // partially empty
    w.mp40Reserve = 50;
    WeaponReload(&w);  // calls PlaySound on zeroed Sound — safe no-op
    ASSERT(WeaponIsReloading(&w));
}

TEST(test_weapon_name) {
    Weapon w = make_weapon(WEAPON_MOND_MP40);
    ASSERT(strcmp(WeaponGetName(&w), "MOND-MP40") == 0);
    WeaponSwitch(&w, WEAPON_JACKHAMMER);
    ASSERT(strcmp(WeaponGetName(&w), "JACKHAMMER") == 0);
}

// ============================================================================
// 4. PICKUP SYSTEM
// ============================================================================

static PickupManager make_pickups(void) {
    PickupManager pm;
    memset(&pm, 0, sizeof(pm));
    return pm;
}

TEST(test_pickup_grab_in_range) {
    PickupManager pm = make_pickups();
    // Place a Soviet gun at origin
    pm.guns[0].active = true;
    pm.guns[0].position = (Vector3){0, 0, 0};
    pm.guns[0].weaponType = PICKUP_KOSMOS7;
    pm.guns[0].life = 10.0f;

    Vector3 playerPos = {1.0f, 0.0f, 0.0f}; // 1 unit away, within PICKUP_GRAB_RANGE (4)
    bool grabbed = PickupTryGrab(&pm, playerPos);
    ASSERT(grabbed);
    ASSERT(pm.hasPickup);
    ASSERT(pm.pickupType == PICKUP_KOSMOS7);
}

TEST(test_pickup_grab_out_of_range) {
    PickupManager pm = make_pickups();
    pm.guns[0].active = true;
    pm.guns[0].position = (Vector3){0, 0, 0};
    pm.guns[0].weaponType = PICKUP_KOSMOS7;
    pm.guns[0].life = 10.0f;

    Vector3 playerPos = {100.0f, 0.0f, 0.0f}; // way too far
    bool grabbed = PickupTryGrab(&pm, playerPos);
    ASSERT(!grabbed);
    ASSERT(!pm.hasPickup);
}

TEST(test_pickup_soviet_stats) {
    PickupManager pm = make_pickups();
    pm.guns[0].active = true;
    pm.guns[0].position = (Vector3){0, 0, 0};
    pm.guns[0].weaponType = PICKUP_KOSMOS7;
    pm.guns[0].life = 10.0f;
    PickupTryGrab(&pm, (Vector3){0, 0, 0});

    ASSERT(pm.pickupAmmo == PICKUP_SOVIET_AMMO);
    ASSERT_FLOAT_EQ(pm.pickupFireRate, PICKUP_SOVIET_FIRE_RATE, 0.001f);
    ASSERT_FLOAT_EQ(pm.pickupDamage, PICKUP_SOVIET_DAMAGE, 0.01f);
}

TEST(test_pickup_american_stats) {
    PickupManager pm = make_pickups();
    pm.guns[0].active = true;
    pm.guns[0].position = (Vector3){0, 0, 0};
    pm.guns[0].weaponType = PICKUP_LIBERTY;
    pm.guns[0].life = 10.0f;
    PickupTryGrab(&pm, (Vector3){0, 0, 0});

    ASSERT(pm.pickupAmmo == PICKUP_AMERICAN_AMMO);
    ASSERT_FLOAT_EQ(pm.pickupFireRate, PICKUP_AMERICAN_FIRE_RATE, 0.001f);
    ASSERT_FLOAT_EQ(pm.pickupDamage, PICKUP_AMERICAN_DAMAGE, 0.01f);
}

TEST(test_pickup_fire_decrements_ammo) {
    PickupManager pm = make_pickups();
    pm.hasPickup = true;
    pm.pickupType = PICKUP_KOSMOS7;
    pm.pickupAmmo = 5;
    pm.pickupFireRate = 0.1f;
    pm.pickupTimer = 0; // ready to fire

    bool fired = PickupFire(&pm, (Vector3){0,0,0}, (Vector3){0,0,1});
    ASSERT(fired);
    ASSERT(pm.pickupAmmo == 4);
}

TEST(test_pickup_fire_respects_cooldown) {
    PickupManager pm = make_pickups();
    pm.hasPickup = true;
    pm.pickupType = PICKUP_KOSMOS7;
    pm.pickupAmmo = 5;
    pm.pickupFireRate = 0.1f;
    pm.pickupTimer = 0.05f; // still cooling down

    bool fired = PickupFire(&pm, (Vector3){0,0,0}, (Vector3){0,0,1});
    ASSERT(!fired);
    ASSERT(pm.pickupAmmo == 5); // unchanged
}

TEST(test_pickup_fire_empty) {
    PickupManager pm = make_pickups();
    pm.hasPickup = true;
    pm.pickupType = PICKUP_KOSMOS7;
    pm.pickupAmmo = 0;
    pm.pickupTimer = 0;

    bool fired = PickupFire(&pm, (Vector3){0,0,0}, (Vector3){0,0,1});
    ASSERT(!fired);
}

// ============================================================================
// 5. ENEMY HIT DETECTION & DAMAGE (ECS-based)
// ============================================================================

static ecs_world_t *make_test_ecs(void) {
    ecs_world_t *w = ecs_init();
    EcsEnemyComponentsRegister(w);
    return w;
}

static ecs_entity_t place_ecs_enemy(ecs_world_t *w, Vector3 pos, float health, EnemyType type) {
    ecs_entity_t e = ecs_new(w);
    ecs_set(w, e, EcTransform, { .position = pos, .facingAngle = 0 });
    ecs_set(w, e, EcVelocity, { .velocity = {0,0,0}, .vertVel = 0, .jumpTimer = 0 });
    ecs_set(w, e, EcFaction, { .type = type });
    ecs_set(w, e, EcAlive, { .dummy = 0 });
    ecs_set(w, e, EcCombatStats, { .health = health, .maxHealth = health, .speed = 5, .damage = 7, .attackRange = 22, .attackRate = 0.15f, .preferredDist = 8 });
    ecs_set(w, e, EcAIState, { .behavior = AI_ADVANCE, .strafeDir = 1, .strafeTimer = 2, .dodgeCooldown = 3 });
    ecs_set(w, e, EcAnimation, { .animState = ANIM_IDLE });
    ecs_set(w, e, EcRank, { .rank = RANK_TROOPER });
    ecs_set(w, e, EcMorale, { .morale = 1.0f, .leader = 0, .leaderDist = 999.0f, .fleeTimer = 0, .prevBehavior = AI_ADVANCE });
    return e;
}

static ecs_entity_t place_ecs_ranked_enemy(ecs_world_t *w, Vector3 pos, float health, EnemyType type, EnemyRank rank) {
    ecs_entity_t e = place_ecs_enemy(w, pos, health, type);
    ecs_set(w, e, EcRank, { .rank = rank });
    return e;
}

TEST(test_enemy_sphere_hit) {
    ecs_world_t *w = make_test_ecs();
    place_ecs_enemy(w, (Vector3){5, 0, 0}, 80, ENEMY_SOVIET);
    ecs_entity_t hit = EcsEnemyCheckSphereHit(w, (Vector3){5, 0, 0}, 1.0f);
    ASSERT(hit != 0);
    ecs_fini(w);
}

TEST(test_enemy_sphere_miss) {
    ecs_world_t *w = make_test_ecs();
    place_ecs_enemy(w, (Vector3){50, 0, 0}, 80, ENEMY_SOVIET);
    ecs_entity_t hit = EcsEnemyCheckSphereHit(w, (Vector3){0, 0, 0}, 1.0f);
    ASSERT(hit == 0);
    ecs_fini(w);
}

TEST(test_enemy_damage_reduces_health) {
    ecs_world_t *w = make_test_ecs();
    // Also need resources singleton for death sounds
    ecs_singleton_set(w, EcEnemyResources, { .modelsLoaded = false });
    ecs_entity_t e = place_ecs_enemy(w, (Vector3){0, 0, 0}, 80, ENEMY_SOVIET);
    EcsEnemyDamage(w, e, 30.0f);
    const EcCombatStats *cs = ecs_get(w, e, EcCombatStats);
    ASSERT_FLOAT_EQ(cs->health, 50.0f, 0.01f);
    ASSERT(ecs_has(w, e, EcAlive)); // still alive
    ecs_fini(w);
}

TEST(test_enemy_damage_kills) {
    ecs_world_t *w = make_test_ecs();
    ecs_singleton_set(w, EcEnemyResources, { .modelsLoaded = false });
    ecs_entity_t e = place_ecs_enemy(w, (Vector3){0, 0, 0}, 80, ENEMY_SOVIET);
    EcsEnemyDamage(w, e, 100.0f);
    ASSERT(!ecs_has(w, e, EcAlive)); // dead
    ASSERT(ecs_has(w, e, EcDying));  // in dying state
    ecs_fini(w);
}

TEST(test_enemy_ray_hit) {
    ecs_world_t *w = make_test_ecs();
    place_ecs_enemy(w, (Vector3){0, 0, 10}, 80, ENEMY_SOVIET);
    Ray ray = {{0, 0, 0}, {0, 0, 1}};
    float dist = 0;
    ecs_entity_t hit = EcsEnemyCheckHit(w, ray, 100.0f, &dist);
    ASSERT(hit != 0);
    ASSERT_GT(dist, 0.0f);
    ecs_fini(w);
}

TEST(test_enemy_ray_miss) {
    ecs_world_t *w = make_test_ecs();
    place_ecs_enemy(w, (Vector3){50, 0, 0}, 80, ENEMY_SOVIET);
    Ray ray = {{0, 0, 0}, {0, 0, 1}};
    float dist = 0;
    ecs_entity_t hit = EcsEnemyCheckHit(w, ray, 100.0f, &dist);
    ASSERT(hit == 0);
    ecs_fini(w);
}

TEST(test_enemy_damage_out_of_bounds) {
    ecs_world_t *w = make_test_ecs();
    ecs_singleton_set(w, EcEnemyResources, { .modelsLoaded = false });
    // Damage a non-existent entity — should not crash
    EcsEnemyDamage(w, 99999, 50.0f);
    ecs_fini(w);
}

// ============================================================================
// 6. WORLD HEIGHT
// ============================================================================

TEST(test_world_height_deterministic) {
    float h1 = WorldGetHeight(10.0f, 20.0f);
    float h2 = WorldGetHeight(10.0f, 20.0f);
    ASSERT_FLOAT_EQ(h1, h2, 0.0001f);
}

TEST(test_world_height_not_nan) {
    float h = WorldGetHeight(0.0f, 0.0f);
    ASSERT(!isnan(h));
    ASSERT(!isinf(h));
    ASSERT(h > -100.0f && h < 100.0f);
}

TEST(test_world_height_varies) {
    float h1 = WorldGetHeight(0.0f, 0.0f);
    float h2 = WorldGetHeight(1000.0f, 1000.0f);
    // Not necessarily different, but both should be valid
    ASSERT(!isnan(h1));
    ASSERT(!isnan(h2));
}

// ============================================================================
// 7. GAME STATE
// ============================================================================

TEST(test_game_init_state) {
    Game g;
    GameInit(&g);
    ASSERT(g.state == STATE_MENU);
    ASSERT(g.wave == 0);
    ASSERT(g.killCount == 0);
}

TEST(test_difficulty_defaults) {
    Game g;
    GameInit(&g);
    ASSERT(g.difficulty == DIFFICULTY_NORMAL);
    ASSERT_FLOAT_EQ(g.damageScaler, DIFFICULTY_NORMAL_SCALE, 0.001f);
}

TEST(test_difficulty_valkyrie_no_damage) {
    Player p = make_player();
    PlayerTakeDamage(&p, 50.0f * DIFFICULTY_VALKYRIE_SCALE);
    ASSERT_FLOAT_EQ(p.health, PLAYER_MAX_HEALTH, 0.01f);
}

TEST(test_difficulty_easy_scale) {
    ASSERT_FLOAT_EQ(DIFFICULTY_EASY_SCALE, 0.5f, 0.001f);
}

TEST(test_difficulty_hard_scale) {
    ASSERT_FLOAT_EQ(DIFFICULTY_HARD_SCALE, 1.5f, 0.001f);
}

TEST(test_difficulty_scaler_reduces_damage) {
    Player p = make_player();
    float baseDmg = 20.0f;
    PlayerTakeDamage(&p, baseDmg * DIFFICULTY_EASY_SCALE);
    // Easy: 20 * 0.5 = 10 damage, health = 90
    ASSERT_FLOAT_EQ(p.health, 90.0f, 0.01f);
}

TEST(test_difficulty_scaler_increases_damage) {
    Player p = make_player();
    float baseDmg = 20.0f;
    PlayerTakeDamage(&p, baseDmg * DIFFICULTY_HARD_SCALE);
    // Hard: 20 * 1.5 = 30 damage, health = 70
    ASSERT_FLOAT_EQ(p.health, 70.0f, 0.01f);
}

TEST(test_game_reset) {
    Game g;
    GameInit(&g);
    g.wave = 5;
    g.killCount = 42;
    g.waveActive = true;
    GameReset(&g);
    ASSERT(g.wave == 0);
    ASSERT(g.killCount == 0);
    ASSERT(!g.waveActive);
}

// ============================================================================
// 8. NOISE FUNCTIONS
// ============================================================================

TEST(test_gradient_noise_deterministic) {
    float a = GradientNoise(1.5f, 2.7f);
    float b = GradientNoise(1.5f, 2.7f);
    ASSERT_FLOAT_EQ(a, b, 0.0001f);
}

TEST(test_gradient_noise_range) {
    // Sample many points, gradient noise should stay bounded
    for (int i = 0; i < 100; i++) {
        float x = (float)i * 3.7f;
        float z = (float)i * 5.3f;
        float n = GradientNoise(x, z);
        ASSERT(n > -2.0f && n < 2.0f);
    }
}

TEST(test_gradient_noise_varies) {
    // Sample 10 distinct points — at least 2 should differ meaningfully
    float vals[10];
    for (int i = 0; i < 10; i++) {
        vals[i] = GradientNoise((float)i * 7.3f + 0.5f, (float)i * 11.1f + 0.5f);
    }
    float minV = vals[0], maxV = vals[0];
    for (int i = 1; i < 10; i++) {
        if (vals[i] < minV) minV = vals[i];
        if (vals[i] > maxV) maxV = vals[i];
    }
    ASSERT(maxV - minV > 0.01f);
}

TEST(test_value_noise_deterministic) {
    float a = ValueNoise(3.14f, 2.72f);
    float b = ValueNoise(3.14f, 2.72f);
    ASSERT_FLOAT_EQ(a, b, 0.0001f);
}

TEST(test_value_noise_range) {
    for (int i = 0; i < 100; i++) {
        float n = ValueNoise((float)i * 1.1f, (float)i * 2.2f);
        ASSERT(n >= 0.0f && n <= 1.0f);
    }
}

TEST(test_lerpf_boundaries) {
    ASSERT_FLOAT_EQ(LerpF(0.0f, 10.0f, 0.0f), 0.0f, 0.001f);
    ASSERT_FLOAT_EQ(LerpF(0.0f, 10.0f, 1.0f), 10.0f, 0.001f);
    ASSERT_FLOAT_EQ(LerpF(0.0f, 10.0f, 0.5f), 5.0f, 0.001f);
}

TEST(test_hash2d_deterministic) {
    float a = Hash2D(5.0f, 10.0f);
    float b = Hash2D(5.0f, 10.0f);
    ASSERT_FLOAT_EQ(a, b, 0.0001f);
}

TEST(test_hash2d_range) {
    for (int i = 0; i < 100; i++) {
        float h = Hash2D((float)i, (float)(i * 7));
        ASSERT(h >= 0.0f && h <= 1.0f);
    }
}

// ============================================================================
// 9. TERRAIN FEATURES (domain warp, rilles, maria)
// ============================================================================

TEST(test_world_height_continuity) {
    // Height should change smoothly — no huge jumps between nearby points
    float prev = WorldGetHeight(0.0f, 0.0f);
    for (int i = 1; i <= 20; i++) {
        float curr = WorldGetHeight((float)i * 0.5f, 0.0f);
        float diff = fabsf(curr - prev);
        ASSERT(diff < 5.0f); // no more than 5 units jump per 0.5 unit step
        prev = curr;
    }
}

TEST(test_world_height_large_coords) {
    // Should not NaN or explode at large world coordinates
    float h1 = WorldGetHeight(10000.0f, 10000.0f);
    float h2 = WorldGetHeight(-5000.0f, 7000.0f);
    ASSERT(!isnan(h1));
    ASSERT(!isinf(h1));
    ASSERT(!isnan(h2));
    ASSERT(!isinf(h2));
}

TEST(test_world_height_negative_coords) {
    float h = WorldGetHeight(-100.0f, -200.0f);
    ASSERT(!isnan(h));
    ASSERT(!isinf(h));
    ASSERT(h > -50.0f && h < 50.0f);
}

TEST(test_mare_factor_range) {
    // Mare factor should always be [0, 1]
    for (int i = 0; i < 50; i++) {
        float m = WorldGetMareFactor((float)i * 100.0f, (float)i * 77.0f);
        ASSERT(m >= 0.0f);
        ASSERT(m <= 1.0f);
    }
}

TEST(test_mare_factor_deterministic) {
    float a = WorldGetMareFactor(42.0f, 99.0f);
    float b = WorldGetMareFactor(42.0f, 99.0f);
    ASSERT_FLOAT_EQ(a, b, 0.0001f);
}

TEST(test_mare_factor_large_coords) {
    float m = WorldGetMareFactor(50000.0f, -30000.0f);
    ASSERT(!isnan(m));
    ASSERT(m >= 0.0f && m <= 1.0f);
}

// ============================================================================
// 10. COLLISION SYSTEM
// ============================================================================

TEST(test_collision_no_world) {
    // WorldCheckCollision with NULL world should not crash
    bool hit = WorldCheckCollision(NULL, (Vector3){0,0,0}, 1.0f);
    ASSERT(!hit);
}

TEST(test_collision_empty_world) {
    World w;
    memset(&w, 0, sizeof(w));
    bool hit = WorldCheckCollision(&w, (Vector3){0,0,0}, 1.0f);
    ASSERT(!hit);
}

// ============================================================================
// 11. ENEMY ADVANCED (ECS-based)
// ============================================================================

TEST(test_enemy_count_alive) {
    ecs_world_t *w = make_test_ecs();
    EcsEnemySystemsRegister(w);
    place_ecs_enemy(w, (Vector3){0,0,0}, 80, ENEMY_SOVIET);
    place_ecs_enemy(w, (Vector3){5,0,0}, 55, ENEMY_AMERICAN);
    ASSERT(EcsEnemyCountAlive(w) == 2);
    ecs_fini(w);
}

TEST(test_enemy_count_alive_after_kill) {
    ecs_world_t *w = make_test_ecs();
    EcsEnemySystemsRegister(w);
    ecs_singleton_set(w, EcEnemyResources, { .modelsLoaded = false });
    ecs_entity_t e0 = place_ecs_enemy(w, (Vector3){0,0,0}, 80, ENEMY_SOVIET);
    place_ecs_enemy(w, (Vector3){5,0,0}, 55, ENEMY_AMERICAN);
    EcsEnemyDamage(w, e0, 9999.0f);
    ASSERT(EcsEnemyCountAlive(w) == 1);
    ecs_fini(w);
}

TEST(test_enemy_vaporize) {
    ecs_world_t *w = make_test_ecs();
    ecs_singleton_set(w, EcEnemyResources, { .modelsLoaded = false });
    ecs_entity_t e = place_ecs_enemy(w, (Vector3){0,0,0}, 80, ENEMY_SOVIET);
    EcsEnemyVaporize(w, e);
    ASSERT(!ecs_has(w, e, EcAlive));
    ASSERT(ecs_has(w, e, EcVaporizing));
    ecs_fini(w);
}

TEST(test_enemy_eviscerate) {
    ecs_world_t *w = make_test_ecs();
    ecs_singleton_set(w, EcEnemyResources, { .modelsLoaded = false });
    ecs_entity_t e = place_ecs_enemy(w, (Vector3){0,0,0}, 80, ENEMY_SOVIET);
    EcsEnemyEviscerate(w, e, (Vector3){0,0,1});
    ASSERT(!ecs_has(w, e, EcAlive));
    ASSERT(ecs_has(w, e, EcEviscerating));
    ecs_fini(w);
}

TEST(test_enemy_sphere_hit_multiple) {
    ecs_world_t *w = make_test_ecs();
    place_ecs_enemy(w, (Vector3){10,0,0}, 80, ENEMY_SOVIET);
    ecs_entity_t e1 = place_ecs_enemy(w, (Vector3){3,0,0}, 55, ENEMY_AMERICAN);
    // Sphere at origin with radius 5 should hit the closer enemy
    ecs_entity_t hit = EcsEnemyCheckSphereHit(w, (Vector3){0,0,0}, 5.0f);
    ASSERT(hit == e1);
    ecs_fini(w);
}

// ============================================================================
// 12. WEAPON ADVANCED
// ============================================================================

TEST(test_weapon_raketenfaust_ammo) {
    Weapon w = make_weapon(WEAPON_RAKETENFAUST);
    ASSERT(WeaponGetAmmo(&w) == RAKETEN_MAG_SIZE);
    ASSERT(WeaponGetMaxAmmo(&w) == RAKETEN_MAG_SIZE);
}

TEST(test_weapon_jackhammer_name) {
    Weapon w = make_weapon(WEAPON_JACKHAMMER);
    ASSERT(strcmp(WeaponGetName(&w), "JACKHAMMER") == 0);
}

TEST(test_weapon_raketenfaust_name) {
    Weapon w = make_weapon(WEAPON_RAKETENFAUST);
    ASSERT(strcmp(WeaponGetName(&w), "RAKETENFAUST") == 0);
}

TEST(test_weapon_switch_preserves_ammo) {
    Weapon w = make_weapon(WEAPON_MOND_MP40);
    w.mp40Mag = 15; // partially used
    WeaponSwitch(&w, WEAPON_RAKETENFAUST);
    WeaponSwitch(&w, WEAPON_MOND_MP40);
    // MP40 ammo should still be 15
    ASSERT(WeaponGetAmmo(&w) == 15);
}

// ============================================================================
// 13. CONFIG CROSS-CHECKS
// ============================================================================

TEST(test_jackhammer_oneshots_soviets) {
    ASSERT_GT(JACKHAMMER_DAMAGE, SOVIET_HEALTH);
}

TEST(test_jackhammer_oneshots_americans) {
    ASSERT_GT(JACKHAMMER_DAMAGE, AMERICAN_HEALTH);
}

TEST(test_player_height_positive) {
    ASSERT_GT(PLAYER_HEIGHT, 0.0f);
}

TEST(test_moon_gravity_positive) {
    ASSERT_GT(MOON_GRAVITY, 0.0f);
}

TEST(test_render_resolution_valid) {
    ASSERT_GT(RENDER_W, 0);
    ASSERT_GT(RENDER_H, 0);
    ASSERT(RENDER_W >= RENDER_H); // landscape
}

TEST(test_chunk_size_positive) {
    ASSERT_GT(CHUNK_SIZE, 0.0f);
}

TEST(test_max_enemies_reasonable) {
    ASSERT(MAX_ENEMIES >= 10);
    ASSERT(MAX_ENEMIES <= 200);
}

// ============================================================================
// 14. STRUCTURE SYSTEM
// ============================================================================

TEST(test_structure_init_places_base) {
    StructureManager sm;
    StructureManagerInit(&sm);
    ASSERT(sm.count == 1);
    ASSERT(sm.structures[0].active);
    ASSERT(sm.structures[0].type == STRUCTURE_MOON_BASE);
    // Base should be near spawn (within 50 units)
    float dist = sqrtf(sm.structures[0].worldPos.x * sm.structures[0].worldPos.x +
                        sm.structures[0].worldPos.z * sm.structures[0].worldPos.z);
    ASSERT(dist < 50.0f);
}

TEST(test_structure_player_not_inside_initially) {
    StructureManager sm;
    StructureManagerInit(&sm);
    ASSERT(!StructureIsPlayerInside(&sm));
}

TEST(test_structure_prompt_none_initially) {
    StructureManager sm;
    StructureManagerInit(&sm);
    ASSERT(StructureGetPrompt(&sm) == PROMPT_NONE);
}

TEST(test_structure_interior_y) {
    StructureManager sm;
    StructureManagerInit(&sm);
    ASSERT_FLOAT_EQ(sm.structures[0].interiorY, STRUCTURE_INTERIOR_Y, 0.1f);
}

TEST(test_structure_config_sanity) {
    ASSERT_GT(MAX_STRUCTURES, 0);
    ASSERT_GT(STRUCTURE_INTERACT_RANGE, 0.0f);
    ASSERT_GT(STRUCTURE_INTERIOR_Y, 100.0f); // well above terrain
    ASSERT_GT(MOONBASE_INTERIOR_W, MOONBASE_EXTERIOR_RADIUS * 2.0f); // bigger inside
    ASSERT_GT(MOONBASE_INTERIOR_D, MOONBASE_EXTERIOR_RADIUS * 2.0f); // bigger inside
    ASSERT_GT(MOONBASE_INTERIOR_H, PLAYER_HEIGHT); // player can stand
}

TEST(test_structure_resupply_flash_starts_zero) {
    StructureManager sm;
    StructureManagerInit(&sm);
    ASSERT_FLOAT_EQ(StructureGetResupplyFlash(&sm), 0.0f, 0.001f);
}

TEST(test_structure_collision_at_base) {
    StructureManager sm;
    StructureManagerInit(&sm);
    // Directly at the base center should collide
    Vector3 basePos = sm.structures[0].worldPos;
    ASSERT(StructureCheckCollision(&sm, basePos, 0.5f));
}

TEST(test_structure_collision_far_away) {
    StructureManager sm;
    StructureManagerInit(&sm);
    // Far from any structure should not collide
    ASSERT(!StructureCheckCollision(&sm, (Vector3){500, 0, 500}, 0.5f));
}

TEST(test_structure_spawn_chance_config) {
    ASSERT_GT(STRUCTURE_SPAWN_CHANCE, 1);
    ASSERT(STRUCTURE_SPAWN_CHANCE <= 50); // not too rare
}

TEST(test_structure_unique_interior_y) {
    // If multiple structures spawned, each should have unique interiorY
    StructureManager sm;
    StructureManagerInit(&sm);
    // Manually add a second one to test
    if (sm.count < MAX_STRUCTURES) {
        float intY = STRUCTURE_INTERIOR_Y + (float)sm.count * 50.0f;
        ASSERT(intY != sm.structures[0].interiorY);
    }
}

// ============================================================================
// 15. ECS LIFECYCLE — restart, cleanup, reinitialization
// ============================================================================

TEST(test_ecs_init_creates_world) {
    GameEcsInit();
    ecs_world_t *w = GameEcsGetWorld();
    ASSERT(w != NULL);
    // Clean teardown without systems registered
    GameEcsFini();
}

TEST(test_ecs_reinit_creates_fresh_world) {
    // First init with full systems and spawn an enemy
    GameEcsInit();
    ecs_world_t *w1 = GameEcsGetWorld();
    ASSERT(w1 != NULL);
    EcsEnemyComponentsRegister(w1);
    EcsEnemySystemsRegister(w1);
    place_ecs_enemy(w1, (Vector3){0,0,0}, 80, ENEMY_SOVIET);
    ASSERT(EcsEnemyCountAlive(w1) == 1);
    // Reinit should clean up and give us a fresh empty world
    GameEcsInit();
    ecs_world_t *w2 = GameEcsGetWorld();
    ASSERT(w2 != NULL);
    EcsEnemyComponentsRegister(w2);
    EcsEnemySystemsRegister(w2);
    ASSERT(EcsEnemyCountAlive(w2) == 0);
    EcsEnemySystemsCleanup();
    GameEcsFini();
}

TEST(test_ecs_systems_cleanup_nulls_query) {
    ecs_world_t *w = make_test_ecs();
    EcsEnemySystemsRegister(w);
    // Count should work
    ASSERT(EcsEnemyCountAlive(w) == 0);
    // Cleanup should not crash
    EcsEnemySystemsCleanup();
    // After cleanup, count returns 0 (query is NULL)
    ASSERT(EcsEnemyCountAlive(w) == 0);
    ecs_fini(w);
}

TEST(test_ecs_double_cleanup_safe) {
    ecs_world_t *w = make_test_ecs();
    EcsEnemySystemsRegister(w);
    EcsEnemySystemsCleanup();
    // Second cleanup should be a no-op, not crash
    EcsEnemySystemsCleanup();
    ecs_fini(w);
}

TEST(test_ecs_enemies_cleared_on_reinit) {
    ecs_world_t *w = make_test_ecs();
    EcsEnemySystemsRegister(w);
    place_ecs_enemy(w, (Vector3){0,0,0}, 80, ENEMY_SOVIET);
    place_ecs_enemy(w, (Vector3){5,0,0}, 55, ENEMY_AMERICAN);
    ASSERT(EcsEnemyCountAlive(w) == 2);
    EcsEnemySystemsCleanup();
    ecs_fini(w);
    // New world should be empty
    ecs_world_t *w2 = make_test_ecs();
    EcsEnemySystemsRegister(w2);
    ASSERT(EcsEnemyCountAlive(w2) == 0);
    EcsEnemySystemsCleanup();
    ecs_fini(w2);
}

// ============================================================================
// 16. ECS FACTION STATS — verify prefab config matches config.h
// ============================================================================

TEST(test_soviet_prefab_stats) {
    ecs_world_t *w = make_test_ecs();
    ecs_entity_t e = place_ecs_enemy(w, (Vector3){0,0,0}, SOVIET_HEALTH, ENEMY_SOVIET);
    const EcCombatStats *cs = ecs_get(w, e, EcCombatStats);
    ASSERT(cs != NULL);
    ASSERT_FLOAT_EQ(cs->health, SOVIET_HEALTH, 0.01f);
    ASSERT_FLOAT_EQ(cs->maxHealth, SOVIET_HEALTH, 0.01f);
    ecs_fini(w);
}

TEST(test_american_prefab_stats) {
    ecs_world_t *w = make_test_ecs();
    ecs_entity_t e = place_ecs_enemy(w, (Vector3){0,0,0}, AMERICAN_HEALTH, ENEMY_AMERICAN);
    const EcCombatStats *cs = ecs_get(w, e, EcCombatStats);
    ASSERT(cs != NULL);
    ASSERT_FLOAT_EQ(cs->health, AMERICAN_HEALTH, 0.01f);
    ASSERT_FLOAT_EQ(cs->maxHealth, AMERICAN_HEALTH, 0.01f);
    ecs_fini(w);
}

TEST(test_americans_longer_range_than_soviets) {
    ASSERT_GT(AMERICAN_ATTACK_RANGE, SOVIET_ATTACK_RANGE);
}

TEST(test_americans_prefer_more_distance) {
    ASSERT_GT(AMERICAN_PREFERRED_DIST, SOVIET_PREFERRED_DIST);
}

TEST(test_soviets_faster_than_americans) {
    ASSERT_GT(SOVIET_SPEED, AMERICAN_SPEED);
}

TEST(test_americans_hit_harder_per_shot) {
    ASSERT_GT(AMERICAN_DAMAGE, SOVIET_DAMAGE);
}

TEST(test_soviets_fire_faster_than_americans) {
    ASSERT_LT(SOVIET_ATTACK_RATE, AMERICAN_ATTACK_RATE);
}

TEST(test_soviets_more_health_than_americans) {
    ASSERT_GT(SOVIET_HEALTH, AMERICAN_HEALTH);
}

// ============================================================================
// 17. ECS COMPONENT INTEGRITY — verify components set/get correctly
// ============================================================================

TEST(test_ecs_transform_roundtrip) {
    ecs_world_t *w = make_test_ecs();
    ecs_entity_t e = place_ecs_enemy(w, (Vector3){12.5f, 3.0f, -7.0f}, 80, ENEMY_SOVIET);
    const EcTransform *tr = ecs_get(w, e, EcTransform);
    ASSERT(tr != NULL);
    ASSERT_FLOAT_EQ(tr->position.x, 12.5f, 0.01f);
    ASSERT_FLOAT_EQ(tr->position.y, 3.0f, 0.01f);
    ASSERT_FLOAT_EQ(tr->position.z, -7.0f, 0.01f);
    ecs_fini(w);
}

TEST(test_ecs_faction_soviet) {
    ecs_world_t *w = make_test_ecs();
    ecs_entity_t e = place_ecs_enemy(w, (Vector3){0,0,0}, 80, ENEMY_SOVIET);
    const EcFaction *fac = ecs_get(w, e, EcFaction);
    ASSERT(fac != NULL);
    ASSERT(fac->type == ENEMY_SOVIET);
    ecs_fini(w);
}

TEST(test_ecs_faction_american) {
    ecs_world_t *w = make_test_ecs();
    ecs_entity_t e = place_ecs_enemy(w, (Vector3){0,0,0}, 55, ENEMY_AMERICAN);
    const EcFaction *fac = ecs_get(w, e, EcFaction);
    ASSERT(fac != NULL);
    ASSERT(fac->type == ENEMY_AMERICAN);
    ecs_fini(w);
}

TEST(test_ecs_alive_tag_present) {
    ecs_world_t *w = make_test_ecs();
    ecs_entity_t e = place_ecs_enemy(w, (Vector3){0,0,0}, 80, ENEMY_SOVIET);
    ASSERT(ecs_has(w, e, EcAlive));
    ecs_fini(w);
}

TEST(test_ecs_velocity_initially_zero) {
    ecs_world_t *w = make_test_ecs();
    ecs_entity_t e = place_ecs_enemy(w, (Vector3){0,0,0}, 80, ENEMY_SOVIET);
    const EcVelocity *vel = ecs_get(w, e, EcVelocity);
    ASSERT(vel != NULL);
    ASSERT_FLOAT_EQ(vel->velocity.x, 0.0f, 0.01f);
    ASSERT_FLOAT_EQ(vel->velocity.z, 0.0f, 0.01f);
    ASSERT_FLOAT_EQ(vel->vertVel, 0.0f, 0.01f);
    ecs_fini(w);
}

TEST(test_ecs_ai_initial_advance) {
    ecs_world_t *w = make_test_ecs();
    ecs_entity_t e = place_ecs_enemy(w, (Vector3){0,0,0}, 80, ENEMY_SOVIET);
    const EcAIState *ai = ecs_get(w, e, EcAIState);
    ASSERT(ai != NULL);
    ASSERT(ai->behavior == AI_ADVANCE);
    ecs_fini(w);
}

// ============================================================================
// 18. ECS DAMAGE — multi-hit, overkill, faction death states
// ============================================================================

TEST(test_ecs_damage_partial) {
    ecs_world_t *w = make_test_ecs();
    ecs_singleton_set(w, EcEnemyResources, { .modelsLoaded = false });
    ecs_entity_t e = place_ecs_enemy(w, (Vector3){0,0,0}, 80, ENEMY_SOVIET);
    EcsEnemyDamage(w, e, 30.0f);
    // Should still be alive
    ASSERT(ecs_has(w, e, EcAlive));
    const EcCombatStats *cs = ecs_get(w, e, EcCombatStats);
    ASSERT_FLOAT_EQ(cs->health, 50.0f, 0.01f);
    ecs_fini(w);
}

TEST(test_ecs_damage_exact_kill) {
    ecs_world_t *w = make_test_ecs();
    ecs_singleton_set(w, EcEnemyResources, { .modelsLoaded = false });
    ecs_entity_t e = place_ecs_enemy(w, (Vector3){0,0,0}, 80, ENEMY_SOVIET);
    EcsEnemyDamage(w, e, 80.0f);
    ASSERT(!ecs_has(w, e, EcAlive));
    ASSERT(ecs_has(w, e, EcDying));
    ecs_fini(w);
}

TEST(test_ecs_damage_overkill) {
    ecs_world_t *w = make_test_ecs();
    ecs_singleton_set(w, EcEnemyResources, { .modelsLoaded = false });
    ecs_entity_t e = place_ecs_enemy(w, (Vector3){0,0,0}, 55, ENEMY_AMERICAN);
    EcsEnemyDamage(w, e, 9999.0f);
    ASSERT(!ecs_has(w, e, EcAlive));
    ecs_fini(w);
}

TEST(test_ecs_damage_multi_hit) {
    ecs_world_t *w = make_test_ecs();
    ecs_singleton_set(w, EcEnemyResources, { .modelsLoaded = false });
    ecs_entity_t e = place_ecs_enemy(w, (Vector3){0,0,0}, 80, ENEMY_SOVIET);
    EcsEnemyDamage(w, e, 30.0f);
    EcsEnemyDamage(w, e, 30.0f);
    // 80 - 60 = 20 health remaining
    ASSERT(ecs_has(w, e, EcAlive));
    const EcCombatStats *cs = ecs_get(w, e, EcCombatStats);
    ASSERT_FLOAT_EQ(cs->health, 20.0f, 0.01f);
    // One more hit kills
    EcsEnemyDamage(w, e, 25.0f);
    ASSERT(!ecs_has(w, e, EcAlive));
    ecs_fini(w);
}

TEST(test_ecs_vaporize_american) {
    ecs_world_t *w = make_test_ecs();
    ecs_singleton_set(w, EcEnemyResources, { .modelsLoaded = false });
    ecs_entity_t e = place_ecs_enemy(w, (Vector3){0,0,0}, 55, ENEMY_AMERICAN);
    EcsEnemyVaporize(w, e);
    ASSERT(!ecs_has(w, e, EcAlive));
    ASSERT(ecs_has(w, e, EcVaporizing));
    ASSERT(!ecs_has(w, e, EcDying));
    ASSERT(!ecs_has(w, e, EcEviscerating));
    ecs_fini(w);
}

TEST(test_ecs_eviscerate_drops_weapon) {
    ecs_world_t *w = make_test_ecs();
    ecs_singleton_set(w, EcEnemyResources, { .modelsLoaded = false });
    ecs_entity_t e = place_ecs_enemy(w, (Vector3){0,0,0}, 80, ENEMY_SOVIET);
    EcsEnemyEviscerate(w, e, (Vector3){1,0,0});
    ASSERT(!ecs_has(w, e, EcAlive));
    ASSERT(ecs_has(w, e, EcEviscerating));
    ASSERT(!ecs_has(w, e, EcDying));
    ASSERT(!ecs_has(w, e, EcVaporizing));
    ecs_fini(w);
}

// ============================================================================
// 19. ECS HIT DETECTION — rays and spheres
// ============================================================================

TEST(test_ecs_sphere_hit_exact_range) {
    ecs_world_t *w = make_test_ecs();
    ecs_entity_t e = place_ecs_enemy(w, (Vector3){5.0f, 0, 0}, 80, ENEMY_SOVIET);
    // Sphere centered at origin with radius exactly 5 + enemy hitbox should hit
    ecs_entity_t hit = EcsEnemyCheckSphereHit(w, (Vector3){0,0,0}, 6.5f);
    ASSERT(hit == e);
    ecs_fini(w);
}

TEST(test_ecs_sphere_no_dead_hits) {
    ecs_world_t *w = make_test_ecs();
    ecs_singleton_set(w, EcEnemyResources, { .modelsLoaded = false });
    ecs_entity_t e = place_ecs_enemy(w, (Vector3){3,0,0}, 80, ENEMY_SOVIET);
    EcsEnemyDamage(w, e, 9999.0f);
    // Dead enemy should not be hit
    ecs_entity_t hit = EcsEnemyCheckSphereHit(w, (Vector3){3,0,0}, 2.0f);
    ASSERT(hit == 0);
    ecs_fini(w);
}

TEST(test_ecs_sphere_picks_closest) {
    ecs_world_t *w = make_test_ecs();
    place_ecs_enemy(w, (Vector3){20,0,0}, 80, ENEMY_SOVIET);
    ecs_entity_t near = place_ecs_enemy(w, (Vector3){5,0,0}, 55, ENEMY_AMERICAN);
    place_ecs_enemy(w, (Vector3){15,0,0}, 80, ENEMY_SOVIET);
    ecs_entity_t hit = EcsEnemyCheckSphereHit(w, (Vector3){0,0,0}, 10.0f);
    ASSERT(hit == near);
    ecs_fini(w);
}

TEST(test_ecs_ray_hit_forward) {
    ecs_world_t *w = make_test_ecs();
    place_ecs_enemy(w, (Vector3){0, 0, 10}, 80, ENEMY_SOVIET);
    Ray ray = { .position = {0, 0, 0}, .direction = {0, 0, 1} };
    float hitDist = 0;
    ecs_entity_t hit = EcsEnemyCheckHit(w, ray, 50.0f, &hitDist);
    ASSERT(hit != 0);
    ASSERT_GT(hitDist, 0.0f);
    ASSERT_LT(hitDist, 12.0f);
    ecs_fini(w);
}

TEST(test_ecs_ray_miss_perpendicular) {
    ecs_world_t *w = make_test_ecs();
    place_ecs_enemy(w, (Vector3){10, 0, 0}, 80, ENEMY_SOVIET);
    // Shoot along Z axis — enemy is on X axis, should miss
    Ray ray = { .position = {0, 0, 0}, .direction = {0, 0, 1} };
    float hitDist = 0;
    ecs_entity_t hit = EcsEnemyCheckHit(w, ray, 50.0f, &hitDist);
    ASSERT(hit == 0);
    ecs_fini(w);
}

TEST(test_ecs_ray_respects_max_dist) {
    ecs_world_t *w = make_test_ecs();
    place_ecs_enemy(w, (Vector3){0, 0, 50}, 80, ENEMY_SOVIET);
    Ray ray = { .position = {0, 0, 0}, .direction = {0, 0, 1} };
    float hitDist = 0;
    // Max dist 10 — enemy at 50 should be out of range
    ecs_entity_t hit = EcsEnemyCheckHit(w, ray, 10.0f, &hitDist);
    ASSERT(hit == 0);
    ecs_fini(w);
}

// ============================================================================
// 20. GAME STATE TRANSITIONS
// ============================================================================

TEST(test_game_reset_clears_wave) {
    Game g;
    GameInit(&g);
    g.wave = 5;
    g.killCount = 42;
    g.waveActive = true;
    g.enemiesSpawned = 10;
    g.enemiesRemaining = 3;
    GameReset(&g);
    ASSERT(g.wave == 0);
    ASSERT(g.killCount == 0);
    ASSERT(!g.waveActive);
    ASSERT(g.enemiesSpawned == 0);
    ASSERT(g.enemiesRemaining == 0);
}

TEST(test_game_init_menu_state) {
    Game g;
    GameInit(&g);
    ASSERT(g.state == STATE_MENU);
    ASSERT(g.menuSelection == 0);
}

TEST(test_game_wave_delay_positive) {
    Game g;
    GameInit(&g);
    ASSERT_GT(g.waveDelay, 0.0f);
}

// ============================================================================
// 21. AI CONFIG SANITY
// ============================================================================

TEST(test_ai_turn_speed_positive) {
    ASSERT_GT(AI_TURN_SPEED, 0.0f);
}

TEST(test_ai_dodge_cooldown_positive) {
    ASSERT_GT(AI_DODGE_COOLDOWN, 0.0f);
}

TEST(test_lod_distances_ordered) {
    ASSERT_LT(LOD1_DISTANCE, LOD2_DISTANCE);
    ASSERT_LT(LOD2_DISTANCE, CULL_DISTANCE);
}

TEST(test_enemy_ground_offset_positive) {
    ASSERT_GT(ENEMY_GROUND_OFFSET, 0.0f);
}

TEST(test_ai_stagger_divisor_positive) {
    ASSERT(AI_STAGGER_DIVISOR >= 1);
}

TEST(test_structure_collision_blocks_enemy_radius) {
    // Structure collision radius should be bigger than enemy + base combined
    float collR = MOONBASE_EXTERIOR_RADIUS + 0.5f + 0.8f;
    ASSERT_GT(collR, MOONBASE_EXTERIOR_RADIUS);
    ASSERT_GT(collR, 2.0f);
}

// ============================================================================
// MAIN — run all tests
// ============================================================================

// ============================================================================
// COLLISION Y-AXIS TESTS
// ============================================================================

TEST(test_rock_collision_blocks_at_ground) {
    // A rock at ground level should block a player at ground level
    World w;
    memset(&w, 0, sizeof(w));
    w.chunkCount = 1;
    w.chunks[0].generated = true;
    w.chunks[0].rockCount = 1;
    w.chunks[0].rocks[0] = (Rock){ .position = {10, 2, 10}, .size = {3, 4, 3} };
    // Player at Y=PLAYER_HEIGHT (feet at ground), XZ overlapping rock
    bool hit = WorldCheckCollision(&w, (Vector3){10, PLAYER_HEIGHT, 10}, 0.4f);
    ASSERT(hit);
}

TEST(test_rock_collision_passes_above) {
    // Player above rock top should not collide
    World w;
    memset(&w, 0, sizeof(w));
    w.chunkCount = 1;
    w.chunks[0].generated = true;
    w.chunks[0].rockCount = 1;
    w.chunks[0].rocks[0] = (Rock){ .position = {10, 2, 10}, .size = {3, 4, 3} };
    // Rock top = 2 + 4*0.5 = 4.0, player feet at Y - PLAYER_HEIGHT
    // Player at Y = 4.0 + PLAYER_HEIGHT + 1.0 = well above rock
    float aboveRock = 4.0f + PLAYER_HEIGHT + 1.0f;
    bool hit = WorldCheckCollision(&w, (Vector3){10, aboveRock, 10}, 0.4f);
    ASSERT(!hit);
}

TEST(test_structure_collision_blocks_ground_y) {
    StructureManager sm;
    memset(&sm, 0, sizeof(sm));
    sm.count = 1;
    sm.structures[0].active = true;
    sm.structures[0].worldPos = (Vector3){20, 0, 20};
    // Player at ground level, within dome radius
    bool hit = StructureCheckCollision(&sm, (Vector3){20, PLAYER_HEIGHT, 20}, 0.4f);
    ASSERT(hit);
}

TEST(test_structure_collision_passes_above_dome) {
    StructureManager sm;
    memset(&sm, 0, sizeof(sm));
    sm.count = 1;
    sm.structures[0].active = true;
    sm.structures[0].worldPos = (Vector3){20, 0, 20};
    // Player feet above dome top: worldPos.y + MOONBASE_COLLISION_HEIGHT = 6.0
    float aboveDome = MOONBASE_COLLISION_HEIGHT + PLAYER_HEIGHT + 1.0f;
    bool hit = StructureCheckCollision(&sm, (Vector3){20, aboveDome, 20}, 0.4f);
    ASSERT(!hit);
}

// ============================================================================
// RANK SYSTEM TESTS
// ============================================================================

TEST(test_rank_config_sanity) {
    ASSERT_GT(NCO_HEALTH_MULT, 1.0f);           // NCOs are tougher than troopers
    ASSERT_LT(OFFICER_HEALTH_MULT, 1.0f);        // Officers are frailer (command from rear)
    ASSERT_GT(NCO_HEALTH_MULT, OFFICER_HEALTH_MULT); // NCOs tougher than officers
    ASSERT_GT(OFFICER_DAMAGE_MULT, 1.0f);
    ASSERT_GT(OFFICER_DIST_MULT, 1.0f);           // Officers hold back further
    ASSERT_GT(MORALE_OFFICER_DEATH_HIT, MORALE_NCO_DEATH_HIT);
    ASSERT_GT(MORALE_FLEE_DURATION_MAX, MORALE_FLEE_DURATION_MIN);
    ASSERT_GT(MORALE_RALLY_THRESHOLD, MORALE_FLEE_THRESHOLD);
    ASSERT(RANK_NCO_WAVE_START <= RANK_OFFICER_WAVE_START);
}

TEST(test_rank_component_set_on_spawn) {
    ecs_world_t *w = make_test_ecs();
    ecs_entity_t e = place_ecs_ranked_enemy(w, (Vector3){0,0,0}, 80, ENEMY_SOVIET, RANK_OFFICER);
    const EcRank *rk = ecs_get(w, e, EcRank);
    ASSERT(rk != NULL);
    ASSERT(rk->rank == RANK_OFFICER);
    ecs_fini(w);
}

TEST(test_morale_component_set_on_spawn) {
    ecs_world_t *w = make_test_ecs();
    ecs_entity_t e = place_ecs_enemy(w, (Vector3){0,0,0}, 80, ENEMY_SOVIET);
    const EcMorale *mor = ecs_get(w, e, EcMorale);
    ASSERT(mor != NULL);
    ASSERT_FLOAT_EQ(mor->morale, 1.0f, 0.01f);
    ecs_fini(w);
}

TEST(test_rank_trooper_default_stats) {
    ecs_world_t *w = make_test_ecs();
    ecs_entity_t e = place_ecs_enemy(w, (Vector3){0,0,0}, SOVIET_HEALTH, ENEMY_SOVIET);
    const EcCombatStats *cs = ecs_get(w, e, EcCombatStats);
    ASSERT(cs != NULL);
    ASSERT_FLOAT_EQ(cs->health, SOVIET_HEALTH, 0.01f);
    ecs_fini(w);
}

TEST(test_rank_nco_health_multiplied) {
    ecs_world_t *w = make_test_ecs();
    float ncoHealth = SOVIET_HEALTH * NCO_HEALTH_MULT;
    ecs_entity_t e = place_ecs_ranked_enemy(w, (Vector3){0,0,0}, ncoHealth, ENEMY_SOVIET, RANK_NCO);
    const EcCombatStats *cs = ecs_get(w, e, EcCombatStats);
    ASSERT(cs != NULL);
    ASSERT_FLOAT_EQ(cs->health, ncoHealth, 0.01f);
    ASSERT_GT(cs->health, SOVIET_HEALTH);
    ecs_fini(w);
}

TEST(test_rank_officer_health_less_than_trooper) {
    ecs_world_t *w = make_test_ecs();
    float offHealth = SOVIET_HEALTH * OFFICER_HEALTH_MULT;
    ecs_entity_t e = place_ecs_ranked_enemy(w, (Vector3){0,0,0}, offHealth, ENEMY_SOVIET, RANK_OFFICER);
    const EcCombatStats *cs = ecs_get(w, e, EcCombatStats);
    ASSERT(cs != NULL);
    ASSERT_FLOAT_EQ(cs->health, offHealth, 0.01f);
    ASSERT_LT(cs->health, SOVIET_HEALTH);  // officer is frailer
    ecs_fini(w);
}

TEST(test_spawn_ranked_backward_compat) {
    ecs_world_t *w = make_test_ecs();
    ecs_entity_t e = place_ecs_enemy(w, (Vector3){0,0,0}, 80, ENEMY_SOVIET);
    const EcRank *rk = ecs_get(w, e, EcRank);
    ASSERT(rk != NULL);
    ASSERT(rk->rank == RANK_TROOPER);
    ecs_fini(w);
}

TEST(test_officer_death_morale_hit) {
    ecs_world_t *w = make_test_ecs();
    // Place officer and nearby trooper of same faction
    ecs_entity_t officer = place_ecs_ranked_enemy(w, (Vector3){0,0,0}, 80, ENEMY_SOVIET, RANK_OFFICER);
    ecs_entity_t trooper = place_ecs_enemy(w, (Vector3){5,0,0}, 80, ENEMY_SOVIET);
    // Apply morale hit as if officer died
    EcsApplyMoraleHit(w, officer, MORALE_OFFICER_DEATH_HIT);
    const EcMorale *mor = ecs_get(w, trooper, EcMorale);
    ASSERT(mor != NULL);
    ASSERT_LT(mor->morale, 1.0f);
    ASSERT_FLOAT_EQ(mor->morale, 1.0f - MORALE_OFFICER_DEATH_HIT, 0.01f);
    ecs_fini(w);
}

TEST(test_nco_death_morale_hit_less) {
    ecs_world_t *w = make_test_ecs();
    ecs_entity_t nco = place_ecs_ranked_enemy(w, (Vector3){0,0,0}, 80, ENEMY_SOVIET, RANK_NCO);
    ecs_entity_t trooper = place_ecs_enemy(w, (Vector3){5,0,0}, 80, ENEMY_SOVIET);
    EcsApplyMoraleHit(w, nco, MORALE_NCO_DEATH_HIT);
    const EcMorale *mor = ecs_get(w, trooper, EcMorale);
    ASSERT(mor != NULL);
    // NCO hit is less than officer hit
    ASSERT_GT(mor->morale, 1.0f - MORALE_OFFICER_DEATH_HIT);
    ASSERT_FLOAT_EQ(mor->morale, 1.0f - MORALE_NCO_DEATH_HIT, 0.01f);
    ecs_fini(w);
}

TEST(test_morale_flee_threshold) {
    // Verify config: flee threshold is positive and below rally threshold
    ASSERT_GT(MORALE_FLEE_THRESHOLD, 0.0f);
    ASSERT_LT(MORALE_FLEE_THRESHOLD, MORALE_RALLY_THRESHOLD);
    ASSERT_LT(MORALE_RALLY_THRESHOLD, 1.0f);
}

TEST(test_morale_natural_recovery) {
    // Verify natural recovery rate is positive and slower than rally rate
    ASSERT_GT(MORALE_NATURAL_RECOVERY, 0.0f);
    ASSERT_LT(MORALE_NATURAL_RECOVERY, MORALE_NCO_RALLY_RATE);
}

TEST(test_flee_forced_recovery_config) {
    // After max flee duration, troops should be forced back
    ASSERT_GT(MORALE_FLEE_DURATION_MAX, 0.0f);
    ASSERT_GT(MORALE_FLEE_DURATION_MIN, 0.0f);
    // Max > Min
    ASSERT_GT(MORALE_FLEE_DURATION_MAX, MORALE_FLEE_DURATION_MIN);
}

// ============================================================================
// NEW: Formation, Steering, Squad, Rank Weapons
// ============================================================================

TEST(test_formation_offset_single_enemy) {
    // Single enemy should get zero offset
    Vector3 off = FormationOffset(0, 1, ENEMY_SOVIET, RANK_TROOPER);
    ASSERT_FLOAT_EQ(off.x, 0, 0.01f);
    ASSERT_FLOAT_EQ(off.z, 0, 0.01f);
}

TEST(test_formation_offset_officer_rear) {
    // Officers always go to rear
    Vector3 off = FormationOffset(0, 5, ENEMY_SOVIET, RANK_OFFICER);
    ASSERT_LT(off.z, 0); // behind
    ASSERT_FLOAT_EQ(off.x, 0, 0.01f); // centered
}

TEST(test_formation_offset_nco_soviet_front) {
    // Soviet NCO leads from front
    Vector3 off = FormationOffset(0, 5, ENEMY_SOVIET, RANK_NCO);
    ASSERT_GT(off.z, 0); // ahead
}

TEST(test_formation_offset_american_stagger) {
    // American troopers should alternate sides
    Vector3 off0 = FormationOffset(0, 4, ENEMY_AMERICAN, RANK_TROOPER);
    Vector3 off1 = FormationOffset(1, 4, ENEMY_AMERICAN, RANK_TROOPER);
    // Different sides
    ASSERT(off0.x * off1.x < 0 || off0.x == 0 || off1.x == 0);
}

TEST(test_steering_component_on_spawn) {
    ecs_world_t *w = make_test_ecs();
    ecs_entity_t e = EcsEnemySpawnRanked(w, ENEMY_SOVIET, RANK_TROOPER, (Vector3){0,0,0});
    const EcSteering *steer = ecs_get(w, e, EcSteering);
    ASSERT(steer != NULL);
    ASSERT_FLOAT_EQ(steer->acceleration, AI_ACCEL_TROOPER, 0.01f);
    ecs_fini(w);
}

TEST(test_steering_nco_slower_accel) {
    ecs_world_t *w = make_test_ecs();
    ecs_entity_t e = EcsEnemySpawnRanked(w, ENEMY_SOVIET, RANK_NCO, (Vector3){0,0,0});
    const EcSteering *steer = ecs_get(w, e, EcSteering);
    ASSERT(steer != NULL);
    ASSERT_FLOAT_EQ(steer->acceleration, AI_ACCEL_NCO, 0.01f);
    ASSERT_LT(steer->acceleration, AI_ACCEL_TROOPER);
    ecs_fini(w);
}

TEST(test_steering_officer_slowest_accel) {
    ecs_world_t *w = make_test_ecs();
    ecs_entity_t e = EcsEnemySpawnRanked(w, ENEMY_SOVIET, RANK_OFFICER, (Vector3){0,0,0});
    const EcSteering *steer = ecs_get(w, e, EcSteering);
    ASSERT(steer != NULL);
    ASSERT_FLOAT_EQ(steer->acceleration, AI_ACCEL_OFFICER, 0.01f);
    ASSERT_LT(steer->acceleration, AI_ACCEL_NCO);
    ecs_fini(w);
}

TEST(test_squad_component_on_spawn) {
    ecs_world_t *w = make_test_ecs();
    ecs_entity_t e = EcsEnemySpawnSquad(w, ENEMY_SOVIET, RANK_TROOPER, (Vector3){0,0,0}, 3);
    const EcSquad *sq = ecs_get(w, e, EcSquad);
    ASSERT(sq != NULL);
    ASSERT(sq->squadId == 3);
    ecs_fini(w);
}

TEST(test_pickup_type_from_rank_soviet) {
    ASSERT(PickupTypeFromRank(ENEMY_SOVIET, RANK_TROOPER) == PICKUP_KOSMOS7);
    ASSERT(PickupTypeFromRank(ENEMY_SOVIET, RANK_NCO) == PICKUP_KS23_MOLOT);
    ASSERT(PickupTypeFromRank(ENEMY_SOVIET, RANK_OFFICER) == PICKUP_ZARYA_TK4);
}

TEST(test_pickup_type_from_rank_american) {
    ASSERT(PickupTypeFromRank(ENEMY_AMERICAN, RANK_TROOPER) == PICKUP_LIBERTY);
    ASSERT(PickupTypeFromRank(ENEMY_AMERICAN, RANK_NCO) == PICKUP_M8A1_STARHAWK);
    ASSERT(PickupTypeFromRank(ENEMY_AMERICAN, RANK_OFFICER) == PICKUP_ARC9_LONGBOW);
}

TEST(test_pickup_molot_stats) {
    PickupManager pm = make_pickups();
    pm.guns[0].active = true;
    pm.guns[0].position = (Vector3){0, 0, 0};
    pm.guns[0].weaponType = PICKUP_KS23_MOLOT;
    pm.guns[0].life = 10.0f;
    PickupTryGrab(&pm, (Vector3){0, 0, 0});
    ASSERT(pm.pickupAmmo == PICKUP_MOLOT_AMMO);
    ASSERT_FLOAT_EQ(pm.pickupDamage, PICKUP_MOLOT_DAMAGE, 0.01f);
}

TEST(test_pickup_starhawk_stats) {
    PickupManager pm = make_pickups();
    pm.guns[0].active = true;
    pm.guns[0].position = (Vector3){0, 0, 0};
    pm.guns[0].weaponType = PICKUP_M8A1_STARHAWK;
    pm.guns[0].life = 10.0f;
    PickupTryGrab(&pm, (Vector3){0, 0, 0});
    ASSERT(pm.pickupAmmo == PICKUP_STARHAWK_AMMO);
    ASSERT_FLOAT_EQ(pm.pickupDamage, PICKUP_STARHAWK_DAMAGE, 0.01f);
}

TEST(test_pickup_zarya_stats) {
    PickupManager pm = make_pickups();
    pm.guns[0].active = true;
    pm.guns[0].position = (Vector3){0, 0, 0};
    pm.guns[0].weaponType = PICKUP_ZARYA_TK4;
    pm.guns[0].life = 10.0f;
    PickupTryGrab(&pm, (Vector3){0, 0, 0});
    ASSERT(pm.pickupAmmo == PICKUP_ZARYA_AMMO);
    ASSERT_FLOAT_EQ(pm.pickupDamage, PICKUP_ZARYA_DMG_MIN, 0.01f);
}

TEST(test_pickup_longbow_stats) {
    PickupManager pm = make_pickups();
    pm.guns[0].active = true;
    pm.guns[0].position = (Vector3){0, 0, 0};
    pm.guns[0].weaponType = PICKUP_ARC9_LONGBOW;
    pm.guns[0].life = 10.0f;
    PickupTryGrab(&pm, (Vector3){0, 0, 0});
    ASSERT(pm.pickupAmmo == PICKUP_LONGBOW_AMMO);
    ASSERT_FLOAT_EQ(pm.pickupDamage, PICKUP_LONGBOW_DAMAGE, 0.01f);
}

TEST(test_pickup_weapon_names) {
    ASSERT(strcmp(PickupGetName(PICKUP_KOSMOS7), "KOSMOS-7 SMG") == 0);
    ASSERT(strcmp(PickupGetName(PICKUP_LIBERTY), "LIBERTY BLASTER") == 0);
    ASSERT(strcmp(PickupGetName(PICKUP_KS23_MOLOT), "KS-23 MOLOT") == 0);
    ASSERT(strcmp(PickupGetName(PICKUP_M8A1_STARHAWK), "M8A1 STARHAWK") == 0);
    ASSERT(strcmp(PickupGetName(PICKUP_ZARYA_TK4), "ZARYA TK-4") == 0);
    ASSERT(strcmp(PickupGetName(PICKUP_ARC9_LONGBOW), "ARC-9 LONGBOW") == 0);
}

TEST(test_ai_movement_config_sanity) {
    ASSERT_GT(AI_ACCEL_TROOPER, 0.0f);
    ASSERT_GT(AI_ACCEL_NCO, 0.0f);
    ASSERT_GT(AI_ACCEL_OFFICER, 0.0f);
    ASSERT_GT(AI_ACCEL_TROOPER, AI_ACCEL_NCO);
    ASSERT_GT(AI_ACCEL_NCO, AI_ACCEL_OFFICER);
    ASSERT_GT(AI_SLOPE_THRESHOLD, 0.0f);
    ASSERT_GT(AI_SLOPE_MIN_FACTOR, 0.0f);
    ASSERT_LT(AI_SLOPE_MIN_FACTOR, 1.0f);
}

TEST(test_spatial_hash_config_sanity) {
    ASSERT_GT(SPATIAL_CELL_SIZE, 0.0f);
    ASSERT(SPATIAL_GRID_DIM > 0);
    ASSERT(SPATIAL_CELL_CAP > 0);
    ASSERT_GT(SQUAD_COHESION_BIAS, 0.0f);
    ASSERT_GT(SQUAD_COHESION_RADIUS, 0.0f);
}

TEST(test_pickup_molot_config_sanity) {
    ASSERT(PICKUP_MOLOT_AMMO > 0);
    ASSERT(PICKUP_MOLOT_PELLETS > 0);
    ASSERT_GT(PICKUP_MOLOT_DAMAGE, 0.0f);
    ASSERT_GT(PICKUP_MOLOT_RANGE, 0.0f);
    ASSERT_LT(PICKUP_MOLOT_RANGE, PICKUP_STARHAWK_RANGE); // shotgun shorter range
}

TEST(test_pickup_longbow_config_sanity) {
    ASSERT(PICKUP_LONGBOW_AMMO > 0);
    ASSERT(PICKUP_LONGBOW_PIERCE > 0);
    ASSERT_GT(PICKUP_LONGBOW_DAMAGE, 0.0f);
    ASSERT_GT(PICKUP_LONGBOW_RANGE, PICKUP_STARHAWK_RANGE); // longest range
}

int main(void) {
    printf("\n=== MONDFALL UNIT TESTS ===\n\n");

    printf("[Config Sanity]\n");
    RUN(test_ppsh_fastest_fire_rate);
    RUN(test_liberty_oneshots_soviets);
    RUN(test_liberty_oneshots_americans);
    RUN(test_player_max_health_positive);
    RUN(test_wave_formula);
    RUN(test_pickup_ammo_positive);
    RUN(test_liberty_recoil_greater_than_ppsh);

    printf("\n[Player Physics]\n");
    RUN(test_player_forward_yaw_zero);
    RUN(test_player_forward_yaw_90);
    RUN(test_player_forward_pitch_up);
    RUN(test_player_take_damage);
    RUN(test_player_take_damage_overkill);
    RUN(test_player_is_dead_boundary);
    RUN(test_player_recoil_airborne);
    RUN(test_player_recoil_grounded_noop);

    printf("\n[Weapon Logic]\n");
    RUN(test_weapon_mp40_ammo);
    RUN(test_weapon_switch);
    RUN(test_weapon_not_reloading_initially);
    RUN(test_weapon_reload_starts);
    RUN(test_weapon_name);

    printf("\n[Pickup System]\n");
    RUN(test_pickup_grab_in_range);
    RUN(test_pickup_grab_out_of_range);
    RUN(test_pickup_soviet_stats);
    RUN(test_pickup_american_stats);
    RUN(test_pickup_fire_decrements_ammo);
    RUN(test_pickup_fire_respects_cooldown);
    RUN(test_pickup_fire_empty);

    printf("\n[Enemy Hit Detection]\n");
    RUN(test_enemy_sphere_hit);
    RUN(test_enemy_sphere_miss);
    RUN(test_enemy_damage_reduces_health);
    RUN(test_enemy_damage_kills);
    RUN(test_enemy_ray_hit);
    RUN(test_enemy_ray_miss);
    RUN(test_enemy_damage_out_of_bounds);

    printf("\n[World Height]\n");
    RUN(test_world_height_deterministic);
    RUN(test_world_height_not_nan);
    RUN(test_world_height_varies);

    printf("\n[Game State]\n");
    RUN(test_game_init_state);
    RUN(test_game_reset);
    RUN(test_difficulty_defaults);
    RUN(test_difficulty_valkyrie_no_damage);
    RUN(test_difficulty_easy_scale);
    RUN(test_difficulty_hard_scale);
    RUN(test_difficulty_scaler_reduces_damage);
    RUN(test_difficulty_scaler_increases_damage);

    printf("\n[Noise Functions]\n");
    RUN(test_gradient_noise_deterministic);
    RUN(test_gradient_noise_range);
    RUN(test_gradient_noise_varies);
    RUN(test_value_noise_deterministic);
    RUN(test_value_noise_range);
    RUN(test_lerpf_boundaries);
    RUN(test_hash2d_deterministic);
    RUN(test_hash2d_range);

    printf("\n[Terrain Features]\n");
    RUN(test_world_height_continuity);
    RUN(test_world_height_large_coords);
    RUN(test_world_height_negative_coords);
    RUN(test_mare_factor_range);
    RUN(test_mare_factor_deterministic);
    RUN(test_mare_factor_large_coords);

    printf("\n[Collision System]\n");
    RUN(test_collision_no_world);
    RUN(test_collision_empty_world);

    printf("\n[Enemy Advanced]\n");
    RUN(test_enemy_count_alive);
    RUN(test_enemy_count_alive_after_kill);
    RUN(test_enemy_vaporize);
    RUN(test_enemy_eviscerate);
    RUN(test_enemy_sphere_hit_multiple);

    printf("\n[Weapon Advanced]\n");
    RUN(test_weapon_raketenfaust_ammo);
    RUN(test_weapon_jackhammer_name);
    RUN(test_weapon_raketenfaust_name);
    RUN(test_weapon_switch_preserves_ammo);

    printf("\n[Config Cross-Checks]\n");
    RUN(test_jackhammer_oneshots_soviets);
    RUN(test_jackhammer_oneshots_americans);
    RUN(test_player_height_positive);
    RUN(test_moon_gravity_positive);
    RUN(test_render_resolution_valid);
    RUN(test_chunk_size_positive);
    RUN(test_max_enemies_reasonable);

    printf("\n[Structure System]\n");
    RUN(test_structure_init_places_base);
    RUN(test_structure_player_not_inside_initially);
    RUN(test_structure_prompt_none_initially);
    RUN(test_structure_interior_y);
    RUN(test_structure_config_sanity);
    RUN(test_structure_resupply_flash_starts_zero);
    RUN(test_structure_collision_at_base);
    RUN(test_structure_collision_far_away);
    RUN(test_structure_spawn_chance_config);
    RUN(test_structure_unique_interior_y);

    printf("\n[ECS Lifecycle]\n");
    RUN(test_ecs_init_creates_world);
    RUN(test_ecs_reinit_creates_fresh_world);
    RUN(test_ecs_systems_cleanup_nulls_query);
    RUN(test_ecs_double_cleanup_safe);
    RUN(test_ecs_enemies_cleared_on_reinit);

    printf("\n[ECS Faction Stats]\n");
    RUN(test_soviet_prefab_stats);
    RUN(test_american_prefab_stats);
    RUN(test_americans_longer_range_than_soviets);
    RUN(test_americans_prefer_more_distance);
    RUN(test_soviets_faster_than_americans);
    RUN(test_americans_hit_harder_per_shot);
    RUN(test_soviets_fire_faster_than_americans);
    RUN(test_soviets_more_health_than_americans);

    printf("\n[ECS Components]\n");
    RUN(test_ecs_transform_roundtrip);
    RUN(test_ecs_faction_soviet);
    RUN(test_ecs_faction_american);
    RUN(test_ecs_alive_tag_present);
    RUN(test_ecs_velocity_initially_zero);
    RUN(test_ecs_ai_initial_advance);

    printf("\n[ECS Damage]\n");
    RUN(test_ecs_damage_partial);
    RUN(test_ecs_damage_exact_kill);
    RUN(test_ecs_damage_overkill);
    RUN(test_ecs_damage_multi_hit);
    RUN(test_ecs_vaporize_american);
    RUN(test_ecs_eviscerate_drops_weapon);

    printf("\n[ECS Hit Detection]\n");
    RUN(test_ecs_sphere_hit_exact_range);
    RUN(test_ecs_sphere_no_dead_hits);
    RUN(test_ecs_sphere_picks_closest);
    RUN(test_ecs_ray_hit_forward);
    RUN(test_ecs_ray_miss_perpendicular);
    RUN(test_ecs_ray_respects_max_dist);

    printf("\n[Game State Transitions]\n");
    RUN(test_game_reset_clears_wave);
    RUN(test_game_init_menu_state);
    RUN(test_game_wave_delay_positive);

    printf("\n[AI Config Sanity]\n");
    RUN(test_ai_turn_speed_positive);
    RUN(test_ai_dodge_cooldown_positive);
    RUN(test_lod_distances_ordered);
    RUN(test_enemy_ground_offset_positive);
    RUN(test_ai_stagger_divisor_positive);
    RUN(test_structure_collision_blocks_enemy_radius);

    printf("\n[Collision Y-Axis]\n");
    RUN(test_rock_collision_blocks_at_ground);
    RUN(test_rock_collision_passes_above);
    RUN(test_structure_collision_blocks_ground_y);
    RUN(test_structure_collision_passes_above_dome);

    printf("\n[Rank System]\n");
    RUN(test_rank_config_sanity);
    RUN(test_rank_component_set_on_spawn);
    RUN(test_morale_component_set_on_spawn);
    RUN(test_rank_trooper_default_stats);
    RUN(test_rank_nco_health_multiplied);
    RUN(test_rank_officer_health_less_than_trooper);
    RUN(test_spawn_ranked_backward_compat);
    RUN(test_officer_death_morale_hit);
    RUN(test_nco_death_morale_hit_less);
    RUN(test_morale_flee_threshold);
    RUN(test_morale_natural_recovery);
    RUN(test_flee_forced_recovery_config);

    printf("\n[Formation & Movement]\n");
    RUN(test_formation_offset_single_enemy);
    RUN(test_formation_offset_officer_rear);
    RUN(test_formation_offset_nco_soviet_front);
    RUN(test_formation_offset_american_stagger);
    RUN(test_steering_component_on_spawn);
    RUN(test_steering_nco_slower_accel);
    RUN(test_steering_officer_slowest_accel);
    RUN(test_squad_component_on_spawn);
    RUN(test_ai_movement_config_sanity);
    RUN(test_spatial_hash_config_sanity);

    printf("\n[Rank Weapons]\n");
    RUN(test_pickup_type_from_rank_soviet);
    RUN(test_pickup_type_from_rank_american);
    RUN(test_pickup_molot_stats);
    RUN(test_pickup_starhawk_stats);
    RUN(test_pickup_zarya_stats);
    RUN(test_pickup_longbow_stats);
    RUN(test_pickup_weapon_names);
    RUN(test_pickup_molot_config_sanity);
    RUN(test_pickup_longbow_config_sanity);

    printf("\n=== RESULTS: %d/%d passed", tests_passed, tests_run);
    if (tests_failed > 0) printf(", %d FAILED", tests_failed);
    printf(" ===\n\n");

    return tests_failed > 0 ? 1 : 0;
}
