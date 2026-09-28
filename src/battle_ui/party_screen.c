#include "battle_ui/party_screen.h"
#include "battle/battle_draw.h"
#include "battle/battle_member.h"
#include "battle/battle_move.h"
#include "battle/battle_slot.h"
#include "battle/battle_state.h"
#include "data/element_database.h"
#include "data/race_database.h"
#include "data/status_database.h"
#include "assets/sprite_assets.h"
#include "core/input.h"
#include "core/video.h"
#include "core/gba_types.h"

/*
 * Party screen (UI-only, no turn consumption).
 * Browse mode: member carousel + faceplate visible together; Up/Down change member,
 * Left/Right change faceplate. Detail mode: drill into equipment/stats/moves rows.
 * Member lookup: s_selectedPartyIndex -> s_partyEntries[].battleSlot -> BattleMember.
 *
 * Clock Dial Carousel: angle-based party selector on right panel (29C2).
 * Deferred: connector lines (29E),
 * real element icons, equipment data, MoveData-backed descriptions, scroll reactivation.
 * Element icon colors: MoveData.element via ElementDatabase (Mode 3 placeholders).
 */

/* Layout */
#define PARTY_SHELL_MARGIN 8
#define PARTY_SHELL_X 8
#define PARTY_SHELL_Y 8
#define PARTY_SHELL_W 224
#define PARTY_SHELL_H 144
#define PARTY_DIVIDER_X 120
#define PARTY_SPLIT_Y 98 /* Horizontal split; top panel y=9..97, description below */
#define PARTY_TOP_Y0 8
#define PARTY_TOP_Y1 (PARTY_SPLIT_Y - 1)
#define PARTY_DESC_Y0 (PARTY_SPLIT_Y + 1)
#define PARTY_DESC_Y1 (PARTY_SHELL_Y + PARTY_SHELL_H)

#define PARTY_PANEL_LEFT_X PARTY_SHELL_X
#define PARTY_PANEL_LEFT_W (PARTY_DIVIDER_X - PARTY_SHELL_X)
#define PARTY_PANEL_RIGHT_X (PARTY_DIVIDER_X + 1)
#define PARTY_PANEL_RIGHT_W (PARTY_SHELL_X + PARTY_SHELL_W - PARTY_PANEL_RIGHT_X)

#define PARTY_LINE_MAX 48
#define PARTY_NAMEPLATE_NAME_MAX 12

#define PARTY_SCREEN_BG RGB15(4, 4, 16)
#define PARTY_SCREEN_BORDER RGB15(12, 12, 28)
#define PARTY_SCREEN_PANEL_FILL RGB15(6, 6, 18)
#define PARTY_SCREEN_DIVIDER RGB15(10, 10, 24)
#define PARTY_SCREEN_TITLE RGB15(24, 24, 31)
#define PARTY_SCREEN_BODY RGB15(20, 20, 28)
#define PARTY_SCREEN_MUTED RGB15(14, 14, 22)
#define PARTY_SCREEN_FOOTER RGB15(16, 16, 24)
#define PARTY_SCREEN_STAT_INCREASE RGB15(31, 0, 0)
#define PARTY_SCREEN_STAT_DECREASE RGB15(0, 0, 31)

#define PARTY_PLATE_FILL_SEL RGB15(12, 12, 24)
#define PARTY_PLATE_BORDER_SEL RGB15(24, 24, 31)
#define PARTY_PLATE_TEXT_SEL RGB15(31, 31, 31)

#define PARTY_PLATE_FILL_BACK RGB15(6, 6, 14)
#define PARTY_PLATE_BORDER_BACK RGB15(10, 10, 18)
#define PARTY_PLATE_TEXT_BACK RGB15(16, 16, 22)

#define PARTY_PLATE_FILL_BACK_FAR RGB15(4, 4, 10)
#define PARTY_PLATE_BORDER_BACK_FAR RGB15(8, 8, 14)
#define PARTY_PLATE_TEXT_BACK_FAR RGB15(12, 12, 16)

#define PARTY_PLATE_FILL_DOWN RGB15(8, 4, 6)
#define PARTY_PLATE_BORDER_DOWN RGB15(14, 6, 8)
#define PARTY_PLATE_TEXT_DOWN RGB15(18, 10, 12)

#define PARTY_SELECTOR_CLEAR_X 121
#define PARTY_SELECTOR_CLEAR_Y 9
#define PARTY_SELECTOR_CLEAR_W 111
#define PARTY_SELECTOR_CLEAR_H (PARTY_TOP_Y1 - PARTY_SELECTOR_CLEAR_Y + 1)

#define PARTY_DESC_CLEAR_X 9
#define PARTY_DESC_CLEAR_Y (PARTY_DESC_Y0 + 1)
#define PARTY_DESC_CLEAR_W 222
#define PARTY_DESC_CLEAR_H ((PARTY_SHELL_Y + PARTY_SHELL_H - 1) - PARTY_DESC_CLEAR_Y + 1)

#define PARTY_DESC_TITLE_Y (PARTY_DESC_Y0 + 3)
#define PARTY_DESC_BODY_Y (PARTY_DESC_TITLE_Y + 12)
#define PARTY_DESC_MOVE_BODY_Y (PARTY_DESC_TITLE_Y + 12)
#define PARTY_DESC_CONTROLS_Y (PARTY_DESC_Y0 + 28)
#define PARTY_MOVE_DESC_LINE_SPACING 10
#define PARTY_MOVE_DESC_CHARS_PER_LINE 34
#define PARTY_MOVE_DESC_TITLE_MAX 36
#define PARTY_MOVE_DESC_VISIBLE_LINES 3
#define PARTY_MOVE_DESC_MAX_LINES 8

#define PARTY_FACEPLATE_CLEAR_X 9
#define PARTY_FACEPLATE_CLEAR_Y 9
#define PARTY_FACEPLATE_CLEAR_W 110
#define PARTY_FACEPLATE_CLEAR_H (PARTY_TOP_Y1 - PARTY_FACEPLATE_CLEAR_Y + 1)

/* Top-left faceplate layout (usable panel y=9..97, divider at y=98) */
#define PARTY_FACE_TITLE_X_WIDE (PARTY_FACEPLATE_CLEAR_X + 28)
#define PARTY_FACE_TITLE_X_NARROW (PARTY_FACEPLATE_CLEAR_X + 40)
#define PARTY_FACE_TEXT_X (PARTY_PANEL_LEFT_X + 14)
#define PARTY_FACE_HIGHLIGHT_X (PARTY_PANEL_LEFT_X + 12)
#define PARTY_FACE_HIGHLIGHT_W 96

#define PARTY_CHAR_TITLE_Y (PARTY_TOP_Y0 + 16)
#define PARTY_CHAR_SHEET_Y (PARTY_TOP_Y0 + 28)
#define PARTY_CHAR_PROMPT_Y (PARTY_TOP_Y0 + 46)
#define PARTY_CHAR_NAME_Y (PARTY_TOP_Y0 + 62)

#define PARTY_EQUIP_TITLE_Y (PARTY_TOP_Y0 + 10)
#define PARTY_EQUIP_TITLE_X (PARTY_FACEPLATE_CLEAR_X + 10)
#define PARTY_EQUIP_PORTRAIT_SHIFT_X 6
#define PARTY_AKSIL_PORTRAIT_EXTRA_SHIFT_X 3
#define PARTY_EQUIP_SILHOUETTE_X ((PARTY_PANEL_LEFT_X + 14) - PARTY_EQUIP_PORTRAIT_SHIFT_X)
#define PARTY_EQUIP_SILHOUETTE_Y (PARTY_TOP_Y0 + 26)
#define PARTY_EQUIP_SILHOUETTE_W 22
#define PARTY_EQUIP_SILHOUETTE_H 30
#define PARTY_AKSIL_PORTRAIT_X \
    (10 - PARTY_EQUIP_PORTRAIT_SHIFT_X - PARTY_AKSIL_PORTRAIT_EXTRA_SHIFT_X)
#define PARTY_AKSIL_PORTRAIT_Y 22
#define PARTY_PROTAGONIST_PORTRAIT_X PARTY_AKSIL_PORTRAIT_X
#define PARTY_PROTAGONIST_PORTRAIT_Y PARTY_AKSIL_PORTRAIT_Y
#define PARTY_EQUIP_INFO_X ((PARTY_PANEL_LEFT_X + 46) - PARTY_EQUIP_PORTRAIT_SHIFT_X)
#define PARTY_EQUIP_INFO_LIFT 7
#define PARTY_EQUIP_INFO_ROW_SPACING 10
#define PARTY_EQUIP_HP_Y (PARTY_TOP_Y0 + 30 - PARTY_EQUIP_INFO_LIFT)
#define PARTY_EQUIP_LP_Y (PARTY_EQUIP_HP_Y + PARTY_EQUIP_INFO_ROW_SPACING)
#define PARTY_EQUIP_RACE_Y (PARTY_EQUIP_LP_Y + PARTY_EQUIP_INFO_ROW_SPACING)
#define PARTY_EQUIP_GEAR1_Y (PARTY_EQUIP_RACE_Y + PARTY_EQUIP_INFO_ROW_SPACING)
#define PARTY_EQUIP_GEAR2_Y (PARTY_EQUIP_GEAR1_Y + PARTY_EQUIP_INFO_ROW_SPACING)
#define PARTY_EQUIP_GEAR3_Y (PARTY_EQUIP_GEAR2_Y + PARTY_EQUIP_INFO_ROW_SPACING)
#define PARTY_EQUIP_GEAR_CLIP_X PARTY_EQUIP_INFO_X
#define PARTY_EQUIP_GEAR_CLIP_W (PARTY_DIVIDER_X - PARTY_EQUIP_GEAR_CLIP_X - 2)
#define PARTY_EQUIP_GEAR_HIGHLIGHT_W PARTY_EQUIP_GEAR_CLIP_W
#define PARTY_EQUIP_SLOT_HIGHLIGHT_H 10

#define PARTY_STATS_TITLE_Y (PARTY_TOP_Y0 + 10)
#define PARTY_STATS_FIRST_ROW_Y (PARTY_TOP_Y0 + 22)
#define PARTY_STATS_ROW_SPACING 9
#define PARTY_STATS_HIGHLIGHT_H 9
#define PARTY_STATS_VALUE_ROW_COUNT 6
#define PARTY_STATS_STATUS_ROW_Y (PARTY_STATS_FIRST_ROW_Y + (PARTY_STATS_VALUE_ROW_COUNT * PARTY_STATS_ROW_SPACING))
#define PARTY_STATUS_ICON_SIZE 5
#define PARTY_STATUS_ICON_GAP 2
#define PARTY_STATUS_ICONS_X (PARTY_FACE_TEXT_X + 28)

#define PARTY_MOVES_TITLE_Y (PARTY_TOP_Y0 + 10)
#define PARTY_MOVES_FIRST_ROW_Y (PARTY_TOP_Y0 + 24)
#define PARTY_MOVES_ROW_SPACING 11
#define PARTY_MOVE_ICON_X (PARTY_PANEL_LEFT_X + 10)
#define PARTY_MOVE_ICON_SIZE 8
#define PARTY_MOVE_NAME_X (PARTY_PANEL_LEFT_X + 22)
#define PARTY_MOVE_SC_X (PARTY_PANEL_LEFT_X + 86)
#define PARTY_MOVE_ROW_HIGHLIGHT_X (PARTY_PANEL_LEFT_X + 8)
#define PARTY_MOVE_ROW_HIGHLIGHT_W (PARTY_DIVIDER_X - PARTY_MOVE_ROW_HIGHLIGHT_X - 2)
#define PARTY_MOVE_ROW_HIGHLIGHT_H 10

#define PARTY_FACEPLATE_NAME_CLEAR_X PARTY_FACEPLATE_CLEAR_X
#define PARTY_FACEPLATE_NAME_CLEAR_Y PARTY_CHAR_NAME_Y
#define PARTY_FACEPLATE_NAME_CLEAR_W PARTY_FACEPLATE_CLEAR_W
#define PARTY_FACEPLATE_NAME_CLEAR_H 12

#define PARTY_CTRL_BACK_ONLY "B: Back"

#define PARTY_SCREEN_DIRTY_NONE 0
#define PARTY_SCREEN_DIRTY_ALL (1 << 0)
#define PARTY_SCREEN_DIRTY_SELECTOR (1 << 1)
#define PARTY_SCREEN_DIRTY_DESCRIPTION (1 << 2)
#define PARTY_SCREEN_DIRTY_FACEPLATE (1 << 3)

#define PARTY_SCREEN_DIRTY_SELECTION \
    (PARTY_SCREEN_DIRTY_SELECTOR | PARTY_SCREEN_DIRTY_DESCRIPTION | PARTY_SCREEN_DIRTY_FACEPLATE)

#define PARTY_DIAL_PIVOT_X 134
#define PARTY_DIAL_PIVOT_Y 52
#define PARTY_DIAL_RADIUS_X 14
#define PARTY_DIAL_RADIUS_Y 30
#define PARTY_DIAL_ANGLE_UNITS 256
#define PARTY_DIAL_FIX_SHIFT 8

#define PARTY_DIAL_FRONT_NAME_MAX 14
#define PARTY_DIAL_SIDE_NAME_MAX 10
#define PARTY_DIAL_BACK_NAME_MAX 10

#define PARTY_DIAL_FRONT_COS_THRESHOLD 170
#define PARTY_DIAL_SIDE_COS_THRESHOLD 48

#define PARTY_DIAL_ANIM_FRAMES 12

#define PARTY_SELECTOR_MAX_X 231
#define PARTY_SELECTOR_MIN_X 121
#define PARTY_SELECTOR_MIN_Y 9
#define PARTY_SELECTOR_MAX_Y 97

typedef struct PartyMemberEntry {
    int battleSlot;
} PartyMemberEntry;

typedef struct PartyNameplateLayout {
    int x;
    int y;
    int w;
    int h;
    u16 fillColor;
    u16 borderColor;
    u16 textColor;
    int nameMaxChars;
} PartyNameplateLayout;

typedef enum PartyUiMode {
    PARTY_UI_MODE_FACEPLATE_BROWSE = 0,
    PARTY_UI_MODE_FACEPLATE_DETAIL
} PartyUiMode;

typedef enum PartyFaceplate {
    PARTY_FACEPLATE_EQUIPMENT = 0,
    PARTY_FACEPLATE_STATS,
    PARTY_FACEPLATE_MOVES,
    PARTY_FACEPLATE_COUNT
} PartyFaceplate;

typedef enum PartyEquipmentSlot {
    PARTY_EQUIPMENT_WEAPON = 0,
    PARTY_EQUIPMENT_ARMOR,
    PARTY_EQUIPMENT_ACCESSORY,
    PARTY_EQUIPMENT_COUNT
} PartyEquipmentSlot;

typedef enum PartyStatField {
    PARTY_STAT_FIELD_HP = 0,
    PARTY_STAT_FIELD_ATTACK,
    PARTY_STAT_FIELD_DEFENCE,
    PARTY_STAT_FIELD_SPEED,
    PARTY_STAT_FIELD_EVASION,
    PARTY_STAT_FIELD_LUCK,
    PARTY_STAT_FIELD_STATUS,
    PARTY_STAT_FIELD_COUNT
} PartyStatField;

static int s_isOpen;
static int s_selectionInitialized;
static int s_partyDirty;
static int s_shellDrawn;
static PartyMemberEntry s_partyEntries[BATTLE_PLAYER_SLOT_COUNT];
static int s_partyCount;
static char s_dialNameBuffer[24];
static int s_selectedPartyIndex;
static int s_circleCarouselAnimating;
static int s_circleCarouselFrame;
static int s_circleCarouselDirection;
static int s_circleCarouselTargetIndex;
static PartyUiMode s_partyUiMode;
static PartyFaceplate s_selectedFaceplate;
static PartyEquipmentSlot s_selectedEquipmentSlot;
static PartyStatField s_selectedStatField;
static int s_selectedMoveSlot;
/* Scroll offset for move detail body; input scrolling disabled while descriptions fit. */
static int s_moveDescriptionScroll;
static char s_moveDescLines[PARTY_MOVE_DESC_MAX_LINES][PARTY_MOVE_DESC_CHARS_PER_LINE + 1];
static int s_moveDescLineCount;

static void PartyScreen_ResetFaceplateToFirstPage(void)
{
    s_selectedFaceplate = PARTY_FACEPLATE_EQUIPMENT;
    s_selectedEquipmentSlot = PARTY_EQUIPMENT_WEAPON;
    s_selectedStatField = PARTY_STAT_FIELD_HP;
    s_selectedMoveSlot = 0;
    s_moveDescriptionScroll = 0;
    s_moveDescLineCount = 0;
}

static void PartyScreen_ResetUiMode(void)
{
    s_partyUiMode = PARTY_UI_MODE_FACEPLATE_BROWSE;
    PartyScreen_ResetFaceplateToFirstPage();
}

static void PartyScreen_AdvanceEquipmentSlot(int delta)
{
    int next = (int)s_selectedEquipmentSlot + delta;

    while (next < 0)
    {
        next += PARTY_EQUIPMENT_COUNT;
    }
    while (next >= PARTY_EQUIPMENT_COUNT)
    {
        next -= PARTY_EQUIPMENT_COUNT;
    }
    s_selectedEquipmentSlot = (PartyEquipmentSlot)next;
}

static void PartyScreen_AdvanceStatField(int delta)
{
    int next = (int)s_selectedStatField + delta;

    while (next < 0)
    {
        next += PARTY_STAT_FIELD_COUNT;
    }
    while (next >= PARTY_STAT_FIELD_COUNT)
    {
        next -= PARTY_STAT_FIELD_COUNT;
    }
    s_selectedStatField = (PartyStatField)next;
}

static void PartyScreen_AdvanceMoveSlot(int delta)
{
    int next = s_selectedMoveSlot + delta;

    while (next < 0)
    {
        next += BATTLE_MEMBER_EQUIPPED_MOVE_SLOT_COUNT;
    }
    while (next >= BATTLE_MEMBER_EQUIPPED_MOVE_SLOT_COUNT)
    {
        next -= BATTLE_MEMBER_EQUIPPED_MOVE_SLOT_COUNT;
    }
    s_selectedMoveSlot = next;
    s_moveDescriptionScroll = 0;
}

static int PartyScreen_GetMoveRowY(int slot)
{
    return PARTY_MOVES_FIRST_ROW_Y + (slot * PARTY_MOVES_ROW_SPACING);
}

static int PartyScreen_GetStatFieldRowY(PartyStatField field)
{
    if (field == PARTY_STAT_FIELD_STATUS)
    {
        return PARTY_STATS_STATUS_ROW_Y;
    }
    return PARTY_STATS_FIRST_ROW_Y + ((int)field * PARTY_STATS_ROW_SPACING);
}

static void PartyScreen_MarkFaceplateDescriptionDirty(void)
{
    s_partyDirty = PARTY_SCREEN_DIRTY_FACEPLATE | PARTY_SCREEN_DIRTY_DESCRIPTION;
    BattleDraw_MarkDirty(BATTLE_DRAW_DIRTY_PARTY);
}

static void PartyScreen_SelectPartyIndexInstant(int newIndex)
{
    if (s_partyCount <= 0)
    {
        return;
    }
    while (newIndex < 0)
    {
        newIndex += s_partyCount;
    }
    while (newIndex >= s_partyCount)
    {
        newIndex -= s_partyCount;
    }
    if (newIndex == s_selectedPartyIndex)
    {
        return;
    }
    s_selectedPartyIndex = newIndex;
    /* Keep the current faceplate/page when switching party characters. */
    s_partyDirty = PARTY_SCREEN_DIRTY_SELECTION;
    BattleDraw_MarkDirty(BATTLE_DRAW_DIRTY_PARTY);
}

static void PartyScreen_ResetCircleCarouselAnimation(void)
{
    s_circleCarouselAnimating = 0;
    s_circleCarouselFrame = 0;
    s_circleCarouselDirection = 0;
    s_circleCarouselTargetIndex = 0;
}

static void PartyScreen_StartCircleCarouselRotation(int direction)
{
    int targetIndex;

    if (s_partyCount <= 1 || s_circleCarouselAnimating)
    {
        return;
    }

    if (direction < 0)
    {
        targetIndex = (s_selectedPartyIndex + s_partyCount - 1) % s_partyCount;
    }
    else
    {
        targetIndex = (s_selectedPartyIndex + 1) % s_partyCount;
    }

    if (targetIndex == s_selectedPartyIndex)
    {
        return;
    }

    s_circleCarouselTargetIndex = targetIndex;
    s_circleCarouselDirection = direction;
    s_circleCarouselAnimating = 1;
    s_circleCarouselFrame = 0;
    s_partyDirty = PARTY_SCREEN_DIRTY_SELECTOR;
    BattleDraw_MarkDirty(BATTLE_DRAW_DIRTY_PARTY);
}

static void PartyScreen_AdvanceCircleCarouselAnimation(void)
{
    int maxFrame = PARTY_DIAL_ANIM_FRAMES - 1;
    int targetIndex;

    s_partyDirty = PARTY_SCREEN_DIRTY_SELECTOR;
    BattleDraw_MarkDirty(BATTLE_DRAW_DIRTY_PARTY);

    if (s_circleCarouselFrame >= maxFrame)
    {
        targetIndex = s_circleCarouselTargetIndex;
        PartyScreen_ResetCircleCarouselAnimation();
        PartyScreen_SelectPartyIndexInstant(targetIndex);
        return;
    }

    s_circleCarouselFrame++;
}

static void PartyScreen_AppendChar(char *dest, int *cursor, char c)
{
    if (*cursor < PARTY_LINE_MAX - 1)
    {
        dest[*cursor] = c;
        (*cursor)++;
    }
}

static void PartyScreen_AppendString(char *dest, int *cursor, const char *text)
{
    while (*text)
    {
        PartyScreen_AppendChar(dest, cursor, *text);
        text++;
    }
}

static void PartyScreen_AppendNumber(char *dest, int *cursor, int value)
{
    char digits[8];
    int count = 0;
    int temp = value;

    if (temp == 0)
    {
        PartyScreen_AppendChar(dest, cursor, '0');
        return;
    }
    while (temp > 0 && count < 8)
    {
        digits[count++] = (char)('0' + (temp % 10));
        temp /= 10;
    }
    while (count > 0)
    {
        PartyScreen_AppendChar(dest, cursor, digits[--count]);
    }
}

static void PartyScreen_AppendSignedNumber(char *dest, int *cursor, int value)
{
    if (value < 0)
    {
        PartyScreen_AppendChar(dest, cursor, '-');
        PartyScreen_AppendNumber(dest, cursor, -value);
        return;
    }
    if (value > 0)
    {
        PartyScreen_AppendChar(dest, cursor, '+');
    }
    PartyScreen_AppendNumber(dest, cursor, value);
}

static void PartyScreen_DrawStatValueLine(
    int x,
    int y,
    const char *label,
    int base,
    int effective,
    int statModPercent)
{
    char line[28];
    int cursor = 0;
    int displayPercent = 0;
    int showSuffix = 0;
    u16 color = PARTY_SCREEN_BODY;

    PartyScreen_AppendString(line, &cursor, label);
    PartyScreen_AppendNumber(line, &cursor, effective);
    if (statModPercent != 0)
    {
        displayPercent = statModPercent;
        showSuffix = 1;
    }
    else if (base > 0 && effective != base)
    {
        displayPercent = ((effective - base) * 100) / base;
        showSuffix = 1;
    }
    if (showSuffix)
    {
        PartyScreen_AppendChar(line, &cursor, ' ');
        PartyScreen_AppendChar(line, &cursor, '(');
        PartyScreen_AppendSignedNumber(line, &cursor, displayPercent);
        PartyScreen_AppendChar(line, &cursor, '%');
        PartyScreen_AppendChar(line, &cursor, ')');
        if (displayPercent > 0)
        {
            color = PARTY_SCREEN_STAT_INCREASE;
        }
        else if (displayPercent < 0)
        {
            color = PARTY_SCREEN_STAT_DECREASE;
        }
    }
    line[cursor] = '\0';
    Video_DrawText(x, y, line, color);
}

static void PartyScreen_DrawEvasionValueLine(int x, int y, const BattleMember *member)
{
    char line[20];
    int cursor = 0;
    int modPercent;
    u16 color = PARTY_SCREEN_BODY;

    if (!member)
    {
        Video_DrawText(x, y, "EVA %0", PARTY_SCREEN_BODY);
        return;
    }

    modPercent = BattleMember_GetStatModPercent(member, BATTLE_STAT_EVASIVENESS);
    PartyScreen_AppendString(line, &cursor, "EVA ");
    if (modPercent != 0)
    {
        PartyScreen_AppendSignedNumber(line, &cursor, modPercent);
        PartyScreen_AppendChar(line, &cursor, '%');
        color = modPercent > 0 ? PARTY_SCREEN_STAT_INCREASE : PARTY_SCREEN_STAT_DECREASE;
    }
    else
    {
        PartyScreen_AppendChar(line, &cursor, '%');
        PartyScreen_AppendNumber(line, &cursor, member->stats.evasiveness);
    }
    line[cursor] = '\0';
    Video_DrawText(x, y, line, color);
}

static void PartyScreen_CopyTruncated(char *dest, int maxLen, const char *text)
{
    int i = 0;

    if (!dest || maxLen <= 0)
    {
        return;
    }
    if (!text)
    {
        dest[0] = '\0';
        return;
    }
    while (text[i] && i < maxLen - 1)
    {
        dest[i] = text[i];
        i++;
    }
    dest[i] = '\0';
}

static void PartyScreen_RefreshEntries(const BattleState *state)
{
    int slot;

    s_partyCount = 0;
    if (!state)
    {
        return;
    }
    for (slot = 0; slot < BATTLE_PLAYER_SLOT_COUNT; slot++)
    {
        if (BattleState_IsSlotOccupied(state, BATTLE_SIDE_PLAYER, slot))
        {
            s_partyEntries[s_partyCount].battleSlot = slot;
            s_partyCount++;
        }
    }
    if (s_partyCount <= 0)
    {
        s_selectedPartyIndex = 0;
        return;
    }
    if (s_selectedPartyIndex >= s_partyCount)
    {
        s_selectedPartyIndex = s_partyCount - 1;
    }
    if (s_selectedPartyIndex < 0)
    {
        s_selectedPartyIndex = 0;
    }
}

static const BattleMember *PartyScreen_GetSelectedMember(const BattleState *state)
{
    if (!state || s_partyCount <= 0)
    {
        return 0;
    }
    return BattleState_GetMemberAtSlotConst(
        state,
        BATTLE_SIDE_PLAYER,
        s_partyEntries[s_selectedPartyIndex].battleSlot);
}

static void PartyScreen_InitSelection(const BattleState *state)
{
    int protagonistSlot;
    int i;

    s_selectedPartyIndex = 0;
    if (!state || s_partyCount <= 0)
    {
        return;
    }
    protagonistSlot = BattleState_FindProtagonistSlot(state);
    if (protagonistSlot >= 0)
    {
        for (i = 0; i < s_partyCount; i++)
        {
            if (s_partyEntries[i].battleSlot == protagonistSlot)
            {
                s_selectedPartyIndex = i;
                return;
            }
        }
    }
}

static void PartyScreen_EnsureSelectionInitialized(const BattleState *state)
{
    if (!s_isOpen || s_selectionInitialized || !state)
    {
        return;
    }
    PartyScreen_RefreshEntries(state);
    PartyScreen_InitSelection(state);
    s_selectionInitialized = 1;
}

static int PartyScreen_GetRelativeOffsetFor(int partyIndex, int centerIndex)
{
    if (s_partyCount <= 0)
    {
        return 0;
    }
    return (partyIndex - centerIndex + s_partyCount) % s_partyCount;
}

static const s16 s_partyDialCos[256] = {
    256, 256, 256, 255, 255, 254, 253, 252, 251, 250, 248, 247, 245, 243, 241, 239,
    237, 234, 231, 229, 226, 223, 220, 216, 213, 209, 206, 202, 198, 194, 190, 185,
    181, 177, 172, 167, 162, 157, 152, 147, 142, 137, 132, 126, 121, 115, 109, 104,
    98, 92, 86, 80, 74, 68, 62, 56, 50, 44, 38, 31, 25, 19, 13, 6,
    0, -6, -13, -19, -25, -31, -38, -44, -50, -56, -62, -68, -74, -80, -86, -92,
    -98, -104, -109, -115, -121, -126, -132, -137, -142, -147, -152, -157, -162, -167, -172, -177,
    -181, -185, -190, -194, -198, -202, -206, -209, -213, -216, -220, -223, -226, -229, -231, -234,
    -237, -239, -241, -243, -245, -247, -248, -250, -251, -252, -253, -254, -255, -255, -256, -256,
    -256, -256, -256, -255, -255, -254, -253, -252, -251, -250, -248, -247, -245, -243, -241, -239,
    -237, -234, -231, -229, -226, -223, -220, -216, -213, -209, -206, -202, -198, -194, -190, -185,
    -181, -177, -172, -167, -162, -157, -152, -147, -142, -137, -132, -126, -121, -115, -109, -104,
    -98, -92, -86, -80, -74, -68, -62, -56, -50, -44, -38, -31, -25, -19, -13, -6,
    0, 6, 13, 19, 25, 31, 38, 44, 50, 56, 62, 68, 74, 80, 86, 92,
    98, 104, 109, 115, 121, 126, 132, 137, 142, 147, 152, 157, 162, 167, 172, 177,
    181, 185, 190, 194, 198, 202, 206, 209, 213, 216, 220, 223, 226, 229, 231, 234,
    237, 239, 241, 243, 245, 247, 248, 250, 251, 252, 253, 254, 255, 255, 256, 256,
};

static const s16 s_partyDialSin[256] = {
    0, 6, 13, 19, 25, 31, 38, 44, 50, 56, 62, 68, 74, 80, 86, 92,
    98, 104, 109, 115, 121, 126, 132, 137, 142, 147, 152, 157, 162, 167, 172, 177,
    181, 185, 190, 194, 198, 202, 206, 209, 213, 216, 220, 223, 226, 229, 231, 234,
    237, 239, 241, 243, 245, 247, 248, 250, 251, 252, 253, 254, 255, 255, 256, 256,
    256, 256, 256, 255, 255, 254, 253, 252, 251, 250, 248, 247, 245, 243, 241, 239,
    237, 234, 231, 229, 226, 223, 220, 216, 213, 209, 206, 202, 198, 194, 190, 185,
    181, 177, 172, 167, 162, 157, 152, 147, 142, 137, 132, 126, 121, 115, 109, 104,
    98, 92, 86, 80, 74, 68, 62, 56, 50, 44, 38, 31, 25, 19, 13, 6,
    0, -6, -13, -19, -25, -31, -38, -44, -50, -56, -62, -68, -74, -80, -86, -92,
    -98, -104, -109, -115, -121, -126, -132, -137, -142, -147, -152, -157, -162, -167, -172, -177,
    -181, -185, -190, -194, -198, -202, -206, -209, -213, -216, -220, -223, -226, -229, -231, -234,
    -237, -239, -241, -243, -245, -247, -248, -250, -251, -252, -253, -254, -255, -255, -256, -256,
    -256, -256, -256, -255, -255, -254, -253, -252, -251, -250, -248, -247, -245, -243, -241, -239,
    -237, -234, -231, -229, -226, -223, -220, -216, -213, -209, -206, -202, -198, -194, -190, -185,
    -181, -177, -172, -167, -162, -157, -152, -147, -142, -137, -132, -126, -121, -115, -109, -104,
    -98, -92, -86, -80, -74, -68, -62, -56, -50, -44, -38, -31, -25, -19, -13, -6,
};

static int PartyScreen_NormalizeDialAngle(int angle)
{
    angle %= PARTY_DIAL_ANGLE_UNITS;
    if (angle < 0)
    {
        angle += PARTY_DIAL_ANGLE_UNITS;
    }
    return angle;
}

static int PartyScreen_GetSignedDialOffset(int relOffset)
{
    if (relOffset == 0)
    {
        return 0;
    }
    if (relOffset * 2 <= s_partyCount)
    {
        return relOffset;
    }
    return relOffset - s_partyCount;
}

static int PartyScreen_GetDialAnimAngleOffset(void)
{
    int angleStep;
    int maxFrame;

    if (!s_circleCarouselAnimating || s_partyCount <= 0)
    {
        return 0;
    }

    angleStep = PARTY_DIAL_ANGLE_UNITS / s_partyCount;
    maxFrame = PARTY_DIAL_ANIM_FRAMES - 1;
    if (maxFrame <= 0)
    {
        return s_circleCarouselDirection * angleStep;
    }
    return (s_circleCarouselDirection * angleStep * s_circleCarouselFrame) / maxFrame;
}

static int PartyScreen_GetMemberDialAngle(int partyIndex)
{
    int relOffset;
    int signedOffset;
    int angleStep;
    int angle;

    relOffset = PartyScreen_GetRelativeOffsetFor(partyIndex, s_selectedPartyIndex);
    signedOffset = PartyScreen_GetSignedDialOffset(relOffset);
    angleStep = PARTY_DIAL_ANGLE_UNITS / s_partyCount;
    angle = signedOffset * angleStep + PartyScreen_GetDialAnimAngleOffset();
    return PartyScreen_NormalizeDialAngle(angle);
}

static int PartyScreen_DialCos(int angle)
{
    return (int)s_partyDialCos[PartyScreen_NormalizeDialAngle(angle)];
}

static int PartyScreen_DialSin(int angle)
{
    return (int)s_partyDialSin[PartyScreen_NormalizeDialAngle(angle)];
}

static void PartyScreen_ClampDialNameplateLayout(PartyNameplateLayout *layout)
{
    if (layout->x < PARTY_SELECTOR_MIN_X)
    {
        layout->x = PARTY_SELECTOR_MIN_X;
    }
    if (layout->x + layout->w > PARTY_SELECTOR_MAX_X)
    {
        layout->x = PARTY_SELECTOR_MAX_X - layout->w;
    }
    if (layout->y < PARTY_SELECTOR_MIN_Y)
    {
        layout->y = PARTY_SELECTOR_MIN_Y;
    }
    if (layout->y + layout->h > PARTY_SELECTOR_MAX_Y)
    {
        layout->y = PARTY_SELECTOR_MAX_Y - layout->h;
    }
}

static void PartyScreen_BuildDialNameplateLayout(int angle, PartyNameplateLayout *layout)
{
    int cosVal;
    int sinVal;
    int anchorX;
    int anchorY;

    cosVal = PartyScreen_DialCos(angle);
    sinVal = PartyScreen_DialSin(angle);
    anchorX = PARTY_DIAL_PIVOT_X + ((cosVal * PARTY_DIAL_RADIUS_X) >> PARTY_DIAL_FIX_SHIFT);
    anchorY = PARTY_DIAL_PIVOT_Y + ((sinVal * PARTY_DIAL_RADIUS_Y) >> PARTY_DIAL_FIX_SHIFT);

    layout->fillColor = PARTY_PLATE_FILL_BACK;
    layout->borderColor = PARTY_PLATE_BORDER_BACK;
    layout->textColor = PARTY_PLATE_TEXT_BACK;
    layout->nameMaxChars = PARTY_DIAL_SIDE_NAME_MAX;
    layout->w = 72;
    layout->h = 14;

    if (cosVal >= PARTY_DIAL_FRONT_COS_THRESHOLD)
    {
        layout->w = 88;
        layout->h = 18;
        layout->fillColor = PARTY_PLATE_FILL_SEL;
        layout->borderColor = PARTY_PLATE_BORDER_SEL;
        layout->textColor = PARTY_PLATE_TEXT_SEL;
        layout->nameMaxChars = PARTY_DIAL_FRONT_NAME_MAX;
    }
    else if (cosVal >= PARTY_DIAL_SIDE_COS_THRESHOLD)
    {
        layout->fillColor = PARTY_PLATE_FILL_BACK;
        layout->borderColor = PARTY_PLATE_BORDER_BACK;
        layout->textColor = PARTY_PLATE_TEXT_BACK;
        layout->nameMaxChars = PARTY_DIAL_SIDE_NAME_MAX;
        layout->w = 72;
        layout->h = 14;
    }
    else
    {
        layout->w = 64;
        layout->h = 12;
        layout->fillColor = PARTY_PLATE_FILL_BACK;
        layout->borderColor = PARTY_PLATE_BORDER_BACK;
        layout->textColor = PARTY_PLATE_TEXT_BACK;
        layout->nameMaxChars = PARTY_DIAL_BACK_NAME_MAX;
    }

    layout->x = anchorX;
    layout->y = anchorY - (layout->h / 2);
    PartyScreen_ClampDialNameplateLayout(layout);
}

static int PartyScreen_GetDialDrawPriority(int angle)
{
    return PartyScreen_DialCos(angle);
}

static void PartyScreen_DrawNameplateBody(
    const PartyNameplateLayout *layout,
    int isAlive)
{
    u16 fillColor = layout->fillColor;
    u16 borderColor = layout->borderColor;

    if (!isAlive)
    {
        fillColor = PARTY_PLATE_FILL_DOWN;
        borderColor = PARTY_PLATE_BORDER_DOWN;
    }

    Video_DrawRect(layout->x, layout->y, layout->w, layout->h, fillColor);
    Video_DrawRect(layout->x, layout->y, layout->w, 1, borderColor);
    Video_DrawRect(layout->x, layout->y + layout->h - 1, layout->w, 1, borderColor);
    Video_DrawRect(layout->x, layout->y, 1, layout->h, borderColor);
    Video_DrawRect(layout->x + layout->w - 1, layout->y, 1, layout->h, borderColor);
}

static void PartyScreen_DrawNameplateText(
    const PartyNameplateLayout *layout,
    const char *name,
    int isAlive)
{
    char label[16];
    u16 textColor = layout->textColor;
    int textX;
    int textY;

    if (!name)
    {
        return;
    }

    if (!isAlive)
    {
        textColor = PARTY_PLATE_TEXT_DOWN;
    }

    PartyScreen_CopyTruncated(label, layout->nameMaxChars + 1, name);
    textX = layout->x + 4;
    textY = layout->y + ((layout->h - 7) / 2);
    Video_DrawText(textX, textY, label, textColor);
}

static void PartyScreen_DrawNameplate(
    const PartyNameplateLayout *layout,
    const char *name,
    int isAlive,
    int drawText)
{
    PartyScreen_DrawNameplateBody(layout, isAlive);
    if (drawText)
    {
        PartyScreen_DrawNameplateText(layout, name, isAlive);
    }
}

static void PartyScreen_DrawDialCarousel(const BattleState *state)
{
    int drawOrder[BATTLE_PLAYER_SLOT_COUNT];
    int drawPriority[BATTLE_PLAYER_SLOT_COUNT];
    int i;
    int j;
    int partyIndex;
    int angle;
    PartyNameplateLayout layout;
    const BattleMember *member;
    int tempIndex;
    int tempPriority;

    if (!state || s_partyCount <= 0)
    {
        Video_DrawText(PARTY_PANEL_RIGHT_X + 12, PARTY_TOP_Y0 + 40, "NO ALLIES", PARTY_SCREEN_MUTED);
        return;
    }

    for (i = 0; i < s_partyCount; i++)
    {
        drawOrder[i] = i;
        angle = PartyScreen_GetMemberDialAngle(i);
        drawPriority[i] = PartyScreen_GetDialDrawPriority(angle);
    }

    for (i = 0; i < s_partyCount - 1; i++)
    {
        for (j = i + 1; j < s_partyCount; j++)
        {
            if (drawPriority[j] < drawPriority[i])
            {
                tempPriority = drawPriority[i];
                drawPriority[i] = drawPriority[j];
                drawPriority[j] = tempPriority;
                tempIndex = drawOrder[i];
                drawOrder[i] = drawOrder[j];
                drawOrder[j] = tempIndex;
            }
        }
    }

    for (i = 0; i < s_partyCount; i++)
    {
        partyIndex = drawOrder[i];
        angle = PartyScreen_GetMemberDialAngle(partyIndex);
        member = BattleState_GetMemberAtSlotConst(
            state,
            BATTLE_SIDE_PLAYER,
            s_partyEntries[partyIndex].battleSlot);
        if (!member)
        {
            continue;
        }
        PartyScreen_BuildDialNameplateLayout(angle, &layout);
        BattleMember_FormatNameWithLevel(member, s_dialNameBuffer, (int)sizeof(s_dialNameBuffer));
        PartyScreen_DrawNameplate(
            &layout,
            s_dialNameBuffer,
            BattleState_IsAliveOccupiedSlot(
                state,
                BATTLE_SIDE_PLAYER,
                s_partyEntries[partyIndex].battleSlot),
            1);
    }
}

static void PartyScreen_DrawCarousel(const BattleState *state)
{
    PartyScreen_DrawDialCarousel(state);
}

static void PartyScreen_DrawHpBarPlaceholder(
    int x,
    int y,
    int w,
    int h,
    int currentHp,
    int maxHp)
{
    int fillW;

    Video_DrawRect(x, y, w, h, PARTY_PLATE_FILL_BACK);
    Video_DrawRect(x, y, w, 1, PARTY_SCREEN_BORDER);
    Video_DrawRect(x, y + h - 1, w, 1, PARTY_SCREEN_BORDER);
    Video_DrawRect(x, y, 1, h, PARTY_SCREEN_BORDER);
    Video_DrawRect(x + w - 1, y, 1, h, PARTY_SCREEN_BORDER);

    if (maxHp <= 0)
    {
        return;
    }
    fillW = (currentHp * (w - 2)) / maxHp;
    if (fillW < 0)
    {
        fillW = 0;
    }
    if (fillW > w - 2)
    {
        fillW = w - 2;
    }
    if (fillW > 0)
    {
        Video_DrawRect(x + 1, y + 1, fillW, h - 2, RGB15(8, 22, 10));
    }
}

static void PartyScreen_GetOverviewEquipmentLines(
    const BattleMember *member,
    const char **outWpn,
    const char **outArm,
    const char **outAcc)
{
    if (BattleMember_IsAksil(member))
    {
        *outWpn = "WPN SLINGSHOT";
        *outArm = "ARM JACKET";
        *outAcc = "ACC HOT BRACELET";
    }
    else
    {
        *outWpn = "WPN GLOWSWORD";
        *outArm = "ARM TUNIC";
        *outAcc = "ACC HEADBAND";
    }
}

static void PartyScreen_GetEquipmentDetailName(
    const BattleMember *member,
    PartyEquipmentSlot slot,
    const char **outName)
{
    if (BattleMember_IsAksil(member))
    {
        switch (slot)
        {
        case PARTY_EQUIPMENT_WEAPON:
            *outName = "SLINGSHOT";
            break;
        case PARTY_EQUIPMENT_ARMOR:
            *outName = "JACKET";
            break;
        default:
            *outName = "HOT BRACELET";
            break;
        }
    }
    else
    {
        switch (slot)
        {
        case PARTY_EQUIPMENT_WEAPON:
            *outName = "GLOWSWORD";
            break;
        case PARTY_EQUIPMENT_ARMOR:
            *outName = "TUNIC";
            break;
        default:
            *outName = "HEADBAND";
            break;
        }
    }
}

static void PartyScreen_DrawOverviewEquipmentLine(int rowY, const char *text)
{
    Video_DrawTextClipped(
        PARTY_EQUIP_INFO_X,
        rowY,
        text,
        PARTY_SCREEN_MUTED,
        PARTY_EQUIP_GEAR_CLIP_X,
        PARTY_EQUIP_GEAR_CLIP_W,
        0);
}

static int PartyScreen_GetEquipmentGearRowY(PartyEquipmentSlot slot)
{
    switch (slot)
    {
    case PARTY_EQUIPMENT_WEAPON:
        return PARTY_EQUIP_GEAR1_Y;
    case PARTY_EQUIPMENT_ARMOR:
        return PARTY_EQUIP_GEAR2_Y;
    case PARTY_EQUIPMENT_ACCESSORY:
        return PARTY_EQUIP_GEAR3_Y;
    default:
        return PARTY_EQUIP_GEAR1_Y;
    }
}

static void PartyScreen_DrawEquipmentSlotHighlight(PartyEquipmentSlot slot)
{
    int x = PARTY_EQUIP_INFO_X - 2;
    int y = PartyScreen_GetEquipmentGearRowY(slot) - 1;
    int w = PARTY_EQUIP_GEAR_HIGHLIGHT_W;
    int h = PARTY_EQUIP_SLOT_HIGHLIGHT_H;

    Video_DrawRect(x, y, w, 1, PARTY_PLATE_BORDER_SEL);
    Video_DrawRect(x, y + h - 1, w, 1, PARTY_PLATE_BORDER_SEL);
    Video_DrawRect(x, y, 1, h, PARTY_PLATE_BORDER_SEL);
    Video_DrawRect(x + w - 1, y, 1, h, PARTY_PLATE_BORDER_SEL);
}

static void PartyScreen_DrawTopLeftEquipmentFaceplate(
    const BattleState *state,
    const BattleMember *member,
    int isAlive,
    int highlightSlot)
{
    char line[24];
    int cursor;
    char titleLine[PARTY_NAMEPLATE_NAME_MAX + 1];
    int silhouetteX = PARTY_EQUIP_SILHOUETTE_X;
    int silhouetteY = PARTY_EQUIP_SILHOUETTE_Y;

    /* Placeholder equipment layout; real gear data deferred. */
    if (member && member->name)
    {
        BattleMember_FormatNameWithLevel(member, titleLine, (int)sizeof(titleLine));
    }
    else
    {
        PartyScreen_CopyTruncated(titleLine, PARTY_NAMEPLATE_NAME_MAX, "Member");
    }
    Video_DrawText(PARTY_EQUIP_TITLE_X, PARTY_EQUIP_TITLE_Y, titleLine, PARTY_SCREEN_TITLE);

    if (BattleMember_IsAksil(member))
    {
        SpriteAssets_DrawAksilFront(PARTY_AKSIL_PORTRAIT_X, PARTY_AKSIL_PORTRAIT_Y);
    }
    else if (BattleMember_IsProtagonist(member))
    {
        SpriteAssets_DrawProtagonistFront(
            PARTY_PROTAGONIST_PORTRAIT_X, PARTY_PROTAGONIST_PORTRAIT_Y);
    }
    else
    {
        Video_DrawRect(
            silhouetteX,
            silhouetteY,
            PARTY_EQUIP_SILHOUETTE_W,
            PARTY_EQUIP_SILHOUETTE_H,
            PARTY_PLATE_FILL_BACK);
        Video_DrawRect(silhouetteX, silhouetteY, PARTY_EQUIP_SILHOUETTE_W, 1, PARTY_SCREEN_BORDER);
        Video_DrawRect(
            silhouetteX,
            silhouetteY + PARTY_EQUIP_SILHOUETTE_H - 1,
            PARTY_EQUIP_SILHOUETTE_W,
            1,
            PARTY_SCREEN_BORDER);
        Video_DrawRect(silhouetteX, silhouetteY, 1, PARTY_EQUIP_SILHOUETTE_H, PARTY_SCREEN_BORDER);
        Video_DrawRect(
            silhouetteX + PARTY_EQUIP_SILHOUETTE_W - 1,
            silhouetteY,
            1,
            PARTY_EQUIP_SILHOUETTE_H,
            PARTY_SCREEN_BORDER);
    }

    PartyScreen_DrawHpBarPlaceholder(
        PARTY_EQUIP_INFO_X,
        PARTY_EQUIP_HP_Y,
        54,
        6,
        member->currentHp,
        member->maxHp);

    cursor = 0;
    PartyScreen_AppendString(line, &cursor, "LP ");
    PartyScreen_AppendNumber(line, &cursor, member->currentLp);
    line[cursor] = '\0';
    Video_DrawText(PARTY_EQUIP_INFO_X, PARTY_EQUIP_LP_Y, line, PARTY_SCREEN_BODY);

    cursor = 0;
    PartyScreen_AppendString(line, &cursor, "RACE ");
    PartyScreen_AppendString(line, &cursor, RaceDatabase_GetName(BattleMember_GetRace(member)));
    line[cursor] = '\0';
    Video_DrawText(PARTY_EQUIP_INFO_X, PARTY_EQUIP_RACE_Y, line, PARTY_SCREEN_BODY);

    const char *wpnLine;
    const char *armLine;
    const char *accLine;

    PartyScreen_GetOverviewEquipmentLines(member, &wpnLine, &armLine, &accLine);
    PartyScreen_DrawOverviewEquipmentLine(PARTY_EQUIP_GEAR1_Y, wpnLine);
    PartyScreen_DrawOverviewEquipmentLine(PARTY_EQUIP_GEAR2_Y, armLine);
    PartyScreen_DrawOverviewEquipmentLine(PARTY_EQUIP_GEAR3_Y, accLine);

    if (highlightSlot >= 0 && highlightSlot < PARTY_EQUIPMENT_COUNT)
    {
        PartyScreen_DrawEquipmentSlotHighlight((PartyEquipmentSlot)highlightSlot);
    }
    (void)state;
    (void)isAlive;
}

static void PartyScreen_DrawStatFieldHighlight(PartyStatField field)
{
    int rowY = PartyScreen_GetStatFieldRowY(field);
    int x = PARTY_FACE_HIGHLIGHT_X;
    int y = rowY - 1;
    int w = PARTY_FACE_HIGHLIGHT_W;
    int h = PARTY_STATS_HIGHLIGHT_H;

    Video_DrawRect(x, y, w, 1, PARTY_PLATE_BORDER_SEL);
    Video_DrawRect(x, y + h - 1, w, 1, PARTY_PLATE_BORDER_SEL);
    Video_DrawRect(x, y, 1, h, PARTY_PLATE_BORDER_SEL);
    Video_DrawRect(x + w - 1, y, 1, h, PARTY_PLATE_BORDER_SEL);
}

static int PartyScreen_HasAnyStatus(const BattleMember *member)
{
    return BattleMember_HasAnyStatus(member);
}

static void PartyScreen_BuildStatusSummary(const BattleMember *member, char *buffer, int bufferSize)
{
    int i;
    int cursor = 0;
    int hasAny = 0;
    BattleStatusId balanceStatus;

    if (!buffer || bufferSize <= 0)
    {
        return;
    }
    buffer[0] = '\0';
    if (!member)
    {
        return;
    }

    for (i = 0; i < BATTLE_MEMBER_STATUS_SLOT_COUNT; i++)
    {
        BattleStatusId status;
        const char *shortName;

        status = BattleMember_GetStatusAt(member, i);
        if (status == BATTLE_STATUS_NONE)
        {
            continue;
        }
        shortName = StatusDatabase_GetShortName(status);
        if (hasAny)
        {
            PartyScreen_AppendString(buffer, &cursor, ", ");
        }
        else
        {
            PartyScreen_AppendString(buffer, &cursor, "Active: ");
            hasAny = 1;
        }
        PartyScreen_AppendString(buffer, &cursor, shortName);
        if (cursor >= bufferSize - 1)
        {
            break;
        }
    }
    balanceStatus = BattleMember_GetBalanceStatus(member);
    if (balanceStatus != BATTLE_STATUS_NONE)
    {
        if (hasAny)
        {
            PartyScreen_AppendString(buffer, &cursor, ", ");
        }
        else
        {
            PartyScreen_AppendString(buffer, &cursor, "Active: ");
            hasAny = 1;
        }
        PartyScreen_AppendString(buffer, &cursor, "BAL ");
        PartyScreen_AppendString(buffer, &cursor, StatusDatabase_GetShortName(balanceStatus));
    }
    if (cursor < bufferSize)
    {
        buffer[cursor] = '\0';
    }
    else
    {
        buffer[bufferSize - 1] = '\0';
    }
}

static void PartyScreen_DrawStatusSlots(const BattleMember *member, int iconsX, int iconsY)
{
    int i;
    BattleStatusId balanceStatus;
    u16 balanceFill;
    u16 balanceBorder;

    balanceStatus = member ? BattleMember_GetBalanceStatus(member) : BATTLE_STATUS_NONE;
    balanceFill = StatusDatabase_GetIconFillColor(balanceStatus);
    balanceBorder = StatusDatabase_GetIconBorderColor(balanceStatus);

    for (i = 0; i < BATTLE_MEMBER_STATUS_SLOT_COUNT; i++)
    {
        BattleStatusId status = BATTLE_STATUS_NONE;
        u16 fillColor;
        u16 borderColor;
        int iconX;

        if (member)
        {
            status = BattleMember_GetStatusAt(member, i);
        }
        if (balanceStatus != BATTLE_STATUS_NONE)
        {
            fillColor = balanceFill;
            borderColor = balanceBorder;
        }
        else
        {
            fillColor = StatusDatabase_GetIconFillColor(status);
            borderColor = StatusDatabase_GetIconBorderColor(status);
        }
        iconX = iconsX + (i * (PARTY_STATUS_ICON_SIZE + PARTY_STATUS_ICON_GAP));

        Video_DrawRect(iconX, iconsY, PARTY_STATUS_ICON_SIZE, PARTY_STATUS_ICON_SIZE, fillColor);
        Video_DrawRect(iconX, iconsY, PARTY_STATUS_ICON_SIZE, 1, borderColor);
        Video_DrawRect(iconX, iconsY + PARTY_STATUS_ICON_SIZE - 1, PARTY_STATUS_ICON_SIZE, 1, borderColor);
        Video_DrawRect(iconX, iconsY, 1, PARTY_STATUS_ICON_SIZE, borderColor);
        Video_DrawRect(iconX + PARTY_STATUS_ICON_SIZE - 1, iconsY, 1, PARTY_STATUS_ICON_SIZE, borderColor);
    }
}

static void PartyScreen_DrawTopLeftStatsFaceplate(const BattleMember *member, int highlightField)
{
    char line[24];
    int cursor;
    int y = PARTY_STATS_FIRST_ROW_Y;

    Video_DrawText(PARTY_FACE_TITLE_X_NARROW, PARTY_STATS_TITLE_Y, "STATS", PARTY_SCREEN_TITLE);

    cursor = 0;
    PartyScreen_AppendString(line, &cursor, "HP ");
    PartyScreen_AppendNumber(line, &cursor, member->currentHp);
    PartyScreen_AppendChar(line, &cursor, '/');
    PartyScreen_AppendNumber(line, &cursor, member->maxHp);
    line[cursor] = '\0';
    Video_DrawText(PARTY_FACE_TEXT_X, y, line, PARTY_SCREEN_BODY);
    y += PARTY_STATS_ROW_SPACING;

    PartyScreen_DrawStatValueLine(
        PARTY_FACE_TEXT_X,
        y,
        "ATK ",
        member->stats.attack,
        BattleMember_GetEffectiveAttack(member),
        BattleMember_GetStatModPercent(member, BATTLE_STAT_ATTACK));
    y += PARTY_STATS_ROW_SPACING;

    PartyScreen_DrawStatValueLine(
        PARTY_FACE_TEXT_X,
        y,
        "DEF ",
        member->stats.defence,
        BattleMember_GetEffectiveDefence(member),
        BattleMember_GetStatModPercent(member, BATTLE_STAT_DEFENCE));
    y += PARTY_STATS_ROW_SPACING;

    PartyScreen_DrawStatValueLine(
        PARTY_FACE_TEXT_X,
        y,
        "SPD ",
        member->stats.speed,
        BattleMember_GetEffectiveSpeed(member),
        BattleMember_GetStatModPercent(member, BATTLE_STAT_SPEED));
    y += PARTY_STATS_ROW_SPACING;

    PartyScreen_DrawEvasionValueLine(PARTY_FACE_TEXT_X, y, member);
    y += PARTY_STATS_ROW_SPACING;

    cursor = 0;
    PartyScreen_AppendString(line, &cursor, "LCK ");
    PartyScreen_AppendNumber(line, &cursor, member->stats.luck);
    line[cursor] = '\0';
    Video_DrawText(PARTY_FACE_TEXT_X, y, line, PARTY_SCREEN_BODY);

    Video_DrawText(PARTY_FACE_TEXT_X, PARTY_STATS_STATUS_ROW_Y, "STS", PARTY_SCREEN_BODY);
    PartyScreen_DrawStatusSlots(member, PARTY_STATUS_ICONS_X, PARTY_STATS_STATUS_ROW_Y);

    if (highlightField >= 0 && highlightField < PARTY_STAT_FIELD_COUNT)
    {
        PartyScreen_DrawStatFieldHighlight((PartyStatField)highlightField);
    }
}

static BattleElementId PartyScreen_GetEquippedMoveElement(const BattleMember *member, int slot)
{
    const MoveData *moveData;

    if (!member || member->equippedMoves[slot] == MOVE_NONE)
    {
        return BATTLE_ELEMENT_NONE;
    }

    moveData = MoveData_Get(member->equippedMoves[slot]);
    if (!moveData || !ElementDatabase_IsValid(moveData->element))
    {
        return BATTLE_ELEMENT_NONE;
    }
    return moveData->element;
}

/* Mode 3 icon placeholders driven by MoveData.element; replace with sprites/OAM later. */
static void PartyScreen_DrawMoveElementIconForElement(int x, int y, BattleElementId element)
{
    u16 fill = ElementDatabase_GetIconFillColor(element);
    u16 border = ElementDatabase_GetIconBorderColor(element);

    Video_DrawRect(x, y, PARTY_MOVE_ICON_SIZE, PARTY_MOVE_ICON_SIZE, fill);
    Video_DrawRect(x, y, PARTY_MOVE_ICON_SIZE, 1, border);
    Video_DrawRect(x, y + PARTY_MOVE_ICON_SIZE - 1, PARTY_MOVE_ICON_SIZE, 1, border);
    Video_DrawRect(x, y, 1, PARTY_MOVE_ICON_SIZE, border);
    Video_DrawRect(x + PARTY_MOVE_ICON_SIZE - 1, y, 1, PARTY_MOVE_ICON_SIZE, border);
    Video_DrawRect(x + 2, y + 2, 4, 4, border);
}

static void PartyScreen_DrawMoveElementIconEmpty(int x, int y)
{
    Video_DrawRect(x, y, PARTY_MOVE_ICON_SIZE, PARTY_MOVE_ICON_SIZE, RGB15(8, 8, 12));
    Video_DrawRect(x, y, PARTY_MOVE_ICON_SIZE, 1, RGB15(14, 14, 18));
    Video_DrawRect(x, y + PARTY_MOVE_ICON_SIZE - 1, PARTY_MOVE_ICON_SIZE, 1, RGB15(14, 14, 18));
    Video_DrawRect(x, y, 1, PARTY_MOVE_ICON_SIZE, RGB15(14, 14, 18));
    Video_DrawRect(
        x + PARTY_MOVE_ICON_SIZE - 1,
        y,
        1,
        PARTY_MOVE_ICON_SIZE,
        RGB15(14, 14, 18));
}

static void PartyScreen_DrawMoveRowHighlight(int slot)
{
    int rowY = PartyScreen_GetMoveRowY(slot);
    int x = PARTY_MOVE_ROW_HIGHLIGHT_X;
    int y = rowY - 1;
    int w = PARTY_MOVE_ROW_HIGHLIGHT_W;
    int h = PARTY_MOVE_ROW_HIGHLIGHT_H;

    Video_DrawRect(x, y, w, 1, PARTY_PLATE_BORDER_SEL);
    Video_DrawRect(x, y + h - 1, w, 1, PARTY_PLATE_BORDER_SEL);
    Video_DrawRect(x, y, 1, h, PARTY_PLATE_BORDER_SEL);
    Video_DrawRect(x + w - 1, y, 1, h, PARTY_PLATE_BORDER_SEL);
}

static void PartyScreen_DrawMoveRow(
    int slot,
    int rowY,
    const BattleMember *member)
{
    char nameLine[12];
    char scLine[8];
    int cursor;
    const MoveData *moveData;

    if (member->equippedMoves[slot] != MOVE_NONE)
    {
        moveData = MoveData_Get(member->equippedMoves[slot]);
        PartyScreen_DrawMoveElementIconForElement(
            PARTY_MOVE_ICON_X,
            rowY,
            PartyScreen_GetEquippedMoveElement(member, slot));
        if (moveData && moveData->name)
        {
            PartyScreen_CopyTruncated(nameLine, 10, moveData->name);
        }
        else
        {
            PartyScreen_CopyTruncated(nameLine, 10, "Move");
        }
        Video_DrawText(PARTY_MOVE_NAME_X, rowY, nameLine, PARTY_SCREEN_BODY);

        cursor = 0;
        PartyScreen_AppendString(scLine, &cursor, "SC");
        PartyScreen_AppendNumber(scLine, &cursor, member->moveSc[slot]);
        scLine[cursor] = '\0';
        Video_DrawText(PARTY_MOVE_SC_X, rowY, scLine, PARTY_SCREEN_BODY);
    }
    else
    {
        PartyScreen_DrawMoveElementIconEmpty(PARTY_MOVE_ICON_X, rowY);
        Video_DrawText(PARTY_MOVE_NAME_X, rowY, "EMPTY", PARTY_SCREEN_MUTED);
    }
}

static const char *PartyScreen_GetMoveDetailDescriptionText(const MoveData *moveData)
{
    if (moveData && moveData->description && moveData->description[0] != '\0')
    {
        return moveData->description;
    }
    return "No description available.";
}

static void PartyScreen_GetMoveDetailTitleLine(
    char *dest,
    int maxLen,
    MoveId moveId,
    const MoveData *moveData)
{
    char namePart[24];
    int cursor = 0;
    int nameMax;
    const char *suffix;

    int damagePower;

    if (moveId == MOVE_NONE || !moveData)
    {
        PartyScreen_CopyTruncated(dest, maxLen, "EMPTY");
        return;
    }

    damagePower = MoveData_GetDamagePower(moveData);
    if (damagePower > 0)
    {
        suffix = " - Power ";
    }
    else
    {
        suffix = " - Supporting";
    }

    nameMax = maxLen - 1;
    {
        int suffixLen = 0;
        while (suffix[suffixLen])
        {
            suffixLen++;
        }
        if (damagePower > 0)
        {
            int powerDigits = 1;
            if (damagePower >= 10)
            {
                powerDigits = 2;
            }
            if (damagePower >= 100)
            {
                powerDigits = 3;
            }
            suffixLen += powerDigits;
        }
        nameMax = maxLen - suffixLen - 1;
        if (nameMax < 1)
        {
            nameMax = 1;
        }
    }

    if (moveData->name)
    {
        PartyScreen_CopyTruncated(namePart, nameMax + 1, moveData->name);
    }
    else
    {
        PartyScreen_CopyTruncated(namePart, nameMax + 1, "Move");
    }

    PartyScreen_AppendString(dest, &cursor, namePart);
    PartyScreen_AppendString(dest, &cursor, suffix);
    if (damagePower > 0)
    {
        PartyScreen_AppendNumber(dest, &cursor, damagePower);
    }
    if (cursor < maxLen)
    {
        dest[cursor] = '\0';
    }
    else
    {
        dest[maxLen - 1] = '\0';
    }
}

static void PartyScreen_WrapDescriptionText(
    const char *text,
    char lines[][PARTY_MOVE_DESC_CHARS_PER_LINE + 1],
    int maxLines,
    int *outLineCount)
{
    /* Used for move detail body; scroll input disabled while text fits (see Update). */
    int lineIndex = 0;
    int lineCursor = 0;
    int i = 0;

    if (!text || !outLineCount || maxLines <= 0)
    {
        if (outLineCount)
        {
            *outLineCount = 0;
        }
        return;
    }

    lines[0][0] = '\0';

    while (text[i] && lineIndex < maxLines)
    {
        while (text[i] == ' ')
        {
            i++;
        }
        if (!text[i])
        {
            break;
        }

        {
            int wordStart = i;
            int wordLen = 0;

            while (text[i] && text[i] != ' ')
            {
                i++;
            }
            wordLen = i - wordStart;

            while (wordLen > 0 && lineIndex < maxLines)
            {
                int chunkLen = wordLen;
                int needsSpace = 0;

                if (lineCursor > 0)
                {
                    if (lineCursor >= PARTY_MOVE_DESC_CHARS_PER_LINE)
                    {
                        lines[lineIndex][lineCursor] = '\0';
                        lineIndex++;
                        lineCursor = 0;
                        if (lineIndex >= maxLines)
                        {
                            break;
                        }
                        lines[lineIndex][0] = '\0';
                    }
                    else
                    {
                        needsSpace = 1;
                    }
                }

                if (needsSpace)
                {
                    if (lineCursor + 1 + chunkLen > PARTY_MOVE_DESC_CHARS_PER_LINE)
                    {
                        lines[lineIndex][lineCursor] = '\0';
                        lineIndex++;
                        lineCursor = 0;
                        if (lineIndex >= maxLines)
                        {
                            break;
                        }
                        lines[lineIndex][0] = '\0';
                        continue;
                    }
                    lines[lineIndex][lineCursor++] = ' ';
                }

                if (lineCursor + chunkLen > PARTY_MOVE_DESC_CHARS_PER_LINE)
                {
                    chunkLen = PARTY_MOVE_DESC_CHARS_PER_LINE - lineCursor;
                }

                {
                    int c;
                    for (c = 0; c < chunkLen; c++)
                    {
                        lines[lineIndex][lineCursor++] = text[wordStart + c];
                    }
                }
                lines[lineIndex][lineCursor] = '\0';

                wordStart += chunkLen;
                wordLen -= chunkLen;
            }
        }
    }

    if (lineCursor > 0 && lineIndex < maxLines)
    {
        lines[lineIndex][lineCursor] = '\0';
        lineIndex++;
    }

    *outLineCount = lineIndex;
}

static int PartyScreen_GetMoveDescriptionMaxScroll(void)
{
    int maxScroll = s_moveDescLineCount - PARTY_MOVE_DESC_VISIBLE_LINES;
    if (maxScroll < 0)
    {
        return 0;
    }
    return maxScroll;
}

static void PartyScreen_ClampMoveDescriptionScroll(void)
{
    int maxScroll = PartyScreen_GetMoveDescriptionMaxScroll();

    if (s_moveDescriptionScroll > maxScroll)
    {
        s_moveDescriptionScroll = maxScroll;
    }
}

static void PartyScreen_RebuildMoveDescriptionLines(const BattleState *state)
{
    const BattleMember *member;
    const MoveData *moveData;
    const char *descText;
    int slot;

    member = PartyScreen_GetSelectedMember(state);
    slot = s_selectedMoveSlot;
    if (slot < 0 || slot >= BATTLE_MEMBER_EQUIPPED_MOVE_SLOT_COUNT)
    {
        slot = 0;
    }

    if (!member || member->equippedMoves[slot] == MOVE_NONE)
    {
        s_moveDescLineCount = 1;
        PartyScreen_CopyTruncated(
            s_moveDescLines[0],
            PARTY_MOVE_DESC_CHARS_PER_LINE + 1,
            "No move equipped in this slot.");
        s_moveDescLines[1][0] = '\0';
    }
    else
    {
        moveData = MoveData_Get(member->equippedMoves[slot]);
        descText = PartyScreen_GetMoveDetailDescriptionText(moveData);
        PartyScreen_WrapDescriptionText(
            descText,
            s_moveDescLines,
            PARTY_MOVE_DESC_MAX_LINES,
            &s_moveDescLineCount);
    }

    PartyScreen_ClampMoveDescriptionScroll();
}

static void PartyScreen_DrawTopLeftMovesFaceplate(const BattleMember *member, int highlightSlot)
{
    int slot;
    int y;

    Video_DrawText(PARTY_FACE_TITLE_X_NARROW, PARTY_MOVES_TITLE_Y, "MOVES", PARTY_SCREEN_TITLE);

    for (slot = 0; slot < BATTLE_MEMBER_EQUIPPED_MOVE_SLOT_COUNT; slot++)
    {
        y = PartyScreen_GetMoveRowY(slot);
        PartyScreen_DrawMoveRow(slot, y, member);
    }

    if (highlightSlot >= 0 && highlightSlot < BATTLE_MEMBER_EQUIPPED_MOVE_SLOT_COUNT)
    {
        PartyScreen_DrawMoveRowHighlight(highlightSlot);
    }
}

static void PartyScreen_DrawTopLeftFaceplateBrowse(const BattleState *state)
{
    const BattleMember *member;
    int battleSlot;
    int isAlive;

    member = PartyScreen_GetSelectedMember(state);
    if (!member)
    {
        Video_DrawText(PARTY_PANEL_LEFT_X + 8, PARTY_TOP_Y0 + 40, "NO PARTY", PARTY_SCREEN_MUTED);
        return;
    }

    battleSlot = s_partyEntries[s_selectedPartyIndex].battleSlot;
    isAlive = BattleState_IsAliveOccupiedSlot(state, BATTLE_SIDE_PLAYER, battleSlot);

    switch (s_selectedFaceplate)
    {
    case PARTY_FACEPLATE_EQUIPMENT:
        PartyScreen_DrawTopLeftEquipmentFaceplate(state, member, isAlive, -1);
        break;
    case PARTY_FACEPLATE_STATS:
        PartyScreen_DrawTopLeftStatsFaceplate(member, -1);
        break;
    case PARTY_FACEPLATE_MOVES:
        PartyScreen_DrawTopLeftMovesFaceplate(member, -1);
        break;
    default:
        break;
    }
}

static void PartyScreen_DrawTopLeftEquipmentDetail(const BattleState *state)
{
    const BattleMember *member;
    int battleSlot;
    int isAlive;

    member = PartyScreen_GetSelectedMember(state);
    if (!member)
    {
        Video_DrawText(PARTY_PANEL_LEFT_X + 8, PARTY_TOP_Y0 + 40, "NO PARTY", PARTY_SCREEN_MUTED);
        return;
    }

    battleSlot = s_partyEntries[s_selectedPartyIndex].battleSlot;
    isAlive = BattleState_IsAliveOccupiedSlot(state, BATTLE_SIDE_PLAYER, battleSlot);
    PartyScreen_DrawTopLeftEquipmentFaceplate(
        state,
        member,
        isAlive,
        (int)s_selectedEquipmentSlot);
}

static void PartyScreen_DrawTopLeftStatsDetail(const BattleState *state)
{
    const BattleMember *member;

    member = PartyScreen_GetSelectedMember(state);
    if (!member)
    {
        Video_DrawText(PARTY_PANEL_LEFT_X + 8, PARTY_TOP_Y0 + 40, "NO PARTY", PARTY_SCREEN_MUTED);
        return;
    }

    PartyScreen_DrawTopLeftStatsFaceplate(member, (int)s_selectedStatField);
}

static void PartyScreen_DrawTopLeftMovesDetail(const BattleState *state)
{
    const BattleMember *member;

    member = PartyScreen_GetSelectedMember(state);
    if (!member)
    {
        Video_DrawText(PARTY_PANEL_LEFT_X + 8, PARTY_TOP_Y0 + 40, "NO PARTY", PARTY_SCREEN_MUTED);
        return;
    }

    PartyScreen_DrawTopLeftMovesFaceplate(member, s_selectedMoveSlot);
}

static void PartyScreen_DrawTopLeftFaceplateDetail(const BattleState *state)
{
    if (s_selectedFaceplate == PARTY_FACEPLATE_STATS)
    {
        PartyScreen_DrawTopLeftStatsDetail(state);
    }
    else if (s_selectedFaceplate == PARTY_FACEPLATE_MOVES)
    {
        PartyScreen_DrawTopLeftMovesDetail(state);
    }
    else
    {
        PartyScreen_DrawTopLeftEquipmentDetail(state);
    }
}

static void PartyScreen_DrawTopLeftPanel(const BattleState *state)
{
    if (s_partyUiMode == PARTY_UI_MODE_FACEPLATE_DETAIL)
    {
        PartyScreen_DrawTopLeftFaceplateDetail(state);
    }
    else
    {
        PartyScreen_DrawTopLeftFaceplateBrowse(state);
    }
}

static void PartyScreen_DrawDescriptionFaceplateBrowse(void)
{
    switch (s_selectedFaceplate)
    {
    case PARTY_FACEPLATE_EQUIPMENT:
        Video_DrawText(PARTY_SHELL_X + 8, PARTY_DESC_TITLE_Y, "EQUIPMENT", PARTY_SCREEN_TITLE);
        Video_DrawText(
            PARTY_SHELL_X + 8,
            PARTY_DESC_BODY_Y,
            "View battle gear placeholders.",
            PARTY_SCREEN_BODY);
        Video_DrawText(
            PARTY_SHELL_X + 8,
            PARTY_DESC_CONTROLS_Y,
            PARTY_CTRL_BACK_ONLY,
            PARTY_SCREEN_FOOTER);
        return;
    case PARTY_FACEPLATE_STATS:
        Video_DrawText(PARTY_SHELL_X + 8, PARTY_DESC_TITLE_Y, "STATS", PARTY_SCREEN_TITLE);
        Video_DrawText(
            PARTY_SHELL_X + 8,
            PARTY_DESC_BODY_Y,
            "View current battle stats.",
            PARTY_SCREEN_BODY);
        Video_DrawText(
            PARTY_SHELL_X + 8,
            PARTY_DESC_CONTROLS_Y,
            PARTY_CTRL_BACK_ONLY,
            PARTY_SCREEN_FOOTER);
        return;
    case PARTY_FACEPLATE_MOVES:
        Video_DrawText(PARTY_SHELL_X + 8, PARTY_DESC_TITLE_Y, "MOVES", PARTY_SCREEN_TITLE);
        Video_DrawText(
            PARTY_SHELL_X + 8,
            PARTY_DESC_BODY_Y,
            "View equipped moves and SC.",
            PARTY_SCREEN_BODY);
        Video_DrawText(
            PARTY_SHELL_X + 8,
            PARTY_DESC_CONTROLS_Y,
            PARTY_CTRL_BACK_ONLY,
            PARTY_SCREEN_FOOTER);
        return;
    default:
        break;
    }
    Video_DrawText(PARTY_SHELL_X + 8, PARTY_DESC_CONTROLS_Y, PARTY_CTRL_BACK_ONLY, PARTY_SCREEN_FOOTER);
}

static void PartyScreen_DrawDescriptionEquipmentDetail(const BattleState *state)
{
    const BattleMember *member = PartyScreen_GetSelectedMember(state);
    const char *detailName = "Unknown";

    if (member)
    {
        PartyScreen_GetEquipmentDetailName(member, s_selectedEquipmentSlot, &detailName);
    }

    switch (s_selectedEquipmentSlot)
    {
    case PARTY_EQUIPMENT_WEAPON:
        Video_DrawText(PARTY_SHELL_X + 8, PARTY_DESC_TITLE_Y, "WEAPON", PARTY_SCREEN_TITLE);
        Video_DrawText(PARTY_SHELL_X + 8, PARTY_DESC_BODY_Y, detailName, PARTY_SCREEN_BODY);
        break;
    case PARTY_EQUIPMENT_ARMOR:
        Video_DrawText(PARTY_SHELL_X + 8, PARTY_DESC_TITLE_Y, "ARMOR", PARTY_SCREEN_TITLE);
        Video_DrawText(PARTY_SHELL_X + 8, PARTY_DESC_BODY_Y, detailName, PARTY_SCREEN_BODY);
        break;
    case PARTY_EQUIPMENT_ACCESSORY:
        Video_DrawText(PARTY_SHELL_X + 8, PARTY_DESC_TITLE_Y, "ACCESSORY", PARTY_SCREEN_TITLE);
        Video_DrawText(PARTY_SHELL_X + 8, PARTY_DESC_BODY_Y, detailName, PARTY_SCREEN_BODY);
        break;
    default:
        break;
    }
    Video_DrawText(
        PARTY_SHELL_X + 8,
        PARTY_DESC_CONTROLS_Y,
        PARTY_CTRL_BACK_ONLY,
        PARTY_SCREEN_FOOTER);
}

static void PartyScreen_DrawDescriptionStatsDetail(const BattleState *state)
{
    const BattleMember *member;
    char body[PARTY_LINE_MAX];
    int cursor;

    member = PartyScreen_GetSelectedMember(state);

    switch (s_selectedStatField)
    {
    case PARTY_STAT_FIELD_HP:
        Video_DrawText(PARTY_SHELL_X + 8, PARTY_DESC_TITLE_Y, "HEALTH", PARTY_SCREEN_TITLE);
        if (member)
        {
            cursor = 0;
            PartyScreen_AppendString(body, &cursor, "HP ");
            PartyScreen_AppendNumber(body, &cursor, member->currentHp);
            PartyScreen_AppendChar(body, &cursor, '/');
            PartyScreen_AppendNumber(body, &cursor, member->maxHp);
            PartyScreen_AppendString(body, &cursor, ". 0=down.");
            body[cursor] = '\0';
            Video_DrawText(PARTY_SHELL_X + 8, PARTY_DESC_BODY_Y, body, PARTY_SCREEN_BODY);
        }
        else
        {
            Video_DrawText(
                PARTY_SHELL_X + 8,
                PARTY_DESC_BODY_Y,
                "Current and maximum HP.",
                PARTY_SCREEN_BODY);
        }
        break;
    case PARTY_STAT_FIELD_ATTACK:
        Video_DrawText(PARTY_SHELL_X + 8, PARTY_DESC_TITLE_Y, "ATTACK", PARTY_SCREEN_TITLE);
        Video_DrawText(
            PARTY_SHELL_X + 8,
            PARTY_DESC_BODY_Y,
            "Affects physical/basic hit damage.",
            PARTY_SCREEN_BODY);
        break;
    case PARTY_STAT_FIELD_DEFENCE:
        Video_DrawText(PARTY_SHELL_X + 8, PARTY_DESC_TITLE_Y, "DEFENCE", PARTY_SCREEN_TITLE);
        Video_DrawText(
            PARTY_SHELL_X + 8,
            PARTY_DESC_BODY_Y,
            "Reduces incoming damage.",
            PARTY_SCREEN_BODY);
        break;
    case PARTY_STAT_FIELD_SPEED:
        Video_DrawText(PARTY_SHELL_X + 8, PARTY_DESC_TITLE_Y, "SPEED", PARTY_SCREEN_TITLE);
        Video_DrawText(
            PARTY_SHELL_X + 8,
            PARTY_DESC_BODY_Y,
            "Affects run chance and turn order.",
            PARTY_SCREEN_BODY);
        break;
    case PARTY_STAT_FIELD_EVASION:
        Video_DrawText(PARTY_SHELL_X + 8, PARTY_DESC_TITLE_Y, "EVASION", PARTY_SCREEN_TITLE);
        if (member)
        {
            int modPercent = BattleMember_GetStatModPercent(member, BATTLE_STAT_EVASIVENESS);

            cursor = 0;
            PartyScreen_AppendString(body, &cursor, "Dodge chance. Eva ");
            if (modPercent != 0)
            {
                PartyScreen_AppendSignedNumber(body, &cursor, modPercent);
                PartyScreen_AppendChar(body, &cursor, '%');
            }
            else
            {
                PartyScreen_AppendChar(body, &cursor, '%');
                PartyScreen_AppendNumber(body, &cursor, member->stats.evasiveness);
            }
            body[cursor] = '\0';
            Video_DrawText(
                PARTY_SHELL_X + 8,
                PARTY_DESC_BODY_Y,
                body,
                modPercent != 0
                    ? (modPercent > 0 ? PARTY_SCREEN_STAT_INCREASE : PARTY_SCREEN_STAT_DECREASE)
                    : PARTY_SCREEN_BODY);
        }
        else
        {
            Video_DrawText(
                PARTY_SHELL_X + 8,
                PARTY_DESC_BODY_Y,
                "Chance to dodge incoming hits.",
                PARTY_SCREEN_BODY);
        }
        break;
    case PARTY_STAT_FIELD_LUCK:
        Video_DrawText(PARTY_SHELL_X + 8, PARTY_DESC_TITLE_Y, "LUCK", PARTY_SCREEN_TITLE);
        Video_DrawText(
            PARTY_SHELL_X + 8,
            PARTY_DESC_BODY_Y,
            "Affects crits and def negation.",
            PARTY_SCREEN_BODY);
        break;
    case PARTY_STAT_FIELD_STATUS:
        Video_DrawText(PARTY_SHELL_X + 8, PARTY_DESC_TITLE_Y, "STATUS", PARTY_SCREEN_TITLE);
        if (!member || !PartyScreen_HasAnyStatus(member))
        {
            Video_DrawText(
                PARTY_SHELL_X + 8,
                PARTY_DESC_BODY_Y,
                "No active statuses.",
                PARTY_SCREEN_BODY);
        }
        else
        {
            cursor = 0;
            PartyScreen_BuildStatusSummary(member, body, (int)sizeof(body));
            Video_DrawText(PARTY_SHELL_X + 8, PARTY_DESC_BODY_Y, body, PARTY_SCREEN_BODY);
        }
        break;
    default:
        break;
    }
    Video_DrawText(
        PARTY_SHELL_X + 8,
        PARTY_DESC_CONTROLS_Y,
        PARTY_CTRL_BACK_ONLY,
        PARTY_SCREEN_FOOTER);
}

static void PartyScreen_DrawDescriptionMovesDetail(const BattleState *state)
{
    const BattleMember *member;
    const MoveData *moveData;
    char titleLine[PARTY_MOVE_DESC_TITLE_MAX + 1];
    int slot;
    int line;
    int visibleLine;
    int y;

    member = PartyScreen_GetSelectedMember(state);
    slot = s_selectedMoveSlot;
    if (slot < 0 || slot >= BATTLE_MEMBER_EQUIPPED_MOVE_SLOT_COUNT)
    {
        slot = 0;
    }

    PartyScreen_RebuildMoveDescriptionLines(state);

    if (!member || member->equippedMoves[slot] == MOVE_NONE)
    {
        Video_DrawText(PARTY_SHELL_X + 8, PARTY_DESC_TITLE_Y, "EMPTY", PARTY_SCREEN_TITLE);
    }
    else
    {
        moveData = MoveData_Get(member->equippedMoves[slot]);
        PartyScreen_GetMoveDetailTitleLine(
            titleLine,
            (int)sizeof(titleLine),
            member->equippedMoves[slot],
            moveData);
        Video_DrawText(PARTY_SHELL_X + 8, PARTY_DESC_TITLE_Y, titleLine, PARTY_SCREEN_TITLE);
    }

    for (visibleLine = 0; visibleLine < PARTY_MOVE_DESC_VISIBLE_LINES; visibleLine++)
    {
        line = s_moveDescriptionScroll + visibleLine;
        y = PARTY_DESC_MOVE_BODY_Y + (visibleLine * PARTY_MOVE_DESC_LINE_SPACING);
        if (line < s_moveDescLineCount)
        {
            Video_DrawText(
                PARTY_SHELL_X + 8,
                y,
                s_moveDescLines[line],
                PARTY_SCREEN_BODY);
        }
    }
}

static void PartyScreen_DrawDescriptionFaceplateDetail(const BattleState *state)
{
    if (s_selectedFaceplate == PARTY_FACEPLATE_STATS)
    {
        PartyScreen_DrawDescriptionStatsDetail(state);
    }
    else if (s_selectedFaceplate == PARTY_FACEPLATE_MOVES)
    {
        PartyScreen_DrawDescriptionMovesDetail(state);
    }
    else
    {
        PartyScreen_DrawDescriptionEquipmentDetail(state);
    }
}

static void PartyScreen_DrawDescriptionPanel(const BattleState *state)
{
    if (!state || s_partyCount <= 0)
    {
        Video_DrawText(PARTY_SHELL_X + 8, PARTY_DESC_TITLE_Y, "NO PARTY", PARTY_SCREEN_TITLE);
        Video_DrawText(PARTY_SHELL_X + 8, PARTY_DESC_BODY_Y, "No allies deployed.", PARTY_SCREEN_BODY);
        Video_DrawText(PARTY_SHELL_X + 8, PARTY_DESC_CONTROLS_Y, PARTY_CTRL_BACK_ONLY, PARTY_SCREEN_FOOTER);
        return;
    }

    if (s_partyUiMode == PARTY_UI_MODE_FACEPLATE_DETAIL)
    {
        PartyScreen_DrawDescriptionFaceplateDetail(state);
    }
    else
    {
        PartyScreen_DrawDescriptionFaceplateBrowse();
    }
}

static void PartyScreen_DrawBackground(void)
{
    Video_DrawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, PARTY_SCREEN_BG);
    Video_DrawRect(4, 4, SCREEN_WIDTH - 8, 2, PARTY_SCREEN_BORDER);
    Video_DrawRect(4, SCREEN_HEIGHT - 6, SCREEN_WIDTH - 8, 2, PARTY_SCREEN_BORDER);
    Video_DrawRect(4, 4, 2, SCREEN_HEIGHT - 8, PARTY_SCREEN_BORDER);
    Video_DrawRect(SCREEN_WIDTH - 6, 4, 2, SCREEN_HEIGHT - 8, PARTY_SCREEN_BORDER);
}

static void PartyScreen_DrawShellFrame(void)
{
    Video_DrawRect(PARTY_SHELL_X, PARTY_SHELL_Y, PARTY_SHELL_W, PARTY_SHELL_H, PARTY_SCREEN_PANEL_FILL);
    Video_DrawRect(PARTY_SHELL_X, PARTY_SHELL_Y, PARTY_SHELL_W, 1, PARTY_SCREEN_BORDER);
    Video_DrawRect(PARTY_SHELL_X, PARTY_SHELL_Y + PARTY_SHELL_H - 1, PARTY_SHELL_W, 1, PARTY_SCREEN_BORDER);
    Video_DrawRect(PARTY_SHELL_X, PARTY_SHELL_Y, 1, PARTY_SHELL_H, PARTY_SCREEN_BORDER);
    Video_DrawRect(PARTY_SHELL_X + PARTY_SHELL_W - 1, PARTY_SHELL_Y, 1, PARTY_SHELL_H, PARTY_SCREEN_BORDER);

    Video_DrawRect(PARTY_SHELL_X, PARTY_SPLIT_Y, PARTY_SHELL_W, 1, PARTY_SCREEN_DIVIDER);
    Video_DrawRect(PARTY_DIVIDER_X, PARTY_TOP_Y0, 1, PARTY_TOP_Y1 - PARTY_TOP_Y0 + 1, PARTY_SCREEN_DIVIDER);
}

static void PartyScreen_DrawFull(const BattleState *state)
{
    PartyScreen_DrawBackground();
    PartyScreen_DrawShellFrame();
    PartyScreen_DrawTopLeftPanel(state);
    PartyScreen_DrawCarousel(state);
    PartyScreen_DrawDescriptionPanel(state);
}

static void PartyScreen_DrawSelectorRegion(const BattleState *state)
{
    Video_DrawRect(
        PARTY_SELECTOR_CLEAR_X,
        PARTY_SELECTOR_CLEAR_Y,
        PARTY_SELECTOR_CLEAR_W,
        PARTY_SELECTOR_CLEAR_H,
        PARTY_SCREEN_PANEL_FILL);
    PartyScreen_DrawCarousel(state);
}

static void PartyScreen_RestoreDescriptionPanelBorder(void)
{
    Video_DrawRect(
        PARTY_SHELL_X,
        PARTY_SHELL_Y + PARTY_SHELL_H - 1,
        PARTY_SHELL_W,
        1,
        PARTY_SCREEN_BORDER);
}

static void PartyScreen_RestoreFaceplateDivider(void)
{
    Video_DrawRect(
        PARTY_DIVIDER_X,
        PARTY_TOP_Y0,
        1,
        PARTY_TOP_Y1 - PARTY_TOP_Y0 + 1,
        PARTY_SCREEN_DIVIDER);
}

static void PartyScreen_DrawDescriptionRegion(const BattleState *state)
{
    Video_DrawRect(
        PARTY_DESC_CLEAR_X,
        PARTY_DESC_CLEAR_Y,
        PARTY_DESC_CLEAR_W,
        PARTY_DESC_CLEAR_H,
        PARTY_SCREEN_PANEL_FILL);
    PartyScreen_DrawDescriptionPanel(state);
    PartyScreen_RestoreDescriptionPanelBorder();
}

static void PartyScreen_DrawFaceplateDynamicRegion(const BattleState *state)
{
    Video_DrawRect(
        PARTY_FACEPLATE_CLEAR_X,
        PARTY_FACEPLATE_CLEAR_Y,
        PARTY_FACEPLATE_CLEAR_W,
        PARTY_FACEPLATE_CLEAR_H,
        PARTY_SCREEN_PANEL_FILL);
    PartyScreen_DrawTopLeftPanel(state);
    PartyScreen_RestoreFaceplateDivider();
}

void PartyScreen_Init(void)
{
    s_isOpen = 0;
    s_selectionInitialized = 0;
    s_partyDirty = PARTY_SCREEN_DIRTY_NONE;
    s_shellDrawn = 0;
    s_partyCount = 0;
    s_selectedPartyIndex = 0;
    PartyScreen_ResetUiMode();
    PartyScreen_ResetCircleCarouselAnimation();
}

void PartyScreen_Open(void)
{
    s_isOpen = 1;
    s_selectionInitialized = 0;
    s_shellDrawn = 0;
    s_partyDirty = PARTY_SCREEN_DIRTY_ALL;
    PartyScreen_ResetUiMode();
    PartyScreen_ResetCircleCarouselAnimation();
}

int PartyScreen_Update(BattleState *state)
{
    if (!s_isOpen)
    {
        return 0;
    }
    PartyScreen_EnsureSelectionInitialized(state);
    PartyScreen_RefreshEntries(state);

    if (s_circleCarouselAnimating)
    {
        PartyScreen_AdvanceCircleCarouselAnimation();
        return 0;
    }

    if (s_partyUiMode == PARTY_UI_MODE_FACEPLATE_DETAIL)
    {
        if (Input_JustPressed(KEY_B))
        {
            s_partyUiMode = PARTY_UI_MODE_FACEPLATE_BROWSE;
            PartyScreen_MarkFaceplateDescriptionDirty();
            return 0;
        }
        if (s_selectedFaceplate == PARTY_FACEPLATE_STATS)
        {
            if (Input_JustPressed(KEY_UP) || Input_JustPressed(KEY_LEFT))
            {
                PartyScreen_AdvanceStatField(-1);
                PartyScreen_MarkFaceplateDescriptionDirty();
                return 0;
            }
            if (Input_JustPressed(KEY_DOWN) || Input_JustPressed(KEY_RIGHT))
            {
                PartyScreen_AdvanceStatField(1);
                PartyScreen_MarkFaceplateDescriptionDirty();
                return 0;
            }
        }
        else if (s_selectedFaceplate == PARTY_FACEPLATE_MOVES)
        {
            /* Scroll input disabled while descriptions fit; helpers kept for longer text.
             * On restore: Up/Down scroll, Left/Right move, mark description-only dirty. */
            if (Input_JustPressed(KEY_UP))
            {
                PartyScreen_AdvanceMoveSlot(-1);
                PartyScreen_MarkFaceplateDescriptionDirty();
                return 0;
            }
            if (Input_JustPressed(KEY_DOWN))
            {
                PartyScreen_AdvanceMoveSlot(1);
                PartyScreen_MarkFaceplateDescriptionDirty();
                return 0;
            }
        }
        else
        {
            if (Input_JustPressed(KEY_UP) || Input_JustPressed(KEY_LEFT))
            {
                PartyScreen_AdvanceEquipmentSlot(-1);
                PartyScreen_MarkFaceplateDescriptionDirty();
                return 0;
            }
            if (Input_JustPressed(KEY_DOWN) || Input_JustPressed(KEY_RIGHT))
            {
                PartyScreen_AdvanceEquipmentSlot(1);
                PartyScreen_MarkFaceplateDescriptionDirty();
                return 0;
            }
        }
        return 0;
    }

    if (s_partyUiMode == PARTY_UI_MODE_FACEPLATE_BROWSE)
    {
        if (Input_JustPressed(KEY_B))
        {
            s_isOpen = 0;
            s_selectionInitialized = 0;
            s_shellDrawn = 0;
            s_partyDirty = PARTY_SCREEN_DIRTY_NONE;
            PartyScreen_ResetUiMode();
            PartyScreen_ResetCircleCarouselAnimation();
            return 1;
        }
        if (Input_JustPressed(KEY_A))
        {
            if (s_selectedFaceplate == PARTY_FACEPLATE_EQUIPMENT)
            {
                s_partyUiMode = PARTY_UI_MODE_FACEPLATE_DETAIL;
                s_selectedEquipmentSlot = PARTY_EQUIPMENT_WEAPON;
                PartyScreen_MarkFaceplateDescriptionDirty();
            }
            else if (s_selectedFaceplate == PARTY_FACEPLATE_STATS)
            {
                s_partyUiMode = PARTY_UI_MODE_FACEPLATE_DETAIL;
                s_selectedStatField = PARTY_STAT_FIELD_HP;
                PartyScreen_MarkFaceplateDescriptionDirty();
            }
            else if (s_selectedFaceplate == PARTY_FACEPLATE_MOVES)
            {
                s_partyUiMode = PARTY_UI_MODE_FACEPLATE_DETAIL;
                s_selectedMoveSlot = 0;
                s_moveDescriptionScroll = 0;
                PartyScreen_MarkFaceplateDescriptionDirty();
            }
            return 0;
        }
        if (Input_JustPressed(KEY_LEFT))
        {
            s_selectedFaceplate = (PartyFaceplate)(
                (int)s_selectedFaceplate + PARTY_FACEPLATE_COUNT - 1) % PARTY_FACEPLATE_COUNT;
            PartyScreen_MarkFaceplateDescriptionDirty();
            return 0;
        }
        if (Input_JustPressed(KEY_RIGHT))
        {
            s_selectedFaceplate = (PartyFaceplate)(
                (int)s_selectedFaceplate + 1) % PARTY_FACEPLATE_COUNT;
            PartyScreen_MarkFaceplateDescriptionDirty();
            return 0;
        }
        if (s_partyCount > 1)
        {
            if (Input_JustPressed(KEY_UP))
            {
                PartyScreen_StartCircleCarouselRotation(-1);
            }
            else if (Input_JustPressed(KEY_DOWN))
            {
                PartyScreen_StartCircleCarouselRotation(1);
            }
        }
        return 0;
    }

    return 0;
}

void PartyScreen_Draw(const BattleState *state)
{
    PartyScreen_EnsureSelectionInitialized(state);
    PartyScreen_RefreshEntries(state);

    if (!s_shellDrawn || (s_partyDirty & PARTY_SCREEN_DIRTY_ALL))
    {
        PartyScreen_DrawFull(state);
        s_shellDrawn = 1;
        s_partyDirty = PARTY_SCREEN_DIRTY_NONE;
        return;
    }

    if (s_partyDirty & PARTY_SCREEN_DIRTY_SELECTOR)
    {
        PartyScreen_DrawSelectorRegion(state);
    }
    if (s_partyDirty & PARTY_SCREEN_DIRTY_DESCRIPTION)
    {
        PartyScreen_DrawDescriptionRegion(state);
    }
    if (s_partyDirty & PARTY_SCREEN_DIRTY_FACEPLATE)
    {
        PartyScreen_DrawFaceplateDynamicRegion(state);
    }

    s_partyDirty = PARTY_SCREEN_DIRTY_NONE;
}
