#include "ZoneScalingPlayerbots.h"
#include "AllCreatureScript.h"
#include "DatabaseEnv.h"
#include "DBCStores.h"
#include "Group.h"
#include "Log.h"
#include "UnitScript.h"
#include <algorithm>
#include <ctime>
#include <limits>
#include <sstream>

bool ZoneScalingPB::GetQuestLevelRangeForZone(uint32 zoneId, uint8& minLevel, uint8& maxLevel) const
{
    QueryResult result = WorldDatabase.Query(
        "SELECT MIN(QuestLevel), MAX(QuestLevel) FROM quest_template WHERE QuestSortID = {} AND QuestLevel > 0", zoneId);
    if (!result)
        return false;

    Field* fields = result->Fetch();
    if (fields[0].IsNull() || fields[1].IsNull())
        return false;

    minLevel = fields[0].Get<uint8>();
    maxLevel = fields[1].Get<uint8>();
    return true;
}

ZoneScalingPB* ZoneScalingPB::instance()
{
    static ZoneScalingPB instance;
    return &instance;
}

void ZoneScalingPB::LoadConfig()
{
    _enabled   = sConfigMgr->GetOption<bool>("ZoneScaling.Enable", true);
    _raidScaling = sConfigMgr->GetOption<bool>("ZoneScaling.RaidScalingEnable", true);
    _questLevelAutoScale = sConfigMgr->GetOption<bool>("ZoneScaling.QuestLevelAutoScale", false);
    _minLevel  = sConfigMgr->GetOption<uint8>("ZoneScaling.MinLevel", 1);
    _guardLevelOffset = sConfigMgr->GetOption<uint8>("ZoneScaling.GuardLevelOffset", 10);
    _hpDiff    = sConfigMgr->GetOption<float>("ZoneScaling.HPPerLevelDiff", 0.05f);
    _dmgDiff   = sConfigMgr->GetOption<float>("ZoneScaling.DmgPerLevelDiff", 0.05f);
    _armorDiff = sConfigMgr->GetOption<float>("ZoneScaling.ArmorPerLevelDiff", 0.05f);
    _raidSizePerPlayerMult = sConfigMgr->GetOption<float>("ZoneScaling.RaidSizePerPlayerMult", 0.02f);
    _levelBand = sConfigMgr->GetOption<uint8>("ZoneScaling.LevelBand", 1);
    _rescaleIntervalSec = sConfigMgr->GetOption<uint32>("ZoneScaling.RescaleIntervalSec", 30);
    if (_levelBand == 0)
        _levelBand = 1;

    _defaultZoneSettings.maxLevel = sConfigMgr->GetOption<uint8>("ZoneScaling.Default.MaxLevel", 255);
    _defaultZoneSettings.minLevel = _minLevel;
    _defaultZoneSettings.hpDiff = _hpDiff;
    _defaultZoneSettings.dmgDiff = _dmgDiff;
    _defaultZoneSettings.armorDiff = _armorDiff;

    _zoneSettings.clear();
    _areaSettings.clear();
    _scaledCreatures.clear();
    std::string configuredZones = sConfigMgr->GetOption<std::string>("ZoneScaling.ZoneIds", "");
    std::replace(configuredZones.begin(), configuredZones.end(), ',', ' ');
    std::stringstream zoneIds(configuredZones);
    uint32 zoneId;
    while (zoneIds >> zoneId)
    {
        ZoneSettings settings = _defaultZoneSettings;
        std::string prefix = "ZoneScaling.Zone." + std::to_string(zoneId) + ".";
        int32 confMinLevel = sConfigMgr->GetOption<int32>(prefix + "MinLevel", -1);
        int32 confMaxLevel = sConfigMgr->GetOption<int32>(prefix + "MaxLevel", -1);
        if (confMinLevel >= 0)
            settings.minLevel = uint8(confMinLevel);
        if (confMaxLevel >= 0)
            settings.maxLevel = uint8(confMaxLevel);

        if (_questLevelAutoScale)
        {
            uint8 questMinLevel, questMaxLevel;
            if (GetQuestLevelRangeForZone(zoneId, questMinLevel, questMaxLevel))
            {
                settings.minLevel = questMinLevel;
                settings.maxLevel = questMaxLevel;
            }
        }

        settings.hpDiff = sConfigMgr->GetOption<float>(prefix + "HPPerLevelDiff", settings.hpDiff);
        settings.dmgDiff = sConfigMgr->GetOption<float>(prefix + "DmgPerLevelDiff", settings.dmgDiff);
        settings.armorDiff = sConfigMgr->GetOption<float>(prefix + "ArmorPerLevelDiff", settings.armorDiff);
        _zoneSettings[zoneId] = settings;
    }

    std::string configuredAreas = sConfigMgr->GetOption<std::string>("ZoneScaling.AreaIds", "");
    std::replace(configuredAreas.begin(), configuredAreas.end(), ',', ' ');
    std::stringstream areaIds(configuredAreas);
    uint32 areaId;
    while (areaIds >> areaId)
    {
        ZoneSettings settings = _defaultZoneSettings;
        std::string prefix = "ZoneScaling.Area." + std::to_string(areaId) + ".";
        int32 confMinLevel = sConfigMgr->GetOption<int32>(prefix + "MinLevel", -1);
        int32 confMaxLevel = sConfigMgr->GetOption<int32>(prefix + "MaxLevel", -1);
        if (confMinLevel >= 0)
            settings.minLevel = uint8(confMinLevel);
        if (confMaxLevel >= 0)
            settings.maxLevel = uint8(confMaxLevel);

        if (_questLevelAutoScale)
        {
            AreaTableEntry const* areaEntry = sAreaTableStore.LookupEntry(areaId);
            uint32 questZoneId = (areaEntry && areaEntry->zone) ? areaEntry->zone : areaId;

            uint8 questMinLevel, questMaxLevel;
            if (GetQuestLevelRangeForZone(questZoneId, questMinLevel, questMaxLevel))
            {
                settings.minLevel = questMinLevel;
                settings.maxLevel = questMaxLevel;
            }
        }

        settings.hpDiff = sConfigMgr->GetOption<float>(prefix + "HPPerLevelDiff", settings.hpDiff);
        settings.dmgDiff = sConfigMgr->GetOption<float>(prefix + "DmgPerLevelDiff", settings.dmgDiff);
        settings.armorDiff = sConfigMgr->GetOption<float>(prefix + "ArmorPerLevelDiff", settings.armorDiff);
        _areaSettings[areaId] = settings;
    }

    // Keep the race start zones enabled when an older worldserver.conf is used.
    static uint32 const startZoneIds[] = { 1, 12, 141, 3524, 14, 215, 85, 3430 };
    for (uint32 startZoneId : startZoneIds)
    {
        if (_zoneSettings.find(startZoneId) == _zoneSettings.end())
        {
            ZoneSettings settings = _defaultZoneSettings;
            settings.maxLevel = 10;
            _zoneSettings[startZoneId] = settings;
        }
    }

    LOG_INFO("module", "ZoneScalingPlayerbots: loaded");
}

bool ZoneScalingPB::GetSettingsFor(Creature const* creature, Player const* player, ZoneSettings& out) const
{
    auto areaSettings = _areaSettings.find(creature->GetAreaId());
    auto zoneSettings = _zoneSettings.find(player->GetZoneId());
    if (areaSettings == _areaSettings.end() && zoneSettings == _zoneSettings.end())
        return false;

    out = areaSettings != _areaSettings.end() ? areaSettings->second : zoneSettings->second;
    return true;
}

bool ZoneScalingPB::IsDueForPassiveScale(ObjectGuid guid) const
{
    auto it = _scaledCreatures.find(guid);
    if (it == _scaledCreatures.end())
        return true; // never scaled at all yet

    if (_rescaleIntervalSec == 0)
        return false; // passive re-checks disabled; only combat entry can rescale this creature again

    return uint32(::time(nullptr)) >= it->second.nextEvalTime;
}

void ZoneScalingPB::ScaleCreatureToPlayer(Creature* creature, Player* player, bool force)
{
    if (!_enabled || !creature || !player)
        return;

    uint32 now = uint32(::time(nullptr));
    auto existing = _scaledCreatures.find(creature->GetGUID());

    // Passive pass: bail out early if this creature isn't due for a re-check yet.
    // (Combat-entry calls always pass force=true and skip this.)
    if (!force && existing != _scaledCreatures.end() && now < existing->second.nextEvalTime)
        return;

    ZoneSettings settings;
    if (!GetSettingsFor(creature, player, settings))
        return;

    uint8 baseLevel = std::max<uint8>(player->GetLevel(), settings.minLevel);
    baseLevel = std::min<uint8>(baseLevel, settings.maxLevel);

    if (_levelBand > 1)
    {
        uint8 banded = uint8((uint32(baseLevel) / _levelBand) * _levelBand);
        if (banded < settings.minLevel)
            banded = settings.minLevel;
        baseLevel = banded;
    }

    uint8 newLevel;
    if (creature->IsGuard())
    {
        // Guards scale relative to the player-driven base level, not directly to
        // the zone's raw configured maxLevel (which defaults to 255 when unset).
        uint16 guardLevel = uint16(baseLevel) + _guardLevelOffset;
        newLevel = uint8(std::min<uint16>(guardLevel, uint16(settings.maxLevel)));
    }
    else
    {
        newLevel = baseLevel;
    }

    if (existing == _scaledCreatures.end())
    {
        ScaleRecord record;
        record.baseLevel = creature->GetLevel();
        record.baseMaxHealth = creature->GetMaxHealth();
        record.baseMinDamage = creature->GetWeaponDamageRange(BASE_ATTACK, MINDAMAGE);
        record.baseMaxDamage = creature->GetWeaponDamageRange(BASE_ATTACK, MAXDAMAGE);
        record.baseArmor = creature->GetArmor();
        existing = _scaledCreatures.emplace(creature->GetGUID(), record).first;
    }

    auto scheduleNextPassiveCheck = [&]()
    {
        existing->second.level = newLevel;
        existing->second.nextEvalTime = now + _rescaleIntervalSec;
    };

    uint8 oldLevel = creature->GetLevel();
    if (oldLevel == newLevel)
    {
        scheduleNextPassiveCheck();
        return;
    }

    uint32 oldMaxHealth = creature->GetMaxHealth();
    uint32 oldHealth = creature->GetHealth();
    float healthPct = oldMaxHealth ? float(oldHealth) / float(oldMaxHealth) : 1.0f;
    int16 levelDiff = int16(newLevel) - int16(existing->second.baseLevel);

    float healthMultiplier = 1.0f + (settings.hpDiff * levelDiff);
    float damageMultiplier = 1.0f + (settings.dmgDiff * levelDiff);
    float armorMultiplier = 1.0f + (settings.armorDiff * levelDiff);

    Map* map = creature->GetMap();
    Group* group = player->GetGroup();
    if (_raidScaling && map && map->IsRaid() && group && group->isRaidGroup())
    {
        float raidMultiplier = 1.0f + (float(group->GetMembersCount()) * _raidSizePerPlayerMult);
        healthMultiplier *= raidMultiplier;
        damageMultiplier *= raidMultiplier;
        armorMultiplier *= raidMultiplier;
    }

    if (healthMultiplier < 0.1f)
        healthMultiplier = 0.1f;
    if (damageMultiplier < 0.1f)
        damageMultiplier = 0.1f;
    if (armorMultiplier < 0.1f)
        armorMultiplier = 0.1f;

    creature->SetLevel(newLevel);
    creature->UpdateStats(STAT_STRENGTH);
    creature->UpdateStats(STAT_AGILITY);
    creature->UpdateStats(STAT_STAMINA);
    creature->UpdateStats(STAT_INTELLECT);
    creature->UpdateStats(STAT_SPIRIT);

    uint32 newMaxHealth = std::max<uint32>(1, uint32(float(existing->second.baseMaxHealth) * healthMultiplier));
    creature->SetMaxHealth(newMaxHealth);
    creature->SetHealth(std::max<uint32>(1, uint32(float(newMaxHealth) * healthPct)));
    creature->SetBaseWeaponDamage(BASE_ATTACK, MINDAMAGE, existing->second.baseMinDamage * damageMultiplier);
    creature->SetBaseWeaponDamage(BASE_ATTACK, MAXDAMAGE, existing->second.baseMaxDamage * damageMultiplier);
    creature->SetArmor(uint32(float(existing->second.baseArmor) * armorMultiplier));

    scheduleNextPassiveCheck();
}

class ZoneScalingPB_AllCreatureScript : public AllCreatureScript
{
public:
    ZoneScalingPB_AllCreatureScript() : AllCreatureScript("ZoneScalingPB_AllCreatureScript") { }

    void OnAllCreatureUpdate(Creature* creature, uint32 /*diff*/)
    {
        if (!ZoneScalingPB::instance()->IsEnabled())
            return;

        if (!creature->IsAlive())
            return;

        if (creature->IsPet() || creature->IsTotem() || creature->IsCritter())
            return;

        // Mid-fight rescaling is handled by ZoneScalingPB_UnitScript::OnUnitEnterCombat
        // instead - changing a creature's level/stats while it's already trading
        // blows with someone would be jarring and can desync in-flight combat math.
        if (creature->IsInCombat())
            return;

        // Cheap check first: most creatures were already scaled recently and are
        // not due for another look, so skip the player search entirely for them.
        if (!ZoneScalingPB::instance()->IsDueForPassiveScale(creature->GetGUID()))
            return;

        Map* map = creature->GetMap();
        if (!map)
            return;

        Map::PlayerList const& players = map->GetPlayers();
        if (players.IsEmpty())
            return;

        Player* scalingPlayer = nullptr;
        float closestDistSq = std::numeric_limits<float>::max();
        uint32 creatureZoneId = creature->GetZoneId();
        uint32 creatureAreaId = creature->GetAreaId();
        bool hasAreaSettings = ZoneScalingPB::instance()->HasAreaSettings(creatureAreaId);

        for (Map::PlayerList::const_iterator itr = players.begin(); itr != players.end(); ++itr)
        {
            Player* player = itr->GetSource();
            if (!player || !player->IsInWorld() || player->IsGameMaster())
                continue;

            if (player->GetZoneId() != creatureZoneId)
                continue;

            if (hasAreaSettings && player->GetAreaId() != creatureAreaId)
                continue;

            // Pick the nearest matching player, not the highest-level one in the
            // whole zone - otherwise a single max-level player/bot anywhere in the
            // zone drags every creature in that zone up to their level.
            float distSq = creature->GetExactDistSq(player);
            if (!scalingPlayer || distSq < closestDistSq)
            {
                scalingPlayer = player;
                closestDistSq = distSq;
            }
        }

        if (scalingPlayer)
            ZoneScalingPB::instance()->ScaleCreatureToPlayer(creature, scalingPlayer, false);
    }

    void OnCreatureRemoveWorld(Creature* creature) override
    {
        if (creature)
            ZoneScalingPB::instance()->ForgetCreature(creature->GetGUID());
    }
};

// Rescales a creature the instant it enters combat, to the actual player it is
// fighting - this is what fixes the "wrong level" case where the creature had
// previously (passively) picked up the level of some other, unrelated player.
class ZoneScalingPB_UnitScript : public UnitScript
{
public:
    ZoneScalingPB_UnitScript() : UnitScript("ZoneScalingPB_UnitScript") { }

    void OnUnitEnterCombat(Unit* unit, Unit* enemy) override
    {
        if (!ZoneScalingPB::instance()->IsEnabled())
            return;

        Creature* creature = unit ? unit->ToCreature() : nullptr;
        Player* player = enemy ? enemy->ToPlayer() : nullptr;
        if (!creature || !player)
            return;

        if (!creature->IsAlive())
            return;

        if (creature->IsPet() || creature->IsTotem() || creature->IsCritter())
            return;

        ZoneScalingPB::instance()->ScaleCreatureToPlayer(creature, player, true);
    }
};

class ZoneScalingPB_WorldScript : public WorldScript
{
public:
    ZoneScalingPB_WorldScript() : WorldScript("ZoneScalingPB_WorldScript") { }

    void OnStartup() override
    {
        ZoneScalingPB::instance()->LoadConfig();
    }
};

void AddZoneScalingPlayerbotsScripts()
{
    new ZoneScalingPB_WorldScript();
    new ZoneScalingPB_AllCreatureScript();
    new ZoneScalingPB_UnitScript();
}

void Addmod_zone_scaling_playerbotsScripts()
{
    AddZoneScalingPlayerbotsScripts();
}
