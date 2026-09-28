#include "data/move_database.h"
#include "battle/battle_member.h"

const MoveData g_MoveDatabase[] = {
    {
        MOVE_PRESSURE_CANNON,
        "Pressure Cannon",
        BATTLE_ELEMENT_WATER,
        4,
        100,
        0,
        0,
        100,
        125,
        0,
        TARGET_PATTERN_SINGLE_ENEMY,
        "Blast one enemy with high-pressure water.",
        MOVE_CATEGORY_SPECIAL,
        80,
        MOVE_WEAPON_KIND_NONE,
        {
            { MOVE_EFFECT_KIND_DAMAGE, BATTLE_STATUS_NONE, 80, MOVE_EFFECT_PARAM_NONE, MOVE_EFFECT_PARAM_NONE },
            MOVE_EFFECT_DATA_NONE,
            MOVE_EFFECT_DATA_NONE
        }
    },
    {
        MOVE_LUCKY_SWING_TEST,
        "Lucky Swing Test",
        BATTLE_ELEMENT_NONE,
        3,
        100,
        1,
        1,
        200,
        0,
        0,
        TARGET_PATTERN_SINGLE_ENEMY,
        "A temporary debug move for testing LP upgrades. "
        "Not a canon Project Solarfly move. Contact hit. "
        "Spend 1 LP to double this move's damage.",
        MOVE_CATEGORY_PHYSICAL,
        30,
        MOVE_WEAPON_KIND_NONE,
        {
            { MOVE_EFFECT_KIND_DAMAGE, BATTLE_STATUS_NONE, 30, MOVE_EFFECT_PARAM_NONE, MOVE_EFFECT_PARAM_NONE },
            MOVE_EFFECT_DATA_NONE,
            MOVE_EFFECT_DATA_NONE
        }
    },
    {
        MOVE_STEP_TEST,
        "Step Test",
        BATTLE_ELEMENT_NONE,
        99,
        100,
        0,
        0,
        100,
        0,
        0,
        TARGET_PATTERN_ALLY_SLOT,
        "Relocate to an ally's battle tile. Does not deal damage.",
        MOVE_CATEGORY_SUPPORT,
        0,
        MOVE_WEAPON_KIND_NONE,
        {
            { MOVE_EFFECT_KIND_MOVE_TO_SLOT, BATTLE_STATUS_NONE, MOVE_EFFECT_PARAM_NONE, MOVE_EFFECT_PARAM_NONE, MOVE_EFFECT_PARAM_NONE },
            MOVE_EFFECT_DATA_NONE,
            MOVE_EFFECT_DATA_NONE
        }
    },
    {
        MOVE_LONG_SHOT_TEST,
        "Long Shot Test",
        BATTLE_ELEMENT_NONE,
        99,
        100,
        0,
        0,
        100,
        0,
        0,
        TARGET_PATTERN_SINGLE_ENEMY,
        "A ranged strike that grows stronger when used "
        "from distant battle tiles.",
        MOVE_CATEGORY_SPECIAL,
        20,
        MOVE_WEAPON_KIND_NONE,
        {
            { MOVE_EFFECT_KIND_DAMAGE, BATTLE_STATUS_NONE, 20, MOVE_EFFECT_PARAM_NONE, MOVE_EFFECT_PARAM_NONE },
            { MOVE_EFFECT_KIND_DISTANCE_SCALE, BATTLE_STATUS_NONE, MOVE_EFFECT_PARAM_NONE, MOVE_EFFECT_PARAM_NONE, MOVE_EFFECT_PARAM_NONE },
            MOVE_EFFECT_DATA_NONE
        }
    },
    {
        MOVE_BURN_TEST,
        "Burn Test",
        BATTLE_ELEMENT_FIRE,
        3,
        100,
        0,
        0,
        100,
        0,
        0,
        TARGET_PATTERN_SINGLE_ENEMY,
        "Deals light damage and burns the target.",
        MOVE_CATEGORY_STATUS,
        15,
        MOVE_WEAPON_KIND_NONE,
        {
            { MOVE_EFFECT_KIND_DAMAGE, BATTLE_STATUS_NONE, 15, MOVE_EFFECT_PARAM_NONE, MOVE_EFFECT_PARAM_NONE },
            { MOVE_EFFECT_KIND_INFLICT_STATUS, BATTLE_STATUS_BURN, 100, MOVE_EFFECT_PARAM_NONE, MOVE_EFFECT_PARAM_NONE },
            MOVE_EFFECT_DATA_NONE
        }
    },
    {
        MOVE_POISON_TEST,
        "Poison Test",
        BATTLE_ELEMENT_POISON,
        3,
        100,
        0,
        0,
        100,
        0,
        0,
        TARGET_PATTERN_SINGLE_ENEMY,
        "Deals light damage and poisons the target.",
        MOVE_CATEGORY_STATUS,
        10,
        MOVE_WEAPON_KIND_NONE,
        {
            { MOVE_EFFECT_KIND_DAMAGE, BATTLE_STATUS_NONE, 10, MOVE_EFFECT_PARAM_NONE, MOVE_EFFECT_PARAM_NONE },
            { MOVE_EFFECT_KIND_INFLICT_STATUS, BATTLE_STATUS_POISON, 100, MOVE_EFFECT_PARAM_NONE, MOVE_EFFECT_PARAM_NONE },
            MOVE_EFFECT_DATA_NONE
        }
    },
    {
        MOVE_CURSE_TEST,
        "Curse Test",
        BATTLE_ELEMENT_CURSE,
        3,
        100,
        0,
        0,
        100,
        0,
        0,
        TARGET_PATTERN_SINGLE_ENEMY,
        "Deals light damage and curses the target.",
        MOVE_CATEGORY_STATUS,
        12,
        MOVE_WEAPON_KIND_NONE,
        {
            { MOVE_EFFECT_KIND_DAMAGE, BATTLE_STATUS_NONE, 12, MOVE_EFFECT_PARAM_NONE, MOVE_EFFECT_PARAM_NONE },
            { MOVE_EFFECT_KIND_INFLICT_STATUS, BATTLE_STATUS_CURSE, 100, MOVE_EFFECT_PARAM_NONE, MOVE_EFFECT_PARAM_NONE },
            MOVE_EFFECT_DATA_NONE
        }
    },
    {
        MOVE_MATTER_STATUS_TEST,
        "Matter Cycle",
        BATTLE_ELEMENT_NONE,
        99,
        100,
        0,
        0,
        100,
        0,
        0,
        TARGET_PATTERN_SINGLE_ENEMY,
        "Debug: cycle Paralysis, Frozen, Confused.",
        MOVE_CATEGORY_SUPPORT,
        0,
        MOVE_WEAPON_KIND_NONE,
        {
            { MOVE_EFFECT_KIND_DEBUG_STATUS_CYCLE, BATTLE_STATUS_NONE, MOVE_EFFECT_PARAM_NONE, MOVE_EFFECT_PARAM_NONE, MOVE_EFFECT_PARAM_NONE },
            MOVE_EFFECT_DATA_NONE,
            MOVE_EFFECT_DATA_NONE
        }
    },
    {
        MOVE_INSTINCT_STATUS_TEST,
        "Instinct Cycle",
        BATTLE_ELEMENT_NONE,
        99,
        100,
        0,
        0,
        100,
        0,
        0,
        TARGET_PATTERN_SINGLE_ENEMY,
        "Debug: cycle Seeded, Confused2, Downed.",
        MOVE_CATEGORY_SUPPORT,
        0,
        MOVE_WEAPON_KIND_NONE,
        {
            { MOVE_EFFECT_KIND_DEBUG_STATUS_CYCLE, BATTLE_STATUS_NONE, 1, MOVE_EFFECT_PARAM_NONE, MOVE_EFFECT_PARAM_NONE },
            MOVE_EFFECT_DATA_NONE,
            MOVE_EFFECT_DATA_NONE
        }
    },
    {
        MOVE_BALANCE_STATUS_TEST,
        "Balance Cycle",
        BATTLE_ELEMENT_NONE,
        99,
        100,
        0,
        0,
        100,
        0,
        0,
        TARGET_PATTERN_SINGLE_ENEMY,
        "Debug: cycle Dark, Light, Chaos.",
        MOVE_CATEGORY_SUPPORT,
        0,
        MOVE_WEAPON_KIND_NONE,
        {
            { MOVE_EFFECT_KIND_DEBUG_STATUS_CYCLE, BATTLE_STATUS_NONE, 2, MOVE_EFFECT_PARAM_NONE, MOVE_EFFECT_PARAM_NONE },
            MOVE_EFFECT_DATA_NONE,
            MOVE_EFFECT_DATA_NONE
        }
    },
    {
        MOVE_CLEAR_STATUS_TEST,
        "Clear Status",
        BATTLE_ELEMENT_NONE,
        99,
        100,
        0,
        0,
        100,
        0,
        0,
        TARGET_PATTERN_SINGLE_ENEMY,
        "Debug: clear all target statuses.",
        MOVE_CATEGORY_SUPPORT,
        0,
        MOVE_WEAPON_KIND_NONE,
        {
            { MOVE_EFFECT_KIND_CLEAR_STATUS, BATTLE_STATUS_NONE, MOVE_EFFECT_PARAM_NONE, MOVE_EFFECT_PARAM_NONE, MOVE_EFFECT_PARAM_NONE },
            MOVE_EFFECT_DATA_NONE,
            MOVE_EFFECT_DATA_NONE
        }
    },
    {
        MOVE_DARK_TEST,
        "Dark Test",
        BATTLE_ELEMENT_DARK,
        3,
        100,
        0,
        0,
        100,
        0,
        0,
        TARGET_PATTERN_SINGLE_ENEMY,
        "Deals light damage and inflicts Dark status.",
        MOVE_CATEGORY_STATUS,
        15,
        MOVE_WEAPON_KIND_NONE,
        {
            { MOVE_EFFECT_KIND_DAMAGE, BATTLE_STATUS_NONE, 15, MOVE_EFFECT_PARAM_NONE, MOVE_EFFECT_PARAM_NONE },
            { MOVE_EFFECT_KIND_INFLICT_STATUS, BATTLE_STATUS_DARK, 100, MOVE_EFFECT_PARAM_NONE, MOVE_EFFECT_PARAM_NONE },
            MOVE_EFFECT_DATA_NONE
        }
    },
    {
        MOVE_LIGHT_TEST,
        "Light Test",
        BATTLE_ELEMENT_LIGHT,
        3,
        100,
        0,
        0,
        100,
        0,
        0,
        TARGET_PATTERN_SINGLE_ENEMY,
        "Deals light damage and inflicts Light status.",
        MOVE_CATEGORY_STATUS,
        15,
        MOVE_WEAPON_KIND_NONE,
        {
            { MOVE_EFFECT_KIND_DAMAGE, BATTLE_STATUS_NONE, 15, MOVE_EFFECT_PARAM_NONE, MOVE_EFFECT_PARAM_NONE },
            { MOVE_EFFECT_KIND_INFLICT_STATUS, BATTLE_STATUS_LIGHT, 100, MOVE_EFFECT_PARAM_NONE, MOVE_EFFECT_PARAM_NONE },
            MOVE_EFFECT_DATA_NONE
        }
    },
    {
        MOVE_CHAOS_TEST,
        "Chaos Test",
        BATTLE_ELEMENT_CHAOS,
        3,
        100,
        0,
        0,
        100,
        0,
        0,
        TARGET_PATTERN_SINGLE_ENEMY,
        "Deals light damage and inflicts Chaos status.",
        MOVE_CATEGORY_STATUS,
        15,
        MOVE_WEAPON_KIND_NONE,
        {
            { MOVE_EFFECT_KIND_DAMAGE, BATTLE_STATUS_NONE, 15, MOVE_EFFECT_PARAM_NONE, MOVE_EFFECT_PARAM_NONE },
            { MOVE_EFFECT_KIND_INFLICT_STATUS, BATTLE_STATUS_CHAOS, 100, MOVE_EFFECT_PARAM_NONE, MOVE_EFFECT_PARAM_NONE },
            MOVE_EFFECT_DATA_NONE
        }
    },
    {
        MOVE_SWING,
        "Swing",
        BATTLE_ELEMENT_NONE,
        20,
        100,
        1,
        0,
        100,
        0,
        0,
        TARGET_PATTERN_SINGLE_ENEMY,
        "A weak Sword attack that hits 1 target.",
        MOVE_CATEGORY_PHYSICAL,
        30,
        MOVE_WEAPON_KIND_SWORD,
        {
            { MOVE_EFFECT_KIND_DAMAGE, BATTLE_STATUS_NONE, 30, MOVE_EFFECT_PARAM_NONE, MOVE_EFFECT_PARAM_NONE },
            MOVE_EFFECT_DATA_NONE,
            MOVE_EFFECT_DATA_NONE
        }
    },
    {
        MOVE_JAB,
        "Jab",
        BATTLE_ELEMENT_NONE,
        20,
        100,
        1,
        0,
        100,
        0,
        0,
        TARGET_PATTERN_SINGLE_ENEMY,
        "A weak jab that hits 1 target.",
        MOVE_CATEGORY_PHYSICAL,
        30,
        MOVE_WEAPON_KIND_SLINGSHOT,
        {
            { MOVE_EFFECT_KIND_DAMAGE, BATTLE_STATUS_NONE, 30, MOVE_EFFECT_PARAM_NONE, MOVE_EFFECT_PARAM_NONE },
            MOVE_EFFECT_DATA_NONE,
            MOVE_EFFECT_DATA_NONE
        }
    },
    {
        MOVE_DIRECT_ATTACK,
        "Direct Attack",
        BATTLE_ELEMENT_MIGHT,
        14,
        100,
        1,
        0,
        100,
        0,
        0,
        TARGET_PATTERN_SINGLE_ENEMY,
        "A really weak Sword charge attack.",
        MOVE_CATEGORY_PHYSICAL,
        20,
        MOVE_WEAPON_KIND_SWORD,
        {
            { MOVE_EFFECT_KIND_DAMAGE, BATTLE_STATUS_NONE, 20, MOVE_EFFECT_PARAM_NONE, MOVE_EFFECT_PARAM_NONE },
            MOVE_EFFECT_DATA_NONE,
            MOVE_EFFECT_DATA_NONE
        }
    },
    {
        MOVE_FAST_PUNCH,
        "Fast Punch",
        BATTLE_ELEMENT_FIRE,
        4,
        100,
        1,
        0,
        100,
        0,
        0,
        TARGET_PATTERN_SINGLE_ENEMY,
        "A weak jab that hits 1 target, but lets the user move twice next turn.",
        MOVE_CATEGORY_PHYSICAL,
        30,
        MOVE_WEAPON_KIND_NONE,
        {
            { MOVE_EFFECT_KIND_DAMAGE, BATTLE_STATUS_NONE, 30, MOVE_EFFECT_DAMAGE_CRIT_DEFAULT, MOVE_EFFECT_PARAM_NONE },
            { MOVE_EFFECT_KIND_EXTRA_ACTION_NEXT_TURN, BATTLE_STATUS_NONE, MOVE_EFFECT_EXTRA_ACTION_NEXT_TURN_DEFAULT, MOVE_EFFECT_PARAM_NONE, MOVE_EFFECT_PARAM_NONE },
            MOVE_EFFECT_DATA_NONE
        }
    },
    {
        MOVE_KICK,
        "Kick",
        BATTLE_ELEMENT_NONE,
        20,
        100,
        1,
        0,
        100,
        0,
        0,
        TARGET_PATTERN_SINGLE_ENEMY,
        "A weak attack that hits 1 target.",
        MOVE_CATEGORY_PHYSICAL,
        30,
        MOVE_WEAPON_KIND_NONE,
        {
            { MOVE_EFFECT_KIND_DAMAGE, BATTLE_STATUS_NONE, 30, MOVE_EFFECT_PARAM_NONE, MOVE_EFFECT_PARAM_NONE },
            MOVE_EFFECT_DATA_NONE,
            MOVE_EFFECT_DATA_NONE
        }
    },
    {
        MOVE_HEADBUTT,
        "Headbutt",
        BATTLE_ELEMENT_NONE,
        20,
        100,
        1,
        0,
        100,
        0,
        0,
        TARGET_PATTERN_SINGLE_ENEMY,
        "A weak attack that hits 1 target.",
        MOVE_CATEGORY_PHYSICAL,
        30,
        MOVE_WEAPON_KIND_NONE,
        {
            { MOVE_EFFECT_KIND_DAMAGE, BATTLE_STATUS_NONE, 30, MOVE_EFFECT_PARAM_NONE, MOVE_EFFECT_PARAM_NONE },
            MOVE_EFFECT_DATA_NONE,
            MOVE_EFFECT_DATA_NONE
        }
    },
    {
        MOVE_PUNT,
        "Punt",
        BATTLE_ELEMENT_NONE,
        20,
        100,
        1,
        0,
        100,
        0,
        0,
        TARGET_PATTERN_SINGLE_ENEMY,
        "A weak attack that hits 1 target.",
        MOVE_CATEGORY_PHYSICAL,
        30,
        MOVE_WEAPON_KIND_NONE,
        {
            { MOVE_EFFECT_KIND_DAMAGE, BATTLE_STATUS_NONE, 30, MOVE_EFFECT_PARAM_NONE, MOVE_EFFECT_PARAM_NONE },
            MOVE_EFFECT_DATA_NONE,
            MOVE_EFFECT_DATA_NONE
        }
    },
    {
        MOVE_SLAP,
        "Slap",
        BATTLE_ELEMENT_NONE,
        20,
        100,
        1,
        0,
        100,
        0,
        0,
        TARGET_PATTERN_SINGLE_ENEMY,
        "A weak attack that hits 1 target.",
        MOVE_CATEGORY_PHYSICAL,
        30,
        MOVE_WEAPON_KIND_NONE,
        {
            { MOVE_EFFECT_KIND_DAMAGE, BATTLE_STATUS_NONE, 30, MOVE_EFFECT_PARAM_NONE, MOVE_EFFECT_PARAM_NONE },
            MOVE_EFFECT_DATA_NONE,
            MOVE_EFFECT_DATA_NONE
        }
    },
    {
        MOVE_STAB,
        "Stab",
        BATTLE_ELEMENT_NONE,
        20,
        100,
        1,
        0,
        100,
        0,
        0,
        TARGET_PATTERN_SINGLE_ENEMY,
        "A weak attack that hits 1 target.",
        MOVE_CATEGORY_PHYSICAL,
        30,
        MOVE_WEAPON_KIND_NONE,
        {
            { MOVE_EFFECT_KIND_DAMAGE, BATTLE_STATUS_NONE, 30, MOVE_EFFECT_PARAM_NONE, MOVE_EFFECT_PARAM_NONE },
            MOVE_EFFECT_DATA_NONE,
            MOVE_EFFECT_DATA_NONE
        }
    },
    {
        MOVE_PUNCH,
        "Punch",
        BATTLE_ELEMENT_NONE,
        20,
        100,
        1,
        0,
        100,
        0,
        0,
        TARGET_PATTERN_SINGLE_ENEMY,
        "A weak jab that hits 1 target.",
        MOVE_CATEGORY_PHYSICAL,
        30,
        MOVE_WEAPON_KIND_NONE,
        {
            { MOVE_EFFECT_KIND_DAMAGE, BATTLE_STATUS_NONE, 30, MOVE_EFFECT_PARAM_NONE, MOVE_EFFECT_PARAM_NONE },
            MOVE_EFFECT_DATA_NONE,
            MOVE_EFFECT_DATA_NONE
        }
    },
    {
        MOVE_SHOOT,
        "Shoot",
        BATTLE_ELEMENT_NONE,
        20,
        100,
        1,
        0,
        100,
        0,
        0,
        TARGET_PATTERN_SINGLE_ENEMY,
        "A weak attack that hits 1 target.",
        MOVE_CATEGORY_PHYSICAL,
        30,
        MOVE_WEAPON_KIND_NONE,
        {
            { MOVE_EFFECT_KIND_DAMAGE, BATTLE_STATUS_NONE, 30, MOVE_EFFECT_PARAM_NONE, MOVE_EFFECT_PARAM_NONE },
            MOVE_EFFECT_DATA_NONE,
            MOVE_EFFECT_DATA_NONE
        }
    },
    {
        MOVE_FLASH,
        "Flash",
        BATTLE_ELEMENT_NONE,
        20,
        100,
        1,
        0,
        100,
        0,
        0,
        TARGET_PATTERN_SINGLE_ENEMY,
        "A weak attack that hits 1 target.",
        MOVE_CATEGORY_PHYSICAL,
        30,
        MOVE_WEAPON_KIND_NONE,
        {
            { MOVE_EFFECT_KIND_DAMAGE, BATTLE_STATUS_NONE, 30, MOVE_EFFECT_PARAM_NONE, MOVE_EFFECT_PARAM_NONE },
            MOVE_EFFECT_DATA_NONE,
            MOVE_EFFECT_DATA_NONE
        }
    },
    {
        MOVE_SPOUT,
        "Spout",
        BATTLE_ELEMENT_WATER,
        10,
        100,
        0,
        0,
        100,
        0,
        0,
        TARGET_PATTERN_SINGLE_ENEMY,
        "A Water attack that hits 1 target.",
        MOVE_CATEGORY_SPECIAL,
        50,
        MOVE_WEAPON_KIND_NONE,
        {
            { MOVE_EFFECT_KIND_DAMAGE, BATTLE_STATUS_NONE, 50, MOVE_EFFECT_PARAM_NONE, MOVE_EFFECT_PARAM_NONE },
            MOVE_EFFECT_DATA_NONE,
            MOVE_EFFECT_DATA_NONE
        }
    },
    {
        MOVE_LONG_SHOT,
        "Long Shot",
        BATTLE_ELEMENT_FIRE,
        12,
        100,
        0,
        0,
        100,
        0,
        0,
        TARGET_PATTERN_SINGLE_ENEMY,
        "Fire shot; power rises with distance (10-34).",
        MOVE_CATEGORY_SPECIAL,
        10,
        MOVE_WEAPON_KIND_NONE,
        {
            { MOVE_EFFECT_KIND_DAMAGE, BATTLE_STATUS_NONE, 10, MOVE_EFFECT_PARAM_NONE, MOVE_EFFECT_PARAM_NONE },
            MOVE_EFFECT_DATA_NONE,
            MOVE_EFFECT_DATA_NONE
        }
    },
    {
        MOVE_EQUALIZER,
        "Equalizer",
        BATTLE_ELEMENT_DARK,
        4,
        100,
        1,
        0,
        100,
        0,
        0,
        TARGET_PATTERN_SINGLE_ENEMY,
        "A Dark attack that hits 1 target. "
        "Conditional crit effect deferred.",
        MOVE_CATEGORY_PHYSICAL,
        70,
        MOVE_WEAPON_KIND_NONE,
        {
            { MOVE_EFFECT_KIND_DAMAGE, BATTLE_STATUS_NONE, 70, MOVE_EFFECT_PARAM_NONE, MOVE_EFFECT_PARAM_NONE },
            MOVE_EFFECT_DATA_NONE,
            MOVE_EFFECT_DATA_NONE
        }
    },
    {
        MOVE_YOUNG_BLOOD,
        "Young Blood",
        BATTLE_ELEMENT_FIRE,
        2,
        100,
        0,
        0,
        100,
        0,
        0,
        TARGET_PATTERN_SELF,
        "Raises the user's attack by 15%.",
        MOVE_CATEGORY_SUPPORT,
        0,
        MOVE_WEAPON_KIND_NONE,
        {
            {
                MOVE_EFFECT_KIND_STAT_MODIFIER,
                BATTLE_STATUS_NONE,
                BATTLE_STAT_ATTACK,
                15,
                MOVE_EFFECT_STAT_PACK_META(BATTLE_STAT_MOD_DURATION_BATTLE_LONG, 1)
            },
            MOVE_EFFECT_DATA_NONE,
            MOVE_EFFECT_DATA_NONE
        }
    },
    {
        MOVE_TIDE,
        "Tide",
        BATTLE_ELEMENT_WATER,
        3,
        100,
        0,
        0,
        100,
        0,
        0,
        TARGET_PATTERN_ALL_ENEMIES,
        "Lowers all opponents' speed by 15%.",
        MOVE_CATEGORY_SUPPORT,
        0,
        MOVE_WEAPON_KIND_NONE,
        {
            {
                MOVE_EFFECT_KIND_STAT_MODIFIER,
                BATTLE_STATUS_NONE,
                BATTLE_STAT_SPEED,
                -15,
                MOVE_EFFECT_STAT_PACK_META(BATTLE_STAT_MOD_DURATION_BATTLE_LONG, 1)
            },
            MOVE_EFFECT_DATA_NONE,
            MOVE_EFFECT_DATA_NONE
        }
    },
    {
        MOVE_TIPPING_THE_SCALES,
        "Tipping the Scales",
        BATTLE_ELEMENT_CHAOS,
        2,
        100,
        0,
        0,
        100,
        0,
        0,
        TARGET_PATTERN_SINGLE_ALLY,
        "Raises an ally's evasion by 35% and lowers their defence by 15%.",
        MOVE_CATEGORY_SUPPORT,
        0,
        MOVE_WEAPON_KIND_NONE,
        {
            {
                MOVE_EFFECT_KIND_STAT_MODIFIER,
                BATTLE_STATUS_NONE,
                BATTLE_STAT_EVASIVENESS,
                35,
                MOVE_EFFECT_STAT_PACK_META(BATTLE_STAT_MOD_DURATION_BATTLE_LONG, 1)
            },
            {
                MOVE_EFFECT_KIND_STAT_MODIFIER,
                BATTLE_STATUS_NONE,
                BATTLE_STAT_DEFENCE,
                -15,
                MOVE_EFFECT_STAT_PACK_META(BATTLE_STAT_MOD_DURATION_BATTLE_LONG, 1)
            },
            MOVE_EFFECT_DATA_NONE
        }
    },
    {
        MOVE_PEP_TALK,
        "Pep Talk",
        BATTLE_ELEMENT_MIGHT,
        2,
        100,
        0,
        0,
        100,
        0,
        0,
        TARGET_PATTERN_ADJACENT_ALLIES,
        "Give adjacent allies a 15% speed boost. Stacks up to 3 times.",
        MOVE_CATEGORY_SUPPORT,
        0,
        MOVE_WEAPON_KIND_NONE,
        {
            {
                MOVE_EFFECT_KIND_STAT_MODIFIER,
                BATTLE_STATUS_NONE,
                BATTLE_STAT_SPEED,
                15,
                MOVE_EFFECT_STAT_PACK_META(BATTLE_STAT_MOD_DURATION_BATTLE_LONG, 3)
            },
            MOVE_EFFECT_DATA_NONE,
            MOVE_EFFECT_DATA_NONE
        }
    },
    {
        MOVE_FAKIE,
        "Fakie",
        BATTLE_ELEMENT_DARK,
        4,
        100,
        1,
        0,
        100,
        0,
        0,
        TARGET_PATTERN_SINGLE_ENEMY,
        "Hits two times. The second hit has increased crit chance.",
        MOVE_CATEGORY_PHYSICAL,
        30,
        MOVE_WEAPON_KIND_SLINGSHOT,
        {
            { MOVE_EFFECT_KIND_DAMAGE, BATTLE_STATUS_NONE, 15, MOVE_EFFECT_DAMAGE_CRIT_DEFAULT, MOVE_EFFECT_PARAM_NONE },
            {
                MOVE_EFFECT_KIND_DAMAGE,
                BATTLE_STATUS_NONE,
                15,
                MOVE_EFFECT_DAMAGE_CRIT_OVERRIDE(250),
                MOVE_EFFECT_PARAM_NONE
            },
            MOVE_EFFECT_DATA_NONE
        }
    },
    {
        MOVE_SNIPE_DOWN,
        "Snipe Down",
        BATTLE_ELEMENT_NONE,
        12,
        100,
        1,
        0,
        100,
        0,
        0,
        TARGET_PATTERN_SINGLE_ENEMY,
        "A weak slingshot attack that prevents the target "
        "from changing spaces for 3 turns.",
        MOVE_CATEGORY_PHYSICAL,
        35,
        MOVE_WEAPON_KIND_SLINGSHOT,
        {
            { MOVE_EFFECT_KIND_DAMAGE, BATTLE_STATUS_NONE, 35, MOVE_EFFECT_DAMAGE_CRIT_DEFAULT, MOVE_EFFECT_PARAM_NONE },
            { MOVE_EFFECT_KIND_MOVEMENT_LOCK, BATTLE_STATUS_NONE, 3, MOVE_EFFECT_PARAM_NONE, MOVE_EFFECT_PARAM_NONE },
            MOVE_EFFECT_DATA_NONE
        }
    },
    {
        MOVE_HAMMER_FIST,
        "Hammer Fist",
        BATTLE_ELEMENT_NONE,
        8,
        100,
        1,
        0,
        100,
        0,
        0,
        TARGET_PATTERN_SINGLE_ENEMY,
        "Hits the target and adjacent enemies for half damage.",
        MOVE_CATEGORY_PHYSICAL,
        30,
        MOVE_WEAPON_KIND_SWORD,
        {
            { MOVE_EFFECT_KIND_DAMAGE, BATTLE_STATUS_NONE, 30, MOVE_EFFECT_DAMAGE_CRIT_DEFAULT, MOVE_EFFECT_PARAM_NONE },
            { MOVE_EFFECT_KIND_SIDE_SPLASH_DAMAGE, BATTLE_STATUS_NONE, 50, MOVE_EFFECT_PARAM_NONE, MOVE_EFFECT_PARAM_NONE },
            MOVE_EFFECT_DATA_NONE
        }
    },
    {
        MOVE_PHANTOM_PHIST,
        "Phantom Phist",
        BATTLE_ELEMENT_CURSE,
        6,
        100,
        0,
        0,
        100,
        0,
        0,
        TARGET_PATTERN_SINGLE_ENEMY,
        "A ghastly attack; crits Curse the main target.",
        MOVE_CATEGORY_SPECIAL,
        30,
        MOVE_WEAPON_KIND_SWORD,
        {
            { MOVE_EFFECT_KIND_DAMAGE, BATTLE_STATUS_NONE, 30, MOVE_EFFECT_DAMAGE_CRIT_DEFAULT, MOVE_EFFECT_PARAM_NONE },
            { MOVE_EFFECT_KIND_SIDE_SPLASH_DAMAGE, BATTLE_STATUS_NONE, 100, MOVE_EFFECT_PARAM_NONE, MOVE_EFFECT_PARAM_NONE },
            MOVE_EFFECT_DATA_NONE
        }
    },
    {
        MOVE_VORTEX_ESCAPE,
        "Vortex Escape",
        BATTLE_ELEMENT_FIRE,
        6,
        100,
        0,
        1,
        100,
        0,
        0,
        TARGET_PATTERN_ALLY_SLOT,
        "Move to an empty ally-side space. Spend 1 LP to act again immediately.",
        MOVE_CATEGORY_SUPPORT,
        0,
        MOVE_WEAPON_KIND_NONE,
        {
            { MOVE_EFFECT_KIND_MOVE_TO_SLOT, BATTLE_STATUS_NONE, MOVE_EFFECT_PARAM_NONE, MOVE_EFFECT_PARAM_NONE, MOVE_EFFECT_PARAM_NONE },
            { MOVE_EFFECT_KIND_BONUS_ACTION, BATTLE_STATUS_NONE, MOVE_EFFECT_BONUS_ACTION_REQUIRES_LP, MOVE_EFFECT_PARAM_NONE, MOVE_EFFECT_PARAM_NONE },
            MOVE_EFFECT_DATA_NONE
        }
    },
    {
        MOVE_HOLD_BACK,
        "Hold Back",
        BATTLE_ELEMENT_NONE,
        16,
        100,
        0,
        0,
        100,
        0,
        0,
        TARGET_PATTERN_SELF,
        "Take 25% less damage for 1 turn. Your next attack hits all enemies.",
        MOVE_CATEGORY_SUPPORT,
        0,
        MOVE_WEAPON_KIND_NONE,
        {
            { MOVE_EFFECT_KIND_INCOMING_DAMAGE_REDUCTION, BATTLE_STATUS_NONE, 25, 1, MOVE_EFFECT_PARAM_NONE },
            { MOVE_EFFECT_KIND_PENDING_ATTACK_ALL_ENEMIES, BATTLE_STATUS_NONE, MOVE_EFFECT_PARAM_NONE, MOVE_EFFECT_PARAM_NONE, MOVE_EFFECT_PARAM_NONE },
            MOVE_EFFECT_DATA_NONE
        }
    },
    {
        MOVE_FLAME_CHARGE,
        "Flame Charge",
        BATTLE_ELEMENT_FIRE,
        16,
        100,
        0,
        0,
        100,
        0,
        0,
        TARGET_PATTERN_SELF,
        "Turns the next Slingshot attack into a Fire type.",
        MOVE_CATEGORY_SUPPORT,
        0,
        MOVE_WEAPON_KIND_NONE,
        {
            { MOVE_EFFECT_KIND_PENDING_WEAPON_ATTACK_ELEMENT, BATTLE_STATUS_NONE, BATTLE_ELEMENT_FIRE, MOVE_EFFECT_PARAM_NONE, MOVE_EFFECT_PARAM_NONE },
            MOVE_EFFECT_DATA_NONE,
            MOVE_EFFECT_DATA_NONE
        }
    },
    {
        MOVE_INFLUENCE,
        "Influence",
        BATTLE_ELEMENT_CHAOS,
        8,
        100,
        0,
        1,
        100,
        0,
        0,
        TARGET_PATTERN_SINGLE_ENEMY,
        "Encore the target's last move for 1 turn. Spend 1 LP for 2 turns.",
        MOVE_CATEGORY_DISRUPT,
        0,
        MOVE_WEAPON_KIND_NONE,
        {
            { MOVE_EFFECT_KIND_ENCORE, BATTLE_STATUS_NONE, 1, MOVE_EFFECT_PARAM_NONE, MOVE_EFFECT_PARAM_NONE },
            MOVE_EFFECT_DATA_NONE,
            MOVE_EFFECT_DATA_NONE
        }
    },
    {
        MOVE_TEMPERATURE_DIFFERENCE,
        "Temperature Difference",
        BATTLE_ELEMENT_NONE,
        16,
        100,
        0,
        0,
        100,
        0,
        0,
        TARGET_PATTERN_SELF,
        "Creates a current that turns the next move into a Wind type.",
        MOVE_CATEGORY_SUPPORT,
        0,
        MOVE_WEAPON_KIND_NONE,
        {
            { MOVE_EFFECT_KIND_PENDING_NEXT_MOVE_ELEMENT, BATTLE_STATUS_NONE, BATTLE_ELEMENT_WIND, MOVE_EFFECT_PARAM_NONE, MOVE_EFFECT_PARAM_NONE },
            MOVE_EFFECT_DATA_NONE,
            MOVE_EFFECT_DATA_NONE
        }
    },
    {
        MOVE_UPPERCUT,
        "Uppercut",
        BATTLE_ELEMENT_DARK,
        16,
        100,
        1,
        0,
        100,
        0,
        0,
        TARGET_PATTERN_SINGLE_ENEMY,
        "Hits for medium damage.",
        MOVE_CATEGORY_PHYSICAL,
        50,
        MOVE_WEAPON_KIND_NONE,
        {
            { MOVE_EFFECT_KIND_DAMAGE, BATTLE_STATUS_NONE, 50, MOVE_EFFECT_DAMAGE_CRIT_DEFAULT, MOVE_EFFECT_PARAM_NONE },
            MOVE_EFFECT_DATA_NONE,
            MOVE_EFFECT_DATA_NONE
        }
    },
    {
        MOVE_ELECTRO_THERAPY,
        "Electro-Therapy",
        BATTLE_ELEMENT_LIGHT,
        16,
        100,
        0,
        0,
        100,
        0,
        0,
        TARGET_PATTERN_SINGLE_ALLY,
        "Removes status conditions from one ally.",
        MOVE_CATEGORY_SUPPORT,
        0,
        MOVE_WEAPON_KIND_NONE,
        {
            { MOVE_EFFECT_KIND_CLEAR_STATUS, BATTLE_STATUS_NONE, MOVE_EFFECT_PARAM_NONE, MOVE_EFFECT_PARAM_NONE, MOVE_EFFECT_PARAM_NONE },
            MOVE_EFFECT_DATA_NONE,
            MOVE_EFFECT_DATA_NONE
        }
    },
    {
        MOVE_CHARGE,
        "Charge",
        BATTLE_ELEMENT_FIRE,
        16,
        100,
        0,
        0,
        100,
        0,
        0,
        TARGET_PATTERN_SINGLE_ALLY,
        "Increases the speed of one ally.",
        MOVE_CATEGORY_SUPPORT,
        0,
        MOVE_WEAPON_KIND_NONE,
        {
            {
                MOVE_EFFECT_KIND_STAT_MODIFIER,
                BATTLE_STATUS_NONE,
                BATTLE_STAT_SPEED,
                MOVE_CHARGE_SPEED_BOOST_PERCENT,
                MOVE_EFFECT_STAT_PACK_META(BATTLE_STAT_MOD_DURATION_BATTLE_LONG, 1)
            },
            MOVE_EFFECT_DATA_NONE,
            MOVE_EFFECT_DATA_NONE
        }
    },
    {
        MOVE_BUBBLE,
        "Bubble",
        BATTLE_ELEMENT_WATER,
        8,
        100,
        0,
        0,
        100,
        0,
        0,
        TARGET_PATTERN_SINGLE_ALLY,
        "Converts next damage to healing.",
        MOVE_CATEGORY_SUPPORT,
        0,
        MOVE_WEAPON_KIND_NONE,
        {
            { MOVE_EFFECT_KIND_BUBBLE_PROTECTION, BATTLE_STATUS_NONE, MOVE_EFFECT_PARAM_NONE, MOVE_EFFECT_PARAM_NONE, MOVE_EFFECT_PARAM_NONE },
            MOVE_EFFECT_DATA_NONE,
            MOVE_EFFECT_DATA_NONE
        }
    },
    {
        MOVE_WIDE_THROW,
        "Wide-Throw",
        BATTLE_ELEMENT_NONE,
        10,
        100,
        0,
        0,
        100,
        0,
        0,
        MOVE_TARGET_OUTER_ENEMIES,
        "Hits farthest two enemies.",
        MOVE_CATEGORY_PHYSICAL,
        30,
        MOVE_WEAPON_KIND_NONE,
        {
            { MOVE_EFFECT_KIND_DAMAGE, BATTLE_STATUS_NONE, 30, MOVE_EFFECT_PARAM_NONE, MOVE_EFFECT_PARAM_NONE },
            MOVE_EFFECT_DATA_NONE,
            MOVE_EFFECT_DATA_NONE
        }
    },
    {
        MOVE_VERTIGO,
        "Vertigo",
        BATTLE_ELEMENT_MENTAL,
        2,
        100,
        0,
        1,
        100,
        0,
        0,
        TARGET_PATTERN_SINGLE_ENEMY,
        "Wave hit. Stronger with 1 LP.",
        MOVE_CATEGORY_DISRUPT,
        35,
        MOVE_WEAPON_KIND_NONE,
        {
            { MOVE_EFFECT_KIND_VERTIGO_WAVE_DAMAGE, BATTLE_STATUS_NONE, MOVE_EFFECT_PARAM_NONE, MOVE_EFFECT_PARAM_NONE, MOVE_EFFECT_PARAM_NONE },
            MOVE_EFFECT_DATA_NONE,
            MOVE_EFFECT_DATA_NONE
        }
    },
    {
        MOVE_VEIL,
        "Veil",
        BATTLE_ELEMENT_WATER,
        4,
        100,
        0,
        0,
        100,
        0,
        0,
        TARGET_PATTERN_SELF,
        "Creates luck spots.",
        MOVE_CATEGORY_SUPPORT,
        0,
        MOVE_WEAPON_KIND_NONE,
        {
            { MOVE_EFFECT_KIND_VEIL_LUCK_BUFF, BATTLE_STATUS_NONE, MOVE_EFFECT_PARAM_NONE, MOVE_EFFECT_PARAM_NONE, MOVE_EFFECT_PARAM_NONE },
            MOVE_EFFECT_DATA_NONE,
            MOVE_EFFECT_DATA_NONE
        }
    },
    {
        /* 41A metadata only. Distance heal % and self-exclusion not wired yet.
         * Source pack Power column: %80-60-40 (user-noted update %55-35-15 not in local export). */
        MOVE_HEALING_ROLLOUT,
        "Healing Rollout",
        BATTLE_ELEMENT_CURSE,
        3,
        100,
        0,
        0,
        100,
        0,
        0,
        MOVE_TARGET_ALL_ALLIES,
        "Heals allies by distance; does not heal self.",
        MOVE_CATEGORY_SUPPORT,
        0,
        MOVE_WEAPON_KIND_NONE,
        MOVE_EFFECTS_NONE
    },
    {
        /* HP set + bonus in LAST_CHANCE effect; SC maxed after SpendMoveSc. */
        MOVE_LAST_CHANCE,
        "Last Chance",
        BATTLE_ELEMENT_MIGHT,
        1,
        100,
        0,
        0,
        100,
        0,
        0,
        TARGET_PATTERN_SELF,
        "Low HP, max SC, act again.",
        MOVE_CATEGORY_SUPPORT,
        0,
        MOVE_WEAPON_KIND_NONE,
        {
            { MOVE_EFFECT_KIND_LAST_CHANCE, BATTLE_STATUS_NONE, MOVE_EFFECT_PARAM_NONE, MOVE_EFFECT_PARAM_NONE, MOVE_EFFECT_PARAM_NONE },
            MOVE_EFFECT_DATA_NONE,
            MOVE_EFFECT_DATA_NONE
        }
    },
    {
        /* v1: single-target Fire damage; +10% Atk on actual damage.
         * optionalLpCost left at 0 so deferred LP-per-extra-target is inactive. */
        MOVE_FIERY_BLOOD,
        "Fiery Blood",
        BATTLE_ELEMENT_FIRE,
        4,
        100,
        0,
        0,
        100,
        0,
        0,
        TARGET_PATTERN_SINGLE_ENEMY,
        "Attack rises when it hits.",
        MOVE_CATEGORY_SPECIAL,
        20,
        MOVE_WEAPON_KIND_NONE,
        {
            { MOVE_EFFECT_KIND_DAMAGE, BATTLE_STATUS_NONE, 20, MOVE_EFFECT_PARAM_NONE, MOVE_EFFECT_PARAM_NONE },
            {
                MOVE_EFFECT_KIND_ON_DAMAGE_SELF_STAT_MOD,
                BATTLE_STATUS_NONE,
                BATTLE_STAT_ATTACK,
                10,
                MOVE_EFFECT_STAT_PACK_META(BATTLE_STAT_MOD_DURATION_BATTLE_LONG, 3)
            },
            MOVE_EFFECT_DATA_NONE
        }
    },
    {
        MOVE_FIERY_STRIKE,
        "Fiery Strike",
        BATTLE_ELEMENT_FIRE,
        6,
        100,
        1,
        0,
        100,
        0,
        0,
        TARGET_PATTERN_SINGLE_ENEMY,
        "Fire strike; only hits the closest enemy.",
        MOVE_CATEGORY_PHYSICAL,
        50,
        MOVE_WEAPON_KIND_NONE,
        {
            { MOVE_EFFECT_KIND_DAMAGE, BATTLE_STATUS_NONE, 50, MOVE_EFFECT_PARAM_NONE, MOVE_EFFECT_PARAM_NONE },
            MOVE_EFFECT_DATA_NONE,
            MOVE_EFFECT_DATA_NONE
        }
    },
    {
        MOVE_ALLY_SWAP,
        "Ally Swap",
        BATTLE_ELEMENT_NONE,
        8,
        100,
        0,
        0,
        100,
        0,
        0,
        TARGET_PATTERN_SINGLE_ALLY,
        "Switch places with an ally.",
        MOVE_CATEGORY_SUPPORT,
        0,
        MOVE_WEAPON_KIND_NONE,
        {
            { MOVE_EFFECT_KIND_SWAP_WITH_ALLY, BATTLE_STATUS_NONE, MOVE_EFFECT_PARAM_NONE, MOVE_EFFECT_PARAM_NONE, MOVE_EFFECT_PARAM_NONE },
            MOVE_EFFECT_DATA_NONE,
            MOVE_EFFECT_DATA_NONE
        }
    },
    {
        /* v1: damage + automatic 1-slot push (chosen-tile UI deferred). */
        MOVE_RAM,
        "Ram",
        BATTLE_ELEMENT_MIGHT,
        8,
        100,
        1,
        0,
        100,
        0,
        0,
        TARGET_PATTERN_SINGLE_ENEMY,
        "Hit and push the foe.",
        MOVE_CATEGORY_PHYSICAL,
        30,
        MOVE_WEAPON_KIND_NONE,
        {
            { MOVE_EFFECT_KIND_DAMAGE, BATTLE_STATUS_NONE, 30, MOVE_EFFECT_PARAM_NONE, MOVE_EFFECT_PARAM_NONE },
            {
                MOVE_EFFECT_KIND_AUTO_PUSH_ON_DAMAGE,
                BATTLE_STATUS_NONE,
                MOVE_EFFECT_PARAM_NONE,
                MOVE_EFFECT_PARAM_NONE,
                MOVE_EFFECT_PARAM_NONE
            },
            MOVE_EFFECT_DATA_NONE
        }
    },
    {
        MOVE_HAIL_MARY,
        "Hail Mary",
        BATTLE_ELEMENT_CHAOS,
        6,
        67,
        1,
        0,
        100,
        0,
        100,
        TARGET_PATTERN_SINGLE_ENEMY,
        "High crit, risky hit.",
        MOVE_CATEGORY_PHYSICAL,
        100,
        MOVE_WEAPON_KIND_NONE,
        {
            { MOVE_EFFECT_KIND_DAMAGE, BATTLE_STATUS_NONE, 100, MOVE_EFFECT_PARAM_NONE, MOVE_EFFECT_PARAM_NONE },
            MOVE_EFFECT_DATA_NONE,
            MOVE_EFFECT_DATA_NONE
        }
    },
    {
        MOVE_TEAM_FOCUS,
        "Team Focus",
        BATTLE_ELEMENT_MIGHT,
        8,
        100,
        0,
        0,
        100,
        0,
        0,
        TARGET_PATTERN_SELF,
        "Next ally move cannot miss.",
        MOVE_CATEGORY_SUPPORT,
        0,
        MOVE_WEAPON_KIND_NONE,
        {
            { MOVE_EFFECT_KIND_TEAM_FOCUS, BATTLE_STATUS_NONE, MOVE_EFFECT_PARAM_NONE, MOVE_EFFECT_PARAM_NONE, MOVE_EFFECT_PARAM_NONE },
            MOVE_EFFECT_DATA_NONE,
            MOVE_EFFECT_DATA_NONE
        }
    },
    {
        MOVE_IMMENSE_SUPPORT,
        "Immense Support",
        BATTLE_ELEMENT_FIRE,
        8,
        100,
        0,
        0,
        100,
        0,
        0,
        TARGET_PATTERN_SINGLE_ALLY,
        "Next attack deals double damage.",
        MOVE_CATEGORY_SUPPORT,
        0,
        MOVE_WEAPON_KIND_NONE,
        {
            {
                MOVE_EFFECT_KIND_PENDING_NEXT_DAMAGE_DOUBLE,
                BATTLE_STATUS_NONE,
                MOVE_EFFECT_PARAM_NONE,
                MOVE_EFFECT_PARAM_NONE,
                MOVE_EFFECT_PARAM_NONE
            },
            MOVE_EFFECT_DATA_NONE,
            MOVE_EFFECT_DATA_NONE
        }
    },
    {
        /* Badger control: Electric theming via Light element (no Electric enum yet). */
        MOVE_STATIC_LOCK,
        "Static Lock",
        BATTLE_ELEMENT_LIGHT,
        8,
        100,
        0,
        0,
        100,
        0,
        0,
        TARGET_PATTERN_SINGLE_ENEMY,
        "Stops movement briefly.",
        MOVE_CATEGORY_DISRUPT,
        0,
        MOVE_WEAPON_KIND_NONE,
        {
            { MOVE_EFFECT_KIND_MOVEMENT_LOCK, BATTLE_STATUS_NONE, 2, MOVE_EFFECT_PARAM_NONE, MOVE_EFFECT_PARAM_NONE },
            MOVE_EFFECT_DATA_NONE,
            MOVE_EFFECT_DATA_NONE
        }
    }
};

const int g_MoveDatabaseCount = 59;
