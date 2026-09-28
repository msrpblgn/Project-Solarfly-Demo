#include "battle/battle_move.h"
#include "data/move_database.h"

const MoveData *MoveData_Get(MoveId id)
{
    int i;
    for (i = 0; i < g_MoveDatabaseCount; i++)
    {
        if (g_MoveDatabase[i].id == id)
        {
            return &g_MoveDatabase[i];
        }
    }
    return 0;
}

int MoveData_HasDistanceScale(const MoveData *move)
{
    int i;

    if (!move)
    {
        return 0;
    }
    for (i = 0; i < MOVE_MAX_EFFECTS; i++)
    {
        if (move->effects[i].kind == MOVE_EFFECT_KIND_DISTANCE_SCALE)
        {
            return 1;
        }
    }
    return 0;
}

int MoveData_GetDamagePower(const MoveData *move)
{
    int i;

    if (!move)
    {
        return 0;
    }
    for (i = 0; i < MOVE_MAX_EFFECTS; i++)
    {
        if (move->effects[i].kind == MOVE_EFFECT_KIND_DAMAGE && move->effects[i].paramA > 0)
        {
            return move->effects[i].paramA;
        }
    }
    if (move->defaultPower > 0)
    {
        return move->defaultPower;
    }
    return 0;
}

static int MoveData_HasDisqualifyingEffectKind(const MoveData *move)
{
    int i;

    if (!move)
    {
        return 0;
    }
    for (i = 0; i < MOVE_MAX_EFFECTS; i++)
    {
        if (move->effects[i].kind == MOVE_EFFECT_KIND_MOVE_TO_SLOT ||
            move->effects[i].kind == MOVE_EFFECT_KIND_BONUS_ACTION)
        {
            return 1;
        }
    }
    return 0;
}

static int MoveData_HasDamageEffect(const MoveData *move)
{
    int i;

    if (!move)
    {
        return 0;
    }
    for (i = 0; i < MOVE_MAX_EFFECTS; i++)
    {
        if (move->effects[i].kind == MOVE_EFFECT_KIND_DAMAGE)
        {
            return 1;
        }
    }
    return 0;
}

static int MoveData_IsEncoreReplayExcludedMove(MoveId moveId)
{
    return moveId == MOVE_NONE || moveId == MOVE_HOLD_BACK ||
           moveId == MOVE_VORTEX_ESCAPE || moveId == MOVE_TIDE ||
           moveId == MOVE_TIPPING_THE_SCALES || moveId == MOVE_INFLUENCE;
}

int MoveData_QualifiesForLastActionRecording(const MoveData *move)
{
    if (!move)
    {
        return 0;
    }
    if (MoveData_IsEncoreReplayExcludedMove(move->id))
    {
        return 0;
    }
    if (move->category != MOVE_CATEGORY_PHYSICAL && move->category != MOVE_CATEGORY_SPECIAL)
    {
        return 0;
    }
    if (!MoveData_HasDamageEffect(move))
    {
        return 0;
    }
    if (MoveData_HasDisqualifyingEffectKind(move))
    {
        return 0;
    }
    return 1;
}

int MoveData_QualifiesForEncoreReplay(const MoveData *move)
{
    if (!move)
    {
        return 0;
    }
    if (MoveData_QualifiesForLastActionRecording(move))
    {
        return 1;
    }
    if (MoveData_IsEncoreReplayExcludedMove(move->id))
    {
        return 0;
    }
    if (move->targetPattern != MOVE_TARGET_SELF)
    {
        return 0;
    }
    if (move->category != MOVE_CATEGORY_SUPPORT && move->category != MOVE_CATEGORY_STATUS)
    {
        return 0;
    }
    if (MoveData_HasDisqualifyingEffectKind(move))
    {
        return 0;
    }
    return 1;
}
