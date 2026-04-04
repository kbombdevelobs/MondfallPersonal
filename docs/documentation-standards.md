---
title: Documentation Standards
status: Active
owner_area: Documentation
created: 2026-04-04
last_updated: 2026-04-04
last_reviewed: 2026-04-04
parent_doc: CLAUDE.md
related_docs:
  - Index.md
  - docs_manifest.yaml
---

# Documentation Standards

## Purpose

Defines the format, metadata, and maintenance rules for all Mondfall documentation.

## Metadata Format

Every meaningful doc must have YAML frontmatter:

```yaml
---
title: Document Title
status: Active          # Active | Draft | Deprecated | Archived
owner_area: Core        # Core | Enemy | Weapon | World | Audio | Documentation | Testing
created: YYYY-MM-DD
last_updated: YYYY-MM-DD
last_reviewed: YYYY-MM-DD
parent_doc: path/to/parent.md
related_docs:
  - path/to/related1.md
  - path/to/related2.md
---
```

**Rules:**
- `last_updated` changes when content meaningfully changes
- `last_reviewed` changes when the doc is checked for correctness, even without edits
- `status` must be kept current — do not leave deprecated docs marked Active

## Section Template

Use these sections when relevant (skip sections that add no value):

1. **Purpose** — what this doc is for
2. **Scope** — what it covers and doesn't cover
3. **Responsibilities** — what this system/domain owns
4. **Non-Responsibilities** — explicit exclusions to prevent scope creep
5. **Architecture / Structure** — how it's organized
6. **Key Flows** — important data/control flows
7. **Dependencies** — what this depends on
8. **Key Files** — files that matter for this doc's scope
9. **Related Docs** — cross-links
10. **Open Questions / Future Work** — known gaps
11. **Maintenance Notes** — when and how to update this doc

## Document Hierarchy

```
Root CLAUDE.md              ← whole-app architecture + doc rules
├── Folder CLAUDE.md        ← per-domain architecture (src/enemy/, src/weapon/, src/world/)
│   └── (feature docs)      ← specific systems within that domain
├── Feature docs (docs/)    ← cross-cutting systems (rendering, combat, waves, audio)
├── ADRs (docs/decisions/)  ← major architecture decisions
└── Index.md + manifest     ← navigation and machine-readable index
```

## Canonical Source-of-Truth Rules

If docs overlap, the more specific doc wins for its scope:

| Question | Canonical Source |
|----------|-----------------|
| Whole-app architecture | Root `CLAUDE.md` |
| Domain architecture | Folder `CLAUDE.md` |
| Feature behavior/structure | Feature doc |
| Why a decision was made | ADR |
| Where to find docs | `Index.md` |
| Machine-readable doc map | `docs_manifest.yaml` |

More specific docs may refine broader docs but must not silently contradict them.

## Maintenance Obligations

On every pass that changes code, structure, or documentation:

1. **Root CLAUDE.md** — update if app-level architecture, major folders, or doc rules change
2. **Folder CLAUDE.md** — update if features are added/removed/renamed/moved within that domain
3. **Feature docs** — update if feature scope, behavior, interfaces, or dependencies change
4. **Index.md** — update if any doc is added, deleted, renamed, moved, or changes status
5. **docs_manifest.yaml** — update if any doc metadata, status, or relationships change
6. **Cross-links** — update if file paths, doc names, or ownership changes
7. **Metadata** — update `last_updated` on content changes, `last_reviewed` on review

## Anti-Staleness Rules

1. No silent structural changes — repo structure changes must trigger doc updates
2. No orphan docs — every doc must appear in Index.md and docs_manifest.yaml
3. No ghost references — no doc may point to deleted or moved docs
4. No ambiguous ownership — major docs must have an owner_area
5. No stale metadata — timestamps cannot be ignored
6. No duplicate sources of truth — one canonical doc per topic
7. No filler docs — only create docs justified by real architecture
