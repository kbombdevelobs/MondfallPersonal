// Mondfall Unit Tests — no GPU/window required, exercises pure game logic
// Run: make test

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "../src/config.h"
#include "../src/player.h"
#include "../src/weapon.h"
#include "../src/enemy/enemy.h"
#include "../src/pickup.h"
#include "../src/game.h"
#include "../src/world.h"
#include "../src/world/world_noise.h"
#include "../src/structure/structure.h"

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
    pm.guns[0].gunType = ENEMY_SOVIET;
    pm.guns[0].life = 10.0f;

    Vector3 playerPos = {1.0f, 0.0f, 0.0f}; // 1 unit away, within PICKUP_GRAB_RANGE (4)
    bool grabbed = PickupTryGrab(&pm, playerPos);
    ASSERT(grabbed);
    ASSERT(pm.hasPickup);
    ASSERT(pm.pickupType == ENEMY_SOVIET);
}

TEST(test_pickup_grab_out_of_range) {
    PickupManager pm = make_pickups();
    pm.guns[0].active = true;
    pm.guns[0].position = (Vector3){0, 0, 0};
    pm.guns[0].gunType = ENEMY_SOVIET;
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
    pm.guns[0].gunType = ENEMY_SOVIET;
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
    pm.guns[0].gunType = ENEMY_AMERICAN;
    pm.guns[0].life = 10.0f;
    PickupTryGrab(&pm, (Vector3){0, 0, 0});

    ASSERT(pm.pickupAmmo == PICKUP_AMERICAN_AMMO);
    ASSERT_FLOAT_EQ(pm.pickupFireRate, PICKUP_AMERICAN_FIRE_RATE, 0.001f);
    ASSERT_FLOAT_EQ(pm.pickupDamage, PICKUP_AMERICAN_DAMAGE, 0.01f);
}

TEST(test_pickup_fire_decrements_ammo) {
    PickupManager pm = make_pickups();
    pm.hasPickup = true;
    pm.pickupType = ENEMY_SOVIET;
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
    pm.pickupType = ENEMY_SOVIET;
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
    pm.pickupType = ENEMY_SOVIET;
    pm.pickupAmmo = 0;
    pm.pickupTimer = 0;

    bool fired = PickupFire(&pm, (Vector3){0,0,0}, (Vector3){0,0,1});
    ASSERT(!fired);
}

// ============================================================================
// 5. ENEMY HIT DETECTION & DAMAGE
// ============================================================================

static EnemyManager make_enemies(void) {
    EnemyManager em;
    memset(&em, 0, sizeof(em));
    return em;
}

static void place_enemy(EnemyManager *em, int idx, Vector3 pos, float health, EnemyType type) {
    em->enemies[idx].active = true;
    em->enemies[idx].state = ENEMY_ALIVE;
    em->enemies[idx].position = pos;
    em->enemies[idx].health = health;
    em->enemies[idx].type = type;
}

TEST(test_enemy_sphere_hit) {
    EnemyManager em = make_enemies();
    place_enemy(&em, 0, (Vector3){5, 0, 0}, 80, ENEMY_SOVIET);

    int hit = EnemyCheckSphereHit(&em, (Vector3){5, 0, 0}, 1.0f);
    ASSERT(hit == 0);
}

TEST(test_enemy_sphere_miss) {
    EnemyManager em = make_enemies();
    place_enemy(&em, 0, (Vector3){50, 0, 0}, 80, ENEMY_SOVIET);

    int hit = EnemyCheckSphereHit(&em, (Vector3){0, 0, 0}, 1.0f);
    ASSERT(hit == -1);
}

TEST(test_enemy_damage_reduces_health) {
    EnemyManager em = make_enemies();
    place_enemy(&em, 0, (Vector3){0, 0, 0}, 80, ENEMY_SOVIET);

    EnemyDamage(&em, 0, 30.0f);
    ASSERT_FLOAT_EQ(em.enemies[0].health, 50.0f, 0.01f);
    ASSERT(em.enemies[0].state == ENEMY_ALIVE);
}

TEST(test_enemy_damage_kills) {
    EnemyManager em = make_enemies();
    place_enemy(&em, 0, (Vector3){0, 0, 0}, 80, ENEMY_SOVIET);

    EnemyDamage(&em, 0, 100.0f);
    ASSERT(em.enemies[0].state == ENEMY_DYING);
}

TEST(test_enemy_ray_hit) {
    EnemyManager em = make_enemies();
    place_enemy(&em, 0, (Vector3){0, 0, 10}, 80, ENEMY_SOVIET);

    Ray ray = {{0, 0, 0}, {0, 0, 1}};
    float dist = 0;
    int hit = EnemyCheckHit(&em, ray, 100.0f, &dist);
    ASSERT(hit == 0);
    ASSERT_GT(dist, 0.0f);
}

TEST(test_enemy_ray_miss) {
    EnemyManager em = make_enemies();
    place_enemy(&em, 0, (Vector3){50, 0, 0}, 80, ENEMY_SOVIET);

    // Shoot forward along Z, enemy is to the right
    Ray ray = {{0, 0, 0}, {0, 0, 1}};
    float dist = 0;
    int hit = EnemyCheckHit(&em, ray, 100.0f, &dist);
    ASSERT(hit == -1);
}

TEST(test_enemy_damage_out_of_bounds) {
    EnemyManager em = make_enemies();
    // Should not crash
    EnemyDamage(&em, -1, 50.0f);
    EnemyDamage(&em, MAX_ENEMIES + 1, 50.0f);
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
// 11. ENEMY ADVANCED
// ============================================================================

TEST(test_enemy_count_alive) {
    EnemyManager em = make_enemies();
    place_enemy(&em, 0, (Vector3){0,0,0}, 80, ENEMY_SOVIET);
    place_enemy(&em, 1, (Vector3){5,0,0}, 55, ENEMY_AMERICAN);
    ASSERT(EnemyCountAlive(&em) == 2);
}

TEST(test_enemy_count_alive_after_kill) {
    EnemyManager em = make_enemies();
    place_enemy(&em, 0, (Vector3){0,0,0}, 80, ENEMY_SOVIET);
    place_enemy(&em, 1, (Vector3){5,0,0}, 55, ENEMY_AMERICAN);
    EnemyDamage(&em, 0, 9999.0f);
    ASSERT(EnemyCountAlive(&em) == 1);
}

TEST(test_enemy_vaporize) {
    EnemyManager em = make_enemies();
    place_enemy(&em, 0, (Vector3){0,0,0}, 80, ENEMY_SOVIET);
    EnemyVaporize(&em, 0);
    ASSERT(em.enemies[0].state == ENEMY_VAPORIZING);
}

TEST(test_enemy_eviscerate) {
    EnemyManager em = make_enemies();
    place_enemy(&em, 0, (Vector3){0,0,0}, 80, ENEMY_SOVIET);
    EnemyEviscerate(&em, 0, (Vector3){0,0,1});
    ASSERT(em.enemies[0].state == ENEMY_EVISCERATING);
}

TEST(test_enemy_sphere_hit_multiple) {
    EnemyManager em = make_enemies();
    place_enemy(&em, 0, (Vector3){10,0,0}, 80, ENEMY_SOVIET);
    place_enemy(&em, 1, (Vector3){3,0,0}, 55, ENEMY_AMERICAN);
    // Sphere at origin with radius 5 should hit enemy 1 (at dist 3) not enemy 0 (at dist 10)
    int hit = EnemyCheckSphereHit(&em, (Vector3){0,0,0}, 5.0f);
    ASSERT(hit == 1);
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

// ============================================================================
// MAIN — run all tests
// ============================================================================

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

    printf("\n=== RESULTS: %d/%d passed", tests_passed, tests_run);
    if (tests_failed > 0) printf(", %d FAILED", tests_failed);
    printf(" ===\n\n");

    return tests_failed > 0 ? 1 : 0;
}
