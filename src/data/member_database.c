#include "data/member_database.h"

static const MemberTemplate s_memberTemplates[] = {
    {
        MEMBER_TEMPLATE_PROTAGONIST,
        "Protagonist",
        BATTLE_ELEMENT_NONE,
        BATTLE_RACE_HUMAN,
        200,
        9,
        115,
        70,
        90,
        0,
        0,
        16,
        0,
        MEMBER_WEAPON_KIND_SWORD
    },
    {
        MEMBER_TEMPLATE_TEST_DUMMY,
        "Test Dummy",
        BATTLE_ELEMENT_NONE,
        BATTLE_RACE_MONSTER,
        400,
        0,
        10,
        10,
        5,
        0,
        0,
        1,
        0,
        MEMBER_WEAPON_KIND_NONE
    },
    {
        MEMBER_TEMPLATE_AKSIL,
        "Aksil",
        BATTLE_ELEMENT_FIRE,
        BATTLE_RACE_MONSTER,
        170,
        9,
        90,
        65,
        125,
        0,
        0,
        16,
        0,
        MEMBER_WEAPON_KIND_SLINGSHOT
    },
    {
        MEMBER_TEMPLATE_MAREN,
        "Maren",
        BATTLE_ELEMENT_WATER,
        BATTLE_RACE_HUMAN,
        220,
        0,
        80,
        100,
        80,
        0,
        0,
        1,
        0,
        MEMBER_WEAPON_KIND_JAVELIN
    },
    {
        MEMBER_TEMPLATE_ELECTRIC_ARMOR_BADGER,
        "Electric-Armor Badger",
        BATTLE_ELEMENT_NONE,
        BATTLE_RACE_ROBOT,
        200,
        0,
        90,
        45,
        85,
        0,
        0,
        15,
        0,
        MEMBER_WEAPON_KIND_NONE
    },
    {
        MEMBER_TEMPLATE_TUTSIL,
        "Tutsil",
        BATTLE_ELEMENT_FIRE,
        BATTLE_RACE_MONSTER,
        200,
        0,
        90,
        75,
        110,
        0,
        0,
        24,
        0,
        MEMBER_WEAPON_KIND_NONE
    }
};

const MemberTemplate *MemberDatabase_Get(MemberTemplateId id)
{
    int i;
    for (i = 0; i < 6; i++)
    {
        if (s_memberTemplates[i].id == id)
        {
            return &s_memberTemplates[i];
        }
    }
    return 0;
}
