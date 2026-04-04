---
title: "ADR-003: 500-Line File Split Rule"
status: Accepted
owner_area: Documentation
created: 2026-04-04
last_updated: 2026-04-04
last_reviewed: 2026-04-04
parent_doc: docs/decisions/README.md
related_docs: []
---

# ADR-003: 500-Line File Split Rule

## Context

As the codebase grew, several source files became difficult to navigate and reason about. In a C project without IDE-level refactoring tools, large files slow down both human developers and AI coding agents that need to understand file structure quickly. A consistent policy was needed to keep files manageable.

## Decision

Any source file exceeding 500 lines must be split into a subfolder organized by concern. Each subfolder must contain a CLAUDE.md listing files, their purpose, key types, and key functions. The subfolder README serves as the primary navigation aid for both humans and agents.

## Alternatives Considered

- **No limit:** Files grow organically. Leads to monolithic files like a 1500-line `enemy.c` that mixes AI, spawning, hit detection, rendering, and death animations.
- **Smaller limit (300 lines):** Too aggressive for C, where a single system (e.g., weapon logic) naturally needs more space due to explicit memory management, struct definitions, and lack of classes.

## Consequences

- Forces modular organization: `src/enemy/`, `src/weapon/`, `src/world/` each split by concern (logic, rendering, sound, AI).
- Subfolder CLAUDE.md files provide a reliable map for agents and new contributors.
- Requires discipline to enforce; the split must be done proactively, not deferred indefinitely.
- Known debt: `game.c` (620 lines) and `enemy_systems.c` (758 lines) currently exceed this limit and need to be addressed.
- Include paths and the Makefile must be updated when splitting, adding a small cost to each refactor.
