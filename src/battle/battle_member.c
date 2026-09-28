#include "battle/battle_member.h"
#include "battle/battle_move.h"
#include "battle/battle_state.h"
#include "battle/stat_curve.h"
#include "core/random.h"
#include "data/element_database.h"
#include "data/member_database.h"
#include "data/race_database.h"
#include "data/status_database.h"

static void BattleMember_ClearStatModSlot(BattleStatMod *mod)
{
    if (!mod)
    {
        return;
    }
    mod->stat = BATTLE_STAT_NONE;
    mod->percent = 0;
    mod->stacks = 0;
    mod->maxStacks = 0;
    mod->durationTurns = 0;
}

static int BattleMember_IsValidStatId(BattleStatId stat)
{
    return stat > BATTLE_STAT_NONE && stat <= BATTLE_STAT_EVASIVENESS;
}

static int BattleMember_ApplyPercentDelta(int base, int percentTotal)
{
    int result;

    if (percentTotal == 0)
    {
        return base;
    }
    result = base + (base * percentTotal) / 100;
    return result;
}

void BattleMember_ClearStatMods(BattleMember *member)
{
    int i;

    if (!member)
    {
        return;
    }
    for (i = 0; i < BATTLE_STAT_MOD_SLOT_COUNT; i++)
    {
        BattleMember_ClearStatModSlot(&member->statMods[i]);
    }
}

int BattleMember_ApplyStatMod(
    BattleMember *member,
    BattleStatId stat,
    int percent,
    int maxStacks,
    int durationTurns)
{
    int i;
    int emptySlot = -1;

    if (!member || !BattleMember_IsValidStatId(stat) || percent == 0)
    {
        return 0;
    }
    if (maxStacks < 1)
    {
        maxStacks = 1;
    }
    if (durationTurns < 0)
    {
        durationTurns = 0;
    }

    for (i = 0; i < BATTLE_STAT_MOD_SLOT_COUNT; i++)
    {
        if (member->statMods[i].stat == stat)
        {
            if (maxStacks <= 1)
            {
                member->statMods[i].percent = (signed char)percent;
                member->statMods[i].stacks = 1;
                member->statMods[i].maxStacks = 1;
                member->statMods[i].durationTurns = (unsigned char)durationTurns;
            }
            else if (member->statMods[i].stacks < maxStacks)
            {
                member->statMods[i].stacks++;
            }
            member->statMods[i].durationTurns = (unsigned char)durationTurns;
            return 1;
        }
        if (emptySlot < 0 && member->statMods[i].stat == BATTLE_STAT_NONE)
        {
            emptySlot = i;
        }
    }

    if (emptySlot < 0)
    {
        return 0;
    }

    member->statMods[emptySlot].stat = stat;
    member->statMods[emptySlot].percent = (signed char)percent;
    member->statMods[emptySlot].stacks = 1;
    member->statMods[emptySlot].maxStacks = (unsigned char)maxStacks;
    member->statMods[emptySlot].durationTurns = (unsigned char)durationTurns;
    return 1;
}

int BattleMember_CanApplyStatMod(const BattleMember *member, BattleStatId stat, int maxStacks)
{
    int i;
    int emptySlot = -1;

    if (!member || !BattleMember_IsValidStatId(stat))
    {
        return 0;
    }
    if (maxStacks < 1)
    {
        maxStacks = 1;
    }

    for (i = 0; i < BATTLE_STAT_MOD_SLOT_COUNT; i++)
    {
        if (member->statMods[i].stat == stat)
        {
            if (maxStacks <= 1)
            {
                return 1;
            }
            return member->statMods[i].stacks < maxStacks;
        }
        if (emptySlot < 0 && member->statMods[i].stat == BATTLE_STAT_NONE)
        {
            emptySlot = i;
        }
    }

    return emptySlot >= 0;
}

void BattleMember_RemoveStatModForStat(BattleMember *member, BattleStatId stat)
{
    int i;

    if (!member || !BattleMember_IsValidStatId(stat))
    {
        return;
    }
    for (i = 0; i < BATTLE_STAT_MOD_SLOT_COUNT; i++)
    {
        if (member->statMods[i].stat == stat)
        {
            BattleMember_ClearStatModSlot(&member->statMods[i]);
        }
    }
}

void BattleMember_TickStatMods(BattleMember *member)
{
    int i;
    int duration;

    if (!member)
    {
        return;
    }
    for (i = 0; i < BATTLE_STAT_MOD_SLOT_COUNT; i++)
    {
        if (!BattleMember_IsValidStatId(member->statMods[i].stat))
        {
            continue;
        }
        duration = member->statMods[i].durationTurns;
        if (duration <= 0)
        {
            continue;
        }
        duration--;
        if (duration <= 0)
        {
            BattleMember_ClearStatModSlot(&member->statMods[i]);
        }
        else
        {
            member->statMods[i].durationTurns = (unsigned char)duration;
        }
    }
}

int BattleMember_GetStatModPercent(const BattleMember *member, BattleStatId stat)
{
    int i;
    int total = 0;

    if (!member || !BattleMember_IsValidStatId(stat))
    {
        return 0;
    }
    for (i = 0; i < BATTLE_STAT_MOD_SLOT_COUNT; i++)
    {
        if (member->statMods[i].stat == stat)
        {
            total += member->statMods[i].percent * member->statMods[i].stacks;
        }
    }
    return total;
}

void BattleMember_Init(BattleMember *member, const char *name, int maxHp, int maxLp, int slotIndex, int side)
{
    int i;
    member->name = name;
    member->currentHp = maxHp;
    member->maxHp = maxHp;
    member->currentLp = 0;
    member->maxLp = maxLp;
    member->slotIndex = slotIndex;
    member->side = side;
    member->element = BATTLE_ELEMENT_NONE;
    member->weaponKind = MEMBER_WEAPON_KIND_NONE;
    member->race = BATTLE_RACE_HUMAN;
    member->pendingWeaponAttackElement = BATTLE_ELEMENT_NONE;
    member->pendingNextMoveElement = BATTLE_ELEMENT_NONE;
    member->extraActionsNextTurn = 0;
    member->skipNextTurn = 0;
    member->isAlive = 1;
    member->removedFromField = 0;
    member->koBurrowFramesRemaining = 0;
    member->koBurrowYOffset = 0;
    member->luckStage = 0;
    member->guardNextHit = 0;
    member->bubbleProtectionActive = 0;
    member->javelinSlowActive = 0;
    member->veilLuckStacks = 0;
    member->veilDurationTurns = 0;
    member->level = BATTLE_MEMBER_LEVEL_MIN;
    member->skillTreePoints = 0;
    member->movementLockTurns = 0;
    member->incomingDamageReductionPercent = 0;
    member->incomingDamageReductionTurns = 0;
    member->pendingNextDamageDouble = 0;
    member->pendingAttackHitsAllEnemies = 0;
    member->lastResolvedActionType = BATTLE_ACTION_NONE;
    member->lastResolvedMoveId = MOVE_NONE;
    member->lastResolvedMoveSlot = 0;
    member->lastResolvedTargetSide = 0;
    member->lastResolvedTargetSlot = 0;
    member->lastResolvedUseLpBonus = 0;
    member->encoreTurnsRemaining = 0;
    member->encoredActionType = BATTLE_ACTION_NONE;
    member->encoredMoveId = MOVE_NONE;
    member->encoredMoveSlot = 0;
    member->encoredTargetSide = 0;
    member->encoredTargetSlot = 0;
    member->encoredUseLpBonus = 0;
    member->stats.attack = 0;
    member->stats.defence = 0;
    member->stats.speed = 0;
    member->stats.evasiveness = 0;
    member->stats.luck = 0;
    for (i = 0; i < BATTLE_MEMBER_MOVE_SLOT_COUNT; i++)
    {
        member->equippedMoves[i] = MOVE_NONE;
        member->moveSc[i] = 0;
    }
    BattleMember_ClearStatuses(member);
    BattleMember_ClearStatMods(member);
}

void BattleMember_InitEmpty(BattleMember *member, int slotIndex, int side)
{
    if (!member)
    {
        return;
    }
    BattleMember_Init(member, "Empty", 0, 0, slotIndex, side);
    member->isAlive = 0;
}

void BattleMember_InitFromTemplate(BattleMember *member, const MemberTemplate *template, int slotIndex, int side)
{
    StatCurveStats level50Stats;
    StatCurveStats naturalStats;

    if (!member || !template)
    {
        return;
    }
    BattleMember_Init(member, template->name, 0, template->maxLp, slotIndex, side);
    if (ElementDatabase_IsValid(template->element))
    {
        member->element = template->element;
    }
    else
    {
        member->element = BATTLE_ELEMENT_NONE;
    }
    if (RaceDatabase_IsValid(template->race))
    {
        member->race = template->race;
    }
    else
    {
        member->race = BATTLE_RACE_HUMAN;
    }
    if (template->startingLevel < BATTLE_MEMBER_LEVEL_MIN)
    {
        member->level = BATTLE_MEMBER_LEVEL_MIN;
    }
    else if (template->startingLevel > BATTLE_MEMBER_LEVEL_MAX)
    {
        member->level = BATTLE_MEMBER_LEVEL_MAX;
    }
    else
    {
        member->level = (unsigned char)template->startingLevel;
    }

    level50Stats.hp = (u16)template->level50Hp;
    level50Stats.attack = (u16)template->level50Attack;
    level50Stats.defence = (u16)template->level50Defence;
    level50Stats.speed = (u16)template->level50Speed;
    StatCurve_CalculateNaturalStatsAtLevel(&level50Stats, member->level, &naturalStats);

    member->maxHp = naturalStats.hp;
    member->currentHp = naturalStats.hp;
    member->stats.attack = naturalStats.attack;
    member->stats.defence = naturalStats.defence;
    member->stats.speed = naturalStats.speed;
    member->stats.evasiveness = template->baseEvasiveness;
    member->stats.luck = template->baseLuck;
    if (template->skillTreePoints < 0)
    {
        member->skillTreePoints = 0;
    }
    else if (template->skillTreePoints > 255)
    {
        member->skillTreePoints = 255;
    }
    else
    {
        member->skillTreePoints = (unsigned char)template->skillTreePoints;
    }
    if (template->weaponKind == MEMBER_WEAPON_KIND_SLINGSHOT ||
        template->weaponKind == MEMBER_WEAPON_KIND_SWORD ||
        template->weaponKind == MEMBER_WEAPON_KIND_JAVELIN)
    {
        member->weaponKind = template->weaponKind;
    }
    else
    {
        member->weaponKind = MEMBER_WEAPON_KIND_NONE;
    }
}

int BattleMember_GetLevel(const BattleMember *member)
{
    if (!member)
    {
        return BATTLE_MEMBER_LEVEL_MIN;
    }
    return member->level;
}

void BattleMember_SetLevel(BattleMember *member, int level)
{
    if (!member)
    {
        return;
    }
    if (level < BATTLE_MEMBER_LEVEL_MIN)
    {
        level = BATTLE_MEMBER_LEVEL_MIN;
    }
    if (level > BATTLE_MEMBER_LEVEL_MAX)
    {
        level = BATTLE_MEMBER_LEVEL_MAX;
    }
    member->level = (unsigned char)level;
}

int BattleMember_GetSkillTreePoints(const BattleMember *member)
{
    if (!member)
    {
        return 0;
    }
    return member->skillTreePoints;
}

static void BattleMember_AppendCharToBuffer(char *dest, int *cursor, int destSize, char c)
{
    if (*cursor < destSize - 1)
    {
        dest[*cursor] = c;
        (*cursor)++;
    }
}

static void BattleMember_AppendNumberToBuffer(char *dest, int *cursor, int destSize, int value)
{
    char digits[8];
    int count = 0;
    int temp = value;

    if (temp == 0)
    {
        BattleMember_AppendCharToBuffer(dest, cursor, destSize, '0');
        return;
    }
    while (temp > 0 && count < 8)
    {
        digits[count++] = (char)('0' + (temp % 10));
        temp /= 10;
    }
    while (count > 0)
    {
        BattleMember_AppendCharToBuffer(dest, cursor, destSize, digits[--count]);
    }
}

void BattleMember_FormatNameWithLevel(const BattleMember *member, char *dest, int destSize)
{
    const char *name;
    int cursor = 0;
    int level;
    int levelDigits = 1;
    int temp;
    int nameMax;
    int i;

    if (!dest || destSize <= 0)
    {
        return;
    }
    dest[0] = '\0';
    if (!member)
    {
        return;
    }

    level = BattleMember_GetLevel(member);
    temp = level;
    while (temp >= 10)
    {
        levelDigits++;
        temp /= 10;
    }
    nameMax = destSize - 1 - 3 - levelDigits;
    if (nameMax < 1)
    {
        nameMax = 1;
    }

    name = member->name ? member->name : "Member";
    for (i = 0; name[i] && i < nameMax; i++)
    {
        BattleMember_AppendCharToBuffer(dest, &cursor, destSize, name[i]);
    }
    BattleMember_AppendCharToBuffer(dest, &cursor, destSize, ' ');
    BattleMember_AppendCharToBuffer(dest, &cursor, destSize, 'L');
    BattleMember_AppendCharToBuffer(dest, &cursor, destSize, 'v');
    BattleMember_AppendNumberToBuffer(dest, &cursor, destSize, level);
    dest[cursor] = '\0';
}

int BattleMember_GetAttack(const BattleMember *member)
{
    return member ? member->stats.attack : 0;
}

int BattleMember_GetEffectiveAttack(const BattleMember *member)
{
    int attack;
    int modPercent;

    if (!member)
    {
        return 0;
    }
    attack = BattleMember_GetAttack(member);
    modPercent = BattleMember_GetStatModPercent(member, BATTLE_STAT_ATTACK);
    attack = BattleMember_ApplyPercentDelta(attack, modPercent);
    if (BattleMember_HasStatus(member, BATTLE_STATUS_BURN))
    {
        attack = (attack * BATTLE_STATUS_BURN_ATTACK_MULTIPLIER_PERCENT) / 100;
        if (attack < 1)
        {
            attack = 1;
        }
    }
    if (attack < 1)
    {
        attack = 1;
    }
    return attack;
}

int BattleMember_GetDefence(const BattleMember *member)
{
    return member ? member->stats.defence : 0;
}

int BattleMember_GetSpeed(const BattleMember *member)
{
    return member ? member->stats.speed : 0;
}

int BattleMember_GetEffectiveSpeed(const BattleMember *member)
{
    int speed;
    int modPercent;

    if (!member)
    {
        return 0;
    }
    speed = BattleMember_GetSpeed(member);
    modPercent = BattleMember_GetStatModPercent(member, BATTLE_STAT_SPEED);
    speed = BattleMember_ApplyPercentDelta(speed, modPercent);
    if (BattleMember_HasStatus(member, BATTLE_STATUS_FROZEN))
    {
        speed = (speed * BATTLE_STATUS_FROZEN_SPEED_MULTIPLIER_PERCENT) / 100;
    }
    if (speed < 0)
    {
        speed = 0;
    }
    return speed;
}

int BattleMember_ShouldLoseActionToParalysis(const BattleMember *member)
{
    if (!member || !BattleMember_HasStatus(member, BATTLE_STATUS_PARALYSIS))
    {
        return 0;
    }
    return Random_Percent(BATTLE_STATUS_PARALYSIS_SKIP_CHANCE_PERCENT);
}

int BattleMember_ShouldConfusedSelfHit(const BattleMember *member)
{
    if (!member || !BattleMember_HasStatus(member, BATTLE_STATUS_CONFUSED))
    {
        return 0;
    }
    return Random_Percent(BATTLE_STATUS_CONFUSED_SELF_HIT_CHANCE_PERCENT);
}

int BattleMember_ShouldConfused2HitAlly(const BattleMember *member)
{
    if (!member || !BattleMember_HasStatus(member, BATTLE_STATUS_CONFUSED2))
    {
        return 0;
    }
    return Random_Percent(BATTLE_STATUS_CONFUSED2_ALLY_HIT_CHANCE_PERCENT);
}

int BattleMember_ComputeConfusedSelfDamage(const BattleMember *member)
{
    int attack;
    int damage;

    if (!member)
    {
        return 1;
    }
    attack = BattleMember_GetEffectiveAttack(member);
    damage = attack / 4;
    if (damage < 1)
    {
        damage = 1;
    }
    return damage;
}

int BattleMember_GetEvasiveness(const BattleMember *member)
{
    return member ? member->stats.evasiveness : 0;
}

int BattleMember_GetEffectiveEvasiveness(const BattleMember *member)
{
    int evasiveness;
    int modPercent;

    if (!member)
    {
        return 0;
    }
    evasiveness = BattleMember_GetEvasiveness(member);
    modPercent = BattleMember_GetStatModPercent(member, BATTLE_STAT_EVASIVENESS);
    evasiveness = BattleMember_ApplyPercentDelta(evasiveness, modPercent);
    if (evasiveness < 0)
    {
        evasiveness = 0;
    }
    if (evasiveness > 100)
    {
        evasiveness = 100;
    }
    return evasiveness;
}

int BattleMember_GetLuck(const BattleMember *member)
{
    return member ? member->stats.luck : 0;
}

BattleElementId BattleMember_GetElement(const BattleMember *member)
{
    if (!member || !ElementDatabase_IsValid(member->element))
    {
        return BATTLE_ELEMENT_NONE;
    }
    return member->element;
}

BattleElementId BattleMember_GetBasicAttackElement(const BattleMember *member)
{
    if (!member)
    {
        return BATTLE_ELEMENT_NONE;
    }
    switch (member->weaponKind)
    {
    case MEMBER_WEAPON_KIND_SLINGSHOT:
        return BATTLE_ELEMENT_FIRE;
    case MEMBER_WEAPON_KIND_SWORD:
        return BATTLE_ELEMENT_MIGHT;
    case MEMBER_WEAPON_KIND_JAVELIN:
    case MEMBER_WEAPON_KIND_NONE:
    default:
        return BATTLE_ELEMENT_NONE;
    }
}

BattleRaceId BattleMember_GetRace(const BattleMember *member)
{
    if (!member || !RaceDatabase_IsValid(member->race))
    {
        return BATTLE_RACE_HUMAN;
    }
    return member->race;
}

int BattleMember_GetStatusSlotForTrio(StatusTrio trio)
{
    switch (trio)
    {
    case STATUS_TRIO_PAIN:
        return 0;
    case STATUS_TRIO_MATTER:
        return 1;
    case STATUS_TRIO_INSTINCT:
        return 2;
    default:
        return -1;
    }
}

BattleStatusId BattleMember_GetStatusAt(const BattleMember *member, int slot)
{
    BattleStatusId status;

    if (!member || slot < 0 || slot >= BATTLE_MEMBER_STATUS_SLOT_COUNT)
    {
        return BATTLE_STATUS_NONE;
    }
    status = member->statuses[slot];
    if (!StatusDatabase_IsValid(status))
    {
        return BATTLE_STATUS_NONE;
    }
    return status;
}

BattleStatusId BattleMember_GetBalanceStatus(const BattleMember *member)
{
    BattleStatusId status;

    if (!member)
    {
        return BATTLE_STATUS_NONE;
    }
    status = member->balanceStatus;
    if (!StatusDatabase_IsValid(status) || status == BATTLE_STATUS_NONE)
    {
        return BATTLE_STATUS_NONE;
    }
    return status;
}

int BattleMember_HasPrimaryTrioStatus(const BattleMember *member, StatusTrio trio)
{
    int slot;

    if (!member)
    {
        return 0;
    }
    slot = BattleMember_GetStatusSlotForTrio(trio);
    if (slot < 0)
    {
        return 0;
    }
    return BattleMember_GetStatusAt(member, slot) != BATTLE_STATUS_NONE;
}

int BattleMember_HasAllPrimaryStatusTrios(const BattleMember *member)
{
    if (!member)
    {
        return 0;
    }
    return BattleMember_HasPrimaryTrioStatus(member, STATUS_TRIO_PAIN)
        && BattleMember_HasPrimaryTrioStatus(member, STATUS_TRIO_MATTER)
        && BattleMember_HasPrimaryTrioStatus(member, STATUS_TRIO_INSTINCT);
}

int BattleMember_HasBalanceStatus(const BattleMember *member)
{
    return BattleMember_GetBalanceStatus(member) != BATTLE_STATUS_NONE;
}

int BattleMember_IsAksil(const BattleMember *member)
{
    const MemberTemplate *aksilTemplate;

    if (!member || !member->name)
    {
        return 0;
    }
    aksilTemplate = MemberDatabase_Get(MEMBER_TEMPLATE_AKSIL);
    if (!aksilTemplate || !aksilTemplate->name)
    {
        return 0;
    }
    return member->name == aksilTemplate->name;
}

int BattleMember_IsMaren(const BattleMember *member)
{
    const MemberTemplate *marenTemplate;

    if (!member || !member->name)
    {
        return 0;
    }
    marenTemplate = MemberDatabase_Get(MEMBER_TEMPLATE_MAREN);
    if (!marenTemplate || !marenTemplate->name)
    {
        return 0;
    }
    return member->name == marenTemplate->name;
}

int BattleMember_IsProtagonist(const BattleMember *member)
{
    const MemberTemplate *protagonistTemplate;

    if (!member || !member->name)
    {
        return 0;
    }
    protagonistTemplate = MemberDatabase_Get(MEMBER_TEMPLATE_PROTAGONIST);
    if (!protagonistTemplate || !protagonistTemplate->name)
    {
        return 0;
    }
    return member->name == protagonistTemplate->name;
}

int BattleMember_IsTutsil(const BattleMember *member)
{
    const MemberTemplate *tutsilTemplate;

    if (!member || !member->name)
    {
        return 0;
    }
    tutsilTemplate = MemberDatabase_Get(MEMBER_TEMPLATE_TUTSIL);
    if (!tutsilTemplate || !tutsilTemplate->name)
    {
        return 0;
    }
    return member->name == tutsilTemplate->name;
}

int BattleMember_IsElectricArmorBadger(const BattleMember *member)
{
    const MemberTemplate *badgerTemplate;

    if (!member || !member->name)
    {
        return 0;
    }
    badgerTemplate = MemberDatabase_Get(MEMBER_TEMPLATE_ELECTRIC_ARMOR_BADGER);
    if (!badgerTemplate || !badgerTemplate->name)
    {
        return 0;
    }
    return member->name == badgerTemplate->name;
}

int BattleMember_GetStatusDurationAt(const BattleMember *member, int slot)
{
    if (!member || slot < 0 || slot >= BATTLE_MEMBER_STATUS_SLOT_COUNT)
    {
        return 0;
    }
    return member->statusDurations[slot];
}

void BattleMember_SetStatusDurationAt(BattleMember *member, int slot, int duration)
{
    if (!member || slot < 0 || slot >= BATTLE_MEMBER_STATUS_SLOT_COUNT)
    {
        return;
    }
    if (duration < 0)
    {
        duration = 0;
    }
    member->statusDurations[slot] = duration;
}

void BattleMember_ClearStatusSlotMetadata(BattleMember *member, int slot)
{
    if (!member || slot < 0 || slot >= BATTLE_MEMBER_STATUS_SLOT_COUNT)
    {
        return;
    }
    member->statusDurations[slot] = 0;
}

int BattleMember_GetDurationForNewStatus(BattleStatusId status)
{
    if (status == BATTLE_STATUS_DOWNED)
    {
        return BATTLE_STATUS_DOWNED_DURATION_TURNS;
    }
    return BATTLE_STATUS_DURATION_PERMANENT;
}

int BattleMember_SetStatusAt(BattleMember *member, int slot, BattleStatusId status)
{
    if (!member || slot < 0 || slot >= BATTLE_MEMBER_STATUS_SLOT_COUNT)
    {
        return 0;
    }
    if (!StatusDatabase_IsValid(status))
    {
        return 0;
    }
    BattleMember_ClearStatusSlotMetadata(member, slot);
    member->statuses[slot] = status;
    BattleMember_SetStatusDurationAt(
        member,
        slot,
        BattleMember_GetDurationForNewStatus(status));
    return 1;
}

void BattleMember_ClearStatuses(BattleMember *member)
{
    int i;

    if (!member)
    {
        return;
    }
    for (i = 0; i < BATTLE_MEMBER_STATUS_SLOT_COUNT; i++)
    {
        member->statuses[i] = BATTLE_STATUS_NONE;
        member->statusDurations[i] = 0;
    }
    member->balanceStatus = BATTLE_STATUS_NONE;
    member->poisonSeverity = 0;
}

int BattleMember_HasStatus(const BattleMember *member, BattleStatusId status)
{
    int i;
    StatusTrio trio;
    int slot;

    if (!member || status == BATTLE_STATUS_NONE || !StatusDatabase_IsValid(status))
    {
        return 0;
    }
    trio = StatusDatabase_GetTrio(status);
    if (trio == STATUS_TRIO_BALANCE)
    {
        return member->balanceStatus == status;
    }
    slot = BattleMember_GetStatusSlotForTrio(trio);
    if (slot >= 0)
    {
        return member->statuses[slot] == status;
    }
    for (i = 0; i < BATTLE_MEMBER_STATUS_SLOT_COUNT; i++)
    {
        if (member->statuses[i] == status)
        {
            return 1;
        }
    }
    return 0;
}

int BattleMember_IsDowned(const BattleMember *member)
{
    if (!member)
    {
        return 0;
    }
    return BattleMember_HasStatus(member, BATTLE_STATUS_DOWNED);
}

int BattleMember_TickDownedDuration(BattleMember *member)
{
    int slot;
    int duration;

    if (!member || !BattleMember_HasStatus(member, BATTLE_STATUS_DOWNED))
    {
        return 0;
    }
    slot = BattleMember_GetStatusSlotForTrio(STATUS_TRIO_INSTINCT);
    if (slot < 0 || BattleMember_GetStatusAt(member, slot) != BATTLE_STATUS_DOWNED)
    {
        return 0;
    }
    duration = BattleMember_GetStatusDurationAt(member, slot);
    if (duration <= 0)
    {
        return 0;
    }
    duration--;
    BattleMember_SetStatusDurationAt(member, slot, duration);
    if (duration <= 0)
    {
        BattleMember_RemoveStatus(member, BATTLE_STATUS_DOWNED);
        return 1;
    }
    return 0;
}

BattleStatusId BattleMember_TryRecoverAfterActorTurn(BattleMember *member)
{
    int matterSlot;
    int instinctSlot;
    BattleStatusId status;

    if (!member || !member->isAlive)
    {
        return BATTLE_STATUS_NONE;
    }
    matterSlot = BattleMember_GetStatusSlotForTrio(STATUS_TRIO_MATTER);
    if (matterSlot >= 0)
    {
        status = BattleMember_GetStatusAt(member, matterSlot);
        if (status == BATTLE_STATUS_PARALYSIS || status == BATTLE_STATUS_FROZEN ||
            status == BATTLE_STATUS_CONFUSED)
        {
            if (Random_Percent(BATTLE_STATUS_RECOVER_CHANCE_PERCENT))
            {
                BattleMember_RemoveStatus(member, status);
                return status;
            }
        }
    }
    instinctSlot = BattleMember_GetStatusSlotForTrio(STATUS_TRIO_INSTINCT);
    if (instinctSlot >= 0)
    {
        status = BattleMember_GetStatusAt(member, instinctSlot);
        if (status == BATTLE_STATUS_CONFUSED2)
        {
            if (Random_Percent(BATTLE_STATUS_RECOVER_CHANCE_PERCENT))
            {
                BattleMember_RemoveStatus(member, BATTLE_STATUS_CONFUSED2);
                return BATTLE_STATUS_CONFUSED2;
            }
        }
    }
    return BATTLE_STATUS_NONE;
}

int BattleMember_HasAnyStatus(const BattleMember *member)
{
    int i;

    if (!member)
    {
        return 0;
    }
    if (BattleMember_GetBalanceStatus(member) != BATTLE_STATUS_NONE)
    {
        return 1;
    }
    for (i = 0; i < BATTLE_MEMBER_STATUS_SLOT_COUNT; i++)
    {
        if (member->statuses[i] != BATTLE_STATUS_NONE)
        {
            return 1;
        }
    }
    return 0;
}

int BattleMember_GetFirstEmptyStatusSlot(const BattleMember *member)
{
    int i;

    if (!member)
    {
        return -1;
    }
    for (i = 0; i < BATTLE_MEMBER_STATUS_SLOT_COUNT; i++)
    {
        if (member->statuses[i] == BATTLE_STATUS_NONE)
        {
            return i;
        }
    }
    return -1;
}

int BattleMember_AddStatus(BattleMember *member, BattleStatusId status)
{
    int slot;
    StatusTrio trio;

    if (!member || status == BATTLE_STATUS_NONE || !StatusDatabase_IsValid(status))
    {
        return 0;
    }
    if (!member->isAlive)
    {
        return 0;
    }
    if (BattleMember_HasStatus(member, status))
    {
        return 0;
    }
    trio = StatusDatabase_GetTrio(status);
    if (trio == STATUS_TRIO_BALANCE)
    {
        member->balanceStatus = status;
        return 1;
    }
    slot = BattleMember_GetStatusSlotForTrio(trio);
    if (slot < 0)
    {
        /* Debug-only statuses (e.g. TEST_PLACEHOLDER) use Pain slot 0. */
        slot = 0;
    }
    if (trio == STATUS_TRIO_PAIN)
    {
        member->poisonSeverity = 0;
    }
    BattleMember_ClearStatusSlotMetadata(member, slot);
    member->statuses[slot] = status;
    BattleMember_SetStatusDurationAt(
        member,
        slot,
        BattleMember_GetDurationForNewStatus(status));
    return 1;
}

int BattleMember_RemoveStatus(BattleMember *member, BattleStatusId status)
{
    int i;
    StatusTrio trio;
    int slot;

    if (!member || status == BATTLE_STATUS_NONE || !StatusDatabase_IsValid(status))
    {
        return 0;
    }
    trio = StatusDatabase_GetTrio(status);
    if (trio == STATUS_TRIO_BALANCE)
    {
        if (member->balanceStatus == status)
        {
            member->balanceStatus = BATTLE_STATUS_NONE;
            return 1;
        }
        return 0;
    }
    slot = BattleMember_GetStatusSlotForTrio(trio);
    if (slot >= 0 && member->statuses[slot] == status)
    {
        if (trio == STATUS_TRIO_PAIN)
        {
            member->poisonSeverity = 0;
        }
        member->statuses[slot] = BATTLE_STATUS_NONE;
        BattleMember_ClearStatusSlotMetadata(member, slot);
        return 1;
    }
    for (i = 0; i < BATTLE_MEMBER_STATUS_SLOT_COUNT; i++)
    {
        if (member->statuses[i] == status)
        {
            member->statuses[i] = BATTLE_STATUS_NONE;
            BattleMember_ClearStatusSlotMetadata(member, i);
            return 1;
        }
    }
    return 0;
}

int BattleMember_GetEffectiveDefence(const BattleMember *member)
{
    int defence;
    int modPercent;

    if (!member)
    {
        return 1;
    }
    defence = BattleMember_GetDefence(member);
    modPercent = BattleMember_GetStatModPercent(member, BATTLE_STAT_DEFENCE);
    defence = BattleMember_ApplyPercentDelta(defence, modPercent);
    if (BattleMember_HasStatus(member, BATTLE_STATUS_CURSE))
    {
        defence = (defence * BATTLE_STATUS_CURSE_DEFENCE_MULTIPLIER_PERCENT) / 100;
        if (defence < 0)
        {
            defence = 0;
        }
    }
    if (member->guardNextHit)
    {
        defence = (defence * BATTLE_MEMBER_GUARD_DEFENCE_MULTIPLIER_PERCENT) / 100;
    }
    if (defence < 1)
    {
        defence = 1;
    }
    return defence;
}

void BattleMember_SetGuardNextHit(BattleMember *member, int enabled)
{
    if (!member)
    {
        return;
    }
    member->guardNextHit = enabled ? 1 : 0;
}

int BattleMember_HasGuardNextHit(const BattleMember *member)
{
    return member && member->guardNextHit;
}

void BattleMember_ClearGuardNextHit(BattleMember *member)
{
    if (!member)
    {
        return;
    }
    member->guardNextHit = 0;
}

void BattleMember_SetBubbleProtection(BattleMember *member, int enabled)
{
    if (!member)
    {
        return;
    }
    member->bubbleProtectionActive = enabled ? 1 : 0;
}

int BattleMember_HasBubbleProtection(const BattleMember *member)
{
    return member && member->bubbleProtectionActive;
}

void BattleMember_ClearBubbleProtection(BattleMember *member)
{
    if (!member)
    {
        return;
    }
    member->bubbleProtectionActive = 0;
}

int BattleMember_ApplyBubbleDamageConversion(BattleMember *member, int damage)
{
    int healAmount;

    if (!member || damage <= 0 || !BattleMember_HasBubbleProtection(member))
    {
        return 0;
    }

    healAmount = damage / 2;
    if (healAmount > 0)
    {
        BattleMember_Heal(member, healAmount);
    }
    return 1;
}

int BattleMember_HasJavelinSlow(const BattleMember *member)
{
    return member && member->javelinSlowActive;
}

void BattleMember_SetJavelinSlow(BattleMember *member, int active)
{
    if (!member)
    {
        return;
    }
    member->javelinSlowActive = active ? 1 : 0;
}

int BattleMember_IsMovementLocked(const BattleMember *member)
{
    return member && member->movementLockTurns > 0;
}

void BattleMember_SetMovementLock(BattleMember *member, int turns)
{
    if (!member)
    {
        return;
    }
    if (turns < 0)
    {
        turns = 0;
    }
    if (turns > 255)
    {
        turns = 255;
    }
    member->movementLockTurns = (unsigned char)turns;
}

void BattleMember_TickMovementLock(BattleMember *member)
{
    if (!member || member->movementLockTurns <= 0)
    {
        return;
    }
    member->movementLockTurns--;
}

void BattleMember_ClearMovementLock(BattleMember *member)
{
    BattleMember_SetMovementLock(member, 0);
}

void BattleMember_SetIncomingDamageReduction(BattleMember *member, int percent, int turns)
{
    if (!member)
    {
        return;
    }
    if (percent < 0)
    {
        percent = 0;
    }
    if (percent > 100)
    {
        percent = 100;
    }
    if (turns < 0)
    {
        turns = 0;
    }
    if (turns > 255)
    {
        turns = 255;
    }
    if (turns <= 0)
    {
        member->incomingDamageReductionPercent = 0;
        member->incomingDamageReductionTurns = 0;
        return;
    }
    member->incomingDamageReductionPercent = (unsigned char)percent;
    member->incomingDamageReductionTurns = (unsigned char)turns;
}

int BattleMember_GetIncomingDamageReductionPercent(const BattleMember *member)
{
    if (!member || member->incomingDamageReductionTurns <= 0)
    {
        return 0;
    }
    return member->incomingDamageReductionPercent;
}

void BattleMember_TickIncomingDamageReduction(BattleMember *member)
{
    if (!member || member->incomingDamageReductionTurns <= 0)
    {
        return;
    }
    member->incomingDamageReductionTurns--;
    if (member->incomingDamageReductionTurns <= 0)
    {
        member->incomingDamageReductionPercent = 0;
    }
}

void BattleMember_ClearIncomingDamageReduction(BattleMember *member)
{
    BattleMember_SetIncomingDamageReduction(member, 0, 0);
}

void BattleMember_SetPendingAttackHitsAllEnemies(BattleMember *member, int enabled)
{
    if (!member)
    {
        return;
    }
    member->pendingAttackHitsAllEnemies = enabled ? 1 : 0;
}

int BattleMember_HasPendingAttackHitsAllEnemies(const BattleMember *member)
{
    return member && member->pendingAttackHitsAllEnemies;
}

void BattleMember_ClearPendingAttackHitsAllEnemies(BattleMember *member)
{
    if (!member)
    {
        return;
    }
    member->pendingAttackHitsAllEnemies = 0;
}

void BattleMember_SetPendingNextDamageDouble(BattleMember *member, int enabled)
{
    if (!member)
    {
        return;
    }
    member->pendingNextDamageDouble = enabled ? 1 : 0;
}

int BattleMember_HasPendingNextDamageDouble(const BattleMember *member)
{
    return member && member->pendingNextDamageDouble;
}

void BattleMember_ClearPendingNextDamageDouble(BattleMember *member)
{
    if (!member)
    {
        return;
    }
    member->pendingNextDamageDouble = 0;
}

void BattleMember_SetPendingWeaponAttackElement(BattleMember *member, BattleElementId element)
{
    if (!member)
    {
        return;
    }
    if (!ElementDatabase_IsValid(element) || element == BATTLE_ELEMENT_NONE)
    {
        BattleMember_ClearPendingWeaponAttackElement(member);
        return;
    }
    member->pendingWeaponAttackElement = element;
}

int BattleMember_HasPendingWeaponAttackElement(const BattleMember *member)
{
    if (!member)
    {
        return 0;
    }
    return member->pendingWeaponAttackElement != BATTLE_ELEMENT_NONE;
}

BattleElementId BattleMember_GetPendingWeaponAttackElement(const BattleMember *member)
{
    if (!member || !BattleMember_HasPendingWeaponAttackElement(member))
    {
        return BATTLE_ELEMENT_NONE;
    }
    return member->pendingWeaponAttackElement;
}

void BattleMember_ClearPendingWeaponAttackElement(BattleMember *member)
{
    if (!member)
    {
        return;
    }
    member->pendingWeaponAttackElement = BATTLE_ELEMENT_NONE;
}

void BattleMember_SetPendingNextMoveElement(BattleMember *member, BattleElementId element)
{
    if (!member)
    {
        return;
    }
    if (!ElementDatabase_IsValid(element) || element == BATTLE_ELEMENT_NONE)
    {
        BattleMember_ClearPendingNextMoveElement(member);
        return;
    }
    member->pendingNextMoveElement = element;
}

int BattleMember_HasPendingNextMoveElement(const BattleMember *member)
{
    if (!member)
    {
        return 0;
    }
    return member->pendingNextMoveElement != BATTLE_ELEMENT_NONE;
}

BattleElementId BattleMember_GetPendingNextMoveElement(const BattleMember *member)
{
    if (!member || !BattleMember_HasPendingNextMoveElement(member))
    {
        return BATTLE_ELEMENT_NONE;
    }
    return member->pendingNextMoveElement;
}

void BattleMember_ClearPendingNextMoveElement(BattleMember *member)
{
    if (!member)
    {
        return;
    }
    member->pendingNextMoveElement = BATTLE_ELEMENT_NONE;
}

void BattleMember_SetExtraActionsNextTurn(BattleMember *member, int count)
{
    if (!member)
    {
        return;
    }
    if (count < 0)
    {
        count = 0;
    }
    if (count > 255)
    {
        count = 255;
    }
    member->extraActionsNextTurn = (unsigned char)count;
}

int BattleMember_GetExtraActionsNextTurn(const BattleMember *member)
{
    if (!member)
    {
        return 0;
    }
    return member->extraActionsNextTurn;
}

void BattleMember_ClearExtraActionsNextTurn(BattleMember *member)
{
    if (!member)
    {
        return;
    }
    member->extraActionsNextTurn = 0;
}

void BattleMember_SetSkipNextTurn(BattleMember *member, int enabled)
{
    if (!member)
    {
        return;
    }
    member->skipNextTurn = enabled ? 1 : 0;
}

int BattleMember_GetSkipNextTurn(const BattleMember *member)
{
    if (!member)
    {
        return 0;
    }
    return member->skipNextTurn;
}

void BattleMember_ClearSkipNextTurn(BattleMember *member)
{
    if (!member)
    {
        return;
    }
    member->skipNextTurn = 0;
}

void BattleMember_ClearReservedMoveSlot(BattleMember *member)
{
    if (!member)
    {
        return;
    }
    member->equippedMoves[BATTLE_MEMBER_RESERVED_MOVE_SLOT] = MOVE_NONE;
    member->moveSc[BATTLE_MEMBER_RESERVED_MOVE_SLOT] = 0;
}

int BattleMember_IsEquippableMoveSlot(int moveSlot)
{
    return moveSlot >= 0 && moveSlot < BATTLE_MEMBER_EQUIPPED_MOVE_SLOT_COUNT;
}

void BattleMember_EquipMove(BattleMember *member, int moveSlot, MoveId moveId, int startingSc)
{
    const MoveData *move;
    if (!member || !BattleMember_IsEquippableMoveSlot(moveSlot))
    {
        return;
    }
    member->equippedMoves[moveSlot] = moveId;
    move = MoveData_Get(moveId);
    if (startingSc < 0)
    {
        member->moveSc[moveSlot] = move ? move->maxSc : 0;
    }
    else
    {
        member->moveSc[moveSlot] = startingSc;
    }
    BattleMember_ClearReservedMoveSlot(member);
}

int BattleMember_CanUseMoveSlot(const BattleMember *member, int moveSlot)
{
    if (!member || !BattleMember_IsEquippableMoveSlot(moveSlot))
    {
        return 0;
    }
    if (member->equippedMoves[moveSlot] == MOVE_NONE)
    {
        return 0;
    }
    return member->moveSc[moveSlot] > 0;
}

int BattleMember_SpendMoveSc(BattleMember *member, int moveSlot, int amount)
{
    if (!member || !BattleMember_IsEquippableMoveSlot(moveSlot) || amount <= 0)
    {
        return 0;
    }
    if (member->moveSc[moveSlot] < amount)
    {
        return 0;
    }
    member->moveSc[moveSlot] -= amount;
    if (member->moveSc[moveSlot] < 0)
    {
        member->moveSc[moveSlot] = 0;
    }
    return 1;
}

void BattleMember_MaxAllMoveSc(BattleMember *member)
{
    int slot;

    if (!member)
    {
        return;
    }
    for (slot = 0; slot < BATTLE_MEMBER_EQUIPPED_MOVE_SLOT_COUNT; slot++)
    {
        const MoveData *move;

        if (member->equippedMoves[slot] == MOVE_NONE)
        {
            continue;
        }
        move = MoveData_Get(member->equippedMoves[slot]);
        if (!move || move->maxSc <= 0)
        {
            continue;
        }
        member->moveSc[slot] = move->maxSc;
    }
    BattleMember_ClearReservedMoveSlot(member);
}

int BattleMember_SpendLp(BattleMember *member, int amount)
{
    if (!member || amount <= 0)
    {
        return 0;
    }
    if (member->currentLp < amount)
    {
        return 0;
    }
    member->currentLp -= amount;
    if (member->currentLp < 0)
    {
        member->currentLp = 0;
    }
    return 1;
}

int BattleMember_GainLp(BattleMember *member, int amount)
{
    int before;
    int gained;

    if (!member || amount <= 0 || member->maxLp <= 0)
    {
        return 0;
    }
    before = member->currentLp;
    member->currentLp += amount;
    if (member->currentLp > member->maxLp)
    {
        member->currentLp = member->maxLp;
    }
    gained = member->currentLp - before;
    return gained;
}

void BattleMember_BraceRecoverAllMoves(BattleMember *member, int amount)
{
    int slot;

    if (!member || amount <= 0)
    {
        return;
    }
    for (slot = 0; slot < BATTLE_MEMBER_EQUIPPED_MOVE_SLOT_COUNT; slot++)
    {
        const MoveData *move;
        int maxSc;
        int newSc;

        if (member->equippedMoves[slot] == MOVE_NONE)
        {
            continue;
        }
        move = MoveData_Get(member->equippedMoves[slot]);
        if (!move || move->maxSc <= 0)
        {
            continue;
        }
        maxSc = move->maxSc;
        newSc = member->moveSc[slot] + amount;
        if (newSc > maxSc)
        {
            newSc = maxSc;
        }
        member->moveSc[slot] = newSc;
    }
    BattleMember_ClearReservedMoveSlot(member);
}

void BattleMember_TakeDamage(BattleMember *member, int amount)
{
    if (!member || amount <= 0)
    {
        return;
    }
    if (member->removedFromField || !member->isAlive)
    {
        return;
    }
    member->currentHp -= amount;
    if (member->currentHp <= 0)
    {
        member->currentHp = 0;
        member->isAlive = 0;
        BattleMember_StartKoBurrow(member);
    }
}

int BattleMember_IsRemovedFromField(const BattleMember *member)
{
    return member && member->removedFromField;
}

int BattleMember_IsKoBurrowing(const BattleMember *member)
{
    return member && member->koBurrowFramesRemaining > 0;
}

/* Non-linear sink: fast fwip into the ground (not 1px/frame). */
static const signed char s_koBurrowYOffsets[BATTLE_MEMBER_KO_BURROW_FRAMES] = {
    0, 3, 7, 12, 18, 26
};

void BattleMember_StartKoBurrow(BattleMember *member)
{
    if (!member)
    {
        return;
    }
    /* Free the tile immediately; keep struct for bookkeeping. */
    member->removedFromField = 1;
    member->isAlive = 0;
    if (member->currentHp > 0)
    {
        member->currentHp = 0;
    }
    member->koBurrowFramesRemaining = (unsigned char)BATTLE_MEMBER_KO_BURROW_FRAMES;
    member->koBurrowYOffset = s_koBurrowYOffsets[0];
}

int BattleMember_TickKoBurrow(BattleMember *member)
{
    unsigned char elapsed;

    if (!member || member->koBurrowFramesRemaining <= 0)
    {
        return 0;
    }
    member->koBurrowFramesRemaining--;
    if (member->koBurrowFramesRemaining > 0)
    {
        elapsed = (unsigned char)(
            BATTLE_MEMBER_KO_BURROW_FRAMES - member->koBurrowFramesRemaining);
        member->koBurrowYOffset = s_koBurrowYOffsets[elapsed];
    }
    return 1;
}

int BattleMember_Heal(BattleMember *member, int amount)
{
    int healed;

    if (!member || amount <= 0 || !member->isAlive || member->currentHp <= 0 ||
        member->removedFromField)
    {
        return 0;
    }
    if (member->currentHp >= member->maxHp)
    {
        return 0;
    }
    healed = amount;
    if (member->currentHp + healed > member->maxHp)
    {
        healed = member->maxHp - member->currentHp;
    }
    member->currentHp += healed;
    return healed;
}

static int BattleMember_ActionQualifiesForLastActionRecording(const BattleAction *action)
{
    const MoveData *move;

    if (!action)
    {
        return 0;
    }
    if (action->type == BATTLE_ACTION_ATTACK)
    {
        return 1;
    }
    if (action->type != BATTLE_ACTION_SKILL)
    {
        return 0;
    }
    move = MoveData_Get(action->moveId);
    return MoveData_QualifiesForEncoreReplay(move);
}

int BattleMember_HasQualifyingLastAction(const BattleMember *member)
{
    if (!member)
    {
        return 0;
    }
    if (member->lastResolvedActionType == BATTLE_ACTION_ATTACK)
    {
        return 1;
    }
    if (member->lastResolvedActionType != BATTLE_ACTION_SKILL)
    {
        return 0;
    }
    return MoveData_QualifiesForEncoreReplay(MoveData_Get(member->lastResolvedMoveId));
}

int BattleMember_LastActionSupportsEncoreReplayV1(const BattleMember *member)
{
    const MoveData *move;

    if (!member)
    {
        return 0;
    }
    if (member->lastResolvedActionType == BATTLE_ACTION_ATTACK)
    {
        return 1;
    }
    if (member->lastResolvedActionType != BATTLE_ACTION_SKILL)
    {
        return 0;
    }
    move = MoveData_Get(member->lastResolvedMoveId);
    return MoveData_QualifiesForEncoreReplay(move);
}

void BattleMember_RecordResolvedAction(BattleMember *member, const BattleAction *action)
{
    if (!member || !action || !BattleMember_ActionQualifiesForLastActionRecording(action))
    {
        return;
    }
    member->lastResolvedActionType = action->type;
    member->lastResolvedMoveId = action->moveId;
    member->lastResolvedMoveSlot = action->actorMoveSlot;
    member->lastResolvedTargetSide = action->targetSide;
    member->lastResolvedTargetSlot = action->targetSlot;
    member->lastResolvedUseLpBonus = action->useLpBonus;
}

int BattleMember_HasEncore(const BattleMember *member)
{
    return member && member->encoreTurnsRemaining > 0;
}

void BattleMember_ClearEncore(BattleMember *member)
{
    if (!member)
    {
        return;
    }
    member->encoreTurnsRemaining = 0;
    member->encoredActionType = BATTLE_ACTION_NONE;
    member->encoredMoveId = MOVE_NONE;
    member->encoredMoveSlot = 0;
    member->encoredTargetSide = 0;
    member->encoredTargetSlot = 0;
    member->encoredUseLpBonus = 0;
}

int BattleMember_ApplyEncoreFromLastAction(BattleMember *member, int durationTurns)
{
    if (!member || durationTurns <= 0 || !BattleMember_HasQualifyingLastAction(member))
    {
        return 0;
    }
    if (!BattleMember_LastActionSupportsEncoreReplayV1(member))
    {
        return 0;
    }
    member->encoreTurnsRemaining = (unsigned char)durationTurns;
    member->encoredActionType = member->lastResolvedActionType;
    member->encoredMoveId = member->lastResolvedMoveId;
    member->encoredMoveSlot = member->lastResolvedMoveSlot;
    member->encoredTargetSide = BATTLE_ENCORE_RETARGET;
    member->encoredTargetSlot = BATTLE_ENCORE_RETARGET;
    member->encoredUseLpBonus = member->lastResolvedUseLpBonus;
    return 1;
}

void BattleMember_TickEncoreAfterAction(BattleMember *member)
{
    if (!member || member->encoreTurnsRemaining <= 0)
    {
        return;
    }
    member->encoreTurnsRemaining--;
    if (member->encoreTurnsRemaining <= 0)
    {
        BattleMember_ClearEncore(member);
    }
}

static int BattleMember_TryResolveEncoreAttackTarget(
    const BattleMember *member,
    const BattleState *state,
    int *outTargetSide,
    int *outTargetSlot)
{
    int targetSlot;

    if (!member || !state || !outTargetSide || !outTargetSlot)
    {
        return 0;
    }

    if (member->encoredTargetSide == BATTLE_ENCORE_RETARGET ||
        member->encoredTargetSlot == BATTLE_ENCORE_RETARGET)
    {
        if (member->side == BATTLE_SIDE_ENEMY)
        {
            targetSlot = BattleState_FindEnemyTargetPlayerSlot(state);
        }
        else
        {
            targetSlot = -1;
        }
        if (targetSlot < 0)
        {
            return 0;
        }
        *outTargetSide = BATTLE_SIDE_PLAYER;
        *outTargetSlot = targetSlot;
        return 1;
    }

    if (!BattleState_IsAliveOccupiedSlot(
            state, member->encoredTargetSide, member->encoredTargetSlot))
    {
        return 0;
    }
    *outTargetSide = member->encoredTargetSide;
    *outTargetSlot = member->encoredTargetSlot;
    return 1;
}

static int BattleMember_TryResolveEncoreSkillTarget(
    const BattleMember *member,
    const BattleState *state,
    const MoveData *move,
    int *outTargetSide,
    int *outTargetSlot)
{
    int targetSlot;

    if (!member || !state || !move || !outTargetSide || !outTargetSlot)
    {
        return 0;
    }

    if (move->targetPattern == MOVE_TARGET_SELF)
    {
        if (!BattleState_IsAliveOccupiedSlot(state, member->side, member->slotIndex))
        {
            return 0;
        }
        *outTargetSide = member->side;
        *outTargetSlot = member->slotIndex;
        return 1;
    }

    if (member->encoredTargetSide == BATTLE_ENCORE_RETARGET ||
        member->encoredTargetSlot == BATTLE_ENCORE_RETARGET)
    {
        if (member->side == BATTLE_SIDE_ENEMY &&
            move->targetPattern == MOVE_TARGET_SINGLE_ENEMY)
        {
            targetSlot = BattleState_FindEnemyTargetPlayerSlot(state);
            if (targetSlot < 0)
            {
                return 0;
            }
            *outTargetSide = BATTLE_SIDE_PLAYER;
            *outTargetSlot = targetSlot;
            return 1;
        }
        return 0;
    }

    if (!BattleState_IsAliveOccupiedSlot(
            state, member->encoredTargetSide, member->encoredTargetSlot))
    {
        if (member->side == BATTLE_SIDE_ENEMY &&
            move->targetPattern == MOVE_TARGET_SINGLE_ENEMY)
        {
            targetSlot = BattleState_FindEnemyTargetPlayerSlot(state);
            if (targetSlot < 0)
            {
                return 0;
            }
            *outTargetSide = BATTLE_SIDE_PLAYER;
            *outTargetSlot = targetSlot;
            return 1;
        }
        return 0;
    }
    *outTargetSide = member->encoredTargetSide;
    *outTargetSlot = member->encoredTargetSlot;
    return 1;
}

int BattleMember_BuildEncoreForcedAction(
    const BattleMember *member,
    const BattleState *state,
    BattleAction *outAction)
{
    const MoveData *move;
    int targetSide;
    int targetSlot;

    if (!member || !state || !outAction || !BattleMember_HasEncore(member))
    {
        return 0;
    }

    BattleAction_Clear(outAction);
    outAction->actorSide = member->side;
    outAction->actorSlot = member->slotIndex;
    outAction->useLpBonus = 0;

    if (member->encoredActionType == BATTLE_ACTION_ATTACK)
    {
        outAction->type = BATTLE_ACTION_ATTACK;
        outAction->moveId = MOVE_NONE;
        if (!BattleMember_TryResolveEncoreAttackTarget(
                member, state, &targetSide, &targetSlot))
        {
            return 0;
        }
        outAction->targetSide = targetSide;
        outAction->targetSlot = targetSlot;
        return 1;
    }

    if (member->encoredActionType != BATTLE_ACTION_SKILL)
    {
        return 0;
    }

    move = MoveData_Get(member->encoredMoveId);
    if (!move || !MoveData_QualifiesForEncoreReplay(move))
    {
        return 0;
    }

    outAction->type = BATTLE_ACTION_SKILL;
    outAction->moveId = member->encoredMoveId;
    outAction->actorMoveSlot = member->encoredMoveSlot;
    if (!BattleMember_TryResolveEncoreSkillTarget(
            member, state, move, &targetSide, &targetSlot))
    {
        return 0;
    }
    outAction->targetSide = targetSide;
    outAction->targetSlot = targetSlot;
    return 1;
}
