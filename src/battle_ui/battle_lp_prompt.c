#include "battle_ui/battle_lp_prompt.h"
#include "battle_ui/hud_layout.h"
#include "battle_ui/battle_message_queue.h"
#include "battle/battle.h"
#include "battle/battle_action.h"
#include "battle/battle_move.h"
#include "battle/battle_state.h"
#include "core/input.h"
#include "core/video.h"
#include "core/gba_types.h"

#define LP_PROMPT_YES 0
#define LP_PROMPT_NO 1

static void BattleLpPrompt_DrawOption(int x, int y, const char *label, int selected)
{
    u16 color = selected ? HUD_TEXT_HIGHLIGHT : HUD_TEXT_MUTED;

    Video_DrawText(x, y, label, color);
    if (selected)
    {
        Video_DrawRect(x - 4, y + 8, 32, 2, HUD_TEXT_HIGHLIGHT);
    }
}

void BattleLpPrompt_Init(void)
{
}

void BattleLpPrompt_Update(BattleState *state, BattleAction *action)
{
    if (BattleMessageQueue_IsBusy())
    {
        return;
    }
    if (Input_JustPressed(KEY_B))
    {
        BattleState_SetPhase(state, BATTLE_PHASE_TARGET_SELECT);
        return;
    }
    if (Input_JustPressed(KEY_DOWN) || Input_JustPressed(KEY_RIGHT))
    {
        state->lpPromptSelection = LP_PROMPT_NO;
    }
    if (Input_JustPressed(KEY_UP) || Input_JustPressed(KEY_LEFT))
    {
        state->lpPromptSelection = LP_PROMPT_YES;
    }
    if (Input_JustPressed(KEY_A))
    {
        if (state->lpPromptSelection == LP_PROMPT_YES)
        {
            action->useLpBonus = 1;
        }
        else
        {
            action->useLpBonus = 0;
        }
        Battle_BeginResolveAction(state);
    }
}

void BattleLpPrompt_Draw(const BattleState *state, const BattleAction *action)
{
    const MoveData *move;
    char promptLine[24];
    int cursor = 0;
    char costChar;

    if (state->phase != BATTLE_PHASE_LP_PROMPT)
    {
        return;
    }
    move = MoveData_Get(action->moveId);
    if (!move || move->optionalLpCost <= 0)
    {
        return;
    }

    Video_DrawText(8, HUD_PANEL_TITLE_Y, "LP UPGRADE", HUD_TEXT_TITLE);

    promptLine[cursor++] = 'S';
    promptLine[cursor++] = 'P';
    promptLine[cursor++] = 'E';
    promptLine[cursor++] = 'N';
    promptLine[cursor++] = 'D';
    promptLine[cursor++] = ' ';
    costChar = (char)('0' + move->optionalLpCost);
    promptLine[cursor++] = costChar;
    promptLine[cursor++] = ' ';
    promptLine[cursor++] = 'L';
    promptLine[cursor++] = 'P';
    promptLine[cursor++] = '?';
    promptLine[cursor] = '\0';
    Video_DrawText(8, HUD_PANEL_BODY_Y, promptLine, HUD_TEXT_LIGHT);

    BattleLpPrompt_DrawOption(24, HUD_PANEL_BODY2_Y, "YES", state->lpPromptSelection == LP_PROMPT_YES);
    BattleLpPrompt_DrawOption(128, HUD_PANEL_BODY2_Y, "NO", state->lpPromptSelection == LP_PROMPT_NO);
    Video_DrawText(8, HUD_PANEL_FOOTER_Y, "A OK  B BACK", HUD_TEXT_MUTED);
}
