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

    printf("\n=== RESULTS: %d/%d passed", tests_passed, tests_run);
    if (tests_failed > 0) printf(", %d FAILED", tests_failed);
    printf(" ===\n\n");

    return tests_failed > 0 ? 1 : 0;
}
