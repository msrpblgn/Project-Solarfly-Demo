#include "data/race_database.h"

typedef struct RaceInfo {
    BattleRaceId id;
    const char *name;
    const char *shortName;
} RaceInfo;

static const RaceInfo s_RaceInfoTable[] = {
    {
        BATTLE_RACE_HUMAN,
        "Human",
        "HUM"
    },
    {
        BATTLE_RACE_MONSTER,
        "Monster",
        "MON"
    },
    {
        BATTLE_RACE_ROBOT,
        "Robot",
        "ROB"
    }
};

static const RaceInfo *RaceDatabase_FindInfo(BattleRaceId race)
{
    int i;

    for (i = 0; i < (int)(sizeof(s_RaceInfoTable) / sizeof(s_RaceInfoTable[0])); i++)
    {
        if (s_RaceInfoTable[i].id == race)
        {
            return &s_RaceInfoTable[i];
        }
    }
    return &s_RaceInfoTable[0];
}

int RaceDatabase_IsValid(BattleRaceId race)
{
    return (unsigned int)race < (unsigned int)BATTLE_RACE_COUNT;
}

const char *RaceDatabase_GetName(BattleRaceId race)
{
    if (!RaceDatabase_IsValid(race))
    {
        return "Human";
    }
    return RaceDatabase_FindInfo(race)->name;
}

const char *RaceDatabase_GetShortName(BattleRaceId race)
{
    if (!RaceDatabase_IsValid(race))
    {
        return "HUM";
    }
    return RaceDatabase_FindInfo(race)->shortName;
}
