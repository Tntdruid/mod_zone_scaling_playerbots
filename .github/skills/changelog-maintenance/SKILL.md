---
name: changelog-maintenance
description: 'Use when adding, changing, debugging, or reviewing this repository: keep changelog.md up to date with concise user-facing entries under Unreleased, and verify that code, configuration, and documentation changes are represented.'
---

# Changelog Maintenance

Keep `changelog.md` current whenever a repository change affects behavior, configuration, compatibility, documentation, or the developer workflow.

## Workflow

1. Read the current top section of `changelog.md` before editing it. Preserve existing headings and entries.
2. Put unreleased work under `## [Unreleased] - YYYY-MM-DD`, creating that section at the top when it does not exist.
3. Group entries under these headings when useful: `Added`, `Changed`, `Fixed`, `Removed`, and `Documentation`.
4. Write short, user-facing entries. Describe the observable result and mention affected settings, commands, or compatibility when relevant. Do not copy implementation details or dump commit messages.
5. Update the changelog in the same change as the code, configuration, or documentation change. A change is incomplete if its changelog entry is missing.
6. Avoid duplicate entries. Extend or correct an existing unreleased entry when it describes the same behavior.
7. Do not move unreleased entries into a release section unless the user explicitly requests a release or version preparation.
8. Before finishing, inspect the diff and confirm that every behavior-affecting file has a corresponding changelog entry and that the entry is accurate.

## Entry Examples

```markdown
# Changelog

## [Unreleased] - 2026-09-29

### Changed

- Zone scaling now preserves a creature's current health percentage when its maximum health is recalculated.

### Documentation

- Documented the new area-level scaling setting in the distributed configuration file.
```

## Validation

Run the narrowest available checks for the code change, then verify that `changelog.md` is non-empty, starts with the current unreleased work, and contains no internal-only wording or unrelated changes.
