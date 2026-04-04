# Structure System (`src/structure/`)

Moon base structures with geodesic dome exterior and Germanic officers' lounge interior. Provides resupply stations and safe zones.

## Files

| File | Purpose |
|------|---------|
| `structure.h` | Types (`Structure`, `StructureManager`, `StructurePrompt`), API declarations |
| `structure.c` | Core logic: init, multi-door enter/exit teleport, wall collision, resupply |
| `structure_draw.h` | Draw API declarations |
| `structure_draw.c` | Geodesic dome exterior, Germanic interior (portraits, bar, couches, carpet, chandelier) |

## Key Types

- **`Structure`** — Single structure: world position, type, interior Y offset, multiple door positions/angles
- **`StructureManager`** — Manages all structures, tracks player-inside state, which door was entered
- **`StructurePrompt`** — HUD prompt: `PROMPT_NONE`, `PROMPT_ENTER`, `PROMPT_EXIT`, `PROMPT_RESUPPLY`

## Exterior

- Geodesic dome built from ring-of-faceted-panels (5 rings x 12 segments)
- Smaller radius (3.0 units) than interior suggests
- 3 doors at 120-degree intervals with green indicator lights
- Red identification band around base, antenna with blinking light

## Interior — Germanic Officers' Lounge

- **Floor**: Dark wood planks with large red carpet (gold border, diamond motif)
- **Walls**: Wainscoting (dark wood lower) + cream plaster upper, crown moulding
- **Ceiling**: Plaster with exposed wooden beams, iron chandelier with flickering candle lights
- **3 Doors**: South (center), East wall, West wall — each with dark wood frame and green exit light
- **Portraits**: 8 pixel-art generals on walls (uniforms, medals, iron crosses, mustaches)
- **Couches**: Two leather Chesterfield-style couches facing each other with coffee table
- **Bar**: Full bar along east wall — counter, brass rail, bottle shelves, glasses, bar stools
- **Bookshelf**: West wall with colored book spines
- **Map table**: Oak table with tactical map
- **Banners**: Red hanging banners with white circle motif, brass rods, gold fringe
- **Resupply closet**: Military green cabinet with glowing panel on north wall

## Adding New Structures

1. Add new `StructureType` enum value
2. Place it in `StructureManagerInit()` with unique `interiorY` offset (e.g., Y=600)
3. Set door count and angles in the Structure
4. Add exterior + interior drawing in `structure_draw.c`
5. Teleport, collision, and freeze systems work automatically
