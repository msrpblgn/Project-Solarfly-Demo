#include "battle/custom_battle_setup.h"
#include "battle/battle_member.h"
#include "battle/stat_curve.h"
#include "data/member_database.h"

typedef struct CustomBattleSavedFormationSlot {
    int occupied;
    CustomBattleFormation formation;
} CustomBattleSavedFormationSlot;

static CustomBattleFormation s_currentFormation;
static CustomBattleSavedFormationSlot s_savedFormations[CUSTOM_BATTLE_SAVE_SLOT_COUNT];

static void CustomBattleSetup_ApplyDefaultAllyMoves(
    CustomBattleFormationSlot *slot)
{
    int moveSlot;

    if (!slot)
    {
        return;
    }
    for (moveSlot = 0; moveSlot < BATTLE_MEMBER_MOVE_SLOT_COUNT; moveSlot++)
    {
        slot->moveIds[moveSlot] = MOVE_NONE;
    }
    switch (slot->memberTemplateId)
    {
    case MEMBER_TEMPLATE_PROTAGONIST:
        slot->moveIds[0] = MOVE_HOLD_BACK;
        slot->moveIds[1] = MOVE_HAMMER_FIST;
        slot->moveIds[2] = MOVE_INFLUENCE;
        break;
    case MEMBER_TEMPLATE_AKSIL:
        slot->moveIds[0] = MOVE_VORTEX_ESCAPE;
        slot->moveIds[1] = MOVE_FAKIE;
        slot->moveIds[2] = MOVE_ALLY_SWAP;
        break;
    default:
        break;
    }
}

static void CustomBattleSetup_ApplyDefaultEnemyMoves(
    CustomBattleFormationSlot *slot)
{
    int moveSlot;

    if (!slot)
    {
        return;
    }
    for (moveSlot = 0; moveSlot < BATTLE_MEMBER_MOVE_SLOT_COUNT; moveSlot++)
    {
        slot->moveIds[moveSlot] = MOVE_NONE;
    }
    switch (slot->memberTemplateId)
    {
    case MEMBER_TEMPLATE_TUTSIL:
        slot->moveIds[0] = MOVE_VORTEX_ESCAPE;
        slot->moveIds[1] = MOVE_YOUNG_BLOOD;
        slot->moveIds[2] = MOVE_LONG_SHOT;
        break;
    case MEMBER_TEMPLATE_ELECTRIC_ARMOR_BADGER:
        slot->moveIds[0] = MOVE_HEADBUTT;
        slot->moveIds[1] = MOVE_CHARGE;
        slot->moveIds[2] = MOVE_STATIC_LOCK;
        break;
    default:
        break;
    }
}

static int CustomBattleSetup_FindFirstOccupiedFormationSlot(
    const CustomBattleFormationSlot *slots,
    int slotCount,
    int fallbackSlot)
{
    int slot;

    if (!slots || slotCount <= 0)
    {
        return fallbackSlot;
    }
    for (slot = 0; slot < slotCount; slot++)
    {
        if (slots[slot].occupied)
        {
            return slot;
        }
    }
    return fallbackSlot;
}

static void CustomBattleSetup_ClearSlot(CustomBattleFormationSlot *slot)
{
    int moveSlot;

    if (!slot)
    {
        return;
    }
    slot->occupied = 0;
    slot->memberTemplateId = MEMBER_TEMPLATE_NONE;
    slot->level = 1;
    slot->flags = 0;
    for (moveSlot = 0; moveSlot < BATTLE_MEMBER_MOVE_SLOT_COUNT; moveSlot++)
    {
        slot->moveIds[moveSlot] = MOVE_NONE;
    }
}

static int CustomBattleSetup_CountOccupiedSlots(
    const CustomBattleFormationSlot *slots,
    int slotCount)
{
    int slot;
    int count = 0;

    if (!slots || slotCount <= 0)
    {
        return 0;
    }
    for (slot = 0; slot < slotCount; slot++)
    {
        if (slots[slot].occupied)
        {
            count++;
        }
    }
    return count;
}

static void CustomBattleSetup_InitFormationSide(
    CustomBattleFormationSlot *slots,
    int slotCount)
{
    int slot;
    int moveSlot;

    if (!slots || slotCount <= 0)
    {
        return;
    }
    for (slot = 0; slot < slotCount; slot++)
    {
        slots[slot].occupied = 0;
        slots[slot].memberTemplateId = MEMBER_TEMPLATE_NONE;
        slots[slot].level = 1;
        slots[slot].flags = 0;
        for (moveSlot = 0; moveSlot < BATTLE_MEMBER_MOVE_SLOT_COUNT; moveSlot++)
        {
            slots[slot].moveIds[moveSlot] = MOVE_NONE;
        }
    }
}

static void CustomBattleSetup_ApplyFormationSide(
    BattleMember *members,
    const CustomBattleFormationSlot *slots,
    int slotCount,
    int side)
{
    int slot;

    if (!members || !slots || slotCount <= 0)
    {
        return;
    }
    for (slot = 0; slot < slotCount; slot++)
    {
        const MemberTemplate *template;
        MemberTemplate placedTemplate;
        int moveSlot;

        if (!slots[slot].occupied || slots[slot].memberTemplateId == MEMBER_TEMPLATE_NONE)
        {
            continue;
        }
        template = MemberDatabase_Get(slots[slot].memberTemplateId);
        if (!template)
        {
            continue;
        }
        placedTemplate = *template;
        placedTemplate.startingLevel = slots[slot].level;
        BattleMember_InitFromTemplate(&members[slot], &placedTemplate, slot, side);
        for (moveSlot = 0; moveSlot < BATTLE_MEMBER_EQUIPPED_MOVE_SLOT_COUNT; moveSlot++)
        {
            if (slots[slot].moveIds[moveSlot] != MOVE_NONE)
            {
                BattleMember_EquipMove(
                    &members[slot],
                    moveSlot,
                    slots[slot].moveIds[moveSlot],
                    -1);
            }
        }
        BattleMember_ClearReservedMoveSlot(&members[slot]);
    }
}

int CustomBattleSetup_IsEnemySlotOccupied(int slotIndex)
{
    if (slotIndex < 0 || slotIndex >= BATTLE_ENEMY_SLOT_COUNT)
    {
        return 0;
    }
    return s_currentFormation.enemySlots[slotIndex].occupied;
}

int CustomBattleSetup_IsAllySlotOccupied(int slotIndex)
{
    if (slotIndex < 0 || slotIndex >= BATTLE_PLAYER_SLOT_COUNT)
    {
        return 0;
    }
    return s_currentFormation.allySlots[slotIndex].occupied;
}

const CustomBattleFormationSlot *CustomBattleSetup_GetAllySlot(int slotIndex)
{
    if (slotIndex < 0 || slotIndex >= BATTLE_PLAYER_SLOT_COUNT)
    {
        return 0;
    }
    return &s_currentFormation.allySlots[slotIndex];
}

int CustomBattleSetup_SetAllySlot(
    int slotIndex,
    MemberTemplateId memberTemplateId,
    int level)
{
    CustomBattleFormationSlot *slot;

    if (slotIndex < 0 || slotIndex >= BATTLE_PLAYER_SLOT_COUNT)
    {
        return 0;
    }
    if (!MemberDatabase_Get(memberTemplateId))
    {
        return 0;
    }
    if (level < BATTLE_MEMBER_LEVEL_MIN)
    {
        level = BATTLE_MEMBER_LEVEL_MIN;
    }
    if (level > STAT_CURVE_NATURAL_LEVEL_MAX)
    {
        level = STAT_CURVE_NATURAL_LEVEL_MAX;
    }
    slot = &s_currentFormation.allySlots[slotIndex];
    slot->occupied = 1;
    slot->memberTemplateId = memberTemplateId;
    slot->level = level;
    slot->flags = 0;
    CustomBattleSetup_ApplyDefaultAllyMoves(slot);
    return 1;
}

int CustomBattleSetup_SetAllySlotWithMoves(
    int slotIndex,
    MemberTemplateId memberTemplateId,
    int level,
    const MoveId *moveIds)
{
    CustomBattleFormationSlot *slot;
    int moveSlot;

    if (slotIndex < 0 || slotIndex >= BATTLE_PLAYER_SLOT_COUNT)
    {
        return 0;
    }
    if (!MemberDatabase_Get(memberTemplateId))
    {
        return 0;
    }
    if (level < BATTLE_MEMBER_LEVEL_MIN)
    {
        level = BATTLE_MEMBER_LEVEL_MIN;
    }
    if (level > STAT_CURVE_NATURAL_LEVEL_MAX)
    {
        level = STAT_CURVE_NATURAL_LEVEL_MAX;
    }
    slot = &s_currentFormation.allySlots[slotIndex];
    slot->occupied = 1;
    slot->memberTemplateId = memberTemplateId;
    slot->level = level;
    slot->flags = 0;
    for (moveSlot = 0; moveSlot < BATTLE_MEMBER_MOVE_SLOT_COUNT; moveSlot++)
    {
        slot->moveIds[moveSlot] = MOVE_NONE;
    }
    for (moveSlot = 0; moveSlot < BATTLE_MEMBER_EQUIPPED_MOVE_SLOT_COUNT; moveSlot++)
    {
        if (moveIds && moveIds[moveSlot] != MOVE_NONE && MoveData_Get(moveIds[moveSlot]))
        {
            slot->moveIds[moveSlot] = moveIds[moveSlot];
        }
    }
    return 1;
}

void CustomBattleSetup_ClearAllySlot(int slotIndex)
{
    if (slotIndex < 0 || slotIndex >= BATTLE_PLAYER_SLOT_COUNT)
    {
        return;
    }
    CustomBattleSetup_ClearSlot(&s_currentFormation.allySlots[slotIndex]);
}

const CustomBattleFormationSlot *CustomBattleSetup_GetEnemySlot(int slotIndex)
{
    if (slotIndex < 0 || slotIndex >= BATTLE_ENEMY_SLOT_COUNT)
    {
        return 0;
    }
    return &s_currentFormation.enemySlots[slotIndex];
}

int CustomBattleSetup_SetEnemySlot(
    int slotIndex,
    MemberTemplateId memberTemplateId,
    int level)
{
    CustomBattleFormationSlot *slot;

    if (slotIndex < 0 || slotIndex >= BATTLE_ENEMY_SLOT_COUNT)
    {
        return 0;
    }
    if (!MemberDatabase_Get(memberTemplateId))
    {
        return 0;
    }
    if (level < BATTLE_MEMBER_LEVEL_MIN)
    {
        level = BATTLE_MEMBER_LEVEL_MIN;
    }
    if (level > STAT_CURVE_NATURAL_LEVEL_MAX)
    {
        level = STAT_CURVE_NATURAL_LEVEL_MAX;
    }
    slot = &s_currentFormation.enemySlots[slotIndex];
    slot->occupied = 1;
    slot->memberTemplateId = memberTemplateId;
    slot->level = level;
    slot->flags = 0;
    CustomBattleSetup_ApplyDefaultEnemyMoves(slot);
    return 1;
}

void CustomBattleSetup_ClearEnemySlot(int slotIndex)
{
    if (slotIndex < 0 || slotIndex >= BATTLE_ENEMY_SLOT_COUNT)
    {
        return;
    }
    CustomBattleSetup_ClearSlot(&s_currentFormation.enemySlots[slotIndex]);
}

int CustomBattleSetup_SaveCurrentToSlot(int saveSlot)
{
    if (saveSlot < 0 || saveSlot >= CUSTOM_BATTLE_SAVE_SLOT_COUNT)
    {
        return 0;
    }
    s_savedFormations[saveSlot].formation = s_currentFormation;
    s_savedFormations[saveSlot].occupied = 1;
    return 1;
}

int CustomBattleSetup_LoadSlotToCurrent(int saveSlot)
{
    if (saveSlot < 0 || saveSlot >= CUSTOM_BATTLE_SAVE_SLOT_COUNT)
    {
        return 0;
    }
    if (!s_savedFormations[saveSlot].occupied)
    {
        return 0;
    }
    s_currentFormation = s_savedFormations[saveSlot].formation;
    return 1;
}

int CustomBattleSetup_IsSaveSlotOccupied(int saveSlot)
{
    if (saveSlot < 0 || saveSlot >= CUSTOM_BATTLE_SAVE_SLOT_COUNT)
    {
        return 0;
    }
    return s_savedFormations[saveSlot].occupied;
}

void CustomBattleSetup_GetSaveSlotSummary(
    int saveSlot,
    int *outAllyCount,
    int *outEnemyCount)
{
    if (outAllyCount)
    {
        *outAllyCount = 0;
    }
    if (outEnemyCount)
    {
        *outEnemyCount = 0;
    }
    if (saveSlot < 0 || saveSlot >= CUSTOM_BATTLE_SAVE_SLOT_COUNT)
    {
        return;
    }
    if (!s_savedFormations[saveSlot].occupied)
    {
        return;
    }
    if (outAllyCount)
    {
        *outAllyCount = CustomBattleSetup_CountOccupiedSlots(
            s_savedFormations[saveSlot].formation.allySlots,
            BATTLE_PLAYER_SLOT_COUNT);
    }
    if (outEnemyCount)
    {
        *outEnemyCount = CustomBattleSetup_CountOccupiedSlots(
            s_savedFormations[saveSlot].formation.enemySlots,
            BATTLE_ENEMY_SLOT_COUNT);
    }
}

void CustomBattleSetup_InitFormation(CustomBattleFormation *formation)
{
    if (!formation)
    {
        return;
    }
    CustomBattleSetup_InitFormationSide(
        formation->allySlots,
        BATTLE_PLAYER_SLOT_COUNT);
    CustomBattleSetup_InitFormationSide(
        formation->enemySlots,
        BATTLE_ENEMY_SLOT_COUNT);
}

void CustomBattleSetup_ResetCurrentFormation(void)
{
    CustomBattleSetup_InitFormation(&s_currentFormation);
}

CustomBattleFormation *CustomBattleSetup_GetCurrentFormation(void)
{
    return &s_currentFormation;
}

const CustomBattleFormation *CustomBattleSetup_GetCurrentFormationConst(void)
{
    return &s_currentFormation;
}

void CustomBattleSetup_ApplyFormationToState(
    BattleState *state,
    const CustomBattleFormation *formation)
{
    if (!state || !formation)
    {
        return;
    }

    BattleState_Init(state);
    CustomBattleSetup_ApplyFormationSide(
        state->playerSlots,
        formation->allySlots,
        BATTLE_PLAYER_SLOT_COUNT,
        BATTLE_SIDE_PLAYER);
    CustomBattleSetup_ApplyFormationSide(
        state->enemySlots,
        formation->enemySlots,
        BATTLE_ENEMY_SLOT_COUNT,
        BATTLE_SIDE_ENEMY);

    state->activePlayerSlot = CustomBattleSetup_FindFirstOccupiedFormationSlot(
        formation->allySlots,
        BATTLE_PLAYER_SLOT_COUNT,
        BATTLE_CENTER_SLOT);
    state->activeEnemySlot = CustomBattleSetup_FindFirstOccupiedFormationSlot(
        formation->enemySlots,
        BATTLE_ENEMY_SLOT_COUNT,
        BATTLE_CENTER_SLOT);
    state->phase = BATTLE_PHASE_INTRO_MESSAGE;
    BattleState_SetItemCount(state, ITEM_POTION, 3);
    BattleState_SetItemCount(state, ITEM_CURE, 3);
}

const char *CustomBattleSetup_GetIntroMessage(void)
{
    return "Custom Battle!";
}
