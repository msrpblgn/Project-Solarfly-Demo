#include "battle/battle.h"
#include "battle/battle_state.h"
#include "battle/battle_action.h"
#include "battle/custom_battle_setup.h"
#include "battle/battle_member.h"
#include "battle/battle_draw.h"
#include "battle/battle_skills_panel.h"
#include "battle/target_selection.h"
#include "battle/battle_resolve.h"
#include "battle/battle_enemy_turn.h"
#include "battle/test_battle.h"
#include "battle_ui/battle_hud.h"
#include "battle_ui/enemy_status_bar.h"
#include "battle_ui/battle_menu.h"
#include "battle_ui/battle_message_queue.h"
#include "battle_ui/target_cursor.h"
#include "battle_ui/battle_lp_prompt.h"
#include "battle_ui/party_screen.h"
#include "battle_ui/damage_numbers.h"
#include "battle_feel/battle_feedback.h"
#include "battle_feel/battle_pacing.h"
#include "core/input.h"
#include "core/video.h"
#include "core/random.h"
#include "data/status_database.h"
#include "battle/battle_item.h"
#include "assets/bg_assets.h"
#include "assets/palettes.h"

static BattleState s_battleState;
static BattleAction s_pendingAction;

static int s_bgInitialized;
static int s_resolveExecuted;
static int s_resolveAdvancesTurn;
static int s_pendingVictory;
static int s_enemyTurnExecuted;
static int s_statusTicksApplied;
static int s_downedExpiryApplied;
static int s_statModTicksApplied;
static int s_pendingDefeat;
static int s_cmdSelectionOld = -1;
static int s_cmdSelectionNew = -1;
static BattleDemoId s_activeDemoId = BATTLE_DEMO_PETALBURG_SHOWDOWN;
static const char *s_introMessage = "Petalburg Showdown!";

#define RUN_MESSAGE_MAX 48
#define COMMAND_PROMPT_MAX 48
#define STATUS_TICK_MESSAGE_MAX 48
#define DOWNED_EXPIRY_MESSAGE_MAX 48

static char s_runMessage[RUN_MESSAGE_MAX];
static char s_commandPrompt[COMMAND_PROMPT_MAX];
static char s_statusTickMessage[STATUS_TICK_MESSAGE_MAX];
static char s_downedExpiryMessage[DOWNED_EXPIRY_MESSAGE_MAX];

static void Battle_DrawInitialBackground(void)
{
    if (s_activeDemoId == BATTLE_DEMO_PETALBURG_SHOWDOWN)
    {
        Video_Clear(RGB15(12, 20, 8));
        BgAssets_DrawPetalburgBattleBackground();
    }
    else
    {
        BgAssets_DrawPlaceholder(BG_ASSET_BATTLE_PLACEHOLDER);
    }
}

static void Battle_DrawFieldBackground(void)
{
    if (s_activeDemoId == BATTLE_DEMO_PETALBURG_SHOWDOWN)
    {
        BgAssets_DrawPetalburgBattleBackground();
    }
    else
    {
        BgAssets_DrawField(BG_ASSET_BATTLE_PLACEHOLDER);
    }
}

static void Battle_RestoreFieldRect(int x, int y, int w, int h)
{
    if (s_activeDemoId == BATTLE_DEMO_PETALBURG_SHOWDOWN)
    {
        BgAssets_DrawPetalburgBattleBackgroundRegion(x, y, w, h);
    }
    else
    {
        BgAssets_DrawFieldRegion(BG_ASSET_BATTLE_PLACEHOLDER, x, y, w, h);
    }
}

static void Battle_AppendChar(char *dest, int *cursor, char c)
{
    if (*cursor < RUN_MESSAGE_MAX - 1)
    {
        dest[*cursor] = c;
        (*cursor)++;
    }
}

static void Battle_AppendString(char *dest, int *cursor, const char *text)
{
    while (*text)
    {
        Battle_AppendChar(dest, cursor, *text);
        text++;
    }
}

static unsigned int Battle_GetPhaseEnterDirtyMask(BattlePhase phase)
{
    switch (phase)
    {
    case BATTLE_PHASE_TARGET_SELECT:
        return BATTLE_DRAW_DIRTY_FIELD | BATTLE_DRAW_DIRTY_PANEL;
    case BATTLE_PHASE_LP_PROMPT:
        return BATTLE_DRAW_DIRTY_PANEL;
    case BATTLE_PHASE_RESOLVE_ACTION:
        return BATTLE_DRAW_DIRTY_MESSAGE | BATTLE_DRAW_DIRTY_PANEL;
    case BATTLE_PHASE_RUN_FAILED:
        return BATTLE_DRAW_DIRTY_MESSAGE | BATTLE_DRAW_DIRTY_PANEL;
    case BATTLE_PHASE_ENEMY_TURN:
        return BATTLE_DRAW_DIRTY_MESSAGE | BATTLE_DRAW_DIRTY_PANEL;
    case BATTLE_PHASE_VICTORY:
        return BATTLE_DRAW_DIRTY_FIELD | BATTLE_DRAW_DIRTY_PANEL;
    case BATTLE_PHASE_DEFEAT:
        return BATTLE_DRAW_DIRTY_FIELD | BATTLE_DRAW_DIRTY_PANEL;
    case BATTLE_PHASE_ESCAPE:
        return BATTLE_DRAW_DIRTY_MESSAGE | BATTLE_DRAW_DIRTY_PANEL;
    case BATTLE_PHASE_PLAYER_COMMAND:
    case BATTLE_PHASE_PLAYER_SKILL:
    case BATTLE_PHASE_PLAYER_ITEM:
        return BATTLE_DRAW_DIRTY_PANEL;
    case BATTLE_PHASE_PARTY_SCREEN:
        return BATTLE_DRAW_DIRTY_PARTY;
    default:
        return BATTLE_DRAW_DIRTY_PANEL;
    }
}

static void Battle_HandleIntro(void);
static void Battle_HandleResolveAction(void);
static void Battle_HandleRunFailed(void);
static void Battle_HandleEnemyTurn(void);
static int Battle_ApplyTickDamageToMember(BattleMember *member);
static int Battle_ApplyEndOfRoundStatusTicks(BattleState *state);
static void Battle_ApplyEndOfRoundDownedExpiry(BattleState *state);
static void Battle_ApplyEndOfRoundStatModTicks(BattleState *state);

static void Battle_HandlePartyScreen(void)
{
    if (PartyScreen_Update(&s_battleState))
    {
        BattleState_SetPhase(&s_battleState, BATTLE_PHASE_PLAYER_COMMAND);
        BattleDraw_MarkDirty(
            BATTLE_DRAW_DIRTY_FIELD | BATTLE_DRAW_DIRTY_MESSAGE | BATTLE_DRAW_DIRTY_PANEL);
    }
}

int Battle_CanActivePlayerAct(const BattleState *state)
{
    int slot;

    if (!state)
    {
        return 0;
    }
    if (state->phase == BATTLE_PHASE_VICTORY || state->phase == BATTLE_PHASE_DEFEAT ||
        state->phase == BATTLE_PHASE_ESCAPE)
    {
        return 0;
    }
    slot = state->activePlayerSlot;
    if (!BattleState_IsSlotValid(BATTLE_SIDE_PLAYER, slot))
    {
        return 0;
    }
    if (!BattleState_IsAliveOccupiedSlot(state, BATTLE_SIDE_PLAYER, slot))
    {
        return 0;
    }
    if (BattleState_IsPlayerSlotActed(state, slot))
    {
        return 0;
    }
    return 1;
}

static void Battle_BeginNextRound(BattleState *state);
static void Battle_BeginNextPlayerTurnOrEnemyTurn(BattleState *state);

static int Battle_TryEnterTerminalOutcomePhase(BattleState *state)
{
    if (!state)
    {
        return 1;
    }
    if (BattleState_AllEnemiesDefeated(state))
    {
        BattleMessageQueue_EnqueuePriority("Victory!", BATTLE_MESSAGE_PRIORITY_HIGH);
        BattleState_SetPhase(state, BATTLE_PHASE_VICTORY);
        return 1;
    }
    if (BattleState_AllPlayersDefeated(state))
    {
        BattleMessageQueue_EnqueuePriority("Defeat!", BATTLE_MESSAGE_PRIORITY_HIGH);
        BattleState_SetPhase(state, BATTLE_PHASE_DEFEAT);
        return 1;
    }
    return 0;
}

static void Battle_BeginEnemyTurnPhase(BattleState *state)
{
    s_enemyTurnExecuted = 0;
    BattleEnemyTurn_Begin(state);
}

static void Battle_AdvanceToEnemyTurnOrDefeat(BattleState *state)
{
    if (Battle_TryEnterTerminalOutcomePhase(state))
    {
        return;
    }
    Battle_BeginEnemyTurnPhase(state);
}

static void Battle_BeginPlayerCommandPhase(BattleState *state, int slot)
{
    const BattleMember *member;
    int cursor = 0;

    if (!state || !BattleState_IsAliveOccupiedSlot(state, BATTLE_SIDE_PLAYER, slot) ||
        BattleState_IsPlayerSlotActed(state, slot))
    {
        Battle_BeginNextPlayerTurnOrEnemyTurn(state);
        return;
    }
    state->activePlayerSlot = slot;
    BattleMember_ClearGuardNextHit(&state->playerSlots[slot]);
    BattleMember_ClearBubbleProtection(&state->playerSlots[slot]);
    BattleState_SetPhase(state, BATTLE_PHASE_PLAYER_COMMAND);

    member = &state->playerSlots[slot];
    s_commandPrompt[0] = '\0';
    Battle_AppendString(s_commandPrompt, &cursor, "What will ");
    Battle_AppendString(s_commandPrompt, &cursor, member->name);
    Battle_AppendString(s_commandPrompt, &cursor, " do?");
    s_commandPrompt[cursor] = '\0';
    BattleMessageQueue_EnqueuePriority(s_commandPrompt, BATTLE_MESSAGE_PRIORITY_LOW);
}

static void Battle_BeginNextPlayerTurnOrEnemyTurn(BattleState *state)
{
    int slot;

    if (!state)
    {
        return;
    }
    if (Battle_TryEnterTerminalOutcomePhase(state))
    {
        return;
    }
    slot = BattleState_FindNextLivingUnactedPlayerSlot(state, state->activePlayerSlot);
    if (slot >= 0)
    {
        Battle_BeginPlayerCommandPhase(state, slot);
        return;
    }
    Battle_AdvanceToEnemyTurnOrDefeat(state);
}

static void Battle_AdvanceAfterPlayerTurn(BattleState *state)
{
    if (!state)
    {
        return;
    }
    state->isPlayerBonusCommand = 0;
    if (BattleState_IsSlotValid(BATTLE_SIDE_PLAYER, state->activePlayerSlot))
    {
        BattleState_MarkPlayerSlotActed(state, state->activePlayerSlot);
    }
    if (Battle_TryEnterTerminalOutcomePhase(state))
    {
        return;
    }
    Battle_BeginNextPlayerTurnOrEnemyTurn(state);
}

static void Battle_BeginFirstPlayerTurnOfRound(BattleState *state)
{
    int slot;

    if (!state)
    {
        return;
    }
    if (Battle_TryEnterTerminalOutcomePhase(state))
    {
        return;
    }
    slot = BattleState_FindProtagonistSlot(state);
    if (slot >= 0 && BattleState_IsAliveOccupiedSlot(state, BATTLE_SIDE_PLAYER, slot) &&
        !BattleState_IsPlayerSlotActed(state, slot))
    {
        Battle_BeginPlayerCommandPhase(state, slot);
        return;
    }
    slot = BattleState_FindNextLivingUnactedPlayerSlot(state, -1);
    if (slot >= 0)
    {
        Battle_BeginPlayerCommandPhase(state, slot);
        return;
    }
    Battle_AdvanceToEnemyTurnOrDefeat(state);
}

static void Battle_BeginNextRound(BattleState *state)
{
    BattleState_ClearPlayerTurnFlags(state);
    Battle_BeginFirstPlayerTurnOfRound(state);
}

static void Battle_HandleIntro(void)
{
    if (s_battleState.frameCounter == 0)
    {
        BattleMessageQueue_Enqueue(s_introMessage);
    }
    if (!BattleMessageQueue_IsBusy())
    {
        Battle_BeginFirstPlayerTurnOfRound(&s_battleState);
    }
}

static void Battle_HandlePlayerCommand(void)
{
    BattleMenu_Update(&s_battleState);
}

static void Battle_HandlePlayerSkill(void)
{
    BattleMenu_Update(&s_battleState);
}

static void Battle_HandlePlayerItem(void)
{
    BattleMenu_Update(&s_battleState);
}

static void Battle_HandleTargetSelect(void)
{
    TargetSelection_Update(&s_battleState, &s_pendingAction);
}

static void Battle_HandleLpPrompt(void)
{
    BattleLpPrompt_Update(&s_battleState, &s_pendingAction);
}

static void Battle_HandleResolveAction(void)
{
    if (!s_resolveExecuted)
    {
        BattleActionType actionType = s_pendingAction.type;

        s_resolveAdvancesTurn = BattleResolve_Execute(&s_battleState, &s_pendingAction);
        s_pendingVictory = BattleState_AllEnemiesDefeated(&s_battleState);
        s_resolveExecuted = 1;
        if (actionType == BATTLE_ACTION_ATTACK || actionType == BATTLE_ACTION_SKILL ||
            (actionType == BATTLE_ACTION_ITEM && s_resolveAdvancesTurn))
        {
            BattleDraw_MarkDirty(BATTLE_DRAW_DIRTY_FIELD);
        }
        return;
    }
    if (!BattleMessageQueue_IsBusy())
    {
        BattleActionType completedType = s_pendingAction.type;

        BattleAction_Clear(&s_pendingAction);
        s_resolveExecuted = 0;
        if (s_pendingVictory)
        {
            s_pendingVictory = 0;
            BattleMessageQueue_EnqueuePriority("Victory!", BATTLE_MESSAGE_PRIORITY_HIGH);
            BattleState_SetPhase(&s_battleState, BATTLE_PHASE_VICTORY);
        }
        else if (s_resolveAdvancesTurn)
        {
            if (s_battleState.bonusActionPlayerSlot >= 0)
            {
                int bonusSlot = s_battleState.bonusActionPlayerSlot;

                s_battleState.bonusActionPlayerSlot = BATTLE_BONUS_ACTION_SLOT_NONE;
                s_battleState.isPlayerBonusCommand = 1;
                Battle_BeginPlayerCommandPhase(&s_battleState, bonusSlot);
            }
            else
            {
                s_battleState.isPlayerBonusCommand = 0;
                Battle_AdvanceAfterPlayerTurn(&s_battleState);
            }
        }
        else if (completedType == BATTLE_ACTION_ITEM)
        {
            BattleState_SetPhase(&s_battleState, BATTLE_PHASE_PLAYER_ITEM);
        }
        else
        {
            Battle_AdvanceAfterPlayerTurn(&s_battleState);
        }
    }
}

static int Battle_ApplyTickDamageToMember(BattleMember *member)
{
    int i;
    int totalDamage = 0;
    int cursor = 0;
    BattleStatusId status;

    if (!member || !member->isAlive)
    {
        return 0;
    }
    for (i = 0; i < BATTLE_MEMBER_STATUS_SLOT_COUNT; i++)
    {
        status = BattleMember_GetStatusAt(member, i);
        if (status == BATTLE_STATUS_NONE)
        {
            continue;
        }
        if (status == BATTLE_STATUS_POISON)
        {
            totalDamage += StatusDatabase_GetTickDamage(BATTLE_STATUS_POISON) + member->poisonSeverity;
            member->poisonSeverity++;
        }
        else
        {
            totalDamage += StatusDatabase_GetTickDamage(status);
        }
    }
    if (totalDamage <= 0)
    {
        return 0;
    }
    BattleMember_TakeDamage(member, totalDamage);
    if (BattleMember_IsKoBurrowing(member) || BattleMember_IsRemovedFromField(member))
    {
        BattleDraw_MarkDirty(BATTLE_DRAW_DIRTY_FIELD);
    }
    s_statusTickMessage[0] = '\0';
    Battle_AppendString(s_statusTickMessage, &cursor, "Status hurt ");
    Battle_AppendString(s_statusTickMessage, &cursor, member->name);
    Battle_AppendChar(s_statusTickMessage, &cursor, '.');
    s_statusTickMessage[cursor] = '\0';
    BattleMessageQueue_Enqueue(s_statusTickMessage);
    return 1;
}

static int Battle_ApplyEndOfRoundStatusTicks(BattleState *state)
{
    int slot;
    int any = 0;

    if (!state)
    {
        return 0;
    }
    for (slot = 0; slot < BATTLE_PLAYER_SLOT_COUNT; slot++)
    {
        if (!BattleState_IsAliveOccupiedSlot(state, BATTLE_SIDE_PLAYER, slot))
        {
            continue;
        }
        if (Battle_ApplyTickDamageToMember(&state->playerSlots[slot]))
        {
            any = 1;
        }
    }
    for (slot = 0; slot < BATTLE_ENEMY_SLOT_COUNT; slot++)
    {
        if (!BattleState_IsAliveOccupiedSlot(state, BATTLE_SIDE_ENEMY, slot))
        {
            continue;
        }
        if (Battle_ApplyTickDamageToMember(&state->enemySlots[slot]))
        {
            any = 1;
        }
    }
    if (any)
    {
        BattleDraw_MarkDirty(BATTLE_DRAW_DIRTY_FIELD | BATTLE_DRAW_DIRTY_PANEL);
    }
    return any;
}

static void Battle_ApplyEndOfRoundDownedExpiry(BattleState *state)
{
    int slot;
    int cursor;
    int any = 0;

    if (!state)
    {
        return;
    }
    for (slot = 0; slot < BATTLE_PLAYER_SLOT_COUNT; slot++)
    {
        if (!BattleState_IsAliveOccupiedSlot(state, BATTLE_SIDE_PLAYER, slot))
        {
            continue;
        }
        if (BattleMember_TickDownedDuration(&state->playerSlots[slot]))
        {
            cursor = 0;
            s_downedExpiryMessage[0] = '\0';
            Battle_AppendString(s_downedExpiryMessage, &cursor, state->playerSlots[slot].name);
            Battle_AppendString(s_downedExpiryMessage, &cursor, " got back up!");
            s_downedExpiryMessage[cursor] = '\0';
            BattleMessageQueue_Enqueue(s_downedExpiryMessage);
            any = 1;
        }
    }
    for (slot = 0; slot < BATTLE_ENEMY_SLOT_COUNT; slot++)
    {
        if (!BattleState_IsAliveOccupiedSlot(state, BATTLE_SIDE_ENEMY, slot))
        {
            continue;
        }
        if (BattleMember_TickDownedDuration(&state->enemySlots[slot]))
        {
            cursor = 0;
            s_downedExpiryMessage[0] = '\0';
            Battle_AppendString(s_downedExpiryMessage, &cursor, state->enemySlots[slot].name);
            Battle_AppendString(s_downedExpiryMessage, &cursor, " got back up!");
            s_downedExpiryMessage[cursor] = '\0';
            BattleMessageQueue_Enqueue(s_downedExpiryMessage);
            any = 1;
        }
    }
    if (any)
    {
        BattleDraw_MarkDirty(BATTLE_DRAW_DIRTY_FIELD | BATTLE_DRAW_DIRTY_PANEL);
    }
}

static void Battle_ApplyEndOfRoundStatModTicks(BattleState *state)
{
    int slot;
    int any = 0;

    if (!state)
    {
        return;
    }
    for (slot = 0; slot < BATTLE_PLAYER_SLOT_COUNT; slot++)
    {
        if (!BattleState_IsAliveOccupiedSlot(state, BATTLE_SIDE_PLAYER, slot))
        {
            continue;
        }
        BattleMember_TickStatMods(&state->playerSlots[slot]);
        BattleMember_TickMovementLock(&state->playerSlots[slot]);
        BattleMember_TickIncomingDamageReduction(&state->playerSlots[slot]);
        any = 1;
    }
    for (slot = 0; slot < BATTLE_ENEMY_SLOT_COUNT; slot++)
    {
        if (!BattleState_IsAliveOccupiedSlot(state, BATTLE_SIDE_ENEMY, slot))
        {
            continue;
        }
        BattleMember_TickStatMods(&state->enemySlots[slot]);
        BattleMember_TickMovementLock(&state->enemySlots[slot]);
        BattleMember_TickIncomingDamageReduction(&state->enemySlots[slot]);
        any = 1;
    }
    BattleState_TickVeilTiles(state);
    any = 1;
    if (any)
    {
        BattleDraw_MarkDirty(BATTLE_DRAW_DIRTY_FIELD | BATTLE_DRAW_DIRTY_PANEL);
    }
}

static void Battle_HandleEnemyTurn(void)
{
    if (!s_enemyTurnExecuted)
    {
        s_pendingDefeat = BattleEnemyTurn_Execute(&s_battleState);
        s_enemyTurnExecuted = 1;
        BattleDraw_MarkDirty(BATTLE_DRAW_DIRTY_FIELD);
        return;
    }
    if (!BattleMessageQueue_IsBusy())
    {
        if (!s_pendingDefeat && !s_statusTicksApplied)
        {
            s_statusTicksApplied = 1;
            Battle_ApplyEndOfRoundStatusTicks(&s_battleState);
            if (BattleMessageQueue_IsBusy())
            {
                return;
            }
        }
        if (!s_pendingDefeat && !s_downedExpiryApplied)
        {
            s_downedExpiryApplied = 1;
            Battle_ApplyEndOfRoundDownedExpiry(&s_battleState);
            if (BattleMessageQueue_IsBusy())
            {
                return;
            }
        }
        if (!s_pendingDefeat && !s_statModTicksApplied)
        {
            s_statModTicksApplied = 1;
            Battle_ApplyEndOfRoundStatModTicks(&s_battleState);
        }
        s_enemyTurnExecuted = 0;
        s_statusTicksApplied = 0;
        s_downedExpiryApplied = 0;
        s_statModTicksApplied = 0;
        s_pendingDefeat = 0;
        if (Battle_TryEnterTerminalOutcomePhase(&s_battleState))
        {
            return;
        }
        Battle_BeginNextRound(&s_battleState);
    }
}

static void Battle_HandleRunFailed(void)
{
    if (!BattleMessageQueue_IsBusy())
    {
        Battle_AdvanceAfterPlayerTurn(&s_battleState);
    }
}

static void Battle_HandleVictory(void)
{
}

static void Battle_HandleDefeat(void)
{
}

static void Battle_HandleEscape(void)
{
}

void Battle_AttemptRun(BattleState *state)
{
    const BattleMember *runner;
    int chancePercent;
    int cursor = 0;

    if (!state || !Battle_CanActivePlayerAct(state))
    {
        return;
    }
    runner = &state->playerSlots[state->activePlayerSlot];
    chancePercent = BattleState_ComputeRunChancePercent(
        state, BATTLE_SIDE_PLAYER, state->activePlayerSlot);

    s_runMessage[0] = '\0';
    Battle_AppendString(s_runMessage, &cursor, runner->name);
    if (Random_Percent(chancePercent))
    {
        Battle_AppendString(s_runMessage, &cursor, " ran away.");
        s_runMessage[cursor] = '\0';
        BattleMessageQueue_EnqueuePriority(s_runMessage, BATTLE_MESSAGE_PRIORITY_HIGH);
        BattleState_SetPhase(state, BATTLE_PHASE_ESCAPE);
        return;
    }

    Battle_AppendString(s_runMessage, &cursor, " couldn't escape!");
    s_runMessage[cursor] = '\0';
    BattleMessageQueue_EnqueuePriority(s_runMessage, BATTLE_MESSAGE_PRIORITY_HIGH);
    BattleState_SetPhase(state, BATTLE_PHASE_RUN_FAILED);
}

void Battle_BeginResolveAction(BattleState *state)
{
    s_resolveExecuted = 0;
    BattleState_SetPhase(state, BATTLE_PHASE_RESOLVE_ACTION);
}

void Battle_BeginAttackTargetSelect(BattleState *state)
{
    if (!state || !Battle_CanActivePlayerAct(state))
    {
        return;
    }
    BattleAction_Clear(&s_pendingAction);
    s_pendingAction.type = BATTLE_ACTION_ATTACK;
    s_pendingAction.actorSlot = state->activePlayerSlot;
    s_pendingAction.actorSide = BATTLE_SIDE_PLAYER;

    state->targetSide = BATTLE_SIDE_ENEMY;
    state->targetSlot = TargetSelection_GetDefaultSlot(state, BATTLE_SIDE_ENEMY);
    BattleState_SetPhase(state, BATTLE_PHASE_TARGET_SELECT);
}

void Battle_BeginBraceResolve(BattleState *state)
{
    if (!state || !Battle_CanActivePlayerAct(state))
    {
        return;
    }
    BattleAction_Clear(&s_pendingAction);
    s_pendingAction.type = BATTLE_ACTION_BRACE;
    s_pendingAction.actorSlot = state->activePlayerSlot;
    s_pendingAction.actorSide = BATTLE_SIDE_PLAYER;
    Battle_BeginResolveAction(state);
}

void Battle_BeginSkillTargetSelect(BattleState *state, MoveId moveId, int moveSlot)
{
    const MoveData *move;
    int targetSide;
    int defaultSlot;

    if (!state || !Battle_CanActivePlayerAct(state))
    {
        return;
    }
    if (!BattleMember_IsEquippableMoveSlot(moveSlot))
    {
        return;
    }
    move = MoveData_Get(moveId);

    BattleAction_Clear(&s_pendingAction);
    s_pendingAction.type = BATTLE_ACTION_SKILL;
    s_pendingAction.actorSlot = state->activePlayerSlot;
    s_pendingAction.actorSide = BATTLE_SIDE_PLAYER;
    s_pendingAction.moveId = moveId;
    s_pendingAction.actorMoveSlot = moveSlot;
    targetSide = TargetSelection_GetTargetSideForMove(move, s_pendingAction.actorSide);

    if (move && move->targetPattern == TARGET_PATTERN_SELF)
    {
        state->targetSide = BATTLE_SIDE_PLAYER;
        state->targetSlot = state->activePlayerSlot;
        s_pendingAction.targetSide = BATTLE_SIDE_PLAYER;
        s_pendingAction.targetSlot = state->activePlayerSlot;
        TargetSelection_BeginSkillConfirm(state, &s_pendingAction);
        return;
    }

    if (move && (move->targetPattern == TARGET_PATTERN_ALL_ENEMIES ||
                 move->targetPattern == MOVE_TARGET_OUTER_ENEMIES))
    {
        state->targetSide = BATTLE_SIDE_ENEMY;
        state->targetSlot = TargetSelection_GetDefaultEnemySlot(state);
        s_pendingAction.targetSide = BATTLE_SIDE_ENEMY;
        s_pendingAction.targetSlot = state->targetSlot;
        TargetSelection_BeginSkillConfirm(state, &s_pendingAction);
        return;
    }

    if (move && move->targetPattern == TARGET_PATTERN_SINGLE_ALLY)
    {
        int allyTarget;

        state->targetSide = s_pendingAction.actorSide;
        if (move->id == MOVE_ALLY_SWAP)
        {
            allyTarget = TargetSelection_GetDefaultAllySwapSlot(
                state, s_pendingAction.actorSide, s_pendingAction.actorSlot);
            if (allyTarget < 0)
            {
                BattleMessageQueue_Enqueue("No ally to swap with.");
                return;
            }
            state->targetSlot = allyTarget;
        }
        else
        {
            state->targetSlot = TargetSelection_GetDefaultAllyTargetSlot(
                state, s_pendingAction.actorSide, s_pendingAction.actorSlot);
        }
        BattleState_SetPhase(state, BATTLE_PHASE_TARGET_SELECT);
        return;
    }

    if (move && move->targetPattern == TARGET_PATTERN_ADJACENT_ALLIES)
    {
        state->targetSide = s_pendingAction.actorSide;
        state->targetSlot = s_pendingAction.actorSlot;
        s_pendingAction.targetSide = s_pendingAction.actorSide;
        s_pendingAction.targetSlot = s_pendingAction.actorSlot;
        TargetSelection_BeginSkillConfirm(state, &s_pendingAction);
        return;
    }

    state->targetSide = targetSide;
    if (move && move->targetPattern == TARGET_PATTERN_ALLY_SLOT)
    {
        defaultSlot = TargetSelection_GetDefaultMoveSlot(state, state->activePlayerSlot);
        if (defaultSlot < 0)
        {
            /* Movement lock also yields no valid destination; report the real reason
             * instead of falsely claiming every ally slot is occupied. */
            const BattleMember *mover = BattleState_GetMemberAtSlotConst(
                state, BATTLE_SIDE_PLAYER, state->activePlayerSlot);

            if (mover && BattleMember_IsMovementLocked(mover))
            {
                BattleMessageQueue_Enqueue("Cannot move!");
            }
            else
            {
                BattleMessageQueue_Enqueue("No empty tile to move to.");
            }
            return;
        }
        state->targetSlot = defaultSlot;
    }
    else if (move && move->id == MOVE_FIERY_STRIKE)
    {
        state->targetSlot = TargetSelection_GetDefaultFieryStrikeSlot(
            state, s_pendingAction.actorSide, s_pendingAction.actorSlot);
    }
    else
    {
        state->targetSlot = TargetSelection_GetDefaultSlot(state, targetSide);
    }
    BattleState_SetPhase(state, BATTLE_PHASE_TARGET_SELECT);
}

void Battle_BeginItemMenu(BattleState *state)
{
    if (!state || !Battle_CanActivePlayerAct(state))
    {
        return;
    }
    if (BattleMessageQueue_IsBusy())
    {
        return;
    }
    if (!BattleState_HasAnyItems(state))
    {
        BattleMessageQueue_Enqueue("No items available.");
        return;
    }
    state->itemMenuSelection = 0;
    BattleState_SetPhase(state, BATTLE_PHASE_PLAYER_ITEM);
}

void Battle_BeginItemTargetSelect(BattleState *state, ItemId itemId)
{
    if (!state || !Battle_CanActivePlayerAct(state))
    {
        return;
    }
    if (BattleMessageQueue_IsBusy())
    {
        return;
    }
    if (itemId != ITEM_POTION && itemId != ITEM_CURE)
    {
        return;
    }
    if (BattleState_GetItemCount(state, itemId) <= 0)
    {
        if (itemId == ITEM_POTION)
        {
            BattleMessageQueue_Enqueue("No Potion left.");
        }
        else
        {
            BattleMessageQueue_Enqueue("No Cure left.");
        }
        return;
    }

    BattleAction_Clear(&s_pendingAction);
    s_pendingAction.type = BATTLE_ACTION_ITEM;
    s_pendingAction.actorSlot = state->activePlayerSlot;
    s_pendingAction.actorSide = BATTLE_SIDE_PLAYER;
    s_pendingAction.itemId = itemId;

    state->targetSide = BATTLE_SIDE_PLAYER;
    state->targetSlot = TargetSelection_GetDefaultItemTargetSlot(state);
    BattleState_SetPhase(state, BATTLE_PHASE_TARGET_SELECT);
}

int Battle_IsFinished(void)
{
    return s_battleState.phase == BATTLE_PHASE_VICTORY
        || s_battleState.phase == BATTLE_PHASE_DEFEAT
        || s_battleState.phase == BATTLE_PHASE_ESCAPE;
}

BattleDemoId Battle_GetActiveDemoId(void)
{
    return s_activeDemoId;
}

void Battle_BeginPartyScreen(BattleState *state)
{
    if (!state || !Battle_CanActivePlayerAct(state))
    {
        return;
    }
    if (BattleMessageQueue_IsBusy())
    {
        return;
    }
    PartyScreen_Open();
    BattleState_SetPhase(state, BATTLE_PHASE_PARTY_SCREEN);
}

void Battle_Init(void)
{
    Battle_InitWithDemo(BATTLE_DEMO_PETALBURG_SHOWDOWN);
}

void Battle_InitWithDemo(BattleDemoId demoId)
{
    Battle_InitWithDemoAndSettings(
        demoId,
        TUTSIL_DIFFICULTY_NORMAL,
        TEXT_SPEED_NORMAL,
        BORDER_COLOR_BLUE);
}

void Battle_InitWithDemoAndDifficulty(
    BattleDemoId demoId,
    TutsilDifficulty difficulty)
{
    Battle_InitWithDemoAndSettings(
        demoId,
        difficulty,
        TEXT_SPEED_NORMAL,
        BORDER_COLOR_BLUE);
}

void Battle_InitWithDemoAndSettings(
    BattleDemoId demoId,
    TutsilDifficulty difficulty,
    TextSpeed textSpeed,
    BorderColor borderColor)
{
    Random_Init(0x534F4C41u);
    BattleHud_Init();
    BattleHud_SetBorderColor(borderColor);
    BattleMenu_Init();
    PartyScreen_Init();
    BattleLpPrompt_Init();
    BattleMessageQueue_Init();
    BattleMessageQueue_SetTextSpeed(textSpeed);
    BattleEnemyTurn_Init();
    TargetCursor_Init();
    DamageNumbers_Init();
    BattleFeedback_Init();
    BattlePacing_Reset();
    s_activeDemoId = demoId;
    s_introMessage = TestBattle_GetIntroMessage(demoId);
    TestBattle_Setup(&s_battleState, demoId, difficulty);
    BattleAction_Clear(&s_pendingAction);
    s_resolveExecuted = 0;
    s_pendingVictory = 0;
    s_enemyTurnExecuted = 0;
    s_statusTicksApplied = 0;
    s_downedExpiryApplied = 0;
    s_statModTicksApplied = 0;
    s_pendingDefeat = 0;
    s_bgInitialized = 0;
    BattleDraw_MarkAllDirty();
}

void Battle_InitWithCustomFormationAndSettings(
    const CustomBattleFormation *formation,
    TextSpeed textSpeed,
    BorderColor borderColor)
{
    Random_Init(0x534F4C41u);
    BattleHud_Init();
    BattleHud_SetBorderColor(borderColor);
    BattleMenu_Init();
    PartyScreen_Init();
    BattleLpPrompt_Init();
    BattleMessageQueue_Init();
    BattleMessageQueue_SetTextSpeed(textSpeed);
    BattleEnemyTurn_Init();
    TargetCursor_Init();
    DamageNumbers_Init();
    BattleFeedback_Init();
    BattlePacing_Reset();
    s_activeDemoId = BATTLE_DEMO_CUSTOM_BATTLE;
    s_introMessage = CustomBattleSetup_GetIntroMessage();
    CustomBattleSetup_ApplyFormationToState(&s_battleState, formation);
    BattleAction_Clear(&s_pendingAction);
    s_resolveExecuted = 0;
    s_pendingVictory = 0;
    s_enemyTurnExecuted = 0;
    s_statusTicksApplied = 0;
    s_downedExpiryApplied = 0;
    s_statModTicksApplied = 0;
    s_pendingDefeat = 0;
    s_bgInitialized = 0;
    BattleDraw_MarkAllDirty();
}

void Battle_Update(void)
{
    BattlePhase phaseBefore = s_battleState.phase;
    int menuBefore = s_battleState.menuSelection;
    int skillMenuBefore = s_battleState.skillMenuSelection;
    int itemMenuBefore = s_battleState.itemMenuSelection;
    int targetSlotBefore = s_battleState.targetSlot;
    int lpPromptBefore = s_battleState.lpPromptSelection;
    unsigned int newDirty = 0;

    s_battleState.frameCounter++;

    BattleMessageQueue_Update();
    BattlePacing_Update();
    BattleFeedback_Update();
    DamageNumbers_Update();
    TargetCursor_Update(&s_battleState);
    if (BattleState_TickKoBurrowAnimations(&s_battleState))
    {
        /* Field sprites only — avoid forcing panel/message redraw each step. */
        newDirty |= BATTLE_DRAW_DIRTY_FIELD_ANIM;
    }

    switch (s_battleState.phase)
    {
    case BATTLE_PHASE_INIT:
        BattleState_SetPhase(&s_battleState, BATTLE_PHASE_INTRO_MESSAGE);
        break;
    case BATTLE_PHASE_INTRO_MESSAGE:
        Battle_HandleIntro();
        break;
    case BATTLE_PHASE_PLAYER_COMMAND:
        Battle_HandlePlayerCommand();
        break;
    case BATTLE_PHASE_PARTY_SCREEN:
        Battle_HandlePartyScreen();
        break;
    case BATTLE_PHASE_PLAYER_SKILL:
        Battle_HandlePlayerSkill();
        break;
    case BATTLE_PHASE_PLAYER_ITEM:
        Battle_HandlePlayerItem();
        break;
    case BATTLE_PHASE_TARGET_SELECT:
        Battle_HandleTargetSelect();
        break;
    case BATTLE_PHASE_LP_PROMPT:
        Battle_HandleLpPrompt();
        break;
    case BATTLE_PHASE_RESOLVE_ACTION:
        Battle_HandleResolveAction();
        break;
    case BATTLE_PHASE_RUN_FAILED:
        Battle_HandleRunFailed();
        break;
    case BATTLE_PHASE_ENEMY_TURN:
        Battle_HandleEnemyTurn();
        break;
    case BATTLE_PHASE_VICTORY:
        Battle_HandleVictory();
        break;
    case BATTLE_PHASE_DEFEAT:
        Battle_HandleDefeat();
        break;
    case BATTLE_PHASE_ESCAPE:
        Battle_HandleEscape();
        break;
    default:
        break;
    }

    if (s_battleState.phase == BATTLE_PHASE_RESOLVE_ACTION && !s_resolveExecuted)
    {
        Battle_HandleResolveAction();
    }

    if (s_battleState.phase != phaseBefore)
    {
        newDirty |= Battle_GetPhaseEnterDirtyMask(s_battleState.phase);
        if (phaseBefore == BATTLE_PHASE_PLAYER_SKILL &&
            s_battleState.phase != BATTLE_PHASE_PLAYER_SKILL)
        {
            /* Restore field pixels under the expanded Skills strip (y=108..111). */
            newDirty |= BATTLE_DRAW_DIRTY_FIELD;
            BattleMenu_ClearSkillCardSlide();
        }
        if (s_battleState.phase == BATTLE_PHASE_PLAYER_SKILL &&
            phaseBefore != BATTLE_PHASE_PLAYER_SKILL)
        {
            BattleMenu_ClearSkillCardSlide();
        }
    }
    if (s_battleState.menuSelection != menuBefore ||
        s_battleState.skillMenuSelection != skillMenuBefore ||
        s_battleState.itemMenuSelection != itemMenuBefore ||
        s_battleState.lpPromptSelection != lpPromptBefore)
    {
        if (s_battleState.menuSelection != menuBefore &&
            s_battleState.phase == BATTLE_PHASE_PLAYER_COMMAND &&
            phaseBefore == BATTLE_PHASE_PLAYER_COMMAND &&
            s_battleState.skillMenuSelection == skillMenuBefore &&
            s_battleState.itemMenuSelection == itemMenuBefore &&
            s_battleState.lpPromptSelection == lpPromptBefore)
        {
            s_cmdSelectionOld = menuBefore;
            s_cmdSelectionNew = s_battleState.menuSelection;
            newDirty |= BATTLE_DRAW_DIRTY_PANEL_CMD;
        }
        else
        {
            newDirty |= BATTLE_DRAW_DIRTY_PANEL;
        }
    }
    if (s_battleState.phase == BATTLE_PHASE_PARTY_SCREEN &&
        phaseBefore != BATTLE_PHASE_PARTY_SCREEN)
    {
        newDirty &= ~(BATTLE_DRAW_DIRTY_PANEL | BATTLE_DRAW_DIRTY_PANEL_CMD);
        newDirty |= BATTLE_DRAW_DIRTY_PARTY;
    }
    if (s_battleState.targetSlot != targetSlotBefore)
    {
        newDirty |= BATTLE_DRAW_DIRTY_FIELD;
    }
    if (BattleMessageQueue_TakeDisplayChange())
    {
        newDirty |= BATTLE_DRAW_DIRTY_MESSAGE;
    }

    BattleDraw_MarkDirty(newDirty);
}

void Battle_Draw(void)
{
    unsigned int dirty;

    dirty = BattleDraw_ConsumeDirty();
    if (!dirty)
    {
        return;
    }

    if (!s_bgInitialized)
    {
        Battle_DrawInitialBackground();
        s_bgInitialized = 1;
        dirty = BATTLE_DRAW_DIRTY_ALL;
    }

    /* Full field dirty may overlap message chrome; FIELD_ANIM must not. */
    if ((dirty & BATTLE_DRAW_DIRTY_FIELD) && BattleMessageQueue_IsBusy())
    {
        dirty |= BATTLE_DRAW_DIRTY_PANEL;
    }

    if (s_battleState.phase == BATTLE_PHASE_PARTY_SCREEN)
    {
        if (dirty & BATTLE_DRAW_DIRTY_PARTY)
        {
            PartyScreen_Draw(&s_battleState);
        }
        return;
    }

    if (dirty & BATTLE_DRAW_DIRTY_FIELD)
    {
        Battle_DrawFieldBackground();
        BattleHud_DrawCombatants(&s_battleState, &s_pendingAction);
        EnemyStatusBar_DrawAll(&s_battleState);
        TargetCursor_DrawSprite(&s_battleState);
        DamageNumbers_Draw();
        BattleFeedback_Draw();
        BattleState_ClearKoBurrowDirtyMasks(&s_battleState);
    }
    else if (dirty & BATTLE_DRAW_DIRTY_FIELD_ANIM)
    {
        /* Emerald-style: leave BG/panel/message stable; only patch KO rectangles. */
        BattleHud_DrawKoBurrowDirtyRects(
            &s_battleState,
            &s_pendingAction,
            Battle_RestoreFieldRect);
        BattleState_ClearKoBurrowDirtyMasks(&s_battleState);
    }

    if (dirty & BATTLE_DRAW_DIRTY_MESSAGE)
    {
        dirty |= BATTLE_DRAW_DIRTY_PANEL;
    }

    if (dirty & BATTLE_DRAW_DIRTY_PANEL_CMD)
    {
        if (!BattleMessageQueue_IsBusy())
        {
            BattleMenu_DrawCommandSelectionChange(s_cmdSelectionOld, s_cmdSelectionNew);
        }
    }

    if (dirty & BATTLE_DRAW_DIRTY_PANEL)
    {
        if (s_battleState.phase == BATTLE_PHASE_PLAYER_SKILL)
        {
            Battle_DrawPlayerSkillPanel(&s_battleState);
            if (!BattleMessageQueue_IsBusy() && !BattleMenu_IsSkillCardSlideActive())
            {
                TargetCursor_DrawPanel(&s_battleState);
                BattleLpPrompt_Draw(&s_battleState, &s_pendingAction);
            }
        }
        else
        {
            BattleHud_DrawPanelChrome(&s_battleState);
            if (BattleMessageQueue_IsBusy())
            {
                BattleMessageQueue_Draw();
            }
            else
            {
                BattleMenu_Draw(&s_battleState);
                TargetCursor_DrawPanel(&s_battleState);
                BattleLpPrompt_Draw(&s_battleState, &s_pendingAction);
            }
        }
    }
}
