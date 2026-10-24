# Changelog

## [Unreleased] - 2026-09-29

### Changed

- Friendly quest NPCs now scale to a nearby friendly player's level when entering combat with creatures.

- Passiv reskalering af idle-creatures er nu slået fra som standard for at undgå levelspring mellem spillere med forskellige levels; combat-entry-skalering er uændret.

- `OnAllCreatureUpdate` is now marked `override`, fixing the `-Winconsistent-missing-override` warning.

- Low-level creatures gain health through compounded per-level scaling when scaled up.

- Optional `ZoneScaling.Zone.<id>.*` and `ZoneScaling.Area.<id>.*` overrides now load silently when omitted, removing startup spam of `Config: Missing property ...` for defaults-only entries.

### Fixed

- Config values followed by an inline `# comment` were silently ignored, because AzerothCore cannot parse them (this affected every zone MinLevel with a zone name after it). The config no longer uses inline comments.

- Manual `MinLevel`/`MaxLevel` on a zone or area now always win over quest-derived ranges; zones without them still use quest data. Start zones are set to max level 10 and Deadwind Pass to 50-60, where quest data gave wrong ranges.

- `ZoneScaling.DungeonLevelOffset` (default 1) sets the level offset for all dungeon and raid zones, replacing the per-dungeon `LevelOffset` lines.

- The default config is cleaned up from 386 to 196 lines: unused manual open-world ranges and outdated notes are removed.

- Normal dungeons now scale to one level for the whole run (the highest level among the players and bots in the instance or their group), so mobs no longer get different levels depending on who they saw first. Dungeon bosses get `ZoneScaling.DungeonBossLevelOffset` (default 2) extra levels. Heroic dungeons and raids keep their native levels, and dungeons use only the manual MinLevel/MaxLevel instead of quest-derived ranges.

- Quest-derived zone level ranges now ignore stray quests (wrong zone or holiday quests): the range grows from the median quest level and stops at jumps of more than 5 levels. Loch Modan goes from 10-44 to 10-22.

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
