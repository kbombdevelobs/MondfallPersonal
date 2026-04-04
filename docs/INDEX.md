# Mondfall Source Index

Quick reference for every source file in the project.

## Core

| File | Purpose |
|------|---------|
| `src/main.c` | Entry point, game loop, state machine, rendering pipeline, asset loading |
| `src/config.h` | **All tunable gameplay constants** — physics, weapons, enemies, world, audio, colors |
| `src/game.h/c` | Game state machine (menu/playing/paused/game over), wave progression logic |

## Player & Combat

| File | Purpose |
|------|---------|
| `src/player.h/c` | FPS camera, WASD + mouse look, moon gravity, jump, ground pound, head bob |
| `src/weapon.h/c` | 3 weapons (Mond-MP40, Raketenfaust beam, Jackhammer), ammo, reload, procedural viewmodels |
| `src/combat.h/c` | Hit detection, damage resolution, kill counting, pickup drops — extracted from main.c |
| `src/pickup.h/c` | Dropped enemy weapons (KOSMOS-7, LIBERTY BLASTER), grab, fire, first-person draw |

## Enemies & Waves

| File | Purpose |
|------|---------|
| `src/enemy.h/c` | Astronaut enemies, Soviet/American factions, AI behaviors, death animations, radio sounds |
| `src/lander.h/c` | Moon lander wave delivery — descent, enemy deployment, self-destruct, klaxon |

## World

| File | Purpose |
|------|---------|
| `src/world.h/c` | Infinite chunked terrain, heightmap noise, craters, boulders, skybox stars |

## UI & Audio

| File | Purpose |
|------|---------|
| `src/hud.h/c` | Health bar, ammo, wave counter, crosshair, ACHTUNG alert, radio transmission overlay |
| `src/audio.h/c` | Procedural Erika march music (synthesized from MIDI data) |
| `src/sound_gen.h/c` | Shared procedural sound utilities — wave creation, noise/tone gen, radio degradation |

## Shaders

| File | Purpose |
|------|---------|
| `assets/crt.fs` | CRT post-processing — scanlines, phosphor mask, bloom, chromatic aberration, film grain |
| `assets/hud.fs` | HUD visor curve distortion |

## Build

| File | Purpose |
|------|---------|
| `Makefile` | clang C99 build with raylib, macOS frameworks |
