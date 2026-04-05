---
title: "ADR-005: Procedural Generation for All Assets"
status: Accepted
owner_area: Core
created: 2026-04-04
last_updated: 2026-04-04
last_reviewed: 2026-04-04
parent_doc: docs/decisions/README.md
related_docs:
  - docs/procedural-audio.md
---

# ADR-005: Procedural Generation for All Assets

## Context

The game requires terrain, weapon sounds, music, weapon viewmodels, enemy models, and particle effects. An external asset pipeline (importing textures, 3D models, sound files) would add tooling complexity, increase repository size, and create dependencies on asset creation tools. The retro aesthetic also means high-fidelity assets are unnecessary.

## Decision

Generate all game assets procedurally at runtime:
- **Terrain:** Multi-octave Perlin gradient noise with domain warping, craters, rilles, and maria biomes.
- **Sounds:** Waveform synthesis for all weapon fire, explosion, and ambient effects.
- **Music:** Erika march synthesized from MIDI note data with brass square waves and drums.
- **Models:** Weapon viewmodels and enemy astronauts built from `DrawCube`/`DrawSphere` primitives via `rlPushMatrix`/`rlPopMatrix`.
- **Particles:** Cached unit sphere and unit cube meshes reused with transform matrices.

## Alternatives Considered

- **Pre-made assets:** Traditional pipeline with imported textures, models, and audio files. Requires asset creation tools, increases repo size significantly, and adds an import/export workflow.
- **Hybrid approach:** Procedural terrain and models but pre-recorded audio. Reduces synthesis complexity but still requires an audio pipeline.

## Consequences

- Zero external asset dependencies for core gameplay. The game builds and runs from source alone.
- Tiny repository size (no binary assets in version control for core systems).
- Infinite terrain variety from noise-based generation.
- Fast iteration: changing a weapon model means editing code, not re-exporting from a 3D tool.
- Trade-off: limited visual and audio fidelity compared to hand-crafted assets. The retro aesthetic mitigates this.
- One exception: enemy death sounds are loaded from mp3 files (`sounds/soviet_death_sounds/`, `sounds/american_death_sounds/`) because convincing voice acting cannot be procedurally generated.
