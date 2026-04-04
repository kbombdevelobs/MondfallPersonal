---
title: "ADR-001: C99 with raylib"
status: Accepted
owner_area: Core
created: 2026-04-04
last_updated: 2026-04-04
last_reviewed: 2026-04-04
parent_doc: docs/decisions/README.md
related_docs: []
---

# ADR-001: C99 with raylib

## Context

Mondfall is a retro first-person shooter set on the Moon, targeting macOS ARM64 (Apple Silicon). The project needed a graphics framework that could support 3D rendering, input handling, audio, and shader post-processing without imposing a heavy runtime or editor workflow. The goal was minimal abstraction overhead and full control over the game loop and rendering pipeline.

## Decision

Use C99 as the implementation language with raylib 5.5 as the sole framework dependency, built with clang via Make.

## Alternatives Considered

- **Unity:** Full-featured engine but requires C#, brings a large runtime, and imposes an editor-centric workflow. Too heavy for a project that intentionally targets low-res retro rendering.
- **Godot:** Lightweight engine but GDScript adds overhead and the node/scene model is unnecessary for a single-screen FPS with procedural content.
- **SDL2:** Proven C library for cross-platform games, but significantly lower-level than raylib. Would require more boilerplate for window management, input, audio mixing, and 3D rendering setup.

## Consequences

- Full control over the game loop, memory layout, and rendering pipeline.
- Fast compile times (sub-second incremental builds).
- No editor dependency; the entire project builds from the command line.
- raylib provides just enough abstraction (window, input, OpenGL context, audio) without hiding the underlying systems.
- Trade-off: everything beyond what raylib provides (terrain generation, ECS, AI, procedural audio, HUD) must be built from scratch.
- Single-platform target (macOS ARM64) simplifies build configuration but limits portability unless explicitly addressed later.
