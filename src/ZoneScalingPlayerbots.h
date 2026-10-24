#ifndef ZONE_SCALING_PLAYERBOTS_H
#define ZONE_SCALING_PLAYERBOTS_H

#include "ScriptMgr.h"
#include "Player.h"
#include "Creature.h"
#include "Config.h"
#include "Map.h"
#include "World.h"
#include <unordered_map>
#include <unordered_set>

class ZoneScalingPB
{
public:
    static ZoneScalingPB* instance();

    void LoadConfig();
    bool IsEnabled() const { return _enabled; }
    bool HasAreaSettings(uint32 areaId) const { return _areaSettings.find(areaId) != _areaSettings.end(); }
    bool IsExcludedEntry(uint32 entry) const { return _excludedEntries.count(entry) != 0; }

    // force=false (passive pass): only rescales creatures that have never been
    // scaled yet, or whose passive re-check interval has elapsed. Used for the
    // idle/roaming case, where we track the nearest player over time.
    // force=true (combat pass): always rescales to the given player, overriding
    // whatever level the creature picked up passively before the fight started.
    void ScaleCreatureToPlayer(Creature* creature, Player* player, bool force);

    // Cheap pre-check so the caller can skip the (more expensive) nearest-player
    // search entirely for creatures that were already scaled recently.
    bool IsDueForPassiveScale(ObjectGuid guid) const;

    // True once the creature has been scaled to a player opponent during its
    // current fight. Cleared again when the creature leaves combat.
    bool IsCombatScaled(ObjectGuid guid) const;
    void ClearCombatScaled(ObjectGuid guid);

    void ForgetCreature(ObjectGuid guid) { _scaledCreatures.erase(guid); }

private:
    struct ZoneSettings
    {
        uint8 minLevel = 1;
        uint8 maxLevel = 83;
        int32 levelOffset = 0;
        float hpDiff = 0.05f;
        float dmgDiff = 0.05f;
        float armorDiff = 0.05f;
    };

    // Bookkeeping per already-scaled creature: the level we last set it to, and
    // (for the passive pass only) when it's next allowed to be re-checked.
    struct ScaleRecord
    {
        uint8 level = 0;
        uint32 nextEvalTime = 0; // unix time, seconds; only used by the passive pass
        uint8 baseLevel = 0;
        uint32 baseMaxHealth = 0;
        float baseMinDamage = 0.0f;
        float baseMaxDamage = 0.0f;
        uint32 baseArmor = 0;
        bool combatScaled = false; // scaled to a player opponent in the current fight
    };

    ZoneScalingPB() = default;

    // Looks up MIN/MAX(QuestLevel) in quest_template for quests sorted into the given zone.
    bool GetQuestLevelRangeForZone(uint32 zoneId, uint8& minLevel, uint8& maxLevel) const;

    // Resolves the ZoneSettings that apply to this creature/player pair (area
    // settings take priority over zone settings). Returns false if neither the
    // creature's area nor the player's zone is configured for scaling.
    bool GetSettingsFor(Creature const* creature, Player const* player, ZoneSettings& out) const;

    bool _enabled = false;
    bool _raidScaling = true;
    bool _questLevelAutoScale = false;
    uint8 _minLevel = 1;
    uint8 _guardLevelOffset = 10;
    float _hpDiff = 0.05f;
    float _dmgDiff = 0.05f;
    float _armorDiff = 0.05f;
    float _raidSizePerPlayerMult = 0.02f;

    // Round the scaling target down to the nearest multiple of this many levels,
    // to cut down on needless re-scaling churn near a level boundary. 1 = off.
    uint8 _levelBand = 2;

    // How often (seconds) an idle, already-scaled creature is allowed to be
    // re-checked by the passive pass. 0 disables passive re-checks entirely -
    // creatures then only ever rescale once at first sight and again on combat
    // entry (via ZoneScalingPB_UnitScript).
    uint32 _rescaleIntervalSec = 30;

    ZoneSettings _defaultZoneSettings;
    std::unordered_map<uint32, ZoneSettings> _zoneSettings;
    std::unordered_map<uint32, ZoneSettings> _areaSettings;
    std::unordered_map<ObjectGuid, ScaleRecord> _scaledCreatures;

    // Creature entries that are never scaled (e.g. Taming the Beast targets,
    // whose taming rods refuse beasts above a fixed level).
    std::unordered_set<uint32> _excludedEntries;
};

#endif
