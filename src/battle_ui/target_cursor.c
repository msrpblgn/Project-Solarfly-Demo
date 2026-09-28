#include "battle_ui/target_cursor.h"
#include "battle_ui/battle_slot_layout.h"
#include "battle_ui/hud_layout.h"
#include "assets/sprite_assets.h"
#include "core/video.h"
#include "core/gba_types.h"

void TargetCursor_Init(void)
{
}

void TargetCursor_Update(BattleState *state)
{
    (void)state;
}

void TargetCursor_DrawSprite(const BattleState *state)
{
    const BattleSlotLayout *layout;

    if (state->phase != BATTLE_PHASE_TARGET_SELECT)
    {
        return;
    }

    layout = BattleSlotLayout_Get(state->targetSide, state->targetSlot);
    if (!layout)
    {
        return;
    }

    SpriteAssets_DrawPlaceholder(
        SPRITE_ASSET_TARGET_CURSOR,
        layout->cursorX,
        layout->cursorY);
}

void TargetCursor_DrawPanel(const BattleState *state)
{
    if (state->phase != BATTLE_PHASE_TARGET_SELECT)
    {
        return;
    }

    Video_DrawText(8, HUD_PANEL_TITLE_Y, "TARGET", HUD_TEXT_TITLE);
    Video_DrawText(8, HUD_PANEL_BODY_Y, "LEFT RIGHT MOVE", HUD_TEXT_MUTED);
    Video_DrawText(8, HUD_PANEL_FOOTER_Y, "A OK  B BACK", HUD_TEXT_MUTED);
}
