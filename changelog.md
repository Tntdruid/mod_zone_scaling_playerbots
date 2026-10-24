# Changelog

## [Unreleased] - 2026-09-29

### Changed

- Friendly quest NPCs now scale to a nearby friendly player's level when entering combat with creatures.

- Passiv reskalering af idle-creatures er nu slået fra som standard for at undgå levelspring mellem spillere med forskellige levels; combat-entry-skalering er uændret.

- `OnAllCreatureUpdate` is now marked `override`, fixing the `-Winconsistent-missing-override` warning.

- Low-level creatures gain health through compounded per-level scaling when scaled up.

- Optional `ZoneScaling.Zone.<id>.*` and `ZoneScaling.Area.<id>.*` overrides now load silently when omitted, removing startup spam of `Config: Missing property ...` for defaults-only entries.

### Fixed

- Hunters can tame beasts that this module scaled above their level: Tame Beast now drops the beast to the hunter's level when the cast starts.

- Taming the Beast quest beasts are no longer scaled, so hunter taming rods work again. The list is configurable via `ZoneScaling.ExcludeCreatureEntries`.

- Trigger creatures and unselectable creatures, such as the invisible casters behind campfires and traps, are no longer scaled, so their damage stays unchanged.

- Creatures pulled by a pet, guardian or totem (including playerbots' pets) now scale to the owning player's level instead of not scaling at all.

- Creatures that entered combat against a non-player (e.g. an NPC) are now scaled as soon as their victim becomes a player or a player's pet, such as a bot in your party.

- Bloodmyst Isles explicit level range is no longer overwritten by lower quest-derived bounds.

- Enabled creature scaling in Bloodmyst Isle.

- `ZoneScaling.LevelOffset` and its zone/area overrides now adjust the target creature level before the configured level bounds are applied.

### Added

- Added repository guidance and a reusable skill for keeping the changelog current with code, configuration, documentation, and workflow changes.
