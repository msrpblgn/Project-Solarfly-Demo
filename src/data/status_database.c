#include "data/status_database.h"

typedef struct StatusInfo {
    BattleStatusId id;
    const char *name;
    const char *shortName;
    const char *inflictPastTense;
    StatusTrio trio;
    u16 fillColor;
    u16 borderColor;
    int tickDamage;
} StatusInfo;

static const StatusInfo s_StatusInfoTable[] = {
    {
        BATTLE_STATUS_NONE,
        "None",
        "---",
        "",
        STATUS_TRIO_NONE,
        RGB15(12, 12, 14),
        RGB15(18, 18, 20),
        0
    },
    {
        BATTLE_STATUS_BURN,
        "Burn",
        "BRN",
        "burned",
        STATUS_TRIO_PAIN,
        RGB15(28, 12, 0),
        RGB15(31, 20, 0),
        2
    },
    {
        BATTLE_STATUS_POISON,
        "Poison",
        "PSN",
        "poisoned",
        STATUS_TRIO_PAIN,
        RGB15(10, 22, 8),
        RGB15(14, 31, 12),
        3
    },
    {
        BATTLE_STATUS_CURSE,
        "Curse",
        "CRS",
        "cursed",
        STATUS_TRIO_PAIN,
        RGB15(8, 4, 20),
        RGB15(18, 10, 28),
        1
    },
    {
        BATTLE_STATUS_PARALYSIS,
        "Paralysis",
        "PAR",
        "paralyzed",
        STATUS_TRIO_MATTER,
        RGB15(24, 24, 4),
        RGB15(31, 31, 8),
        0
    },
    {
        BATTLE_STATUS_FROZEN,
        "Frozen",
        "FRZ",
        "frozen",
        STATUS_TRIO_MATTER,
        RGB15(8, 16, 28),
        RGB15(12, 24, 31),
        0
    },
    {
        BATTLE_STATUS_CONFUSED,
        "Confused",
        "CNF",
        "confused",
        STATUS_TRIO_MATTER,
        RGB15(22, 8, 22),
        RGB15(28, 12, 28),
        0
    },
    {
        BATTLE_STATUS_SEEDED,
        "Seeded",
        "SED",
        "seeded",
        STATUS_TRIO_INSTINCT,
        RGB15(8, 20, 6),
        RGB15(12, 28, 10),
        0
    },
    {
        BATTLE_STATUS_CONFUSED2,
        "Confused2",
        "CF2",
        "mentally confused",
        STATUS_TRIO_INSTINCT,
        RGB15(16, 6, 24),
        RGB15(24, 10, 31),
        0
    },
    {
        BATTLE_STATUS_DOWNED,
        "Downed",
        "DWN",
        "downed",
        STATUS_TRIO_INSTINCT,
        RGB15(18, 6, 6),
        RGB15(28, 10, 10),
        0
    },
    {
        BATTLE_STATUS_DARK,
        "Dark",
        "DRK",
        "darkened",
        STATUS_TRIO_BALANCE,
        RGB15(4, 4, 8),
        RGB15(10, 10, 16),
        0
    },
    {
        BATTLE_STATUS_LIGHT,
        "Light",
        "LGT",
        "lit",
        STATUS_TRIO_BALANCE,
        RGB15(28, 28, 20),
        RGB15(31, 31, 26),
        0
    },
    {
        BATTLE_STATUS_CHAOS,
        "Chaos",
        "CHS",
        "warped",
        STATUS_TRIO_BALANCE,
        RGB15(24, 8, 20),
        RGB15(31, 14, 28),
        0
    },
    {
        BATTLE_STATUS_TEST_PLACEHOLDER,
        "Test Placeholder",
        "TST",
        "affected",
        STATUS_TRIO_NONE,
        RGB15(20, 10, 24),
        RGB15(28, 14, 30),
        0
    }
};

static const StatusInfo *StatusDatabase_FindInfo(BattleStatusId status)
{
    int i;

    for (i = 0; i < (int)(sizeof(s_StatusInfoTable) / sizeof(s_StatusInfoTable[0])); i++)
    {
        if (s_StatusInfoTable[i].id == status)
        {
            return &s_StatusInfoTable[i];
        }
    }
    return &s_StatusInfoTable[0];
}

int StatusDatabase_IsValid(BattleStatusId status)
{
    return (unsigned int)status < (unsigned int)BATTLE_STATUS_COUNT;
}

const char *StatusDatabase_GetName(BattleStatusId status)
{
    if (!StatusDatabase_IsValid(status))
    {
        return "None";
    }
    return StatusDatabase_FindInfo(status)->name;
}

const char *StatusDatabase_GetShortName(BattleStatusId status)
{
    if (!StatusDatabase_IsValid(status))
    {
        return "---";
    }
    return StatusDatabase_FindInfo(status)->shortName;
}

const char *StatusDatabase_GetInflictPastTense(BattleStatusId status)
{
    if (!StatusDatabase_IsValid(status))
    {
        return "affected";
    }
    return StatusDatabase_FindInfo(status)->inflictPastTense;
}

StatusTrio StatusDatabase_GetTrio(BattleStatusId status)
{
    if (!StatusDatabase_IsValid(status))
    {
        return STATUS_TRIO_NONE;
    }
    return StatusDatabase_FindInfo(status)->trio;
}

u16 StatusDatabase_GetIconFillColor(BattleStatusId status)
{
    if (!StatusDatabase_IsValid(status))
    {
        return s_StatusInfoTable[0].fillColor;
    }
    return StatusDatabase_FindInfo(status)->fillColor;
}

u16 StatusDatabase_GetIconBorderColor(BattleStatusId status)
{
    if (!StatusDatabase_IsValid(status))
    {
        return s_StatusInfoTable[0].borderColor;
    }
    return StatusDatabase_FindInfo(status)->borderColor;
}

int StatusDatabase_GetTickDamage(BattleStatusId status)
{
    if (!StatusDatabase_IsValid(status))
    {
        return 0;
    }
    return StatusDatabase_FindInfo(status)->tickDamage;
}
