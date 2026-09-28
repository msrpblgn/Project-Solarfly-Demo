#ifndef BATTLE_DRAW_H
#define BATTLE_DRAW_H

#define BATTLE_DRAW_DIRTY_FIELD   0x01
#define BATTLE_DRAW_DIRTY_MESSAGE 0x02
#define BATTLE_DRAW_DIRTY_PANEL   0x04
#define BATTLE_DRAW_DIRTY_PARTY   0x08
#define BATTLE_DRAW_DIRTY_PANEL_CMD 0x10
/* Field sprites/bg only; does not force panel/message redraw when messages are busy. */
#define BATTLE_DRAW_DIRTY_FIELD_ANIM 0x20
#define BATTLE_DRAW_DIRTY_ALL     (BATTLE_DRAW_DIRTY_FIELD | BATTLE_DRAW_DIRTY_MESSAGE | BATTLE_DRAW_DIRTY_PANEL)

#define BATTLE_FIELD_HEIGHT    112
#define BATTLE_PANEL_Y         112
#define BATTLE_PANEL_HEIGHT    48

void BattleDraw_MarkDirty(unsigned int flags);
void BattleDraw_MarkAllDirty(void);
unsigned int BattleDraw_PeekDirty(void);
unsigned int BattleDraw_ConsumeDirty(void);
void BattleDraw_ClearDirty(void);

#endif
