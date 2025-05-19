#include "Prestige.h"
#include "Chat.h"
#include "Config.h"
#include "Spell.h"
#include "Player.h"
#include <AI/ScriptedAI/ScriptedGossip.h>

auto& prestigeConfigSettings = PrestigeConfigSettings::Instance();

void PrestigePlayerScript::OnPlayerLogin(Player* player)
{
    if (!player)
    {
        return;
    }
    if (!prestigeConfigSettings.IsPrestigeEnabled())
    {
        return;
    }
    auto prestigeStats = LoadPrestigeStatsForPlayer(player);
    if (!prestigeStats)
    {
        return;
    }
    CheckForUpdatedMaxStats(player);
    ApplyPrestigeStats(player, prestigeStats);
    InitPrestigeExpTnl(player);
}

void CheckForUpdatedMaxStats(Player* player)
{
    auto prestigeStats = GetPrestigeStats(player); 
    if (!prestigeStats)
    {
        return;
    }
    for (int x = PRESTIGE_STAT_STAMINA; x<PRESTIGE_STAT_CONFIRMSPEND; ++x)
    {
        uint32 statMax = prestigeConfigSettings.GetStatMaximum(x);
        if(prestigeStats->stats[x] > statMax)
        {
            uint16 difference = prestigeStats->stats[x] - statMax;
            prestigeStats->stats[x] = statMax;
            prestigeStats->stats[PRESTIGE_STAT_UNALLOCATED] += difference;
            std::string statName = StatNames[x];
            player->SendSystemMessage(Acore::StringFormat("|cffFF0000Warning: Your points spent in {} appear to be above the server's max allowed value!|r",statName));
            player->SendSystemMessage(Acore::StringFormat("|cffFF0000Your points in {}: {}|r", statName, prestigeStats->stats[x]));
            player->SendSystemMessage(Acore::StringFormat("|cffFF0000Maximum allowed points in {}: {}|r", statName, statMax));
            player->SendSystemMessage("|cffFF0000Please use your prestige menu to reallocate the refunded points.|r");
        }
    }
}

void PrestigePlayerScript::OnPlayerLogout(Player* player)
{
    if (!player)
    {
        return;
    }
    if (!prestigeConfigSettings.IsPrestigeEnabled())
    {
        return;
    }
    SavePrestigeStatsForPlayer(player);
}

/*called when a player gains a prestige level via onlevelchanged, kept seperate incase we decide to have bonuses outside of prestige level*/
void AddPrestigePoint(Player* player)
{
    auto prestigeStats = GetPrestigeStats(player);
    if (!prestigeStats)
    {
        LOG_INFO("module", "Failed to get prestige stats for player '{}' with guid '{}'.", player->GetName(), player->GetGUID().GetRawValue());
        return;
    }
    prestigeStats->stats[PRESTIGE_STAT_UNALLOCATED] += 1;
}

void PrestigePlayerScript::OnPlayerLeaveCombat(Player* player)
{
    if (!player)
    {
        return;
    }
    if (player->IsDuringRemoveFromWorld() || !player->IsInWorld())
    {
        return;
    }
    if (!prestigeConfigSettings.IsPrestigeEnabled())
    {
        return;
    }
    if (!prestigeConfigSettings.IsPVPDisabled())
    {
        return;
    }
    auto prestigeStats = GetPrestigeStats(player);
    if (!prestigeStats)
    {
        return;
    }
    ApplyPrestigeStats(player, prestigeStats);
}

void PrestigePlayerScript::OnPlayerEnterCombat(Player* player, Unit* enemy)
{
    ClosePrestigeMenuInCombat(player);
}

void ClosePrestigeMenuInCombat(Player* player)
{
    if (player->PlayerTalkClass->GetGossipMenu().GetMenuId() == PRESTIGE_MENU_ID)
    {
        CloseGossipMenuFor(player);
    }
}

/*This is where new Prestige Stats are generated if they do not exist*/
PrestigeStats* GetPrestigeStats(Player* player)
{
    auto guid = player->GetGUID().GetRawValue();
    auto stats = prestigeStatMap.find(guid);
    if (stats == prestigeStatMap.end())
    {
        PrestigeStats newPrestigeStats2;

        for (int x = 0; x<PRESTIGE_STAT_MAX; ++x)
        {
            newPrestigeStats2.stats[x] = 0;
        }
        newPrestigeStats2.stats[PRESTIGE_STAT_CONFIRMSPEND]=SETTING_CONFIRM_SPEND;
        auto result = prestigeStatMap.emplace(guid, newPrestigeStats2);
        stats = result.first;
    }

    return &stats->second;
}

void ClearPrestigeStats()
{
    prestigeStatMap.clear();
}

void LoadPrestigeStats()
{
    auto qResult = CharacterDatabase.Query("SELECT * FROM character_prestige_stats");
    if (!qResult)
    {
        LOG_ERROR("module", "Failed to load from 'character_prestige_stats' table.");
        return;
    }
    LOG_INFO("module", "Loading player attriboosts from 'character_prestige_stats'..");
    int count = 0;
    do
    {
        auto fields = qResult->Fetch();
        uint64 guid = fields[0].Get<uint64>();

        PrestigeStats prestigeStats;
        for (int x = 0; x<PRESTIGE_STAT_MAX; ++x)
        {
            prestigeStats.stats[x] = fields[x+1].Get<uint32>();
        }
        prestigeStatMap.emplace(guid, prestigeStats);

        count++;
    } while (qResult->NextRow());

    LOG_INFO("module", "Loaded '{}' player prestige stats.", count);
}

PrestigeStats* LoadPrestigeStatsForPlayer(Player* player)
{
    if (!player)
    {
        return nullptr;
    }

    auto guid = player->GetGUID().GetRawValue();
    auto qResult = CharacterDatabase.Query("SELECT * FROM character_prestige_stats WHERE guid = {}", guid);

    if (!qResult)
    {
        return nullptr;
    }

    auto fields = qResult->Fetch();

        PrestigeStats loadedPrestigeStats;
        for (int x = 0; x<PRESTIGE_STAT_MAX; ++x)
        {
            loadedPrestigeStats.stats[x] = fields[x+1].Get<uint32>();
        }

    auto prestigeStats = prestigeStatMap.find(guid);

    if (prestigeStats == prestigeStatMap.end())
    {
        prestigeStatMap.emplace(guid, loadedPrestigeStats);
    }
    else
    {
        prestigeStats->second = loadedPrestigeStats;
    }
    return &prestigeStats->second;
}

void SavePrestigeStats()
{
    for (auto prestigeStats = prestigeStatMap.begin(); prestigeStats != prestigeStatMap.end(); ++prestigeStats)
    {
        CharacterDatabase.Execute(
        "INSERT INTO `character_prestige_stats` "
        "(guid, prestigelevel, unallocated, "
        "stamina, strength, agility, intellect, spirit, "
        "defenserating, dodgerating, parryrating, blockrating, blockvalue, bonusarmor, resilrating, "
        "attackpower, critrating, hitrating, hasterating, expertiserating, armorpenrating, spellpen, mp5, spellpower, "
        "resistfire, resistfrost, resistnature, resistshadow, resistarcane, resistall, confirmspend) "

        "VALUES ({}, {}, {}, "
        "{}, {}, {}, {}, {}, "
        "{}, {}, {}, {}, {}, {}, {}, "
        "{}, {}, {}, {}, {}, {}, {}, {}, {}, "
        "{}, {}, {}, {}, {}, {}, {}) "

        "ON DUPLICATE KEY UPDATE "
        "prestigelevel={}, unallocated={}, stamina={}, strength={}, agility={}, intellect={}, spirit={}, "
        "defenserating={}, dodgerating={}, parryrating={}, blockrating={}, blockvalue={}, bonusarmor={}, resilrating={}, "
        "attackpower={}, critrating={}, hitrating={}, hasterating={}, expertiserating={}, armorpenrating={}, spellpen={}, mp5={}, spellpower={}, "
        "resistfire={}, resistfrost={}, resistnature={}, resistshadow={}, resistarcane={}, resistall={}, confirmspend={}",

        prestigeStats->first, // guid
        prestigeStats->second.stats[PRESTIGE_STAT_PRESTIGELEVEL],
        prestigeStats->second.stats[PRESTIGE_STAT_UNALLOCATED],
        prestigeStats->second.stats[PRESTIGE_STAT_STAMINA],
        prestigeStats->second.stats[PRESTIGE_STAT_STRENGTH],
        prestigeStats->second.stats[PRESTIGE_STAT_AGILITY],
        prestigeStats->second.stats[PRESTIGE_STAT_INTELLECT],
        prestigeStats->second.stats[PRESTIGE_STAT_SPIRIT],
        prestigeStats->second.stats[PRESTIGE_STAT_DEFENSE_RATING],
        prestigeStats->second.stats[PRESTIGE_STAT_DODGE_RATING],
        prestigeStats->second.stats[PRESTIGE_STAT_PARRY_RATING],
        prestigeStats->second.stats[PRESTIGE_STAT_BLOCK_RATING],
        prestigeStats->second.stats[PRESTIGE_STAT_BLOCK_VALUE],
        prestigeStats->second.stats[PRESTIGE_STAT_BONUS_ARMOR],
        prestigeStats->second.stats[PRESTIGE_STAT_RESILIENCE_RATING],
        prestigeStats->second.stats[PRESTIGE_STAT_ATTACK_POWER],
        prestigeStats->second.stats[PRESTIGE_STAT_CRIT_RATING],
        prestigeStats->second.stats[PRESTIGE_STAT_HIT_RATING],
        prestigeStats->second.stats[PRESTIGE_STAT_HASTE_RATING],
        prestigeStats->second.stats[PRESTIGE_STAT_EXPERTISE_RATING],
        prestigeStats->second.stats[PRESTIGE_STAT_ARMOR_PEN_RATING],
        prestigeStats->second.stats[PRESTIGE_STAT_SPELL_PENETRATION],
        prestigeStats->second.stats[PRESTIGE_STAT_MP5],
        prestigeStats->second.stats[PRESTIGE_STAT_SPELL_POWER],
        prestigeStats->second.stats[PRESTIGE_STAT_RESIST_FIRE],
        prestigeStats->second.stats[PRESTIGE_STAT_RESIST_FROST],
        prestigeStats->second.stats[PRESTIGE_STAT_RESIST_NATURE],
        prestigeStats->second.stats[PRESTIGE_STAT_RESIST_SHADOW],
        prestigeStats->second.stats[PRESTIGE_STAT_RESIST_ARCANE],
        prestigeStats->second.stats[PRESTIGE_STAT_RESIST_ALL],
        prestigeStats->second.stats[PRESTIGE_STAT_CONFIRMSPEND],

        prestigeStats->second.stats[PRESTIGE_STAT_PRESTIGELEVEL],
        prestigeStats->second.stats[PRESTIGE_STAT_UNALLOCATED],
        prestigeStats->second.stats[PRESTIGE_STAT_STAMINA],
        prestigeStats->second.stats[PRESTIGE_STAT_STRENGTH],
        prestigeStats->second.stats[PRESTIGE_STAT_AGILITY],
        prestigeStats->second.stats[PRESTIGE_STAT_INTELLECT],
        prestigeStats->second.stats[PRESTIGE_STAT_SPIRIT],
        prestigeStats->second.stats[PRESTIGE_STAT_DEFENSE_RATING],
        prestigeStats->second.stats[PRESTIGE_STAT_DODGE_RATING],
        prestigeStats->second.stats[PRESTIGE_STAT_PARRY_RATING],
        prestigeStats->second.stats[PRESTIGE_STAT_BLOCK_RATING],
        prestigeStats->second.stats[PRESTIGE_STAT_BLOCK_VALUE],
        prestigeStats->second.stats[PRESTIGE_STAT_BONUS_ARMOR],
        prestigeStats->second.stats[PRESTIGE_STAT_RESILIENCE_RATING],
        prestigeStats->second.stats[PRESTIGE_STAT_ATTACK_POWER],
        prestigeStats->second.stats[PRESTIGE_STAT_CRIT_RATING],
        prestigeStats->second.stats[PRESTIGE_STAT_HIT_RATING],
        prestigeStats->second.stats[PRESTIGE_STAT_HASTE_RATING],
        prestigeStats->second.stats[PRESTIGE_STAT_EXPERTISE_RATING],
        prestigeStats->second.stats[PRESTIGE_STAT_ARMOR_PEN_RATING],
        prestigeStats->second.stats[PRESTIGE_STAT_SPELL_PENETRATION],
        prestigeStats->second.stats[PRESTIGE_STAT_MP5],
        prestigeStats->second.stats[PRESTIGE_STAT_SPELL_POWER],
        prestigeStats->second.stats[PRESTIGE_STAT_RESIST_FIRE],
        prestigeStats->second.stats[PRESTIGE_STAT_RESIST_FROST],
        prestigeStats->second.stats[PRESTIGE_STAT_RESIST_NATURE],
        prestigeStats->second.stats[PRESTIGE_STAT_RESIST_SHADOW],
        prestigeStats->second.stats[PRESTIGE_STAT_RESIST_ARCANE],
        prestigeStats->second.stats[PRESTIGE_STAT_RESIST_ALL],
        prestigeStats->second.stats[PRESTIGE_STAT_CONFIRMSPEND]
        );
    }
}

void SavePrestigeStatsForPlayer(Player* player)
{
    auto prestigeStats = GetPrestigeStats(player);
    if (!prestigeStats)
    {
        return;
    }
    
    auto guid = player->GetGUID().GetRawValue();

    CharacterDatabase.Execute(
        "INSERT INTO `character_prestige_stats` "
        "(guid, prestigelevel, unallocated, "
        "stamina, strength, agility, intellect, spirit, "
        "defenserating, dodgerating, parryrating, blockrating, blockvalue, bonusarmor, resilrating, "
        "attackpower, critrating, hitrating, hasterating, expertiserating, armorpenrating, spellpen, mp5, spellpower, "
        "resistfire, resistfrost, resistnature, resistshadow, resistarcane, resistall, confirmspend) "

        "VALUES ({}, {}, {}, "
        "{}, {}, {}, {}, {}, "
        "{}, {}, {}, {}, {}, {}, {}, "
        "{}, {}, {}, {}, {}, {}, {}, {}, {}, "
        "{}, {}, {}, {}, {}, {}, {}) "

        "ON DUPLICATE KEY UPDATE "
        "prestigelevel={}, unallocated={}, stamina={}, strength={}, agility={}, intellect={}, spirit={}, "
        "defenserating={}, dodgerating={}, parryrating={}, blockrating={}, blockvalue={}, bonusarmor={}, resilrating={}, "
        "attackpower={}, critrating={}, hitrating={}, hasterating={}, expertiserating={}, armorpenrating={}, spellpen={}, mp5={}, spellpower={}, "
        "resistfire={}, resistfrost={}, resistnature={}, resistshadow={}, resistarcane={}, resistall={}, confirmspend={}",

        guid,
        prestigeStats->stats[PRESTIGE_STAT_PRESTIGELEVEL],
        prestigeStats->stats[PRESTIGE_STAT_UNALLOCATED],
        prestigeStats->stats[PRESTIGE_STAT_STAMINA],
        prestigeStats->stats[PRESTIGE_STAT_STRENGTH],
        prestigeStats->stats[PRESTIGE_STAT_AGILITY],
        prestigeStats->stats[PRESTIGE_STAT_INTELLECT],
        prestigeStats->stats[PRESTIGE_STAT_SPIRIT],
        prestigeStats->stats[PRESTIGE_STAT_DEFENSE_RATING],
        prestigeStats->stats[PRESTIGE_STAT_DODGE_RATING],
        prestigeStats->stats[PRESTIGE_STAT_PARRY_RATING],
        prestigeStats->stats[PRESTIGE_STAT_BLOCK_RATING],
        prestigeStats->stats[PRESTIGE_STAT_BLOCK_VALUE],
        prestigeStats->stats[PRESTIGE_STAT_BONUS_ARMOR],
        prestigeStats->stats[PRESTIGE_STAT_RESILIENCE_RATING],
        prestigeStats->stats[PRESTIGE_STAT_ATTACK_POWER],
        prestigeStats->stats[PRESTIGE_STAT_CRIT_RATING],
        prestigeStats->stats[PRESTIGE_STAT_HIT_RATING],
        prestigeStats->stats[PRESTIGE_STAT_HASTE_RATING],
        prestigeStats->stats[PRESTIGE_STAT_EXPERTISE_RATING],
        prestigeStats->stats[PRESTIGE_STAT_ARMOR_PEN_RATING],
        prestigeStats->stats[PRESTIGE_STAT_SPELL_PENETRATION],
        prestigeStats->stats[PRESTIGE_STAT_MP5],
        prestigeStats->stats[PRESTIGE_STAT_SPELL_POWER],
        prestigeStats->stats[PRESTIGE_STAT_RESIST_FIRE],
        prestigeStats->stats[PRESTIGE_STAT_RESIST_FROST],
        prestigeStats->stats[PRESTIGE_STAT_RESIST_NATURE],
        prestigeStats->stats[PRESTIGE_STAT_RESIST_SHADOW],
        prestigeStats->stats[PRESTIGE_STAT_RESIST_ARCANE],
        prestigeStats->stats[PRESTIGE_STAT_RESIST_ALL],
        prestigeStats->stats[PRESTIGE_STAT_CONFIRMSPEND],

        prestigeStats->stats[PRESTIGE_STAT_PRESTIGELEVEL],
        prestigeStats->stats[PRESTIGE_STAT_UNALLOCATED],
        prestigeStats->stats[PRESTIGE_STAT_STAMINA],
        prestigeStats->stats[PRESTIGE_STAT_STRENGTH],
        prestigeStats->stats[PRESTIGE_STAT_AGILITY],
        prestigeStats->stats[PRESTIGE_STAT_INTELLECT],
        prestigeStats->stats[PRESTIGE_STAT_SPIRIT],
        prestigeStats->stats[PRESTIGE_STAT_DEFENSE_RATING],
        prestigeStats->stats[PRESTIGE_STAT_DODGE_RATING],
        prestigeStats->stats[PRESTIGE_STAT_PARRY_RATING],
        prestigeStats->stats[PRESTIGE_STAT_BLOCK_RATING],
        prestigeStats->stats[PRESTIGE_STAT_BLOCK_VALUE],
        prestigeStats->stats[PRESTIGE_STAT_BONUS_ARMOR],
        prestigeStats->stats[PRESTIGE_STAT_RESILIENCE_RATING],
        prestigeStats->stats[PRESTIGE_STAT_ATTACK_POWER],
        prestigeStats->stats[PRESTIGE_STAT_CRIT_RATING],
        prestigeStats->stats[PRESTIGE_STAT_HIT_RATING],
        prestigeStats->stats[PRESTIGE_STAT_HASTE_RATING],
        prestigeStats->stats[PRESTIGE_STAT_EXPERTISE_RATING],
        prestigeStats->stats[PRESTIGE_STAT_ARMOR_PEN_RATING],
        prestigeStats->stats[PRESTIGE_STAT_SPELL_PENETRATION],
        prestigeStats->stats[PRESTIGE_STAT_MP5],
        prestigeStats->stats[PRESTIGE_STAT_SPELL_POWER],
        prestigeStats->stats[PRESTIGE_STAT_RESIST_FIRE],
        prestigeStats->stats[PRESTIGE_STAT_RESIST_FROST],
        prestigeStats->stats[PRESTIGE_STAT_RESIST_NATURE],
        prestigeStats->stats[PRESTIGE_STAT_RESIST_SHADOW],
        prestigeStats->stats[PRESTIGE_STAT_RESIST_ARCANE],
        prestigeStats->stats[PRESTIGE_STAT_RESIST_ALL],
        prestigeStats->stats[PRESTIGE_STAT_CONFIRMSPEND]
    );
}


void ApplyPrestigeStats(Player* player, PrestigeStats* prestigeStats)
{
        for (int x = PRESTIGE_STAT_STAMINA; x<PRESTIGE_STAT_CONFIRMSPEND; ++x)
        {
            if (prestigeStats->stats[x] > 0)
            {
                int statTotal = prestigeStats->stats[x] * prestigeConfigSettings.GetStatPerPoint(x);
                int stackcount256 = statTotal / 256;
                int stackcount1 = statTotal % 256;
                if (stackcount256 > 0)
                {
                    auto spellAura256 = player->AddAura(StatToSpell256[x], player);
                    spellAura256->SetStackAmount(stackcount256);
                }
                auto spellAura1 = player->AddAura(StatToSpell[x], player);
                spellAura1->SetStackAmount(stackcount1);

            }
        }
}

void DisablePrestigeStats(Player* player)
{
    auto prestigeStats = GetPrestigeStats(player);
    if (!prestigeStats)
    {
        return;
    }

    for (int x = PRESTIGE_STAT_STAMINA; x<PRESTIGE_STAT_CONFIRMSPEND; ++x)
    {
        player->RemoveAura(StatToSpell[x]);
        player->RemoveAura(StatToSpell256[x]);
    }
}

bool TryAddPrestigeStat(PrestigeStats* prestigeStats, uint32 statToUpdate)
{
            if (prestigeStats->stats[statToUpdate] < prestigeConfigSettings.GetStatMaximum(statToUpdate))
            {
                prestigeStats->stats[statToUpdate] +=1;
                prestigeStats->stats[PRESTIGE_STAT_UNALLOCATED] -= 1;
                return true;
            }
            else
            {
                return false;
            }        
    return false;
}

void RespecPrestigeStats(PrestigeStats* prestigeStats)
{
prestigeStats->stats[PRESTIGE_STAT_UNALLOCATED] += GetTotalPrestigeStats(prestigeStats);
    for (int x = PRESTIGE_STAT_STAMINA; x<PRESTIGE_STAT_CONFIRMSPEND; ++x)
    {
        prestigeStats->stats[x]=0;
    }
}

bool HasPrestigeStats(Player* player)
{
    auto prestigeStats = GetPrestigeStats(player);
    if (!prestigeStats)
    {
        return false;
    }

    return GetTotalPrestigeStats(prestigeStats) > 0;
}

uint32 GetPrestigeStatsToSpend(Player* player)
{
    auto prestigeStats = GetPrestigeStats(player);
    if (!prestigeStats)
    {
        return 0;
    }
    return prestigeStats->stats[PRESTIGE_STAT_UNALLOCATED];
}

uint32 GetTotalPrestigeStats(PrestigeStats* prestigeStats)
{
    uint32 total = 0;
    for (int x = PRESTIGE_STAT_STAMINA; x<PRESTIGE_STAT_CONFIRMSPEND; ++x)
    {
        total += prestigeStats->stats[x];
    }
    return total;
}

uint32 GetPrestigeLevel(Player* player)
{
    auto prestigeStats = GetPrestigeStats(player);
    if (!prestigeStats)
    {
        return 0;
    }
    return prestigeStats->stats[PRESTIGE_STAT_PRESTIGELEVEL];
}

bool IsConfirmationToSpendStatPointsEnabled(Player* player, uint32 setting)
{
    if (!player)
    {
        return false;
    }

    auto prestigeStats = GetPrestigeStats(player);
    if (!prestigeStats)
    {
        return false;
    }

    return (prestigeStats->stats[PRESTIGE_STAT_CONFIRMSPEND] & setting) == setting;
}

void ToggleConfirmationToSpendStatPoints(Player* player, uint32 setting)
{
    if (!player)
    {
        return;
    }

    auto prestigeStats = GetPrestigeStats(player);
    if (!prestigeStats)
    {
        return;
    }

    if (IsConfirmationToSpendStatPointsEnabled(player, setting))
    {
        prestigeStats->stats[PRESTIGE_STAT_CONFIRMSPEND] -= setting;
    }
    else
    {
        prestigeStats->stats[PRESTIGE_STAT_CONFIRMSPEND] += setting;
    }
}

void PrestigeWorldScript::OnAfterConfigLoad(bool reload)
{
    if (reload)
    {
        SavePrestigeStats();
        ClearPrestigeStats();
    }

    prestigeConfigSettings.SetIntendedMaxLevel(sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL)); //example: 80
    auto newMaxLevelForPrestigeCalcs = sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL)+1; //example: 81
    sWorld->setIntConfig(CONFIG_MAX_PLAYER_LEVEL, newMaxLevelForPrestigeCalcs); //setting the max level to 81
    LOG_INFO("server.loading", "Prestige system enabled! Setting the new max level to ({}). Players will be able to progress to that level, but will automatically be sent back a level when they gain it.", sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL));
    
    
    prestigeConfigSettings.SetStatMaximum(PRESTIGE_STAT_STAMINA, sConfigMgr->GetOption<uint32>("Prestige.Max.Stamina", 1)); 
    prestigeConfigSettings.SetStatPerPoint(PRESTIGE_STAT_STAMINA, sConfigMgr->GetOption<uint32>("Prestige.StatPerPoint.Stamina", 1));

    prestigeConfigSettings.SetStatMaximum(PRESTIGE_STAT_STRENGTH, sConfigMgr->GetOption<uint32>("Prestige.Max.Strength", 1)); 
    prestigeConfigSettings.SetStatPerPoint(PRESTIGE_STAT_STRENGTH, sConfigMgr->GetOption<uint32>("Prestige.StatPerPoint.Strength", 1));

    prestigeConfigSettings.SetStatMaximum(PRESTIGE_STAT_AGILITY, sConfigMgr->GetOption<uint32>("Prestige.Max.Agility", 1)); 
    prestigeConfigSettings.SetStatPerPoint(PRESTIGE_STAT_AGILITY, sConfigMgr->GetOption<uint32>("Prestige.StatPerPoint.Agility", 1));

    prestigeConfigSettings.SetStatMaximum(PRESTIGE_STAT_INTELLECT, sConfigMgr->GetOption<uint32>("Prestige.Max.Intellect", 1)); 
    prestigeConfigSettings.SetStatPerPoint(PRESTIGE_STAT_INTELLECT, sConfigMgr->GetOption<uint32>("Prestige.StatPerPoint.Intellect", 1));

    prestigeConfigSettings.SetStatMaximum(PRESTIGE_STAT_SPIRIT, sConfigMgr->GetOption<uint32>("Prestige.Max.Spirit", 1)); 
    prestigeConfigSettings.SetStatPerPoint(PRESTIGE_STAT_SPIRIT, sConfigMgr->GetOption<uint32>("Prestige.StatPerPoint.Spirit", 1));

    prestigeConfigSettings.SetStatMaximum(PRESTIGE_STAT_ATTACK_POWER, sConfigMgr->GetOption<uint32>("Prestige.Max.AttackPower", 1)); 
    prestigeConfigSettings.SetStatPerPoint(PRESTIGE_STAT_ATTACK_POWER, sConfigMgr->GetOption<uint32>("Prestige.StatPerPoint.AttackPower", 1));

    prestigeConfigSettings.SetStatMaximum(PRESTIGE_STAT_SPELL_POWER, sConfigMgr->GetOption<uint32>("Prestige.Max.SpellPower", 1)); 
    prestigeConfigSettings.SetStatPerPoint(PRESTIGE_STAT_SPELL_POWER, sConfigMgr->GetOption<uint32>("Prestige.StatPerPoint.SpellPower", 1));

    prestigeConfigSettings.SetStatMaximum(PRESTIGE_STAT_CRIT_RATING, sConfigMgr->GetOption<uint32>("Prestige.Max.CritRating", 1)); 
    prestigeConfigSettings.SetStatPerPoint(PRESTIGE_STAT_CRIT_RATING, sConfigMgr->GetOption<uint32>("Prestige.StatPerPoint.CritRating", 1));

    prestigeConfigSettings.SetStatMaximum(PRESTIGE_STAT_HIT_RATING, sConfigMgr->GetOption<uint32>("Prestige.Max.HitRating", 1)); 
    prestigeConfigSettings.SetStatPerPoint(PRESTIGE_STAT_HIT_RATING, sConfigMgr->GetOption<uint32>("Prestige.StatPerPoint.HitRating", 1));

    prestigeConfigSettings.SetStatMaximum(PRESTIGE_STAT_EXPERTISE_RATING, sConfigMgr->GetOption<uint32>("Prestige.Max.ExpertiseRating", 1)); 
    prestigeConfigSettings.SetStatPerPoint(PRESTIGE_STAT_EXPERTISE_RATING, sConfigMgr->GetOption<uint32>("Prestige.StatPerPoint.ExpertiseRating", 1));

    prestigeConfigSettings.SetStatMaximum(PRESTIGE_STAT_HASTE_RATING, sConfigMgr->GetOption<uint32>("Prestige.Max.HasteRating", 1)); 
    prestigeConfigSettings.SetStatPerPoint(PRESTIGE_STAT_HASTE_RATING, sConfigMgr->GetOption<uint32>("Prestige.StatPerPoint.HasteRating", 1));

    prestigeConfigSettings.SetStatMaximum(PRESTIGE_STAT_ARMOR_PEN_RATING, sConfigMgr->GetOption<uint32>("Prestige.Max.ArmorPenRating", 1)); 
    prestigeConfigSettings.SetStatPerPoint(PRESTIGE_STAT_ARMOR_PEN_RATING, sConfigMgr->GetOption<uint32>("Prestige.StatPerPoint.ArmorPenRating", 1));

    prestigeConfigSettings.SetStatMaximum(PRESTIGE_STAT_SPELL_PENETRATION, sConfigMgr->GetOption<uint32>("Prestige.Max.SpellPen", 1)); 
    prestigeConfigSettings.SetStatPerPoint(PRESTIGE_STAT_SPELL_PENETRATION, sConfigMgr->GetOption<uint32>("Prestige.StatPerPoint.SpellPen", 1));

    prestigeConfigSettings.SetStatMaximum(PRESTIGE_STAT_MP5, sConfigMgr->GetOption<uint32>("Prestige.Max.Mp5", 1)); 
    prestigeConfigSettings.SetStatPerPoint(PRESTIGE_STAT_MP5, sConfigMgr->GetOption<uint32>("Prestige.StatPerPoint.Mp5", 1));

    prestigeConfigSettings.SetStatMaximum(PRESTIGE_STAT_DEFENSE_RATING, sConfigMgr->GetOption<uint32>("Prestige.Max.DefenseRating", 1)); 
    prestigeConfigSettings.SetStatPerPoint(PRESTIGE_STAT_DEFENSE_RATING, sConfigMgr->GetOption<uint32>("Prestige.StatPerPoint.DefenseRating", 1));

    prestigeConfigSettings.SetStatMaximum(PRESTIGE_STAT_DODGE_RATING, sConfigMgr->GetOption<uint32>("Prestige.Max.DodgeRating", 1)); 
    prestigeConfigSettings.SetStatPerPoint(PRESTIGE_STAT_DODGE_RATING, sConfigMgr->GetOption<uint32>("Prestige.StatPerPoint.DodgeRating", 1));

    prestigeConfigSettings.SetStatMaximum(PRESTIGE_STAT_PARRY_RATING, sConfigMgr->GetOption<uint32>("Prestige.Max.ParryRating", 1)); 
    prestigeConfigSettings.SetStatPerPoint(PRESTIGE_STAT_PARRY_RATING, sConfigMgr->GetOption<uint32>("Prestige.StatPerPoint.ParryRating", 1));

    prestigeConfigSettings.SetStatMaximum(PRESTIGE_STAT_BLOCK_RATING, sConfigMgr->GetOption<uint32>("Prestige.Max.BlockRating", 1)); 
    prestigeConfigSettings.SetStatPerPoint(PRESTIGE_STAT_BLOCK_RATING, sConfigMgr->GetOption<uint32>("Prestige.StatPerPoint.BlockRating", 1));

    prestigeConfigSettings.SetStatMaximum(PRESTIGE_STAT_BLOCK_VALUE, sConfigMgr->GetOption<uint32>("Prestige.Max.BlockValue", 1)); 
    prestigeConfigSettings.SetStatPerPoint(PRESTIGE_STAT_BLOCK_VALUE, sConfigMgr->GetOption<uint32>("Prestige.StatPerPoint.BlockValue", 1));

    prestigeConfigSettings.SetStatMaximum(PRESTIGE_STAT_RESILIENCE_RATING, sConfigMgr->GetOption<uint32>("Prestige.Max.ResilRating", 1)); 
    prestigeConfigSettings.SetStatPerPoint(PRESTIGE_STAT_RESILIENCE_RATING, sConfigMgr->GetOption<uint32>("Prestige.StatPerPoint.ResilRating", 1));

    prestigeConfigSettings.SetStatMaximum(PRESTIGE_STAT_BONUS_ARMOR, sConfigMgr->GetOption<uint32>("Prestige.Max.BonusArmor", 1)); 
    prestigeConfigSettings.SetStatPerPoint(PRESTIGE_STAT_BONUS_ARMOR, sConfigMgr->GetOption<uint32>("Prestige.StatPerPoint.BonusArmor", 1));

    prestigeConfigSettings.SetStatMaximum(PRESTIGE_STAT_RESIST_FIRE, sConfigMgr->GetOption<uint32>("Prestige.Max.ResistFire", 1)); 
    prestigeConfigSettings.SetStatPerPoint(PRESTIGE_STAT_RESIST_FIRE, sConfigMgr->GetOption<uint32>("Prestige.StatPerPoint.ResistFire", 1));

    prestigeConfigSettings.SetStatMaximum(PRESTIGE_STAT_RESIST_FROST, sConfigMgr->GetOption<uint32>("Prestige.Max.ResistFrost", 1)); 
    prestigeConfigSettings.SetStatPerPoint(PRESTIGE_STAT_RESIST_FROST, sConfigMgr->GetOption<uint32>("Prestige.StatPerPoint.ResistFrost", 1));

    prestigeConfigSettings.SetStatMaximum(PRESTIGE_STAT_RESIST_NATURE, sConfigMgr->GetOption<uint32>("Prestige.Max.ResistNature", 1)); 
    prestigeConfigSettings.SetStatPerPoint(PRESTIGE_STAT_RESIST_NATURE, sConfigMgr->GetOption<uint32>("Prestige.StatPerPoint.ResistNature", 1));

    prestigeConfigSettings.SetStatMaximum(PRESTIGE_STAT_RESIST_SHADOW, sConfigMgr->GetOption<uint32>("Prestige.Max.ResistShadow", 1)); 
    prestigeConfigSettings.SetStatPerPoint(PRESTIGE_STAT_RESIST_SHADOW, sConfigMgr->GetOption<uint32>("Prestige.StatPerPoint.ResistShadow", 1));

    prestigeConfigSettings.SetStatMaximum(PRESTIGE_STAT_RESIST_ARCANE, sConfigMgr->GetOption<uint32>("Prestige.Max.ResistArcane", 1)); 
    prestigeConfigSettings.SetStatPerPoint(PRESTIGE_STAT_RESIST_ARCANE, sConfigMgr->GetOption<uint32>("Prestige.StatPerPoint.ResistArcane", 1));

    prestigeConfigSettings.SetStatMaximum(PRESTIGE_STAT_RESIST_ALL, sConfigMgr->GetOption<uint32>("Prestige.Max.ResistAll", 1)); 
    prestigeConfigSettings.SetStatPerPoint(PRESTIGE_STAT_RESIST_ALL, sConfigMgr->GetOption<uint32>("Prestige.StatPerPoint.ResistAll", 1));

    prestigeConfigSettings.SetResetCost(sConfigMgr->GetOption<uint32>("Prestige.ResetCost", 0));
    prestigeConfigSettings.SetPrestigeEnabled(sConfigMgr->GetOption<bool>("Prestige.Enable", false));
    prestigeConfigSettings.SetDisablePVP(sConfigMgr->GetOption<bool>("Prestige.DisablePvP", false));
    prestigeConfigSettings.SetLevelUpFormulaType(sConfigMgr->GetOption<uint16>("Prestige.LevelUpFormula.Type", 1));
    prestigeConfigSettings.SetLevelUpFormulaBase(sConfigMgr->GetOption<uint32>("Prestige.LevelUpFormula.Base", 10000));
    prestigeConfigSettings.SetLevelUpFormulaR(sConfigMgr->GetOption<uint16>("Prestige.LevelUpFormula.r", 1));
    prestigeConfigSettings.SetLevelUpFormulaK(sConfigMgr->GetOption<uint16>("Prestige.LevelUpFormula.k", 1));
 
    LoadPrestigeStats();
}

void PrestigePlayerScript::OnPlayerLevelChanged(Player* player, uint8 oldLevel)
{
    if (oldLevel == prestigeConfigSettings.GetIntendedMaxLevel() && player->GetLevel() == sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL))
    {
        LOG_INFO("module", "Player ({}) has gained a prestige level! Dropping their level back to the intended max level of ({})", player->GetName(), prestigeConfigSettings.GetIntendedMaxLevel());
        player->GiveLevel(oldLevel);
        PrestigeLevelUp(player); 
    }
}

void PrestigeLevelUp(Player* player)
{
    auto prestigeStats = GetPrestigeStats(player);
    if (!prestigeStats)
    {
        LOG_INFO("module", "Failed to get prestige stats for player '{}' with guid '{}'.", player->GetName(), player->GetGUID().GetRawValue());
        return;
    }
    prestigeStats->stats[PRESTIGE_STAT_PRESTIGELEVEL] += 1;
    player->CastSpell(player, 47292, true); //doesn't play levelup visual by default when bouncing up and down levels
    AddPrestigePoint(player);
    ChatHandler(player->GetSession()).PSendSysMessage("You have gained Prestige Level {}!", prestigeStats->stats[PRESTIGE_STAT_PRESTIGELEVEL]);
    InitPrestigeExpTnl(player);
}

void InitPrestigeExpTnl(Player* player)
{
    auto prestigeStats = GetPrestigeStats(player);
    if (!prestigeStats)
    {
        LOG_INFO("module", "Failed to get prestige stats for player '{}' with guid '{}'.", player->GetName(), player->GetGUID().GetRawValue());
        return;
    }

    //only alter stats if at max level
  if (player->GetLevel() == prestigeConfigSettings.GetIntendedMaxLevel()){
  //   2 - Exponential increase with rate. Total exp needed to level = base * (1 + ((prestigelvl*r)/100) * (pow(prestigelvl,2)/k))
        if(prestigeConfigSettings.GetLevelUpFormualType() == 2)
        {
            double newExpTNL = prestigeConfigSettings.GetLevelUpFormulaBase() * (1+(((prestigeStats->stats[PRESTIGE_STAT_PRESTIGELEVEL]+1)*prestigeConfigSettings.GetLevelUpFormulaR()/100))) *(pow(prestigeStats->stats[PRESTIGE_STAT_PRESTIGELEVEL],2)/prestigeConfigSettings.GetLevelUpFormulaK());
            player->SetUInt32Value(PLAYER_NEXT_LEVEL_XP, static_cast<uint32>(newExpTNL));
            return;
        }
       //    1 - Linear increase with rate. Total exp needed to level = base*(100+(prestigelvl*r)/100)
        if(prestigeConfigSettings.GetLevelUpFormualType() == 1)
        {
            double newExpTNL = prestigeConfigSettings.GetLevelUpFormulaBase() * ((100.0 + ((prestigeStats->stats[PRESTIGE_STAT_PRESTIGELEVEL]+1)*prestigeConfigSettings.GetLevelUpFormulaR())) / 100.0);
            player->SetUInt32Value(PLAYER_NEXT_LEVEL_XP, static_cast<uint32>(newExpTNL));
        }
      }
}

/*currently has no additional functionality. Included for future updates*/
bool PrestigeCreatureScript::OnGossipHello(Player* player, Creature* creature)
{
    PrestigeMainMenu(player);
    return true;
}

void PrestigeMainMenu(Player* player)
{
    ClearGossipMenuFor(player);
    auto prestigeStats = GetPrestigeStats(player);
    if (!prestigeStats)
    {
        return;
    }
    if (!prestigeConfigSettings.IsPrestigeEnabled())
    {
        SendGossipMenuFor(player, PRESTIGE_NPC_TEXT_DISABLED,player->GetGUID());
        return;
    }
    if(player->IsInCombat())
    {
        player->SendSystemMessage("You cannot change prestige stats while in combat!");
        return;
    }
    if(player->GetLevel()<prestigeConfigSettings.GetIntendedMaxLevel())
    {
        player->SendSystemMessage(Acore::StringFormat("You cannot access the prestige system until you hit the max level of {}", prestigeConfigSettings.GetIntendedMaxLevel()));
        return;
    }
    std::string optGetPrestigeLevel = Acore::StringFormat("|TInterface\\GossipFrame\\TrainerGossipIcon:16|t Current Prestige Level: {} (click for details)",prestigeStats->stats[PRESTIGE_STAT_PRESTIGELEVEL]);
    AddGossipItemFor(player, GOSSIP_ICON_DOT, optGetPrestigeLevel, GOSSIP_SENDER_MAIN, PRESTIGE_GOSSIP_DISPLAY_CURRENT_STATS);
    AddGossipItemFor(player, GOSSIP_ICON_DOT, Acore::StringFormat("|TInterface\\GossipFrame\\TrainerGossipIcon:16|t |cffFF0000{} |rAttribute(s) to spend.", GetPrestigeStatsToSpend(player)), GOSSIP_SENDER_MAIN, PRESTIGE_GOSSIP_ALLOCATE_MAIN_MENU);
    AddGossipItemFor(player, GOSSIP_ICON_DOT, "|TInterface\\GossipFrame\\TrainerGossipIcon:16|t Core Stats", GOSSIP_SENDER_MAIN, PRESTIGE_GOSSIP_ALLOCATE_CORE_STATS);
    AddGossipItemFor(player, GOSSIP_ICON_DOT, "|TInterface\\GossipFrame\\TrainerGossipIcon:16|t Secondary Stats", GOSSIP_SENDER_MAIN, PRESTIGE_GOSSIP_ALLOCATE_SECONDARY_STATS);
    AddGossipItemFor(player, GOSSIP_ICON_DOT, "|TInterface\\GossipFrame\\TrainerGossipIcon:16|t Defensive Stats", GOSSIP_SENDER_MAIN, PRESTIGE_GOSSIP_ALLOCATE_DEFENSIVE_STATS);
    AddGossipItemFor(player, GOSSIP_ICON_DOT, "|TInterface\\GossipFrame\\TrainerGossipIcon:16|t Resistance Stats", GOSSIP_SENDER_MAIN, PRESTIGE_GOSSIP_ALLOCATE_RESISTANCE_STATS);
    AddGossipItemFor(player, GOSSIP_ICON_DOT, "|TInterface\\GossipFrame\\HealerGossipIcon:16|t Settings", GOSSIP_SENDER_MAIN, PRESTIGE_GOSSIP_SETTINGS);
    if (HasPrestigeStats(player))
    {
        AddGossipItemFor(player, GOSSIP_ICON_DOT, "|TInterface\\GossipFrame\\UnlearnGossipIcon:16|t Reset Attributes", GOSSIP_SENDER_MAIN, PRESTIGE_GOSSIP_ALLOCATE_RESET, "Are you sure you want to reset your attributes?", prestigeConfigSettings.GetResetCost(), false);
    }
    //done to allow players to open the menu from anywhere while still having an id we can detect and close for OnEnterCombat()
    //I am likely doing something incorrect here but this works for now
    player->PlayerTalkClass->GetGossipMenu().SetMenuId(PRESTIGE_MENU_ID);
    SendGossipMenuFor(player, PRESTIGE_NPC_TEXT_GENERIC, player->GetGUID());
    return;
}
void PrestigeCoreStatsMenu(Player* player)
{
    ClearGossipMenuFor(player);
    auto prestigeStats = GetPrestigeStats(player);
    if (!prestigeStats)
    {
        CloseGossipMenuFor(player);
        return;
    }
    AddGossipItemFor(player, GOSSIP_ICON_DOT, Acore::StringFormat("|TInterface\\GossipFrame\\TrainerGossipIcon:16|t |cffFF0000{} |rAttribute(s) to spend.", GetPrestigeStatsToSpend(player)), GOSSIP_SENDER_MAIN, PRESTIGE_GOSSIP_ALLOCATE_CORE_STATS);

    std::string optStamina = 
        Acore::StringFormat("|TInterface\\MINIMAP\\UI-Minimap-ZoomInButton-Up:16|t {}Stamina ({}) {}",
        prestigeStats->stats[PRESTIGE_STAT_STAMINA] >= prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_STAMINA) ? "|cff777777" : "|cff000000",
        Acore::StringFormat("{}/{}", prestigeStats->stats[PRESTIGE_STAT_STAMINA], prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_STAMINA)),
        prestigeStats->stats[PRESTIGE_STAT_STAMINA] >= prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_STAMINA) ? "|cffFF0000(MAXED)|r" : "");

    std::string optStrength = 
        Acore::StringFormat("|TInterface\\MINIMAP\\UI-Minimap-ZoomInButton-Up:16|t {}Strength ({}) {}",
        prestigeStats->stats[PRESTIGE_STAT_STRENGTH] >= prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_STRENGTH) ? "|cff777777" : "|cff000000",
        Acore::StringFormat("{}/{}", prestigeStats->stats[PRESTIGE_STAT_STRENGTH], prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_STRENGTH)),
        prestigeStats->stats[PRESTIGE_STAT_STRENGTH] >= prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_STRENGTH) ? "|cffFF0000(MAXED)|r" : "");

    std::string optAgility = 
        Acore::StringFormat("|TInterface\\MINIMAP\\UI-Minimap-ZoomInButton-Up:16|t {}Agility ({}) {}",
        prestigeStats->stats[PRESTIGE_STAT_AGILITY] >= prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_AGILITY) ? "|cff777777" : "|cff000000",
        Acore::StringFormat("{}/{}", prestigeStats->stats[PRESTIGE_STAT_AGILITY], prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_AGILITY)),
        prestigeStats->stats[PRESTIGE_STAT_AGILITY] >= prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_AGILITY) ? "|cffFF0000(MAXED)|r" : "");

    std::string optIntellect = 
        Acore::StringFormat("|TInterface\\MINIMAP\\UI-Minimap-ZoomInButton-Up:16|t {}Intellect ({}) {}",
        prestigeStats->stats[PRESTIGE_STAT_INTELLECT] >= prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_INTELLECT) ? "|cff777777" : "|cff000000",
        Acore::StringFormat("{}/{}", prestigeStats->stats[PRESTIGE_STAT_INTELLECT], prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_INTELLECT)),
        prestigeStats->stats[PRESTIGE_STAT_INTELLECT] >= prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_INTELLECT) ? "|cffFF0000(MAXED)|r" : "");

    std::string optSpirit = 
        Acore::StringFormat("|TInterface\\MINIMAP\\UI-Minimap-ZoomInButton-Up:16|t {}Spirit ({}) {}",
        prestigeStats->stats[PRESTIGE_STAT_SPIRIT] >= prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_SPIRIT) ? "|cff777777" : "|cff000000",
        Acore::StringFormat("{}/{}", prestigeStats->stats[PRESTIGE_STAT_SPIRIT], prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_SPIRIT)),
        prestigeStats->stats[PRESTIGE_STAT_SPIRIT] >= prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_SPIRIT) ? "|cffFF0000(MAXED)|r" : "");

    if (IsConfirmationToSpendStatPointsEnabled(player, SETTING_CONFIRM_SPEND))
    {
        if (prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_STAMINA) > 0) AddGossipItemFor(player, GOSSIP_ICON_DOT, optStamina, GOSSIP_SENDER_MAIN, PRESTIGE_STAT_STAMINA, "Are you sure you want to spend your points in stamina?", 0, false);
        if (prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_STRENGTH) > 0) AddGossipItemFor(player, GOSSIP_ICON_DOT, optStrength, GOSSIP_SENDER_MAIN, PRESTIGE_STAT_STRENGTH, "Are you sure you want to spend your points in strength?", 0, false);
        if (prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_AGILITY) > 0) AddGossipItemFor(player, GOSSIP_ICON_DOT, optAgility, GOSSIP_SENDER_MAIN, PRESTIGE_STAT_AGILITY, "Are you sure you want to spend your points in agility?", 0, false);
        if (prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_INTELLECT) > 0) AddGossipItemFor(player, GOSSIP_ICON_DOT, optIntellect, GOSSIP_SENDER_MAIN, PRESTIGE_STAT_INTELLECT, "Are you sure you want to spend your points in intellect?", 0, false);
        if (prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_SPIRIT) > 0) AddGossipItemFor(player, GOSSIP_ICON_DOT, optSpirit, GOSSIP_SENDER_MAIN, PRESTIGE_STAT_SPIRIT, "Are you sure you want to spend your points in spirit?", 0, false);
    }
    else
    {
        if (prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_STAMINA) > 0) AddGossipItemFor(player, GOSSIP_ICON_DOT, optStamina, GOSSIP_SENDER_MAIN, PRESTIGE_STAT_STAMINA);
        if (prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_STRENGTH) > 0) AddGossipItemFor(player, GOSSIP_ICON_DOT, optStrength, GOSSIP_SENDER_MAIN, PRESTIGE_STAT_STRENGTH);
        if (prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_AGILITY) > 0) AddGossipItemFor(player, GOSSIP_ICON_DOT, optAgility, GOSSIP_SENDER_MAIN, PRESTIGE_STAT_AGILITY);
        if (prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_INTELLECT) > 0) AddGossipItemFor(player, GOSSIP_ICON_DOT, optIntellect, GOSSIP_SENDER_MAIN, PRESTIGE_STAT_INTELLECT);
        if (prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_SPIRIT) > 0) AddGossipItemFor(player, GOSSIP_ICON_DOT, optSpirit, GOSSIP_SENDER_MAIN, PRESTIGE_STAT_SPIRIT);
    }
    AddGossipItemFor(player, GOSSIP_ICON_DOT, "|TInterface\\MONEYFRAME\\Arrow-Left-Down:16|t Back to main menu", GOSSIP_SENDER_MAIN, PRESTIGE_GOSSIP_ALLOCATE_MAIN_MENU);
    SendGossipMenuFor(player, PRESTIGE_NPC_TEXT_HAS_ATTRIBUTES, player->GetGUID());
    return;
}

void PrestigeSecondaryStatsMenu(Player* player)
{
    ClearGossipMenuFor(player);
    auto prestigeStats = GetPrestigeStats(player);
    if (!prestigeStats)
    {
        CloseGossipMenuFor(player);
        return;
    }
    AddGossipItemFor(player, GOSSIP_ICON_DOT, Acore::StringFormat("|TInterface\\GossipFrame\\TrainerGossipIcon:16|t |cffFF0000{} |rAttribute(s) to spend.", GetPrestigeStatsToSpend(player)), GOSSIP_SENDER_MAIN, PRESTIGE_GOSSIP_ALLOCATE_SECONDARY_STATS);
    
    std::string optSpellPower = 
        Acore::StringFormat("|TInterface\\MINIMAP\\UI-Minimap-ZoomInButton-Up:16|t {}Spell Power ({}) {}",
        prestigeStats->stats[PRESTIGE_STAT_SPELL_POWER] >= prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_SPELL_POWER) ? "|cff777777" : "|cff000000",
        Acore::StringFormat("{}/{}", prestigeStats->stats[PRESTIGE_STAT_SPELL_POWER], prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_SPELL_POWER)),
        prestigeStats->stats[PRESTIGE_STAT_SPELL_POWER] >= prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_SPELL_POWER) ? "|cffFF0000(MAXED)|r" : "");

    std::string optCritRating = 
        Acore::StringFormat("|TInterface\\MINIMAP\\UI-Minimap-ZoomInButton-Up:16|t {}Crit Rating ({}) {}",
        prestigeStats->stats[PRESTIGE_STAT_CRIT_RATING] >= prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_CRIT_RATING) ? "|cff777777" : "|cff000000",
        Acore::StringFormat("{}/{}", prestigeStats->stats[PRESTIGE_STAT_CRIT_RATING], prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_CRIT_RATING)),
        prestigeStats->stats[PRESTIGE_STAT_CRIT_RATING] >= prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_CRIT_RATING) ? "|cffFF0000(MAXED)|r" : "");

    std::string optHitRating = 
        Acore::StringFormat("|TInterface\\MINIMAP\\UI-Minimap-ZoomInButton-Up:16|t {}Hit Rating ({}) {}",
        prestigeStats->stats[PRESTIGE_STAT_HIT_RATING] >= prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_HIT_RATING) ? "|cff777777" : "|cff000000",
        Acore::StringFormat("{}/{}", prestigeStats->stats[PRESTIGE_STAT_HIT_RATING], prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_HIT_RATING)),
        prestigeStats->stats[PRESTIGE_STAT_HIT_RATING] >= prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_HIT_RATING) ? "|cffFF0000(MAXED)|r" : "");

    std::string optHasteRating = 
        Acore::StringFormat("|TInterface\\MINIMAP\\UI-Minimap-ZoomInButton-Up:16|t {}Haste Rating ({}) {}",
        prestigeStats->stats[PRESTIGE_STAT_HASTE_RATING] >= prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_HASTE_RATING) ? "|cff777777" : "|cff000000",
        Acore::StringFormat("{}/{}", prestigeStats->stats[PRESTIGE_STAT_HASTE_RATING], prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_HASTE_RATING)),
        prestigeStats->stats[PRESTIGE_STAT_HASTE_RATING] >= prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_HASTE_RATING) ? "|cffFF0000(MAXED)|r" : "");

    std::string optExpertiseRating = 
        Acore::StringFormat("|TInterface\\MINIMAP\\UI-Minimap-ZoomInButton-Up:16|t {}Expertise Rating ({}) {}",
        prestigeStats->stats[PRESTIGE_STAT_EXPERTISE_RATING] >= prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_EXPERTISE_RATING) ? "|cff777777" : "|cff000000",
        Acore::StringFormat("{}/{}", prestigeStats->stats[PRESTIGE_STAT_EXPERTISE_RATING], prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_EXPERTISE_RATING)),
        prestigeStats->stats[PRESTIGE_STAT_EXPERTISE_RATING] >= prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_EXPERTISE_RATING) ? "|cffFF0000(MAXED)|r" : "");

    std::string optArmorPenRating = 
        Acore::StringFormat("|TInterface\\MINIMAP\\UI-Minimap-ZoomInButton-Up:16|t {}Armor Penetration Rating ({}) {}",
        prestigeStats->stats[PRESTIGE_STAT_ARMOR_PEN_RATING] >= prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_ARMOR_PEN_RATING) ? "|cff777777" : "|cff000000",
        Acore::StringFormat("{}/{}", prestigeStats->stats[PRESTIGE_STAT_ARMOR_PEN_RATING], prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_ARMOR_PEN_RATING)),
        prestigeStats->stats[PRESTIGE_STAT_ARMOR_PEN_RATING] >= prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_ARMOR_PEN_RATING) ? "|cffFF0000(MAXED)|r" : "");

    std::string optAttackPower = 
        Acore::StringFormat("|TInterface\\MINIMAP\\UI-Minimap-ZoomInButton-Up:16|t {}Attack Power ({}) {}",
        prestigeStats->stats[PRESTIGE_STAT_ATTACK_POWER] >= prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_ATTACK_POWER) ? "|cff777777" : "|cff000000",
        Acore::StringFormat("{}/{}", prestigeStats->stats[PRESTIGE_STAT_ATTACK_POWER], prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_ATTACK_POWER)),
        prestigeStats->stats[PRESTIGE_STAT_ATTACK_POWER] >= prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_ATTACK_POWER) ? "|cffFF0000(MAXED)|r" : "");

    std::string optSpellPen = 
        Acore::StringFormat("|TInterface\\MINIMAP\\UI-Minimap-ZoomInButton-Up:16|t {}Spell Penetration ({}) {}",
        prestigeStats->stats[PRESTIGE_STAT_SPELL_PENETRATION] >= prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_SPELL_PENETRATION) ? "|cff777777" : "|cff000000",
        Acore::StringFormat("{}/{}", prestigeStats->stats[PRESTIGE_STAT_SPELL_PENETRATION], prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_SPELL_PENETRATION)),
        prestigeStats->stats[PRESTIGE_STAT_SPELL_PENETRATION] >= prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_SPELL_PENETRATION) ? "|cffFF0000(MAXED)|r" : "");

    std::string optMp5 = 
        Acore::StringFormat("|TInterface\\MINIMAP\\UI-Minimap-ZoomInButton-Up:16|t {}MP5 ({}) {}",
        prestigeStats->stats[PRESTIGE_STAT_MP5] >= prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_MP5) ? "|cff777777" : "|cff000000",
        Acore::StringFormat("{}/{}", prestigeStats->stats[PRESTIGE_STAT_MP5], prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_MP5)),
        prestigeStats->stats[PRESTIGE_STAT_MP5] >= prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_MP5) ? "|cffFF0000(MAXED)|r" : "");


    if (IsConfirmationToSpendStatPointsEnabled(player, SETTING_CONFIRM_SPEND))
    { 
        if (prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_ATTACK_POWER) > 0) AddGossipItemFor(player, GOSSIP_ICON_DOT, optAttackPower, GOSSIP_SENDER_MAIN, PRESTIGE_STAT_ATTACK_POWER, "Are you sure you want to spend your points in attack power?", 0, false);
        if (prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_SPELL_POWER) > 0) AddGossipItemFor(player, GOSSIP_ICON_DOT, optSpellPower, GOSSIP_SENDER_MAIN, PRESTIGE_STAT_SPELL_POWER, "Are you sure you want to spend your points in spell power?", 0, false);
        if (prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_CRIT_RATING) > 0) AddGossipItemFor(player, GOSSIP_ICON_DOT, optCritRating, GOSSIP_SENDER_MAIN, PRESTIGE_STAT_CRIT_RATING, "Are you sure you want to spend your points in crit rating?", 0, false);
        if (prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_HIT_RATING) > 0) AddGossipItemFor(player, GOSSIP_ICON_DOT, optHitRating, GOSSIP_SENDER_MAIN, PRESTIGE_STAT_HIT_RATING, "Are you sure you want to spend your points in hit rating?", 0, false);
        if (prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_EXPERTISE_RATING) > 0) AddGossipItemFor(player, GOSSIP_ICON_DOT, optExpertiseRating, GOSSIP_SENDER_MAIN, PRESTIGE_STAT_EXPERTISE_RATING, "Are you sure you want to spend your points in expertise rating?", 0, false);
        if (prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_HASTE_RATING) > 0) AddGossipItemFor(player, GOSSIP_ICON_DOT, optHasteRating, GOSSIP_SENDER_MAIN, PRESTIGE_STAT_HASTE_RATING, "Are you sure you want to spend your points in haste rating?", 0, false);
        if (prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_ARMOR_PEN_RATING) > 0) AddGossipItemFor(player, GOSSIP_ICON_DOT, optArmorPenRating, GOSSIP_SENDER_MAIN, PRESTIGE_STAT_ARMOR_PEN_RATING, "Are you sure you want to spend your points in armor penetration rating?", 0, false);
        if (prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_SPELL_PENETRATION) > 0) AddGossipItemFor(player, GOSSIP_ICON_DOT, optSpellPen, GOSSIP_SENDER_MAIN, PRESTIGE_STAT_SPELL_PENETRATION, "Are you sure you want to spend your points in spell penetration?", 0, false);
        if (prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_MP5) > 0) AddGossipItemFor(player, GOSSIP_ICON_DOT, optMp5, GOSSIP_SENDER_MAIN, PRESTIGE_STAT_MP5, "Are you sure you want to spend your points in mp5?", 0, false);
    } 
    else 
    { 
        if (prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_ATTACK_POWER) > 0) AddGossipItemFor(player, GOSSIP_ICON_DOT, optAttackPower, GOSSIP_SENDER_MAIN, PRESTIGE_STAT_ATTACK_POWER);
        if (prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_SPELL_POWER) > 0) AddGossipItemFor(player, GOSSIP_ICON_DOT, optSpellPower, GOSSIP_SENDER_MAIN, PRESTIGE_STAT_SPELL_POWER);
        if (prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_CRIT_RATING) > 0) AddGossipItemFor(player, GOSSIP_ICON_DOT, optCritRating, GOSSIP_SENDER_MAIN, PRESTIGE_STAT_CRIT_RATING);
        if (prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_HIT_RATING) > 0) AddGossipItemFor(player, GOSSIP_ICON_DOT, optHitRating, GOSSIP_SENDER_MAIN, PRESTIGE_STAT_HIT_RATING);
        if (prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_EXPERTISE_RATING) > 0) AddGossipItemFor(player, GOSSIP_ICON_DOT, optExpertiseRating, GOSSIP_SENDER_MAIN, PRESTIGE_STAT_EXPERTISE_RATING);
        if (prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_HASTE_RATING) > 0) AddGossipItemFor(player, GOSSIP_ICON_DOT, optHasteRating, GOSSIP_SENDER_MAIN, PRESTIGE_STAT_HASTE_RATING);
        if (prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_ARMOR_PEN_RATING) > 0) AddGossipItemFor(player, GOSSIP_ICON_DOT, optArmorPenRating, GOSSIP_SENDER_MAIN, PRESTIGE_STAT_ARMOR_PEN_RATING);
        if (prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_SPELL_PENETRATION) > 0) AddGossipItemFor(player, GOSSIP_ICON_DOT, optSpellPen, GOSSIP_SENDER_MAIN, PRESTIGE_STAT_SPELL_PENETRATION);
        if (prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_MP5) > 0) AddGossipItemFor(player, GOSSIP_ICON_DOT, optMp5, GOSSIP_SENDER_MAIN, PRESTIGE_STAT_MP5);
    }

    AddGossipItemFor(player, GOSSIP_ICON_DOT, "|TInterface\\MONEYFRAME\\Arrow-Left-Down:16|t Back to main menu", GOSSIP_SENDER_MAIN, PRESTIGE_GOSSIP_ALLOCATE_MAIN_MENU);
    SendGossipMenuFor(player, PRESTIGE_NPC_TEXT_HAS_ATTRIBUTES, player->GetGUID());
    return;
}


void PrestigeDefensiveStatsMenu(Player* player)
{
    ClearGossipMenuFor(player);
    auto prestigeStats = GetPrestigeStats(player);
    if (!prestigeStats)
    {
        CloseGossipMenuFor(player);
        return;
    }
    AddGossipItemFor(player, GOSSIP_ICON_DOT, Acore::StringFormat("|TInterface\\GossipFrame\\TrainerGossipIcon:16|t |cffFF0000{} |rAttribute(s) to spend.", GetPrestigeStatsToSpend(player)), GOSSIP_SENDER_MAIN, PRESTIGE_GOSSIP_ALLOCATE_DEFENSIVE_STATS);
    
    std::string optDefenseRating = 
        Acore::StringFormat("|TInterface\\MINIMAP\\UI-Minimap-ZoomInButton-Up:16|t {}Defense Rating ({}) {}",
        prestigeStats->stats[PRESTIGE_STAT_DEFENSE_RATING] >= prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_DEFENSE_RATING) ? "|cff777777" : "|cff000000",
        Acore::StringFormat("{}/{}", prestigeStats->stats[PRESTIGE_STAT_DEFENSE_RATING], prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_DEFENSE_RATING)),
        prestigeStats->stats[PRESTIGE_STAT_DEFENSE_RATING] >= prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_DEFENSE_RATING) ? "|cffFF0000(MAXED)|r" : "");

    std::string optDodgeRating = 
        Acore::StringFormat("|TInterface\\MINIMAP\\UI-Minimap-ZoomInButton-Up:16|t {}Dodge Rating ({}) {}",
        prestigeStats->stats[PRESTIGE_STAT_DODGE_RATING] >= prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_DODGE_RATING) ? "|cff777777" : "|cff000000",
        Acore::StringFormat("{}/{}", prestigeStats->stats[PRESTIGE_STAT_DODGE_RATING], prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_DODGE_RATING)),
        prestigeStats->stats[PRESTIGE_STAT_DODGE_RATING] >= prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_DODGE_RATING) ? "|cffFF0000(MAXED)|r" : "");

    std::string optParryRating = 
        Acore::StringFormat("|TInterface\\MINIMAP\\UI-Minimap-ZoomInButton-Up:16|t {}Parry Rating ({}) {}",
        prestigeStats->stats[PRESTIGE_STAT_PARRY_RATING] >= prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_PARRY_RATING) ? "|cff777777" : "|cff000000",
        Acore::StringFormat("{}/{}", prestigeStats->stats[PRESTIGE_STAT_PARRY_RATING], prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_PARRY_RATING)),
        prestigeStats->stats[PRESTIGE_STAT_PARRY_RATING] >= prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_PARRY_RATING) ? "|cffFF0000(MAXED)|r" : "");

    std::string optBlockRating = 
        Acore::StringFormat("|TInterface\\MINIMAP\\UI-Minimap-ZoomInButton-Up:16|t {}Block Rating ({}) {}",
        prestigeStats->stats[PRESTIGE_STAT_BLOCK_RATING] >= prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_BLOCK_RATING) ? "|cff777777" : "|cff000000",
        Acore::StringFormat("{}/{}", prestigeStats->stats[PRESTIGE_STAT_BLOCK_RATING], prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_BLOCK_RATING)),
        prestigeStats->stats[PRESTIGE_STAT_BLOCK_RATING] >= prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_BLOCK_RATING) ? "|cffFF0000(MAXED)|r" : "");

    std::string optBlockValue = 
        Acore::StringFormat("|TInterface\\MINIMAP\\UI-Minimap-ZoomInButton-Up:16|t {}Block Value ({}) {}",
        prestigeStats->stats[PRESTIGE_STAT_BLOCK_VALUE] >= prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_BLOCK_VALUE) ? "|cff777777" : "|cff000000",
        Acore::StringFormat("{}/{}", prestigeStats->stats[PRESTIGE_STAT_BLOCK_VALUE], prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_BLOCK_VALUE)),
        prestigeStats->stats[PRESTIGE_STAT_BLOCK_VALUE] >= prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_BLOCK_VALUE) ? "|cffFF0000(MAXED)|r" : "");

    std::string optResilRating = 
        Acore::StringFormat("|TInterface\\MINIMAP\\UI-Minimap-ZoomInButton-Up:16|t {}Resilience Rating ({}) {}",
        prestigeStats->stats[PRESTIGE_STAT_RESILIENCE_RATING] >= prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_RESILIENCE_RATING) ? "|cff777777" : "|cff000000",
        Acore::StringFormat("{}/{}", prestigeStats->stats[PRESTIGE_STAT_RESILIENCE_RATING], prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_RESILIENCE_RATING)),
        prestigeStats->stats[PRESTIGE_STAT_RESILIENCE_RATING] >= prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_RESILIENCE_RATING) ? "|cffFF0000(MAXED)|r" : "");

    std::string optBonusArmor = 
        Acore::StringFormat("|TInterface\\MINIMAP\\UI-Minimap-ZoomInButton-Up:16|t {}Bonus Armor ({}) {}",
        prestigeStats->stats[PRESTIGE_STAT_BONUS_ARMOR] >= prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_BONUS_ARMOR) ? "|cff777777" : "|cff000000",
        Acore::StringFormat("{}/{}", prestigeStats->stats[PRESTIGE_STAT_BONUS_ARMOR], prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_BONUS_ARMOR)),
        prestigeStats->stats[PRESTIGE_STAT_BONUS_ARMOR] >= prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_BONUS_ARMOR) ? "|cffFF0000(MAXED)|r" : "");


    if (IsConfirmationToSpendStatPointsEnabled(player, SETTING_CONFIRM_SPEND))
    {
        if (prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_DEFENSE_RATING) > 0) AddGossipItemFor(player, GOSSIP_ICON_DOT, optDefenseRating, GOSSIP_SENDER_MAIN, PRESTIGE_STAT_DEFENSE_RATING, "Are you sure you want to spend your points in defense rating?", 0, false);
        if (prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_DODGE_RATING) > 0) AddGossipItemFor(player, GOSSIP_ICON_DOT, optDodgeRating, GOSSIP_SENDER_MAIN, PRESTIGE_STAT_DODGE_RATING, "Are you sure you want to spend your points in dodge rating?", 0, false);
        if (prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_PARRY_RATING) > 0) AddGossipItemFor(player, GOSSIP_ICON_DOT, optParryRating, GOSSIP_SENDER_MAIN, PRESTIGE_STAT_PARRY_RATING, "Are you sure you want to spend your points in parry rating?", 0, false);
        if (prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_BLOCK_RATING) > 0) AddGossipItemFor(player, GOSSIP_ICON_DOT, optBlockRating, GOSSIP_SENDER_MAIN, PRESTIGE_STAT_BLOCK_RATING, "Are you sure you want to spend your points in block rating?", 0, false);
        if (prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_BLOCK_VALUE) > 0) AddGossipItemFor(player, GOSSIP_ICON_DOT, optBlockValue, GOSSIP_SENDER_MAIN, PRESTIGE_STAT_BLOCK_VALUE, "Are you sure you want to spend your points in block value?", 0, false);
        if (prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_RESILIENCE_RATING) > 0) AddGossipItemFor(player, GOSSIP_ICON_DOT, optResilRating, GOSSIP_SENDER_MAIN, PRESTIGE_STAT_RESILIENCE_RATING, "Are you sure you want to spend your points in resilience rating?", 0, false);
        if (prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_BONUS_ARMOR) > 0) AddGossipItemFor(player, GOSSIP_ICON_DOT, optBonusArmor, GOSSIP_SENDER_MAIN, PRESTIGE_STAT_BONUS_ARMOR, "Are you sure you want to spend your points in bonus armor?", 0, false);
    }
    else
    {
        if (prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_DEFENSE_RATING) > 0) AddGossipItemFor(player, GOSSIP_ICON_DOT, optDefenseRating, GOSSIP_SENDER_MAIN, PRESTIGE_STAT_DEFENSE_RATING);
        if (prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_DODGE_RATING) > 0) AddGossipItemFor(player, GOSSIP_ICON_DOT, optDodgeRating, GOSSIP_SENDER_MAIN, PRESTIGE_STAT_DODGE_RATING);
        if (prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_PARRY_RATING) > 0) AddGossipItemFor(player, GOSSIP_ICON_DOT, optParryRating, GOSSIP_SENDER_MAIN, PRESTIGE_STAT_PARRY_RATING);
        if (prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_BLOCK_RATING) > 0) AddGossipItemFor(player, GOSSIP_ICON_DOT, optBlockRating, GOSSIP_SENDER_MAIN, PRESTIGE_STAT_BLOCK_RATING);
        if (prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_BLOCK_VALUE) > 0) AddGossipItemFor(player, GOSSIP_ICON_DOT, optBlockValue, GOSSIP_SENDER_MAIN, PRESTIGE_STAT_BLOCK_VALUE);
        if (prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_RESILIENCE_RATING) > 0) AddGossipItemFor(player, GOSSIP_ICON_DOT, optResilRating, GOSSIP_SENDER_MAIN, PRESTIGE_STAT_RESILIENCE_RATING);
        if (prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_BONUS_ARMOR) > 0) AddGossipItemFor(player, GOSSIP_ICON_DOT, optBonusArmor, GOSSIP_SENDER_MAIN, PRESTIGE_STAT_BONUS_ARMOR);
    }

    AddGossipItemFor(player, GOSSIP_ICON_DOT, "|TInterface\\MONEYFRAME\\Arrow-Left-Down:16|t Back to main menu", GOSSIP_SENDER_MAIN, PRESTIGE_GOSSIP_ALLOCATE_MAIN_MENU);
    SendGossipMenuFor(player, PRESTIGE_NPC_TEXT_HAS_ATTRIBUTES, player->GetGUID());
    return;

}

void PrestigeResistanceStatsMenu(Player* player)
{
    ClearGossipMenuFor(player);
    auto prestigeStats = GetPrestigeStats(player);
    if (!prestigeStats)
    {
        CloseGossipMenuFor(player);
        return;
    }
    AddGossipItemFor(player, GOSSIP_ICON_DOT, Acore::StringFormat("|TInterface\\GossipFrame\\TrainerGossipIcon:16|t |cffFF0000{} |rAttribute(s) to spend.", GetPrestigeStatsToSpend(player)), GOSSIP_SENDER_MAIN, PRESTIGE_GOSSIP_ALLOCATE_RESISTANCE_STATS);
    std::string optResistFire = 
        Acore::StringFormat("|TInterface\\MINIMAP\\UI-Minimap-ZoomInButton-Up:16|t {}Resist Fire ({}) {}",
        prestigeStats->stats[PRESTIGE_STAT_RESIST_FIRE] >= prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_RESIST_FIRE) ? "|cff777777" : "|cff000000",
        Acore::StringFormat("{}/{}", prestigeStats->stats[PRESTIGE_STAT_RESIST_FIRE], prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_RESIST_FIRE)),
        prestigeStats->stats[PRESTIGE_STAT_RESIST_FIRE] >= prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_RESIST_FIRE) ? "|cffFF0000(MAXED)|r" : "");

    std::string optResistFrost = 
        Acore::StringFormat("|TInterface\\MINIMAP\\UI-Minimap-ZoomInButton-Up:16|t {}Resist Frost ({}) {}",
        prestigeStats->stats[PRESTIGE_STAT_RESIST_FROST] >= prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_RESIST_FROST) ? "|cff777777" : "|cff000000",
        Acore::StringFormat("{}/{}", prestigeStats->stats[PRESTIGE_STAT_RESIST_FROST], prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_RESIST_FROST)),
        prestigeStats->stats[PRESTIGE_STAT_RESIST_FROST] >= prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_RESIST_FROST) ? "|cffFF0000(MAXED)|r" : "");

    std::string optResistNature = 
        Acore::StringFormat("|TInterface\\MINIMAP\\UI-Minimap-ZoomInButton-Up:16|t {}Resist Nature ({}) {}",
        prestigeStats->stats[PRESTIGE_STAT_RESIST_NATURE] >= prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_RESIST_NATURE) ? "|cff777777" : "|cff000000",
        Acore::StringFormat("{}/{}", prestigeStats->stats[PRESTIGE_STAT_RESIST_NATURE], prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_RESIST_NATURE)),
        prestigeStats->stats[PRESTIGE_STAT_RESIST_NATURE] >= prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_RESIST_NATURE) ? "|cffFF0000(MAXED)|r" : "");

    std::string optResistShadow = 
        Acore::StringFormat("|TInterface\\MINIMAP\\UI-Minimap-ZoomInButton-Up:16|t {}Resist Shadow ({}) {}",
        prestigeStats->stats[PRESTIGE_STAT_RESIST_SHADOW] >= prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_RESIST_SHADOW) ? "|cff777777" : "|cff000000",
        Acore::StringFormat("{}/{}", prestigeStats->stats[PRESTIGE_STAT_RESIST_SHADOW], prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_RESIST_SHADOW)),
        prestigeStats->stats[PRESTIGE_STAT_RESIST_SHADOW] >= prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_RESIST_SHADOW) ? "|cffFF0000(MAXED)|r" : "");

    std::string optResistArcane = 
        Acore::StringFormat("|TInterface\\MINIMAP\\UI-Minimap-ZoomInButton-Up:16|t {}Resist Arcane ({}) {}",
        prestigeStats->stats[PRESTIGE_STAT_RESIST_ARCANE] >= prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_RESIST_ARCANE) ? "|cff777777" : "|cff000000",
        Acore::StringFormat("{}/{}", prestigeStats->stats[PRESTIGE_STAT_RESIST_ARCANE], prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_RESIST_ARCANE)),
        prestigeStats->stats[PRESTIGE_STAT_RESIST_ARCANE] >= prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_RESIST_ARCANE) ? "|cffFF0000(MAXED)|r" : "");

    std::string optResistAll = 
        Acore::StringFormat("|TInterface\\MINIMAP\\UI-Minimap-ZoomInButton-Up:16|t {}Resist All ({}) {}",
        prestigeStats->stats[PRESTIGE_STAT_RESIST_ALL] >= prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_RESIST_ALL) ? "|cff777777" : "|cff000000",
        Acore::StringFormat("{}/{}", prestigeStats->stats[PRESTIGE_STAT_RESIST_ALL], prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_RESIST_ALL)),
        prestigeStats->stats[PRESTIGE_STAT_RESIST_ALL] >= prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_RESIST_ALL) ? "|cffFF0000(MAXED)|r" : "");

    if (IsConfirmationToSpendStatPointsEnabled(player, SETTING_CONFIRM_SPEND)) {
        if (prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_RESIST_FIRE) > 0) AddGossipItemFor(player, GOSSIP_ICON_DOT, optResistFire, GOSSIP_SENDER_MAIN, PRESTIGE_STAT_RESIST_FIRE, "Are you sure you want to spend your points in resist fire?", 0, false);
        if (prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_RESIST_FROST) > 0) AddGossipItemFor(player, GOSSIP_ICON_DOT, optResistFrost, GOSSIP_SENDER_MAIN, PRESTIGE_STAT_RESIST_FROST, "Are you sure you want to spend your points in resist frost?", 0, false);
        if (prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_RESIST_NATURE) > 0) AddGossipItemFor(player, GOSSIP_ICON_DOT, optResistNature, GOSSIP_SENDER_MAIN, PRESTIGE_STAT_RESIST_NATURE, "Are you sure you want to spend your points in resist nature?", 0, false);
        if (prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_RESIST_SHADOW) > 0) AddGossipItemFor(player, GOSSIP_ICON_DOT, optResistShadow, GOSSIP_SENDER_MAIN, PRESTIGE_STAT_RESIST_SHADOW, "Are you sure you want to spend your points in resist shadow?", 0, false);
        if (prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_RESIST_ARCANE) > 0) AddGossipItemFor(player, GOSSIP_ICON_DOT, optResistArcane, GOSSIP_SENDER_MAIN, PRESTIGE_STAT_RESIST_ARCANE, "Are you sure you want to spend your points in resist arcane?", 0, false);
        if (prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_RESIST_ALL) > 0) AddGossipItemFor(player, GOSSIP_ICON_DOT, optResistAll, GOSSIP_SENDER_MAIN, PRESTIGE_STAT_RESIST_ALL, "Are you sure you want to spend your points in resist all?", 0, false);
    } else {
        if (prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_RESIST_FIRE) > 0) AddGossipItemFor(player, GOSSIP_ICON_DOT, optResistFire, GOSSIP_SENDER_MAIN, PRESTIGE_STAT_RESIST_FIRE);
        if (prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_RESIST_FROST) > 0) AddGossipItemFor(player, GOSSIP_ICON_DOT, optResistFrost, GOSSIP_SENDER_MAIN, PRESTIGE_STAT_RESIST_FROST);
        if (prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_RESIST_NATURE) > 0) AddGossipItemFor(player, GOSSIP_ICON_DOT, optResistNature, GOSSIP_SENDER_MAIN, PRESTIGE_STAT_RESIST_NATURE);
        if (prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_RESIST_SHADOW) > 0) AddGossipItemFor(player, GOSSIP_ICON_DOT, optResistShadow, GOSSIP_SENDER_MAIN, PRESTIGE_STAT_RESIST_SHADOW);
        if (prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_RESIST_ARCANE) > 0) AddGossipItemFor(player, GOSSIP_ICON_DOT, optResistArcane, GOSSIP_SENDER_MAIN, PRESTIGE_STAT_RESIST_ARCANE);
        if (prestigeConfigSettings.GetStatMaximum(PRESTIGE_STAT_RESIST_ALL) > 0) AddGossipItemFor(player, GOSSIP_ICON_DOT, optResistAll, GOSSIP_SENDER_MAIN, PRESTIGE_STAT_RESIST_ALL);
    }

        AddGossipItemFor(player, GOSSIP_ICON_DOT, "|TInterface\\MONEYFRAME\\Arrow-Left-Down:16|t Back to main menu", GOSSIP_SENDER_MAIN, PRESTIGE_GOSSIP_ALLOCATE_MAIN_MENU);
        SendGossipMenuFor(player, PRESTIGE_NPC_TEXT_HAS_ATTRIBUTES, player->GetGUID());
        return;
}

void PrestigePlayerScript::OnPlayerGossipSelect(Player* player, uint32 menu_id, uint32 sender, uint32 action)
{
    if (action == PRESTIGE_GOSSIP_ALLOCATE_MAIN_MENU)
    {
        PrestigeMainMenu(player);
        return;
    }

    if (action == PRESTIGE_GOSSIP_SETTINGS)
    {
        PrestigeSettingsMenu(player);
        return;
    }

    if (action == PRESTIGE_GOSSIP_SETTINGS_PROMPT)
    {
        ToggleConfirmationToSpendStatPoints(player, SETTING_CONFIRM_SPEND);
        PrestigeSettingsMenu(player);
        return;
    }

    if (action == PRESTIGE_GOSSIP_ALLOCATE_RESET)
    {
        HandlePrestigeStatAllocation(player, action, true);
        PrestigeMainMenu(player);
        return;
    }

    if (action == PRESTIGE_GOSSIP_ALLOCATE_CORE_STATS)
    {
        PrestigeCoreStatsMenu(player);
        return;
    }
    if (action == PRESTIGE_GOSSIP_ALLOCATE_SECONDARY_STATS)
    {
        PrestigeSecondaryStatsMenu(player);
        return;
    }
    if (action == PRESTIGE_GOSSIP_ALLOCATE_DEFENSIVE_STATS)
    {
        PrestigeDefensiveStatsMenu(player);
        return;
    }
    if (action == PRESTIGE_GOSSIP_ALLOCATE_RESISTANCE_STATS)
    {
        PrestigeResistanceStatsMenu(player);
        return;
    }
    if (action == PRESTIGE_GOSSIP_DISPLAY_CURRENT_STATS)
    {
        PrintPrestigeStats(player);
        PrestigeMainMenu(player);
        return;
    }
    if (action >= PRESTIGE_STAT_STAMINA && action <= PRESTIGE_STAT_SPIRIT)
    {
        HandlePrestigeStatAllocation(player, action, false);
        PrestigeCoreStatsMenu(player);
        return;
    }
    if (action >= PRESTIGE_STAT_ATTACK_POWER && action <= PRESTIGE_STAT_SPELL_POWER)
    {
        HandlePrestigeStatAllocation(player, action, false);
        PrestigeSecondaryStatsMenu(player);
        return;
    }
    if (action >= PRESTIGE_STAT_DEFENSE_RATING && action <= PRESTIGE_STAT_RESILIENCE_RATING)
    {
        HandlePrestigeStatAllocation(player, action, false);
        PrestigeDefensiveStatsMenu(player);
        return;
    }
    if (action >= PRESTIGE_STAT_RESIST_FIRE && action <= PRESTIGE_STAT_RESIST_ALL)
    {
        HandlePrestigeStatAllocation(player, action, false);
        PrestigeResistanceStatsMenu(player);
        return;
    }
    return;
}


void PrintPrestigeStats(Player* player)
{
    auto prestigeStats = GetPrestigeStats(player);
    if (!prestigeStats)
    {
        player->SendSystemMessage("You do not have prestige stats!");
        return;
    }
    player->SendSystemMessage("Prestige Stat Points:");
    player->SendSystemMessage(Acore::StringFormat("Prestige Level: {}", prestigeStats->stats[PRESTIGE_STAT_PRESTIGELEVEL]));
    player->SendSystemMessage(Acore::StringFormat("Stamina: {}", prestigeStats->stats[PRESTIGE_STAT_STAMINA]));
    player->SendSystemMessage(Acore::StringFormat("Strength: {}", prestigeStats->stats[PRESTIGE_STAT_STRENGTH]));
    player->SendSystemMessage(Acore::StringFormat("Agility: {}", prestigeStats->stats[PRESTIGE_STAT_AGILITY]));
    player->SendSystemMessage(Acore::StringFormat("Intellect: {}", prestigeStats->stats[PRESTIGE_STAT_INTELLECT]));
    player->SendSystemMessage(Acore::StringFormat("Spirit: {}", prestigeStats->stats[PRESTIGE_STAT_SPIRIT]));

    player->SendSystemMessage(Acore::StringFormat("Attack Power: {}", prestigeStats->stats[PRESTIGE_STAT_ATTACK_POWER]));
    player->SendSystemMessage(Acore::StringFormat("Spell Power: {}", prestigeStats->stats[PRESTIGE_STAT_SPELL_POWER]));
    player->SendSystemMessage(Acore::StringFormat("Crit Rating: {}", prestigeStats->stats[PRESTIGE_STAT_CRIT_RATING]));
    player->SendSystemMessage(Acore::StringFormat("Hit Rating: {}", prestigeStats->stats[PRESTIGE_STAT_HIT_RATING]));
    player->SendSystemMessage(Acore::StringFormat("Expertise Rating: {}", prestigeStats->stats[PRESTIGE_STAT_EXPERTISE_RATING]));
    player->SendSystemMessage(Acore::StringFormat("Haste Rating: {}", prestigeStats->stats[PRESTIGE_STAT_HASTE_RATING]));
    player->SendSystemMessage(Acore::StringFormat("Armor Penetration Rating: {}", prestigeStats->stats[PRESTIGE_STAT_ARMOR_PEN_RATING]));
    player->SendSystemMessage(Acore::StringFormat("Spell Penetration: {}", prestigeStats->stats[PRESTIGE_STAT_SPELL_PENETRATION]));
    player->SendSystemMessage(Acore::StringFormat("Mana Per 5 Seconds (MP5): {}", prestigeStats->stats[PRESTIGE_STAT_MP5]));

    player->SendSystemMessage(Acore::StringFormat("Defense Rating: {}", prestigeStats->stats[PRESTIGE_STAT_DEFENSE_RATING]));
    player->SendSystemMessage(Acore::StringFormat("Dodge Rating: {}", prestigeStats->stats[PRESTIGE_STAT_DODGE_RATING]));
    player->SendSystemMessage(Acore::StringFormat("Parry Rating: {}", prestigeStats->stats[PRESTIGE_STAT_PARRY_RATING]));
    player->SendSystemMessage(Acore::StringFormat("Block Rating: {}", prestigeStats->stats[PRESTIGE_STAT_BLOCK_RATING]));
    player->SendSystemMessage(Acore::StringFormat("Block Value: {}", prestigeStats->stats[PRESTIGE_STAT_BLOCK_VALUE]));
    player->SendSystemMessage(Acore::StringFormat("Resilience Rating: {}", prestigeStats->stats[PRESTIGE_STAT_RESILIENCE_RATING]));
    player->SendSystemMessage(Acore::StringFormat("Bonus Armor: {}", prestigeStats->stats[PRESTIGE_STAT_BONUS_ARMOR]));

    player->SendSystemMessage(Acore::StringFormat("Fire Resistance: {}", prestigeStats->stats[PRESTIGE_STAT_RESIST_FIRE]));
    player->SendSystemMessage(Acore::StringFormat("Frost Resistance: {}", prestigeStats->stats[PRESTIGE_STAT_RESIST_FROST]));
    player->SendSystemMessage(Acore::StringFormat("Nature Resistance: {}", prestigeStats->stats[PRESTIGE_STAT_RESIST_NATURE]));
    player->SendSystemMessage(Acore::StringFormat("Shadow Resistance: {}", prestigeStats->stats[PRESTIGE_STAT_RESIST_SHADOW]));
    player->SendSystemMessage(Acore::StringFormat("Arcane Resistance: {}", prestigeStats->stats[PRESTIGE_STAT_RESIST_ARCANE]));
    player->SendSystemMessage(Acore::StringFormat("All Resistances: {}", prestigeStats->stats[PRESTIGE_STAT_RESIST_ALL]));
}


bool ChatCommandSendMainMenu(ChatHandler* handler)
{
    Player* player = handler->GetPlayer();
    PrestigeMainMenu(player);
    return true;
}

void PrestigeSettingsMenu(Player* player)
{
    ClearGossipMenuFor(player);

    auto hasPromptSetting = IsConfirmationToSpendStatPointsEnabled(player, SETTING_CONFIRM_SPEND);
    AddGossipItemFor(player, GOSSIP_ICON_DOT, Acore::StringFormat("|TInterface\\GossipFrame\\HealerGossipIcon:16|t Prompt 'Are you sure': {}", hasPromptSetting ? "|cff00FF00Enabled|r" : "|cffFF0000Disabled"), GOSSIP_SENDER_MAIN, PRESTIGE_GOSSIP_SETTINGS_PROMPT);

    AddGossipItemFor(player, GOSSIP_ICON_DOT, "|TInterface\\MONEYFRAME\\Arrow-Left-Down:16|t Back", GOSSIP_SENDER_MAIN, PRESTIGE_GOSSIP_ALLOCATE_MAIN_MENU);

    SendGossipMenuFor(player, PRESTIGE_NPC_TEXT_GENERIC, player->GetGUID());
}

void PrestigePlayerScript::HandlePrestigeStatAllocation(Player* player, uint32 statToUpdate, bool reset)
{
    if (!player)
    {
        return;
    }
    auto prestigeStats = GetPrestigeStats(player);
    if (!prestigeStats)
    {
        return;
    }

    if (reset)
    {
        if (player->HasEnoughMoney(prestigeConfigSettings.GetResetCost()))
        {
            player->SetMoney(player->GetMoney() - prestigeConfigSettings.GetResetCost());
  
            RespecPrestigeStats(prestigeStats);
        }
    }
    else
    {
        if (prestigeStats->stats[PRESTIGE_STAT_UNALLOCATED] < 1)
        {
            ChatHandler(player->GetSession()).SendSysMessage("You have no free attribute points to spend.");
            return;
        }
        auto result = TryAddPrestigeStat(prestigeStats, statToUpdate);
        if (!result)
        {
            ChatHandler(player->GetSession()).SendSysMessage("This attribute is already at the maximum.");
            return;
        }
    }
    DisablePrestigeStats(player);
    ApplyPrestigeStats(player, prestigeStats);
}

void PrestigeWorldScript::OnShutdownInitiate(ShutdownExitCode /*code*/, ShutdownMask /*mask*/)
{
    SavePrestigeStats();
}

void PrestigeUnitScript::OnDamage(Unit* attacker, Unit* victim, uint32& /*damage*/)
{
    if (!attacker || !victim)
    {
        return;
    }

    if (!prestigeConfigSettings.IsPrestigeEnabled())
    {
        return;
    }

    if (!prestigeConfigSettings.IsPVPDisabled())
    {
        return;
    }

    auto p1 = attacker->ToPlayer();
    auto p2 = victim->ToPlayer();

    if (!p1 || !p2)
    {
        return;
    }

    DisablePrestigeStats(p1);
    DisablePrestigeStats(p2);
}

bool ChatCommandListPrestigeStats(ChatHandler* handler)
{
    Player* player = handler->GetPlayer();
    PrintPrestigeStats(player);
    return true;
}
bool ChatCommandGrantPrestigeLevel(ChatHandler* handler)
{
    Player* player = handler->GetPlayer();
    auto target = player->GetSelectedUnit();
    if(!target)
    {
        return true;
    }
    if (target->IsPlayer())
    {
        Player* targetPlayer = target->ToPlayer();
        auto prestigeStats = GetPrestigeStats(targetPlayer);
        if (!prestigeStats)
        {
  return false;
        }
        else
        {
  PrestigeLevelUp(targetPlayer);
  player->SendSystemMessage(Acore::StringFormat("You have granted a prestige level to {}, bringing their total prestige level to {}", targetPlayer->GetName(), GetPrestigeLevel(targetPlayer)));
  LOG_INFO("module", "GM {} has granted a prestige level to {}", player->GetName(), targetPlayer->GetName());
        }
    }
    return true;
}

bool ChatCommandRemovePrestigeLevel(ChatHandler* handler)
{
    Player* player = handler->GetPlayer();
    auto target = player->GetSelectedUnit();
    if(!target)
    {
        return true;
    }
    if (target->IsPlayer())
    {
        Player* targetPlayer = target->ToPlayer();
        auto prestigeStats = GetPrestigeStats(targetPlayer);
        if (!prestigeStats)
        {
  return false;
        }
        else
        {
  auto prestigeStats = GetPrestigeStats(targetPlayer);
  RespecPrestigeStats(prestigeStats);
  prestigeStats->stats[PRESTIGE_STAT_UNALLOCATED]-=1;
  prestigeStats->stats[PRESTIGE_STAT_PRESTIGELEVEL]-=1;
  player->SendSystemMessage(Acore::StringFormat("You have removed a prestige level from {}, bringing their total prestige level to {}", targetPlayer->GetName(), GetPrestigeLevel(targetPlayer)));
  LOG_INFO("module", "GM {} has removed a prestige level from {}", player->GetName(), targetPlayer->GetName());
        }
    }
    return true;
}
void SC_AddPrestigeScripts()
{
    new PrestigeWorldScript();
    new PrestigePlayerScript();
    new PrestigeCreatureScript();
    new PrestigeUnitScript();
    new PrestigeCommand();
}
