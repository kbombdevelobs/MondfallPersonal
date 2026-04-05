# Legacy Files — src/legacy/

These files are **not compiled** and are retained only as historical reference.

## Files

| File | Superseded By | Notes |
|------|---------------|-------|
| `enemy.c` | `src/enemy/enemy_components.c` + `enemy_systems.c` + `enemy_spawn.c` | Original monolithic enemy system (init, AI, spawning, hit detection, damage). Migrated to Flecs ECS. |
| `combat.c` | `src/combat_ecs.c` | Original combat orchestration using manager-struct pattern. Replaced by ECS-based combat processing. |
| `structure_draw.c` | `src/structure/structure_draw_exterior.c` + `structure_draw_interior.c` + `structure_draw_furniture.c` | Original monolithic structure drawing (856 lines). Split per 500-line rule into 3 focused files. |

## Do Not Compile

These files are **not** listed in the Makefile `SRC` variable. They will produce linker errors if compiled alongside their replacements.

## When to Delete

Safe to delete once:
- The ECS migration is fully validated in production
- No developer needs the old code as a reference for porting remaining systems
