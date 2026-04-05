# Structure System (`src/structure/`)

Moon base structures with geodesic dome exterior, underground cylinder shaft, and Germanic officers' lounge interior. Provides limited resupply stations and safe zones. Spawns procedurally across the world.

## Files

| File | Purpose |
|------|---------|
| `structure.h` | Types (`Structure`, `StructureManager`, `StructurePrompt`), API declarations |
| `structure.c` | Core logic: init, procedural spawning, multi-door enter/exit teleport, wall collision, resupply, exterior collision |
| `structure_draw.h` | Draw API declarations |
| `structure_draw_exterior.c` | Geodesic dome, cylinder shaft, airlock corridors, antenna, `StructureManagerDraw()` |
| `structure_draw_interior.c` | Room shell: floor, walls, ceiling, doors, portraits, `StructureManagerDrawInterior()` |
| `structure_draw_furniture.h` | `DrawFurnitureAndDecor()` declaration |
| `structure_draw_furniture.c` | Interior furniture: couch, coffee table, bar, resupply closet, banners, portraits |

## Key Types

- **`Structure`** — Single structure: world position, type, interior Y offset, multiple door positions/angles, `resuppliesLeft` counter
- **`StructureManager`** — Manages up to `MAX_STRUCTURES` (8), tracks player-inside state, which door was entered, empty message timer
- **`StructurePrompt`** — HUD prompt: `PROMPT_NONE`, `PROMPT_ENTER`, `PROMPT_EXIT`, `PROMPT_RESUPPLY`, `PROMPT_EMPTY`

## Key Functions

- `StructureManagerInit(sm)` — Places guaranteed base at (30, terrain, 30)
- `StructureManagerCheckSpawns(sm, playerPos)` — Discovers new bases in nearby chunks (1-in-15 chance per chunk via deterministic hash)
- `StructureManagerUpdate(sm, player, weapon, pickups)` — Handles enter/exit/resupply via E key, interior collision, empty message
- `StructureCheckCollision(sm, pos, radius)` — Exterior circular collision for enemies and player
- `StructureIsPlayerInside(sm)` — Query used by main.c to freeze simulation
- `StructureGetActive()` — Global pointer (like `WorldGetActive()`) for enemy AI access

## Procedural Spawning

- One base always at (30,30) near spawn
- Additional bases in chunks where `ChunkHash(cx,cz) % STRUCTURE_SPAWN_CHANCE == 0` (1-in-15)
- Deterministic position within chunk via seeded random
- Each base gets unique `interiorY` offset (spaced 50 units apart)
- Up to 8 bases coexist; discovered as player explores

## Exterior

- **Cylinder shaft**: 65% of dome radius, extends 8 units underground, 1.0 unit visible above terrain with steel rim and flat cap
- **Geodesic dome**: 4.5-unit radius (5 rings x 12 segments), sits on top of cylinder cap
- **3 Airlock corridors**: 2.0 units wide, 2.2 tall (player height), 2.8 long, bright white shell with yellow/black hazard chevrons, red/yellow striped door frames, steel ribs
- **Indicator lights**: Green blinking = stocked, red blinking = expended (visible from distance)
- **Red band** around dome base, antenna with blinking light on top

## Interior — Germanic Officers' Lounge (24x20 units)

- **Floor**: Dark wood planks with large red carpet (gold border, diamond motif center)
- **Walls**: Dark wood wainscoting + cream plaster upper, moulding rail, crown moulding
- **Ceiling**: Plaster with exposed wooden beams, iron chandelier with 4 flickering candles
- **3 Doors**: South wall center, East wall, West wall — dark wood frames with green exit lights
- **8 Pixel portraits**: Generals with uniforms, shoulder boards, medals, iron crosses, mustaches
- **Leather couches**: Two Chesterfield-style facing each other with coffee table
- **Full bar**: Counter with brass rail, bottle shelves (green/amber/clear), tumblers, 2 bar stools
- **Bookshelf**: West wall with rows of colored book spines
- **Map table**: Oak table with tactical map paper
- **Hanging banners**: Red with white circle, brass rods, gold fringe
- **Resupply cabinet**: Military green (or gray when empty), glowing panel (green/dead red), status lights

## Resupply System

- Each base has `MOONBASE_RESUPPLIES` (3) uses
- HUD shows: `PRESS [E] TO RESUPPLY [2]`
- When depleted: pressing E shows "MEIN GOTT! THE CUPBOARD IS BARE, KAMERAD!" (shaking red text, 3s)
- Prompt changes to "VERSORGUNG ERSCHOEPFT" in red
- Cabinet visuals go dead: gray body, red panel, red status lights
- Exterior door lights switch green → red

## Collision & Enemy AI

- **Player + enemy collision**: Circular cylinder around each structure (dome radius + 0.5)
- **Enemy sliding**: On collision, enemies compute tangent to circle and slide around with outward push
- **Flanking AI**: Both Soviets and Americans detect when a structure blocks their path to player. Instead of bunching up, they crank strafe multiplier and split around both sides. Each enemy's alternating `strafeDir` ensures groups naturally divide into two flanking waves.

## Safe Zone

- When inside, ALL simulation freezes: enemies, landers, waves, combat, pickups
- Wave timer does not advance while inside
- Enemies have no knowledge of player position (player is at Y=500)

## Adding New Structures

1. Add new `StructureType` enum value
2. In `InitStructureAtPos()` or `StructureManagerInit()`, set unique `interiorY` (e.g., Y=600)
3. Set door count, angles, resupply count
4. Add exterior drawing in `structure_draw_exterior.c`, interior in `structure_draw_interior.c`/`structure_draw_furniture.c`
5. Teleport, collision, freeze, spawning, and AI flanking all work automatically
