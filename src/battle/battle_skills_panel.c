#include "battle/battle_skills_panel.h"

#include "battle_ui/battle_menu.h"
#include "battle_ui/battle_message_queue.h"
#include "battle_ui/hud_layout.h"
#include "core/video.h"

/*
 * Single ownership point for Skills panel chrome vs card host.
 * While a skill-card slide is active, skip band clears and draw the menu only
 * (BattleMenu_DrawSkill owns the card viewport + title/count/footer text).
 * Otherwise clear title/footer bands, then draw the menu (card viewport still
 * owned by BattleMenu_DrawSkill — not cleared here).
 */
void Battle_DrawPlayerSkillPanel(const BattleState *state)
{
    int cardTop;
    int cardBottom;
    int titleH;
    int footerY;
    int footerH;

    if (!state)
    {
        return;
    }

    if (BattleMenu_IsSkillCardSlideActive() && !BattleMessageQueue_IsBusy())
    {
        BattleMenu_Draw(state);
        return;
    }

    cardTop = HUD_SKILLS_CARD_VIEW_Y;
    cardBottom = HUD_SKILLS_CARD_VIEW_Y + HUD_SKILLS_CARD_VIEW_H;
    titleH = cardTop - HUD_SKILLS_PANEL_TOP_Y;
    if (titleH > 0)
    {
        Video_DrawRect(
            0,
            HUD_SKILLS_PANEL_TOP_Y,
            SCREEN_WIDTH,
            titleH,
            HUD_SKILLS_PANEL_FILL);
    }
    footerY = cardBottom;
    footerH = SCREEN_HEIGHT - footerY;
    if (footerH > 0)
    {
        Video_DrawRect(
            0,
            footerY,
            SCREEN_WIDTH,
            footerH,
            HUD_SKILLS_PANEL_FILL);
    }

    if (BattleMessageQueue_IsBusy())
    {
        BattleMessageQueue_Draw();
        return;
    }
    BattleMenu_Draw(state);
}
