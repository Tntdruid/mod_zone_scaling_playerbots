---
name: zone-scaling-playerbots
description: 'Use when adding, debugging, or tuning this AzerothCore module: NPC/playerbot zone scaling, creature levels, health, damage, armor, raid scaling, zone or area configuration, rescaling behavior, or reports of NPCs dying too quickly.'
---

# Zone Scaling Playerbots

Use this skill for changes to the zone-scaling module in this repository.

## Workflow

1. Read the relevant implementation in `src/ZoneScalingPlayerbots.cpp` and declarations in `src/ZoneScalingPlayerbots.h` before changing behavior. Check `conf/ZoneScalingPlayerbots.conf.dist` for the documented setting and its intended defaults.
2. Trace the setting from configuration loading through the behavior that uses it. Do not assume a key documented in the config file is implemented; confirm it is parsed and applied in the C++ code.
3. Keep area settings ahead of zone settings, and preserve the distinction between passive scaling to a nearby player and combat-entry scaling to the actual player opponent.
4. Treat creature stats as a stable baseline. Do not compound scaling from values already modified by an earlier scaling pass. Preserve current health proportion when changing maximum health unless the requested behavior explicitly calls for a heal or reset.
5. For reports of NPCs dying too quickly or not dropping loot, first verify whether scaling leaves their health or damage unexpectedly low/high. This module has no loot-generation hook; only change loot behavior if investigation shows the module directly causes that behavior.
6. Keep changes narrow and compatible with the AzerothCore APIs used by this module. Update the distributed config comments when changing a setting's name, default, or behavior.
7. Run the narrowest available validation: editor diagnostics, relevant tests, and/or the module build. Report clearly when a build or runtime check is unavailable.