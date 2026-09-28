#ifndef HP_LP_SC_DISPLAY_H
#define HP_LP_SC_DISPLAY_H

#include "battle/battle_member.h"

void HpLpScDisplay_DrawMember(int x, int y, const BattleMember *member);
void HpLpScDisplay_DrawCompactSlot(int x, int y, const BattleMember *member);
void HpLpScDisplay_DrawCompactSlotStyled(int x, int y, const BattleMember *member, int dimmed);
void HpLpScDisplay_DrawActivePanelTitleStats(int x, int y, const BattleMember *member);

#endif
