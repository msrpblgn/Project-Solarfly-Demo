#ifndef MEMBER_DATABASE_H
#define MEMBER_DATABASE_H

#include "battle/battle_element.h"
#include "battle/battle_member.h"
#include "battle/battle_race.h"

typedef enum MemberTemplateId {
    MEMBER_TEMPLATE_NONE = 0,
    MEMBER_TEMPLATE_PROTAGONIST,
    MEMBER_TEMPLATE_TEST_DUMMY,
    MEMBER_TEMPLATE_AKSIL,
    MEMBER_TEMPLATE_MAREN,
    MEMBER_TEMPLATE_ELECTRIC_ARMOR_BADGER,
    MEMBER_TEMPLATE_TUTSIL
} MemberTemplateId;

typedef struct MemberTemplate {
    MemberTemplateId id;
    const char *name;
    BattleElementId element;
    BattleRaceId race;
    /* Documented natural stats at Level 50; scaled by stat_curve at startingLevel. */
    int level50Hp;
    int maxLp;
    int level50Attack;
    int level50Defence;
    int level50Speed;
    int baseEvasiveness;
    int baseLuck;
    int startingLevel;
    /* TODO(32B+): award skillTreePoints on level up; spend via skill tree menu. */
    int skillTreePoints;
    MemberWeaponKind weaponKind;
} MemberTemplate;

const MemberTemplate *MemberDatabase_Get(MemberTemplateId id);

#endif
