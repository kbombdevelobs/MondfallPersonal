---
title: Testing
status: Active
owner_area: Testing
created: 2026-04-04
last_updated: 2026-04-04
last_reviewed: 2026-04-04
parent_doc: CLAUDE.md
related_docs:
  - CLAUDE.md
---

# Testing

## Running Tests

```bash
make test
```

This compiles `tests/test_game.c` against all game object files (excluding `main.o`) and runs the resulting binary. Tests exit with code 0 on success, 1 on any failure.

**Rule: Always run `make test` before committing. All tests must pass.**

## GPU-Free Constraint

Tests run without creating a window or initializing a GPU context. They exercise pure game logic only: physics calculations, configuration validation, ECS operations, hit detection math, noise functions, and state management. Any test that would require raylib's graphics subsystem is out of scope.

The ECS tests create their own flecs worlds (`ecs_init()`) and register components directly, avoiding any GPU-dependent resource loading.

## Test Framework

A minimal custom framework defined at the top of `test_game.c`:

- `TEST(name)` -- declares a test function
- `RUN(name)` -- executes it, prints PASS/FAIL
- `ASSERT(cond)` -- fails the test if condition is false
- `ASSERT_FLOAT_EQ(a, b, eps)` -- float equality within epsilon
- `ASSERT_GT(a, b)` / `ASSERT_LT(a, b)` -- greater/less than

Results are printed per-category with a final summary line showing passed/total and any failures.

## Test Categories

### 1. Config Sanity (7 tests)
Validates tuning constants catch broken gameplay before it reaches a player. Examples: PPSh must have faster fire rate than MP40, Liberty Blaster must one-shot both factions, wave formula produces expected enemy counts.

### 2. Player Physics (8 tests)
Tests `PlayerGetForward()` direction math at various yaw/pitch values, damage application, death boundary detection, recoil behavior (airborne vs grounded).

### 3. Weapon Logic (5 tests)
MP40 ammo initialization, weapon switching, reload state, weapon name strings.

### 4. Pickup System (7 tests)
Pickup grab range validation, Soviet/American pickup stat verification, fire rate limiting, ammo decrement, empty weapon behavior.

### 5. Enemy Hit Detection (7 tests)
ECS-based ray and sphere hit detection: ray hits at known positions, ray misses when aiming away, sphere overlap detection, out-of-bounds entity damage safety.

### 6. World Height (3 tests)
`WorldGetHeight()` determinism (same input = same output), NaN/Inf safety, valid range.

### 7. Game State (8 tests)
`GameInit()` defaults, `GameReset()` clears wave/kill data, difficulty scaling validation (Valkyrie 0%, Easy 50%, Normal 100%, Hard 150%).

### 8. Noise Functions (7 tests)
`GradientNoise` and `ValueNoise` determinism, output range bounds, variation across sample points, `LerpF` boundary correctness, `Hash2D` determinism and range.

### 9. Terrain Features (6 tests)
Height continuity (no huge jumps between nearby points), large coordinate stability, negative coordinate safety, `WorldGetMareFactor()` range [0,1] and determinism.

### 10. Collision System (2 tests)
NULL world and empty world safety checks for `WorldCheckCollision()`.

### 11. Enemy Advanced (5 tests)
`EcsEnemyCountAlive()` correctness before and after kills, `EcsEnemyVaporize()` removes `EcAlive` and adds `EcVaporizing`, `EcsEnemyEviscerate()` removes `EcAlive` and adds `EcEviscerating`, sphere hit returns closest entity.

### 12. Weapon Advanced (4 tests)
Raketenfaust ammo initialization, weapon name strings for Jackhammer and Raketenfaust, ammo preservation across weapon switches.

### 13. Config Cross-Checks (7 tests)
Jackhammer one-shots both factions, player height and moon gravity are positive, render resolution is valid landscape, chunk size is positive, max enemies is within reasonable bounds (10-200).

### 14. Structure System (2 tests)
Structure spawn chance config validation, unique interior Y-offset per structure.

### 15. ECS Lifecycle (5 tests)
`GameEcsInit()` creates a valid world, reinit creates a fresh empty world (old enemies gone), `EcsEnemySystemsCleanup()` nulls the alive query, double-cleanup is safe (no crash), enemies cleared on reinit.

### 16. ECS Faction Stats (8 tests)
Soviet/American prefab stats match `config.h` constants. Cross-faction comparisons: Americans have longer range and prefer more distance, Soviets are faster with more health and faster fire rate, Americans hit harder per shot.

### 17. ECS Component Integrity (6 tests)
Transform position roundtrip, faction tag correctness for both types, `EcAlive` tag present on spawn, velocity initially zero, AI starts in `AI_ADVANCE`.

### 18. ECS Damage (6 tests)
Partial damage leaves enemy alive with correct remaining health, exact-health damage kills (transitions to `EcDying`), overkill removes `EcAlive`, multi-hit accumulation and eventual kill, vaporize sets `EcVaporizing` (not `EcDying`), eviscerate sets `EcEviscerating` (not `EcDying` or `EcVaporizing`).

### 19. ECS Hit Detection (6 tests)
Sphere hit at exact range, dead enemies excluded from sphere hits, sphere picks closest entity, ray hit on enemy directly ahead, ray miss on perpendicular enemy, ray respects max distance.

### 20. Game State Transitions (3 tests)
`GameReset()` clears wave/kill/spawn state, `GameInit()` starts in `STATE_MENU`, wave delay is positive.

### 21. AI Config Sanity (6 tests)
AI turn speed positive, dodge cooldown positive, LOD distances ordered (LOD1 < LOD2 < CULL), enemy ground offset positive, AI stagger divisor at least 1, structure collision radius blocks enemy approach.

## Adding New Tests

1. Define a test function using `TEST(test_descriptive_name)` in the appropriate category section
2. Add a `RUN(test_descriptive_name)` call in `main()` under the matching category
3. Use `ASSERT`, `ASSERT_FLOAT_EQ`, `ASSERT_GT`, or `ASSERT_LT` for validation
4. For ECS tests, use the `make_test_ecs()` helper which creates a world and registers components
5. Run `make test` to verify

## Key File

| File | Role |
|------|------|
| `tests/test_game.c` | All unit tests (~127 tests across 21 categories) |
