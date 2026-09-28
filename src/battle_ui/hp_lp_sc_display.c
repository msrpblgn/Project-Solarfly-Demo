#include "battle_ui/hp_lp_sc_display.h"
#include "battle_ui/hud_layout.h"
#include "battle/battle_member.h"
#include "battle/battle_move.h"
#include "battle/battle_slot.h"
#include "core/video.h"
#include "core/gba_types.h"

static void HpLpScDisplay_WriteNumber(char *buffer, int value)
{
    int i = 0;
    int digits[8];
    int count = 0;
    int temp = value;
    if (temp == 0)
    {
        buffer[0] = '0';
        buffer[1] = '\0';
        return;
    }
    while (temp > 0 && count < 8)
    {
        digits[count++] = temp % 10;
        temp /= 10;
    }
    while (count > 0)
    {
        buffer[i++] = (char)('0' + digits[--count]);
    }
    buffer[i] = '\0';
}

void HpLpScDisplay_DrawMember(int x, int y, const BattleMember *member)
{
    char line[24];
    int cursor = 0;
    int i;
    char number[8];

    if (!member)
    {
        return;
    }
    BattleMember_FormatNameWithLevel(member, line, (int)sizeof(line));
    Video_DrawText(x, y, line, HUD_TEXT_LIGHT);

    cursor = 0;
    line[cursor++] = 'H';
    line[cursor++] = 'P';
    line[cursor++] = ':';
    HpLpScDisplay_WriteNumber(number, member->currentHp);
    for (i = 0; number[i] && cursor < 23; i++)
    {
        line[cursor++] = number[i];
    }
    line[cursor++] = '/';
    HpLpScDisplay_WriteNumber(number, member->maxHp);
    for (i = 0; number[i] && cursor < 23; i++)
    {
        line[cursor++] = number[i];
    }
    line[cursor] = '\0';
    Video_DrawText(x, y + 10, line, HUD_TEXT_LIGHT);

    if (member->side == BATTLE_SIDE_PLAYER)
    {
        cursor = 0;
        line[cursor++] = 'L';
        line[cursor++] = 'P';
        line[cursor++] = ':';
        HpLpScDisplay_WriteNumber(number, member->currentLp);
        for (i = 0; number[i] && cursor < 23; i++)
        {
            line[cursor++] = number[i];
        }
        line[cursor++] = '/';
        HpLpScDisplay_WriteNumber(number, member->maxLp);
        for (i = 0; number[i] && cursor < 23; i++)
        {
            line[cursor++] = number[i];
        }
        line[cursor] = '\0';
        Video_DrawText(x, y + 20, line, HUD_TEXT_LIGHT);
    }
}

void HpLpScDisplay_DrawCompactSlot(int x, int y, const BattleMember *member)
{
    HpLpScDisplay_DrawCompactSlotStyled(x, y, member, 0);
}

void HpLpScDisplay_DrawCompactSlotStyled(int x, int y, const BattleMember *member, int dimmed)
{
    char line[12];
    int cursor = 0;
    int i;
    char number[8];
    u16 textColor = HUD_TEXT_LIGHT;

    if (!member)
    {
        return;
    }
    if (dimmed || !member->isAlive)
    {
        textColor = HUD_TEXT_MUTED;
    }

    line[cursor++] = 'H';
    line[cursor++] = 'P';
    HpLpScDisplay_WriteNumber(number, member->currentHp);
    for (i = 0; number[i] && cursor < 11; i++)
    {
        line[cursor++] = number[i];
    }
    line[cursor] = '\0';
    Video_DrawText(x, y, line, textColor);
}

static int HpLpScDisplay_GetMaxEquippedSc(const BattleMember *member, int *outHasMove)
{
    int slot;
    int maxSc = 0;

    *outHasMove = 0;
    if (!member)
    {
        return 0;
    }
    for (slot = 0; slot < BATTLE_MEMBER_MOVE_SLOT_COUNT; slot++)
    {
        if (member->equippedMoves[slot] != MOVE_NONE)
        {
            *outHasMove = 1;
            if (member->moveSc[slot] > maxSc)
            {
                maxSc = member->moveSc[slot];
            }
        }
    }
    return maxSc;
}

void HpLpScDisplay_DrawActivePanelTitleStats(int x, int y, const BattleMember *member)
{
    char line[28];
    int cursor = 0;
    int i;
    char number[8];
    int hasMove;
    int maxSc;

    if (!member)
    {
        return;
    }

    line[cursor++] = 'H';
    line[cursor++] = 'P';
    HpLpScDisplay_WriteNumber(number, member->currentHp);
    for (i = 0; number[i] && cursor < 27; i++)
    {
        line[cursor++] = number[i];
    }
    line[cursor++] = '/';
    HpLpScDisplay_WriteNumber(number, member->maxHp);
    for (i = 0; number[i] && cursor < 27; i++)
    {
        line[cursor++] = number[i];
    }
    line[cursor++] = ' ';
    line[cursor++] = 'L';
    line[cursor++] = 'P';
    HpLpScDisplay_WriteNumber(number, member->currentLp);
    for (i = 0; number[i] && cursor < 27; i++)
    {
        line[cursor++] = number[i];
    }
    line[cursor++] = '/';
    HpLpScDisplay_WriteNumber(number, member->maxLp);
    for (i = 0; number[i] && cursor < 27; i++)
    {
        line[cursor++] = number[i];
    }
    line[cursor++] = ' ';
    line[cursor++] = 'S';
    line[cursor++] = 'C';
    maxSc = HpLpScDisplay_GetMaxEquippedSc(member, &hasMove);
    if (hasMove)
    {
        HpLpScDisplay_WriteNumber(number, maxSc);
        for (i = 0; number[i] && cursor < 27; i++)
        {
            line[cursor++] = number[i];
        }
    }
    else
    {
        line[cursor++] = '-';
    }
    line[cursor] = '\0';
    Video_DrawText(x, y, line, HUD_TEXT_LIGHT);
}
