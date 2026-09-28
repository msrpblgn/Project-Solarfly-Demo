#include "data/element_database.h"
#include "data/race_database.h"

typedef struct ElementInfo {
    BattleElementId id;
    const char *name;
    const char *shortName;
    u16 fillColor;
    u16 borderColor;
} ElementInfo;

typedef struct ElementEffectivenessRule {
    BattleElementId attacker;
    BattleElementId defender;
    ElementEffectiveness result;
} ElementEffectivenessRule;

/* Placeholder colors from Godot ELEMENT_COLORS reference; not final art. */
static const ElementInfo s_ElementInfoTable[] = {
    {
        BATTLE_ELEMENT_NONE,
        "None",
        "---",
        RGB15(14, 14, 16),
        RGB15(22, 22, 24)
    },
    {
        BATTLE_ELEMENT_CURSE,
        "Curse",
        "CRS",
        RGB15(7, 2, 8),
        RGB15(12, 4, 14)
    },
    {
        BATTLE_ELEMENT_MIGHT,
        "Might",
        "MGT",
        RGB15(16, 0, 0),
        RGB15(24, 4, 4)
    },
    {
        BATTLE_ELEMENT_CHAOS,
        "Chaos",
        "CHS",
        RGB15(24, 24, 24),
        RGB15(28, 28, 28)
    },
    {
        BATTLE_ELEMENT_FIRE,
        "Fire",
        "FIR",
        RGB15(31, 8, 0),
        RGB15(31, 14, 4)
    },
    {
        BATTLE_ELEMENT_WATER,
        "Water",
        "H2O",
        RGB15(3, 18, 31),
        RGB15(8, 22, 31)
    },
    {
        BATTLE_ELEMENT_NATURE,
        "Nature",
        "NAT",
        RGB15(4, 17, 4),
        RGB15(8, 24, 8)
    },
    {
        BATTLE_ELEMENT_ICE,
        "Ice",
        "ICE",
        RGB15(21, 26, 28),
        RGB15(26, 29, 31)
    },
    {
        BATTLE_ELEMENT_DARK,
        "Dark",
        "DRK",
        RGB15(5, 5, 5),
        RGB15(12, 12, 12)
    },
    {
        BATTLE_ELEMENT_LIGHT,
        "Light",
        "LGT",
        RGB15(31, 26, 0),
        RGB15(31, 29, 8)
    },
    {
        BATTLE_ELEMENT_WIND,
        "Wind",
        "WND",
        RGB15(18, 24, 28),
        RGB15(24, 28, 31)
    },
    {
        BATTLE_ELEMENT_POISON,
        "Poison",
        "PSN",
        RGB15(14, 6, 22),
        RGB15(22, 10, 28)
    },
    {
        BATTLE_ELEMENT_MENTAL,
        "Mental",
        "MNT",
        RGB15(26, 10, 26),
        RGB15(31, 16, 31)
    }
};

/*
 * TODO: Fill with official Project Solarfly Highly Effective / Cannot Inflict rules
 * from the design doc. Godot prototype had partial rules only (e.g. Dark vs Spirit).
 */
static const ElementEffectivenessRule s_ElementEffectivenessRules[] = {
    /* No non-neutral rules encoded until official graph is available. */
};

/*
 * Official Project Solarfly element-vs-race chart.
 * Columns: Human (spirits), Monster, Robot.
 *
 * Each cell yields:
 * - status rule (Always / Cannot / None)
 * - damage multiplier (100 / 150)
 *
 * Note: ElementDatabase_GetEffectivenessAgainstRace maps 150% cells to
 * HIGHLY_EFFECTIVE for legacy UI/reaction compatibility.
 */
static const ElementRaceInteraction s_ElementRaceInteractions[BATTLE_ELEMENT_COUNT][BATTLE_RACE_COUNT] = {
    [BATTLE_ELEMENT_NONE] = {
        { BATTLE_STATUS_NONE, ELEMENT_STATUS_RULE_CANNOT, 100 },
        { BATTLE_STATUS_NONE, ELEMENT_STATUS_RULE_CANNOT, 100 },
        { BATTLE_STATUS_NONE, ELEMENT_STATUS_RULE_CANNOT, 100 }
    },
    [BATTLE_ELEMENT_CURSE] = {
        { BATTLE_STATUS_CURSE, ELEMENT_STATUS_RULE_ALWAYS, 100 },  /* Spirits/Humans */
        { BATTLE_STATUS_CURSE, ELEMENT_STATUS_RULE_CANNOT, 100 },  /* Monster */
        { BATTLE_STATUS_CURSE, ELEMENT_STATUS_RULE_NONE, 100 }      /* Robot */
    },
    [BATTLE_ELEMENT_MIGHT] = {
        { BATTLE_STATUS_DOWNED, ELEMENT_STATUS_RULE_CANNOT, 100 }, /* Spirits/Humans */
        { BATTLE_STATUS_DOWNED, ELEMENT_STATUS_RULE_ALWAYS, 100 }, /* Monster */
        { BATTLE_STATUS_DOWNED, ELEMENT_STATUS_RULE_NONE, 100 }    /* Robot */
    },
    [BATTLE_ELEMENT_CHAOS] = {
        { BATTLE_STATUS_CHAOS, ELEMENT_STATUS_RULE_NONE, 100 },  /* Spirits/Humans */
        { BATTLE_STATUS_CHAOS, ELEMENT_STATUS_RULE_NONE, 100 },  /* Monster */
        { BATTLE_STATUS_CHAOS, ELEMENT_STATUS_RULE_NONE, 150 }   /* Robot */
    },
    [BATTLE_ELEMENT_FIRE] = {
        { BATTLE_STATUS_BURN, ELEMENT_STATUS_RULE_CANNOT, 100 },  /* Spirits/Humans */
        { BATTLE_STATUS_BURN, ELEMENT_STATUS_RULE_NONE, 100 },     /* Monster */
        { BATTLE_STATUS_BURN, ELEMENT_STATUS_RULE_ALWAYS, 100 }   /* Robot */
    },
    [BATTLE_ELEMENT_WATER] = {
        { BATTLE_STATUS_PARALYSIS, ELEMENT_STATUS_RULE_NONE, 100 }, /* Spirits/Humans */
        { BATTLE_STATUS_PARALYSIS, ELEMENT_STATUS_RULE_CANNOT, 100 }, /* Monster */
        { BATTLE_STATUS_PARALYSIS, ELEMENT_STATUS_RULE_ALWAYS, 100 } /* Robot */
    },
    [BATTLE_ELEMENT_NATURE] = {
        { BATTLE_STATUS_SEEDED, ELEMENT_STATUS_RULE_NONE, 100 },  /* Spirits/Humans */
        { BATTLE_STATUS_SEEDED, ELEMENT_STATUS_RULE_CANNOT, 100 }, /* Monster */
        { BATTLE_STATUS_SEEDED, ELEMENT_STATUS_RULE_ALWAYS, 100 }   /* Robot */
    },
    [BATTLE_ELEMENT_ICE] = {
        { BATTLE_STATUS_FROZEN, ELEMENT_STATUS_RULE_CANNOT, 100 }, /* Spirits/Humans */
        { BATTLE_STATUS_FROZEN, ELEMENT_STATUS_RULE_ALWAYS, 100 },  /* Monster */
        { BATTLE_STATUS_FROZEN, ELEMENT_STATUS_RULE_NONE, 100 }     /* Robot */
    },
    [BATTLE_ELEMENT_DARK] = {
        { BATTLE_STATUS_DARK, ELEMENT_STATUS_RULE_NONE, 150 },  /* Spirits/Humans */
        { BATTLE_STATUS_DARK, ELEMENT_STATUS_RULE_NONE, 100 },  /* Monster */
        { BATTLE_STATUS_DARK, ELEMENT_STATUS_RULE_NONE, 100 }   /* Robot */
    },
    [BATTLE_ELEMENT_LIGHT] = {
        { BATTLE_STATUS_LIGHT, ELEMENT_STATUS_RULE_NONE, 100 }, /* Spirits/Humans */
        { BATTLE_STATUS_LIGHT, ELEMENT_STATUS_RULE_NONE, 150 }, /* Monster */
        { BATTLE_STATUS_LIGHT, ELEMENT_STATUS_RULE_NONE, 100 }   /* Robot */
    },
    [BATTLE_ELEMENT_WIND] = {
        { BATTLE_STATUS_CONFUSED, ELEMENT_STATUS_RULE_ALWAYS, 100 }, /* Spirits/Humans */
        { BATTLE_STATUS_CONFUSED, ELEMENT_STATUS_RULE_NONE, 100 },    /* Monster */
        { BATTLE_STATUS_CONFUSED, ELEMENT_STATUS_RULE_CANNOT, 100 }   /* Robot */
    },
    [BATTLE_ELEMENT_POISON] = {
        { BATTLE_STATUS_POISON, ELEMENT_STATUS_RULE_NONE, 100 },  /* Spirits/Humans */
        { BATTLE_STATUS_POISON, ELEMENT_STATUS_RULE_ALWAYS, 100 }, /* Monster */
        { BATTLE_STATUS_POISON, ELEMENT_STATUS_RULE_CANNOT, 100 }  /* Robot */
    },
    [BATTLE_ELEMENT_MENTAL] = {
        { BATTLE_STATUS_CONFUSED2, ELEMENT_STATUS_RULE_ALWAYS, 100 }, /* Spirits/Humans */
        { BATTLE_STATUS_CONFUSED2, ELEMENT_STATUS_RULE_NONE, 100 },    /* Monster */
        { BATTLE_STATUS_CONFUSED2, ELEMENT_STATUS_RULE_CANNOT, 100 }   /* Robot */
    }
};

static const ElementInfo *ElementDatabase_FindInfo(BattleElementId element)
{
    int i;

    for (i = 0; i < (int)(sizeof(s_ElementInfoTable) / sizeof(s_ElementInfoTable[0])); i++)
    {
        if (s_ElementInfoTable[i].id == element)
        {
            return &s_ElementInfoTable[i];
        }
    }
    return &s_ElementInfoTable[0];
}

int ElementDatabase_IsValid(BattleElementId element)
{
    return (unsigned int)element < (unsigned int)BATTLE_ELEMENT_COUNT;
}

const char *ElementDatabase_GetName(BattleElementId element)
{
    if (!ElementDatabase_IsValid(element))
    {
        return "None";
    }
    return ElementDatabase_FindInfo(element)->name;
}

const char *ElementDatabase_GetShortName(BattleElementId element)
{
    if (!ElementDatabase_IsValid(element))
    {
        return "---";
    }
    return ElementDatabase_FindInfo(element)->shortName;
}

u16 ElementDatabase_GetIconFillColor(BattleElementId element)
{
    if (!ElementDatabase_IsValid(element))
    {
        return s_ElementInfoTable[0].fillColor;
    }
    return ElementDatabase_FindInfo(element)->fillColor;
}

u16 ElementDatabase_GetIconBorderColor(BattleElementId element)
{
    if (!ElementDatabase_IsValid(element))
    {
        return s_ElementInfoTable[0].borderColor;
    }
    return ElementDatabase_FindInfo(element)->borderColor;
}

ElementEffectiveness ElementDatabase_GetEffectiveness(
    BattleElementId attacker,
    BattleElementId defender)
{
    int i;

    if (!ElementDatabase_IsValid(attacker) || !ElementDatabase_IsValid(defender))
    {
        return ELEMENT_EFFECT_NEUTRAL;
    }
    if (attacker == BATTLE_ELEMENT_NONE || defender == BATTLE_ELEMENT_NONE)
    {
        return ELEMENT_EFFECT_NEUTRAL;
    }

    for (i = 0; i < (int)(sizeof(s_ElementEffectivenessRules) / sizeof(s_ElementEffectivenessRules[0])); i++)
    {
        if (s_ElementEffectivenessRules[i].attacker == attacker
            && s_ElementEffectivenessRules[i].defender == defender)
        {
            return s_ElementEffectivenessRules[i].result;
        }
    }
    return ELEMENT_EFFECT_NEUTRAL;
}

ElementEffectiveness ElementDatabase_GetEffectivenessAgainstRace(
    BattleElementId element,
    BattleRaceId race)
{
    if (!RaceDatabase_IsValid(race))
    {
        return ELEMENT_EFFECT_NEUTRAL;
    }
    if (!ElementDatabase_IsValid(element))
    {
        return ELEMENT_EFFECT_NEUTRAL;
    }
    if (element == BATTLE_ELEMENT_NONE)
    {
        return ELEMENT_EFFECT_CANNOT_INFLICT;
    }

    if (s_ElementRaceInteractions[element][race].statusRule
        == ELEMENT_STATUS_RULE_CANNOT)
    {
        return ELEMENT_EFFECT_CANNOT_INFLICT;
    }
    if (s_ElementRaceInteractions[element][race].statusRule
        == ELEMENT_STATUS_RULE_ALWAYS)
    {
        return ELEMENT_EFFECT_HIGHLY_EFFECTIVE;
    }
    if (s_ElementRaceInteractions[element][race].damageMultiplierPercent > 100)
    {
        return ELEMENT_EFFECT_HIGHLY_EFFECTIVE;
    }
    return ELEMENT_EFFECT_NEUTRAL;
}

int ElementDatabase_IsBalanceElement(BattleElementId element)
{
    return element == BATTLE_ELEMENT_DARK
        || element == BATTLE_ELEMENT_LIGHT
        || element == BATTLE_ELEMENT_CHAOS;
}

BattleStatusId ElementDatabase_GetBalanceStatusForElement(BattleElementId element)
{
    switch (element)
    {
    case BATTLE_ELEMENT_DARK:
        return BATTLE_STATUS_DARK;
    case BATTLE_ELEMENT_LIGHT:
        return BATTLE_STATUS_LIGHT;
    case BATTLE_ELEMENT_CHAOS:
        return BATTLE_STATUS_CHAOS;
    default:
        return BATTLE_STATUS_NONE;
    }
}

BattleStatusId ElementDatabase_GetStatusForElement(BattleElementId element)
{
    switch (element)
    {
    case BATTLE_ELEMENT_FIRE:
        return BATTLE_STATUS_BURN;
    case BATTLE_ELEMENT_WATER:
        return BATTLE_STATUS_PARALYSIS;
    case BATTLE_ELEMENT_NATURE:
        return BATTLE_STATUS_SEEDED;
    case BATTLE_ELEMENT_ICE:
        return BATTLE_STATUS_FROZEN;
    case BATTLE_ELEMENT_WIND:
        return BATTLE_STATUS_CONFUSED;
    case BATTLE_ELEMENT_POISON:
        return BATTLE_STATUS_POISON;
    case BATTLE_ELEMENT_MENTAL:
        return BATTLE_STATUS_CONFUSED2;
    case BATTLE_ELEMENT_CURSE:
        return BATTLE_STATUS_CURSE;
    case BATTLE_ELEMENT_MIGHT:
        return BATTLE_STATUS_DOWNED;
    case BATTLE_ELEMENT_DARK:
        return BATTLE_STATUS_DARK;
    case BATTLE_ELEMENT_LIGHT:
        return BATTLE_STATUS_LIGHT;
    case BATTLE_ELEMENT_CHAOS:
        return BATTLE_STATUS_CHAOS;
    default:
        return BATTLE_STATUS_NONE;
    }
}

ElementRaceInteraction ElementDatabase_GetInteraction(
    BattleElementId element,
    BattleRaceId race)
{
    ElementRaceInteraction def;

    def.statusId = BATTLE_STATUS_NONE;
    def.statusRule = ELEMENT_STATUS_RULE_CANNOT;
    def.damageMultiplierPercent = 100;

    if (!RaceDatabase_IsValid(race) || !ElementDatabase_IsValid(element))
    {
        return def;
    }
    return s_ElementRaceInteractions[element][race];
}

u16 ElementDatabase_GetDamageMultiplierPercent(
    BattleElementId element,
    BattleRaceId race)
{
    return ElementDatabase_GetInteraction(element, race).damageMultiplierPercent;
}

int ElementDatabase_IsBalanceElementHighlyEffectiveAgainstRace(
    BattleElementId element,
    BattleRaceId race)
{
    if (!ElementDatabase_IsBalanceElement(element))
    {
        return 0;
    }
    return ElementDatabase_GetEffectivenessAgainstRace(element, race)
        == ELEMENT_EFFECT_HIGHLY_EFFECTIVE;
}
