# Mondfall — Nazis on the Moon

A retro FPS set on the Moon. You play as a Nazi moon base defender fighting waves of Soviet and American cosmowarriors who arrive in Apollo-style landers.

Built with **C99** and **raylib 5.5**. Everything is procedural — terrain, weapons, sounds, music, enemies, even the CRT post-processing.

## Build & Run

```bash
brew install raylib   # macOS (Apple Silicon)
make run              # build + launch
make test             # 162 unit tests (no GPU needed)
```

## Controls

| Key | Action |
|-----|--------|
| WASD | Move |
| Mouse | Look |
| Space | Jump (moon gravity) |
| Shift | Sprint |
| Left Click | Fire |
| 1 / 2 / 3 | Mond-MP40 / Raketenfaust / Jackhammer |
| R | Reload |
| E | Pick up dropped weapon / Interact |
| X | Ground pound |
| ESC | Pause / Resume |
| Up/Down or W/S | Navigate menus |
| Left/Right or A/D | Adjust settings |
| Enter | Select menu option |

## Weapons

- **Mond-MP40** — Hitscan SMG with cyan energy beams
- **Raketenfaust** — Death beam that vaporizes everything in its path
- **Jackhammer** — Pneumatic war-pick, one-hit eviscerate kill
- **KOSMOS-7 SMG** — Picked up from dead Soviet troopers, fastest fire rate in game
- **LIBERTY BLASTER** — Picked up from dead American troopers, one-shot rail gun
- **KS-23 Molot** — Picked up from Soviet NCOs, devastating close-range shotgun
- **M8A1 Starhawk** — Picked up from American NCOs, 3-round burst rifle
- **Zarya TK-4** — Picked up from Soviet officers, charged pistol with massive damage
- **ARC-9 Longbow** — Picked up from American officers, piercing beam through multiple enemies

## Screenshot

640x360 internal resolution, CRT shader with scanlines, phosphor mask, bloom, chromatic aberration, and fishbowl distortion. Scaled up with nearest-neighbor filtering.

## License

Do whatever you want with it.
