---
title: Mondfall Documentation Index
status: Active
owner_area: Documentation
created: 2026-04-04
last_updated: 2026-04-04
last_reviewed: 2026-04-04
parent_doc: CLAUDE.md
related_docs:
  - AGENTS.md
  - docs_manifest.yaml
  - docs/documentation-standards.md
---

# Mondfall Documentation Index

## Root Documents

| Doc | Purpose | Read When |
|-----|---------|-----------|
| [CLAUDE.md](CLAUDE.md) | Whole-app architecture, build instructions, game systems, doc rules | Starting any work on the project |
| [AGENTS.md](AGENTS.md) | Agent-facing mirror of `CLAUDE.md` for tooling that reads `AGENTS.md` | Working in tools that prefer `AGENTS.md` |
| [README.md](README.md) | Quick start for players (build, controls, weapons) | First time running the game |
| [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) | System overview, module dependency graph, design patterns | Understanding how systems connect |

## Domain Architecture (Folder CLAUDE.md)

| Doc | Purpose | Read When |
|-----|---------|-----------|
| [src/enemy/CLAUDE.md](src/enemy/CLAUDE.md) | Enemy AI, factions, death types, ECS components/systems | Modifying enemy behavior, spawning, or rendering |
| [src/weapon/CLAUDE.md](src/weapon/CLAUDE.md) | Weapon logic, viewmodels, procedural sounds | Modifying weapons, adding new weapons, tuning balance |
| [src/world/CLAUDE.md](src/world/CLAUDE.md) | Terrain generation, chunks, craters, noise, collision | Modifying terrain, world features, or collision |

## Feature Documentation

| Doc | Purpose | Read When |
|-----|---------|-----------|
| [docs/rendering-pipeline.md](docs/rendering-pipeline.md) | 3-layer render pipeline, CRT shader, HUD visor shader | Modifying visuals, shaders, or render targets |
| [docs/combat-system.md](docs/combat-system.md) | Hit detection, damage, kill tracking, death type dispatch | Modifying combat, damage, or death mechanics |
| [docs/wave-system.md](docs/wave-system.md) | Lander spawning, wave progression, enemy deployment | Modifying wave pacing, lander behavior, or difficulty |
| [docs/ecs-integration.md](docs/ecs-integration.md) | Flecs ECS adoption, components, systems, migration status | Working with ECS, adding components/systems, understanding legacy vs active code |
| [docs/procedural-audio.md](docs/procedural-audio.md) | Sound generation, weapon sounds, Erika march, radio effects | Modifying sounds, adding new audio, tuning music |
| [docs/testing.md](docs/testing.md) | Test strategy, running tests, adding tests, coverage | Adding tests, debugging test failures |
| [docs/ai-movement-analysis.md](docs/ai-movement-analysis.md) | AI movement, bunching, pathfinding deep-dive + rank weapons | Improving AI behavior, adding new pickups, fixing bunching |

## Architecture Decision Records

| Doc | Purpose | Read When |
|-----|---------|-----------|
| [docs/decisions/README.md](docs/decisions/README.md) | ADR index | Looking for why a major decision was made |
| [docs/decisions/ADR-001-c99-raylib.md](docs/decisions/ADR-001-c99-raylib.md) | Why C99 + raylib | Questioning the tech stack choice |
| [docs/decisions/ADR-002-fixed-size-pools.md](docs/decisions/ADR-002-fixed-size-pools.md) | Why static arrays, no heap allocation | Considering dynamic allocation |
| [docs/decisions/ADR-003-file-split-rule.md](docs/decisions/ADR-003-file-split-rule.md) | Why 500-line file limit with subfolders | A file is getting too large |
| [docs/decisions/ADR-004-ecs-flecs.md](docs/decisions/ADR-004-ecs-flecs.md) | Why flecs ECS for enemy system | Extending ECS or questioning the approach |
| [docs/decisions/ADR-005-procedural-assets.md](docs/decisions/ADR-005-procedural-assets.md) | Why procedural generation for everything | Considering external assets |
| [docs/decisions/ADR-006-lowres-crt-pipeline.md](docs/decisions/ADR-006-lowres-crt-pipeline.md) | Why 640x360 render target + CRT shader | Modifying visual style or resolution |

## Documentation Infrastructure

| Doc | Purpose | Read When |
|-----|---------|-----------|
| [docs/documentation-standards.md](docs/documentation-standards.md) | Metadata format, section templates, maintenance rules | Creating or updating any doc |
| [docs/templates/](docs/templates/) | Templates for docs, features, ADRs | Creating a new doc |
| [docs_manifest.yaml](docs_manifest.yaml) | Machine-readable doc registry | Tooling, automated doc checks |

## Navigation Notes

- **Architecture-first**: Start with `CLAUDE.md` or `AGENTS.md` for the big picture, then drill into domain or feature docs
- **Domain docs own their folder**: src/enemy/CLAUDE.md is the authority on the enemy system, not this index
- **ADRs explain "why"**: If you wonder why something is built a certain way, check the ADRs
- **docs_manifest.yaml**: Use this for programmatic access to the doc structure
