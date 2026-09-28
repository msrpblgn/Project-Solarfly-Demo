/*
 * Host stubs for production Skills path integration (not a composition-only copy).
 * Hardware left untested: REG_KEYINPUT, REG_VCOUNT hardware, DMA, real scanout.
 */

#include "battle/battle.h"
#include "battle/battle_draw.h"
#include "battle/battle_state.h"
#include "battle_ui/battle_message_queue.h"
#include "core/input.h"
#include "core/gba_types.h"

static u16 s_hostJustPressed;
static u16 s_hostHeld;
static int s_messageBusy;
static int s_beginSkillTargetCalls;
static MoveId s_lastTargetMove;
static int s_lastTargetSlot;

void HostInput_SetKeys(u16 justPressed, u16 held)
{
    s_hostJustPressed = justPressed;
    s_hostHeld = held;
}

void HostInput_Clear(void)
{
    s_hostJustPressed = 0;
    s_hostHeld = 0;
}

void Input_Init(void)
{
    HostInput_Clear();
}

void Input_Update(void)
{
    /* Host injects edges via HostInput_SetKeys before BattleMenu_Update. */
}

int Input_IsPressed(u16 key)
{
    return (s_hostHeld & key) != 0;
}

int Input_JustPressed(u16 key)
{
    return (s_hostJustPressed & key) != 0;
}

int Battle_CanActivePlayerAct(const BattleState *state)
{
    (void)state;
    return 1;
}

void Battle_BeginSkillTargetSelect(BattleState *state, MoveId moveId, int moveSlot)
{
    s_beginSkillTargetCalls++;
    s_lastTargetMove = moveId;
    s_lastTargetSlot = moveSlot;
    if (state)
    {
        BattleState_SetPhase(state, BATTLE_PHASE_TARGET_SELECT);
        BattleDraw_MarkDirty(BATTLE_DRAW_DIRTY_FIELD | BATTLE_DRAW_DIRTY_PANEL);
    }
}

/* Minimal stubs for other battle.h symbols battle_menu may reference via command path. */
void Battle_BeginAttackTargetSelect(BattleState *state)
{
    (void)state;
}

void Battle_BeginBraceResolve(BattleState *state)
{
    (void)state;
}

void Battle_BeginPartyScreen(BattleState *state)
{
    (void)state;
}

void Battle_BeginItemMenu(BattleState *state)
{
    (void)state;
}

void Battle_BeginItemTargetSelect(BattleState *state, ItemId itemId)
{
    (void)state;
    (void)itemId;
}

void Battle_AttemptRun(BattleState *state)
{
    (void)state;
}

void Battle_BeginResolveAction(BattleState *state)
{
    (void)state;
}

void BattleMessageQueue_Init(void)
{
    s_messageBusy = 0;
}

void BattleMessageQueue_Enqueue(const char *message)
{
    (void)message;
}

void BattleMessageQueue_Draw(void)
{
}

int BattleMessageQueue_IsBusy(void)
{
    return s_messageBusy;
}

void HostMessage_SetBusy(int busy)
{
    s_messageBusy = busy ? 1 : 0;
}

int Host_BeginSkillTargetCallCount(void)
{
    return s_beginSkillTargetCalls;
}

MoveId Host_LastTargetMove(void)
{
    return s_lastTargetMove;
}

int Host_LastTargetSlot(void)
{
    return s_lastTargetSlot;
}

void Host_ResetTargetProbe(void)
{
    s_beginSkillTargetCalls = 0;
    s_lastTargetMove = MOVE_NONE;
    s_lastTargetSlot = -1;
}
