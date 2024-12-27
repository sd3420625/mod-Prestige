#ifndef MODULE_PRESTIGE_H
#define MODULE_PRESTIGE_H
#include <unordered_map>
#include "ScriptMgr.h"
#include "Player.h"

#include <unordered_map>

enum PrestigeConstants
{
    PRESTIGE_MENU_ID = 13133750, //assigned in sql but appeast to not apply properly, currently manually assigining the gossipmenuID with this

    SETTING_CONFIRM_SPEND = 1, 
    PRESTIGE_GOSSIP_ALLOCATE_MAIN_MENU = 101,
    PRESTIGE_GOSSIP_ALLOCATE_RESET = 102,
    PRESTIGE_GOSSIP_SETTINGS = 103,
    PRESTIGE_GOSSIP_SETTINGS_PROMPT = 104,

    PRESTIGE_GOSSIP_ALLOCATE_CORE_STATS = 105,
    PRESTIGE_GOSSIP_ALLOCATE_DEFENSIVE_STATS = 106,
    PRESTIGE_GOSSIP_ALLOCATE_SECONDARY_STATS = 107,
    PRESTIGE_GOSSIP_ALLOCATE_RESISTANCE_STATS = 108,
    PRESTIGE_GOSSIP_DISPLAY_CURRENT_STATS = 109,

    PRESTIGE_NPC_TEXT_HAS_ATTRIBUTES = 441191,
    PRESTIGE_NPC_TEXT_GENERIC = 441190,
    PRESTIGE_NPC_TEXT_DISABLED = 441192,

    STAT_SPELL_STAMINA = 90000,
    STAT_SPELL_AGILITY = 90001,
    STAT_SPELL_INTELLECT = 90002,
    STAT_SPELL_STRENGTH = 90003, 
    STAT_SPELL_SPIRIT = 90004,

    STAT_SPELL_ATTACK_POWER = 90005,
    STAT_SPELL_SPELL_POWER = 90006,
    STAT_SPELL_CRIT_RATING = 90007,
    STAT_SPELL_HIT_RATING = 90008,
    STAT_SPELL_EXPERTISE_RATING = 90009,
    STAT_SPELL_HASTE_RATING = 90010,
    STAT_SPELL_ARMOR_PEN_RATING = 90011,
    STAT_SPELL_SPELL_PENETRATION = 90012,
    STAT_SPELL_MP5 = 90013,

    STAT_SPELL_DEFENSE_RATING = 90014,
    STAT_SPELL_DODGE_RATING = 90015,
    STAT_SPELL_PARRY_RATING = 90016,
    STAT_SPELL_BLOCK_RATING = 90017,
    STAT_SPELL_BLOCK_VALUE = 90018,
    STAT_SPELL_RESILIENCE_RATING = 90019,
    STAT_SPELL_BONUS_ARMOR = 90020,

    STAT_SPELL_RESIST_FIRE = 90021,
    STAT_SPELL_RESIST_FROST = 90022,
    STAT_SPELL_RESIST_NATURE = 90023,
    STAT_SPELL_RESIST_SHADOW = 90024,
    STAT_SPELL_RESIST_ARCANE = 90025,
    STAT_SPELL_RESIST_ALL = 90026,

    STAT_SPELL_STAMINA_256 = 90027,
    STAT_SPELL_AGILITY_256 = 90028,
    STAT_SPELL_INTELLECT_256 = 90029,
    STAT_SPELL_STRENGTH_256 = 90030, 
    STAT_SPELL_SPIRIT_256 = 90031,

    STAT_SPELL_ATTACK_POWER_256 = 90032,
    STAT_SPELL_SPELL_POWER_256 = 90033,
    STAT_SPELL_CRIT_RATING_256 = 90034,
    STAT_SPELL_HIT_RATING_256 = 90035,
    STAT_SPELL_EXPERTISE_RATING_256 = 90036,
    STAT_SPELL_HASTE_RATING_256 = 90037,
    STAT_SPELL_ARMOR_PEN_RATING_256 = 90038,
    STAT_SPELL_SPELL_PENETRATION_256 = 90039,
    STAT_SPELL_MP5_256 = 90040,

    STAT_SPELL_DEFENSE_RATING_256 = 90041,
    STAT_SPELL_DODGE_RATING_256 = 90042,
    STAT_SPELL_PARRY_RATING_256 = 90043,
    STAT_SPELL_BLOCK_RATING_256 = 90044,
    STAT_SPELL_BLOCK_VALUE_256 = 90045,
    STAT_SPELL_RESILIENCE_RATING_256 = 90046,
    STAT_SPELL_BONUS_ARMOR_256 = 90047,

    STAT_SPELL_RESIST_FIRE_256 = 90048,
    STAT_SPELL_RESIST_FROST_256 = 90049,
    STAT_SPELL_RESIST_NATURE_256 = 90050,
    STAT_SPELL_RESIST_SHADOW_256 = 90051,
    STAT_SPELL_RESIST_ARCANE_256 = 90052,
    STAT_SPELL_RESIST_ALL_256 = 90053
};
struct PrestigeStatData
{
    uint32 maxValue;
    uint32 statPerPoint;
};
enum StatType {
    PRESTIGE_STAT_PRESTIGELEVEL,
    PRESTIGE_STAT_UNALLOCATED,
    PRESTIGE_STAT_STAMINA,
    PRESTIGE_STAT_STRENGTH,
    PRESTIGE_STAT_AGILITY,
    PRESTIGE_STAT_INTELLECT,
    PRESTIGE_STAT_SPIRIT,

    PRESTIGE_STAT_DEFENSE_RATING,
    PRESTIGE_STAT_DODGE_RATING,
    PRESTIGE_STAT_PARRY_RATING,
    PRESTIGE_STAT_BLOCK_RATING,
    PRESTIGE_STAT_BLOCK_VALUE,
    PRESTIGE_STAT_BONUS_ARMOR,
    PRESTIGE_STAT_RESILIENCE_RATING,

    PRESTIGE_STAT_ATTACK_POWER,
    PRESTIGE_STAT_CRIT_RATING,
    PRESTIGE_STAT_HIT_RATING,
    PRESTIGE_STAT_HASTE_RATING,
    PRESTIGE_STAT_EXPERTISE_RATING,
    PRESTIGE_STAT_ARMOR_PEN_RATING,
    PRESTIGE_STAT_SPELL_PENETRATION,
    PRESTIGE_STAT_MP5,
    PRESTIGE_STAT_SPELL_POWER,

    PRESTIGE_STAT_RESIST_FIRE,
    PRESTIGE_STAT_RESIST_FROST,
    PRESTIGE_STAT_RESIST_NATURE,
    PRESTIGE_STAT_RESIST_SHADOW,
    PRESTIGE_STAT_RESIST_ARCANE,
    PRESTIGE_STAT_RESIST_ALL,
    PRESTIGE_STAT_CONFIRMSPEND,
    PRESTIGE_STAT_MAX
};
struct PrestigeStats
{
    uint32 stats[PRESTIGE_STAT_MAX];
};
struct PrestigeConfigSettings
{
    private:

        PrestigeStatData StatProperties[PRESTIGE_STAT_MAX];
        uint8 _intendedMaxLevel;
        uint32 _resetCost;
        bool _prestigeEnable;
        bool _disablePVP;
        uint8 _levelUpFormulaType;
        uint32 _levelUpFormulaBase;
        uint16 _levelUpFormulaR;
        uint16 _levelUpFormulaK;

        PrestigeConfigSettings() = default;
        PrestigeConfigSettings(const PrestigeConfigSettings&) = delete;
        PrestigeConfigSettings& operator=(const PrestigeConfigSettings&) = delete;
    public:
        static PrestigeConfigSettings& Instance()
        {
            static PrestigeConfigSettings instance;
            return instance;
        }
        uint32 GetStatMaximum(size_t index)
        {
            return StatProperties[index].maxValue;
        }
        uint32 GetStatPerPoint(size_t index)
        {
            return StatProperties[index].statPerPoint;
        }
        void SetStatMaximum(size_t index, uint32 value)
        {
            StatProperties[index].maxValue = value;
        }
        void SetStatPerPoint(size_t index, uint32 value)
        {
            StatProperties[index].statPerPoint = value;
        }

        uint8 GetIntendedMaxLevel() const { return _intendedMaxLevel; }
        void SetIntendedMaxLevel(uint8 value) { _intendedMaxLevel = value; }

        uint32 GetResetCost() const { return _resetCost; }
        void SetResetCost(uint32 value) { _resetCost = value; }

        bool IsPrestigeEnabled() const { return _prestigeEnable; }
        void SetPrestigeEnabled(bool value) { _prestigeEnable = value; }

        bool IsPVPDisabled() const { return _disablePVP; }
        void SetDisablePVP(bool value) { _disablePVP = value; }  

        uint8 GetLevelUpFormualType() const { return _levelUpFormulaType; }
        void SetLevelUpFormulaType(uint8 value) { _levelUpFormulaType = value; }

        uint32 GetLevelUpFormulaBase() const { return _levelUpFormulaBase; }
        void SetLevelUpFormulaBase(uint32 value) { _levelUpFormulaBase = value; }

        uint16 GetLevelUpFormulaR() const { return _levelUpFormulaR; }
        void SetLevelUpFormulaR(uint16 value) { _levelUpFormulaR = value; }
        
        uint16 GetLevelUpFormulaK() const { return _levelUpFormulaK; }
        void SetLevelUpFormulaK(uint16 value) { _levelUpFormulaK = value; }
};
const char* StatNames[PRESTIGE_STAT_MAX] = {
    "Prestige Level", // STAT_PRESTIGELEVEL
    "Unallocated",    // STAT_UNALLOCATED
    "Stamina",        // PRESTIGE_STAT_STAMINA
    "Strength",       // PRESTIGE_STAT_STRENGTH
    "Agility",        // PRESTIGE_STAT_AGILITY
    "Intellect",      // PRESTIGE_STAT_INTELLECT
    "Spirit",         // PRESTIGE_STAT_SPIRIT

    "Defense Rating", // PRESTIGE_STAT_DEFENSE_RATING
    "Dodge Rating",   // PRESTIGE_STAT_DODGE_RATING
    "Parry Rating",   // PRESTIGE_STAT_PARRY_RATING
    "Block Rating",   // PRESTIGE_STAT_BLOCK_RATING
    "Block Value",    // PRESTIGE_STAT_BLOCK_VALUE
    "Bonus Armor",    // PRESTIGE_STAT_BONUS_ARMOR
    "Resilience Rating", // PRESTIGE_STAT_RESILIENCE_RATING

    "Attack Power",   // PRESTIGE_STAT_ATTACK_POWER
    "Crit Rating",    // PRESTIGE_STAT_CRIT_RATING
    "Hit Rating",     // PRESTIGE_STAT_HIT_RATING
    "Haste Rating",   // PRESTIGE_STAT_HASTE_RATING
    "Expertise Rating", // PRESTIGE_STAT_EXPERTISE_RATING
    "Armor Pen Rating", // PRESTIGE_STAT_ARMOR_PEN_RATING
    "Spell Penetration", // PRESTIGE_STAT_SPELL_PENETRATION
    "MP5",            // PRESTIGE_STAT_MP5
    "Spell Power",    // PRESTIGE_STAT_SPELL_POWER

    "Resist Fire",    // PRESTIGE_STAT_RESIST_FIRE
    "Resist Frost",   // PRESTIGE_STAT_RESIST_FROST
    "Resist Nature",  // PRESTIGE_STAT_RESIST_NATURE
    "Resist Shadow",  // PRESTIGE_STAT_RESIST_SHADOW
    "Resist Arcane",  // PRESTIGE_STAT_RESIST_ARCANE
    "Resist All"      // PRESTIGE_STAT_RESIST_ALL
};




const uint32 StatToSpell[PRESTIGE_STAT_MAX] = {
    0,                           // STAT_PRESTIGELEVEL
    0,                           // STAT_UNALLOCATED
    STAT_SPELL_STAMINA,          // PRESTIGE_STAT_STAMINA
    STAT_SPELL_STRENGTH,         // PRESTIGE_STAT_STRENGTH
    STAT_SPELL_AGILITY,          // PRESTIGE_STAT_AGILITY
    STAT_SPELL_INTELLECT,        // PRESTIGE_STAT_INTELLECT
    STAT_SPELL_SPIRIT,           // PRESTIGE_STAT_SPIRIT

    STAT_SPELL_DEFENSE_RATING,   // PRESTIGE_STAT_DEFENSE_RATING
    STAT_SPELL_DODGE_RATING,     // PRESTIGE_STAT_DODGE_RATING
    STAT_SPELL_PARRY_RATING,     // PRESTIGE_STAT_PARRY_RATING
    STAT_SPELL_BLOCK_RATING,     // PRESTIGE_STAT_BLOCK_RATING
    STAT_SPELL_BLOCK_VALUE,      // PRESTIGE_STAT_BLOCK_VALUE
    STAT_SPELL_BONUS_ARMOR,      // PRESTIGE_STAT_BONUS_ARMOR
    STAT_SPELL_RESILIENCE_RATING,// PRESTIGE_STAT_RESILIENCE_RATING

    STAT_SPELL_ATTACK_POWER,     // PRESTIGE_STAT_ATTACK_POWER
    STAT_SPELL_CRIT_RATING,      // PRESTIGE_STAT_CRIT_RATING
    STAT_SPELL_HIT_RATING,       // PRESTIGE_STAT_HIT_RATING
    STAT_SPELL_HASTE_RATING,     // PRESTIGE_STAT_HASTE_RATING
    STAT_SPELL_EXPERTISE_RATING, // PRESTIGE_STAT_EXPERTISE_RATING
    STAT_SPELL_ARMOR_PEN_RATING, // PRESTIGE_STAT_ARMOR_PEN_RATING
    STAT_SPELL_SPELL_PENETRATION,// PRESTIGE_STAT_SPELL_PENETRATION
    STAT_SPELL_MP5,              // PRESTIGE_STAT_MP5
    STAT_SPELL_SPELL_POWER,      // PRESTIGE_STAT_SPELL_POWER

    STAT_SPELL_RESIST_FIRE,      // PRESTIGE_STAT_RESIST_FIRE
    STAT_SPELL_RESIST_FROST,     // PRESTIGE_STAT_RESIST_FROST
    STAT_SPELL_RESIST_NATURE,    // PRESTIGE_STAT_RESIST_NATURE
    STAT_SPELL_RESIST_SHADOW,    // PRESTIGE_STAT_RESIST_SHADOW
    STAT_SPELL_RESIST_ARCANE,    // PRESTIGE_STAT_RESIST_ARCANE
    STAT_SPELL_RESIST_ALL,       // PRESTIGE_STAT_RESIST_ALL
    0                            
};

const uint32 StatToSpell256[PRESTIGE_STAT_MAX] = {
    0,                                // STAT_PRESTIGELEVEL
    0,                                // STAT_UNALLOCATED
    STAT_SPELL_STAMINA_256,           // PRESTIGE_STAT_STAMINA
    STAT_SPELL_STRENGTH_256,          // PRESTIGE_STAT_STRENGTH
    STAT_SPELL_AGILITY_256,           // PRESTIGE_STAT_AGILITY
    STAT_SPELL_INTELLECT_256,         // PRESTIGE_STAT_INTELLECT
    STAT_SPELL_SPIRIT_256,            // PRESTIGE_STAT_SPIRIT

    STAT_SPELL_DEFENSE_RATING_256,    // PRESTIGE_STAT_DEFENSE_RATING
    STAT_SPELL_DODGE_RATING_256,      // PRESTIGE_STAT_DODGE_RATING
    STAT_SPELL_PARRY_RATING_256,      // PRESTIGE_STAT_PARRY_RATING
    STAT_SPELL_BLOCK_RATING_256,      // PRESTIGE_STAT_BLOCK_RATING
    STAT_SPELL_BLOCK_VALUE_256,       // PRESTIGE_STAT_BLOCK_VALUE
    STAT_SPELL_BONUS_ARMOR_256,       // PRESTIGE_STAT_BONUS_ARMOR
    STAT_SPELL_RESILIENCE_RATING_256, // PRESTIGE_STAT_RESILIENCE_RATING

    STAT_SPELL_ATTACK_POWER_256,      // PRESTIGE_STAT_ATTACK_POWER
    STAT_SPELL_CRIT_RATING_256,       // PRESTIGE_STAT_CRIT_RATING
    STAT_SPELL_HIT_RATING_256,        // PRESTIGE_STAT_HIT_RATING
    STAT_SPELL_HASTE_RATING_256,      // PRESTIGE_STAT_HASTE_RATING
    STAT_SPELL_EXPERTISE_RATING_256,  // PRESTIGE_STAT_EXPERTISE_RATING
    STAT_SPELL_ARMOR_PEN_RATING_256,  // PRESTIGE_STAT_ARMOR_PEN_RATING
    STAT_SPELL_SPELL_PENETRATION_256, // PRESTIGE_STAT_SPELL_PENETRATION
    STAT_SPELL_MP5_256,               // PRESTIGE_STAT_MP5
    STAT_SPELL_SPELL_POWER_256,       // PRESTIGE_STAT_SPELL_POWER

    STAT_SPELL_RESIST_FIRE_256,       // PRESTIGE_STAT_RESIST_FIRE
    STAT_SPELL_RESIST_FROST_256,      // PRESTIGE_STAT_RESIST_FROST
    STAT_SPELL_RESIST_NATURE_256,     // PRESTIGE_STAT_RESIST_NATURE
    STAT_SPELL_RESIST_SHADOW_256,     // PRESTIGE_STAT_RESIST_SHADOW
    STAT_SPELL_RESIST_ARCANE_256,     // PRESTIGE_STAT_RESIST_ARCANE
    STAT_SPELL_RESIST_ALL_256,        // PRESTIGE_STAT_RESIST_ALL
    0                                 // PRESTIGE_STAT_CONFIRMSPEND
};




PrestigeStatData prestigeStatData[PRESTIGE_STAT_MAX];


std::unordered_map<uint64, PrestigeStats> prestigeStatMap;

void AddPrestigePoint(Player* /*player*/);
PrestigeStats* GetPrestigeStats(Player* /*player*/);
void ClearPrestigeStats();
void LoadPrestigeStats();
PrestigeStats* LoadPrestigeStatsForPlayer(Player* /*player*/);
void SavePrestigeStats();
void SavePrestigeStatsForPlayer(Player* /*player*/);
void ApplyPrestigeStats(Player* /*player*/, PrestigeStats* /*attributes*/);
void DisablePrestigeStats(Player* /*player*/);
bool TryAddPrestigeStat(PrestigeStats* /*attributes*/, uint32 /*attribute*/);
void RespecPrestigeStats(PrestigeStats* /*attributes*/);

bool HasPrestigeStats(Player* /*player*/);
bool IsPrestigeStatAtMax(uint32 /*attribute*/, uint32 /*value*/);
uint32 GetPrestigeStatsToSpend(Player* /*player*/);
uint32 GetTotalPrestigeStats(PrestigeStats* /*attributes*/);

/*leaving as a bit flag for future settings*/
bool IsConfirmationToSpendStatPointsEnabled(Player* /*player*/, uint32 /*setting*/);
void ToggleConfirmationToSpendStatPoints(Player* /*player*/, uint32 /*setting*/);

void PrestigeSettingsMenu(Player* /*player*/);
void PrestigeMainMenu(Player* /*player*/);
void PrestigeCoreStatsMenu(Player* /*player*/);
void PrestigeSecondaryStatsMenu(Player* /*player*/);
void PrestigeDefensiveStatsMenu(Player* /*player*/);
void PrestigeResistanceStatsMenu(Player* /*player*/);

bool ChatCommandSendMainMenu(ChatHandler* /*handler*/);
bool ChatCommandListPrestigeStats(ChatHandler* /*handler*/);
bool ChatCommandGrantPrestigeLevel(ChatHandler* /*handler*/);
bool ChatCommandRemovePrestigeLevel(ChatHandler* /*handler*/);

void PrestigeLevelUp(Player* /*player*/);
void InitPrestigeExpTnl(Player* /*player*/);
uint32 GetPrestigeLevel(Player* /*player*/);

void ClosePrestigeMenuInCombat(Player* /*player*/);
void PrintPrestigeStats(Player* /*player*/);
void CheckForUpdatedMaxStats(Player* /*player*/);

class PrestigePlayerScript : public PlayerScript
{
public:
    PrestigePlayerScript() : PlayerScript("PrestigePlayerScript") { }

    virtual void OnLogin(Player* /*player*/) override;
    virtual void OnLogout(Player* /*player*/) override;
    virtual void OnPlayerLeaveCombat(Player* /*player*/) override;
    virtual void OnGossipSelect(Player* player, uint32 menu_id, uint32 sender, uint32 action) override;
    void HandlePrestigeStatAllocation(Player* /*player*/, uint32 /*attribute*/, bool /*reset*/);
    std::string GetPrestigeStatName(uint32 /*attribute*/);
    virtual void OnLevelChanged(Player* /*player*/, uint8 /*oldLevel*/) override;
    virtual void OnPlayerEnterCombat(Player* /*player*/, Unit* /*enemy*/) override;
};

class PrestigeUnitScript : public UnitScript
{
public:
    PrestigeUnitScript() : UnitScript("PrestigeUnitScript") { }
    void OnDamage(Unit* /*attacker*/, Unit* /*victim*/, uint32& /*damage*/);
};

class PrestigeCreatureScript : public CreatureScript
{
public:
    PrestigeCreatureScript() : CreatureScript("PrestigeCreatureScript") { }
    virtual bool OnGossipHello(Player* /*player*/, Creature* /*creature*/) override;
};

class PrestigeWorldScript : public WorldScript
{
public:
    PrestigeWorldScript() : WorldScript("PrestigeWorldScript") { }
    virtual void OnAfterConfigLoad(bool /*reload*/) override;
    virtual void OnShutdownInitiate(ShutdownExitCode /*code*/, ShutdownMask /*mask*/) override;
};
using namespace Acore::ChatCommands;
class PrestigeCommand : public CommandScript
{
    public:
    PrestigeCommand() : CommandScript("PrestigeCommand"){}
    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable PrestigeCommandTable = 
        {
            {"menu", ChatCommandSendMainMenu, SEC_PLAYER, Console::Yes},
            {"stats", ChatCommandListPrestigeStats, SEC_PLAYER, Console::Yes},
            {"grantlevel", ChatCommandGrantPrestigeLevel, SEC_GAMEMASTER, Console::Yes},
            {"removelevel", ChatCommandRemovePrestigeLevel, SEC_GAMEMASTER, Console::Yes}
        };

        static ChatCommandTable PrestigeCommandBaseTable = 
        {
            {"prestige", PrestigeCommandTable}
        };

        return PrestigeCommandBaseTable;
    }
};

#endif
