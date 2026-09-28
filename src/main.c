#include "core/system.h"
#include "core/input.h"
#include "core/video.h"
#include "overworld/overworld.h"
#include "dialogue/dialogue_demo.h"
#include "dialogue/dialogue_host.h"
#include "dialogue/dialogue_scene.h"
#include "dialogue/dialogue_scene_runner.h"
#include "battle/battle.h"
#include "battle/battle_member.h"
#include "battle/battle_move.h"
#include "battle/custom_battle_setup.h"
#include "battle/stat_curve.h"
#include "data/element_database.h"
#include "data/member_database.h"

#define CUSTOM_BATTLE_MENU_PROTAG_WIDTH 66
#define CUSTOM_BATTLE_MENU_AKSIL_WIDTH 64
#define CUSTOM_BATTLE_MENU_MAREN_WIDTH 64
#define CUSTOM_BATTLE_MENU_TUTSIL_WIDTH 66
#define CUSTOM_BATTLE_MENU_BADGER_WIDTH 64
#define CUSTOM_BATTLE_MENU_SPRITE_KEY_COLOR RGB15(31, 0, 31)
#define CUSTOM_ALLY_MOVE_LIST_VISIBLE_ROWS 4
#define CUSTOM_ALLY_MOVE_LIST_ROW_HEIGHT 18
#define CUSTOM_ALLY_MOVE_LIST_FIRST_Y 42
#define APP_LEVEL_REPEAT_INITIAL_DELAY 18
#define APP_LEVEL_REPEAT_INTERVAL 4

typedef struct AppLevelRepeatState {
    int direction;
    int counter;
} AppLevelRepeatState;

extern const u16 protagonist_front_spriteBitmap[];
extern const u16 aksil_front_spriteBitmap[];
extern const u16 maren_front_spriteBitmap[];
extern const u16 tutsil_front_spriteBitmap[];
extern const u16 electric_armor_badger_spriteBitmap[];

static void App_FieldDialogueLock(void *ctx)
{
    (void)ctx;
    Overworld_LockForDialogue();
}

static void App_FieldDialogueUnlock(void *ctx)
{
    (void)ctx;
    Overworld_UnlockAfterDialogue();
}

static void App_FieldDialogueEvent(u16 eventId, void *ctx)
{
    (void)eventId;
    (void)ctx;
}

static void App_FieldDialogueFinished(DialogueResult result, void *ctx)
{
    (void)result;
    (void)ctx;
}

static int App_StartFieldDialogue(TextSpeed speed)
{
    DialogueHost host;

    host.overlayMode = DIALOGUE_OVERLAY_MODE_FIELD;
    host.lock = App_FieldDialogueLock;
    host.unlock = App_FieldDialogueUnlock;
    host.on_event = App_FieldDialogueEvent;
    host.on_finished = App_FieldDialogueFinished;
    host.ctx = 0;
    Dialogue_SetNextMessageSpeed(speed);
    return Dialogue_Start(DialogueScene_GetSharedConversation(), &host, speed);
}

typedef enum AppMode {
    APP_MODE_MAIN_MENU = 0,
    APP_MODE_OVERWORLD,
    APP_MODE_DIALOGUE_DEMO,
    APP_MODE_OPTIONS,
    APP_MODE_CUSTOM_BATTLE,
    APP_MODE_CUSTOM_BATTLE_REVIEW,
    APP_MODE_CUSTOM_BATTLE_DELETE,
    APP_MODE_CUSTOM_BATTLE_DELETE_CONFIRM,
    APP_MODE_CUSTOM_BATTLE_SAVE,
    APP_MODE_CUSTOM_BATTLE_SAVE_CONFIRM,
    APP_MODE_CUSTOM_BATTLE_LOAD,
    APP_MODE_CUSTOM_BATTLE_LOAD_CONFIRM,
    APP_MODE_CUSTOM_BATTLE_ALLY_SELECT,
    APP_MODE_CUSTOM_BATTLE_ALLY_PREVIEW,
    APP_MODE_CUSTOM_BATTLE_ALLY_CONFIRM,
    APP_MODE_CUSTOM_BATTLE_ALLY_CHOOSE_MOVES,
    APP_MODE_CUSTOM_BATTLE_ALLY_MOVE_LIST,
    APP_MODE_CUSTOM_BATTLE_ALLY_PLACE,
    APP_MODE_CUSTOM_BATTLE_ENEMY_SELECT,
    APP_MODE_CUSTOM_BATTLE_ENEMY_PREVIEW,
    APP_MODE_CUSTOM_BATTLE_ENEMY_CONFIRM,
    APP_MODE_CUSTOM_BATTLE_ENEMY_PLACE,
    APP_MODE_CUSTOM_BATTLE_MESSAGE,
    APP_MODE_BATTLE
} AppMode;

typedef enum MenuOption {
    MENU_OPTION_PETALBURG_SHOWDOWN = 0,
    MENU_OPTION_DUMMY_TEST_BATTLE,
    MENU_OPTION_OVERWORLD_TEST,
    MENU_OPTION_DIALOGUE_DEMO,
    MENU_OPTION_CUSTOM_BATTLE,
    MENU_OPTION_OPTIONS,
    MENU_OPTION_COUNT
} MenuOption;

typedef enum OptionsRow {
    OPTIONS_ROW_TEXT_SPEED = 0,
    OPTIONS_ROW_TUTSIL_DIFFICULTY,
    OPTIONS_ROW_COUNT
} OptionsRow;

typedef enum BattleReturnTarget {
    BATTLE_RETURN_MAIN_MENU = 0,
    BATTLE_RETURN_OVERWORLD
} BattleReturnTarget;

#define APP_MENU_BG_COLOR RGB15(3, 3, 6)
#define APP_MAIN_MENU_ROW_X 40
#define APP_MAIN_MENU_ROW_W 180
#define APP_MAIN_MENU_ROW_H 8
#define APP_OPTIONS_ROW_X 22
#define APP_OPTIONS_ROW_W 196
#define APP_OPTIONS_ROW_H 8

typedef enum CustomBattleOption {
    CUSTOM_BATTLE_OPTION_ADD_ALLY = 0,
    CUSTOM_BATTLE_OPTION_ADD_ENEMY,
    CUSTOM_BATTLE_OPTION_DELETE_UNIT,
    CUSTOM_BATTLE_OPTION_REVIEW_BATTLE,
    CUSTOM_BATTLE_OPTION_SAVE_BATTLE,
    CUSTOM_BATTLE_OPTION_LOAD_BATTLE,
    CUSTOM_BATTLE_OPTION_START_BATTLE,
    CUSTOM_BATTLE_OPTION_COUNT
} CustomBattleOption;

typedef enum CustomEnemyOption {
    CUSTOM_ENEMY_OPTION_TUTSIL = 0,
    CUSTOM_ENEMY_OPTION_ARMOR_BADGER,
    CUSTOM_ENEMY_OPTION_SENTIENT_CACTUS,
    CUSTOM_ENEMY_OPTION_COUNT
} CustomEnemyOption;

typedef enum CustomAllyOption {
    CUSTOM_ALLY_OPTION_PROTAG = 0,
    CUSTOM_ALLY_OPTION_AKSIL,
    CUSTOM_ALLY_OPTION_MAREN,
    CUSTOM_ALLY_OPTION_CONALL,
    CUSTOM_ALLY_OPTION_REIKA,
    CUSTOM_ALLY_OPTION_COUNT
} CustomAllyOption;

#define CUSTOM_ALLY_CHOOSE_MOVE_LOOKS_GOOD BATTLE_MEMBER_EQUIPPED_MOVE_SLOT_COUNT

static const MoveId s_protagLearnableMoves[] = {
    MOVE_DIRECT_ATTACK,
    MOVE_PHANTOM_PHIST,
    MOVE_HOLD_BACK,
    MOVE_HAMMER_FIST,
    MOVE_INFLUENCE,
    MOVE_PEP_TALK,
    MOVE_TIPPING_THE_SCALES,
    MOVE_HAIL_MARY,
    MOVE_TEAM_FOCUS,
    MOVE_LAST_CHANCE,
    MOVE_RAM
};

static const MoveId s_aksilLearnableMoves[] = {
    MOVE_JAB,
    MOVE_VORTEX_ESCAPE,
    MOVE_FIERY_STRIKE,
    MOVE_FAKIE,
    MOVE_ALLY_SWAP,
    MOVE_FIERY_BLOOD,
    MOVE_IMMENSE_SUPPORT
};

static const MoveId s_marenLearnableMoves[] = {
    MOVE_KICK,
    MOVE_BUBBLE,
    MOVE_TIDE,
    MOVE_SPOUT,
    MOVE_WIDE_THROW,
    MOVE_VERTIGO,
    MOVE_VEIL
};

static void App_ResetLevelRepeatState(AppLevelRepeatState *state)
{
    if (!state)
    {
        return;
    }
    state->direction = 0;
    state->counter = 0;
}

static int App_TryAdjustPreviewLevel(int *level, int direction, int minLevel, int maxLevel)
{
    if (!level || direction == 0)
    {
        return 0;
    }
    if (direction < 0)
    {
        if (*level <= minLevel)
        {
            return 0;
        }
        (*level)--;
        return 1;
    }
    if (*level >= maxLevel)
    {
        return 0;
    }
    (*level)++;
    return 1;
}

static int App_TickLevelHoldRepeat(
    AppLevelRepeatState *state,
    int *level,
    int minLevel,
    int maxLevel)
{
    int direction = 0;
    int changed = 0;

    if (!state || !level)
    {
        return 0;
    }
    if (Input_IsPressed(KEY_LEFT))
    {
        direction = -1;
    }
    else if (Input_IsPressed(KEY_RIGHT))
    {
        direction = 1;
    }

    if (direction == 0)
    {
        App_ResetLevelRepeatState(state);
        return 0;
    }

    if (direction != state->direction)
    {
        state->direction = direction;
        state->counter = 0;
        return App_TryAdjustPreviewLevel(level, direction, minLevel, maxLevel);
    }

    state->counter++;
    if (state->counter == APP_LEVEL_REPEAT_INITIAL_DELAY)
    {
        changed = App_TryAdjustPreviewLevel(level, direction, minLevel, maxLevel);
    }
    else if (state->counter > APP_LEVEL_REPEAT_INITIAL_DELAY &&
             ((state->counter - APP_LEVEL_REPEAT_INITIAL_DELAY) % APP_LEVEL_REPEAT_INTERVAL) == 0)
    {
        changed = App_TryAdjustPreviewLevel(level, direction, minLevel, maxLevel);
    }
    return changed;
}

static const char *App_GetMemberTemplateShortLabel(MemberTemplateId memberTemplateId);

static const char *App_GetCustomAllyLabel(CustomAllyOption option);

static const char *App_GetCustomEnemySelectLabel(CustomEnemyOption option);

static void App_DrawMenuOption(
    int x,
    int y,
    int selected,
    const char *label)
{
    Video_DrawText(x, y, selected ? "PLAY  " : "      ", RGB15(31, 31, 0));
    Video_DrawText(x + 36, y, label, RGB15(31, 31, 31));
}

static void App_DrawCustomBattleOption(
    int x,
    int y,
    int selected,
    const char *label)
{
    Video_DrawText(
        x,
        y,
        selected ? "> " : "  ",
        selected ? RGB15(31, 31, 0) : RGB15(20, 20, 20));
    Video_DrawText(
        x + 12,
        y,
        label,
        selected ? RGB15(31, 31, 0) : RGB15(31, 31, 31));
}

static void App_DrawNumberText(int x, int y, int value, u16 color)
{
    char digits[12];
    int cursor = 0;
    int i;
    int number;

    if (value == 0)
    {
        Video_DrawText(x, y, "0", color);
        return;
    }
    if (value < 0)
    {
        digits[cursor++] = '-';
        number = -value;
    }
    else
    {
        number = value;
    }
    for (i = 0; number > 0 && i < 10; i++)
    {
        digits[cursor + i] = (char)('0' + (number % 10));
        number /= 10;
    }
    {
        int digitCount = i;
        int start = cursor;
        int end = cursor + digitCount - 1;

        while (start < end)
        {
            char temp = digits[start];
            digits[start] = digits[end];
            digits[end] = temp;
            start++;
            end--;
        }
        cursor += digitCount;
    }
    digits[cursor] = '\0';
    Video_DrawText(x, y, digits, color);
}

static const char *App_GetTextSpeedLabel(int option)
{
    switch (option)
    {
    case TEXT_SPEED_SLOW:
        return "Slow";
    case TEXT_SPEED_FAST:
        return "Fast";
    case TEXT_SPEED_NORMAL:
    default:
        return "Normal";
    }
}

static const char *App_GetTutsilDifficultyLabel(int option)
{
    switch (option)
    {
    case TUTSIL_DIFFICULTY_EASY:
        return "Easy";
    case TUTSIL_DIFFICULTY_HARD:
        return "Hard";
    case TUTSIL_DIFFICULTY_NORMAL:
    default:
        return "Normal";
    }
}

static void App_DrawOptionsRow(
    int x,
    int y,
    int selected,
    const char *label,
    const char *value)
{
    Video_DrawText(x, y, selected ? "EDIT  " : "      ", RGB15(31, 31, 0));
    Video_DrawText(x + 36, y, label, RGB15(31, 31, 31));
    Video_DrawText(x + 102, y, value, RGB15(31, 31, 31));
}

static void App_StartPetalburgShowdown(
    BattleReturnTarget returnTarget,
    TutsilDifficulty tutsilDifficulty,
    TextSpeed textSpeed,
    BorderColor borderColor,
    BattleReturnTarget *outReturnTarget)
{
    Video_EnterMode3Bitmap();
    Battle_InitWithDemoAndSettings(
        BATTLE_DEMO_PETALBURG_SHOWDOWN,
        tutsilDifficulty,
        textSpeed,
        borderColor);
    if (outReturnTarget)
    {
        *outReturnTarget = returnTarget;
    }
}

static void App_DrawMainMenuRow(MenuOption option, MenuOption selectedOption)
{
    static const int s_rowY[MENU_OPTION_COUNT] = {56, 68, 80, 92, 104, 116};
    static const char *const s_labels[MENU_OPTION_COUNT] = {
        "Petalburg Showdown",
        "Dummy Test Battle",
        "Overworld Test",
        "Dialogue Demo",
        "Custom Battle",
        "Options"
    };

    if (option >= MENU_OPTION_COUNT)
    {
        return;
    }
    Video_DrawRect(
        APP_MAIN_MENU_ROW_X,
        s_rowY[option],
        APP_MAIN_MENU_ROW_W,
        APP_MAIN_MENU_ROW_H,
        APP_MENU_BG_COLOR);
    App_DrawMenuOption(
        APP_MAIN_MENU_ROW_X,
        s_rowY[option],
        option == selectedOption,
        s_labels[option]);
}

static void App_DrawMainMenuDirtyRows(
    unsigned int dirtyRows,
    MenuOption selectedOption)
{
    int option;

    for (option = 0; option < MENU_OPTION_COUNT; option++)
    {
        if (dirtyRows & (1u << option))
        {
            App_DrawMainMenuRow((MenuOption)option, selectedOption);
        }
    }
}

static void App_DrawMainMenu(MenuOption selectedOption)
{
    int option;

    Video_Clear(APP_MENU_BG_COLOR);
    Video_DrawText(58, 32, "Project Solarfly", RGB15(31, 31, 31));

    for (option = 0; option < MENU_OPTION_COUNT; option++)
    {
        App_DrawMainMenuRow((MenuOption)option, selectedOption);
    }

    Video_DrawText(88, 140, "A: Start", RGB15(31, 31, 0));
}

static void App_DrawOptionsMenuRow(
    OptionsRow row,
    OptionsRow selectedRow,
    TextSpeed textSpeedOption,
    TutsilDifficulty tutsilDifficultyOption)
{
    static const int s_rowY[OPTIONS_ROW_COUNT] = {62, 78};
    const char *label;
    const char *value;

    if (row >= OPTIONS_ROW_COUNT)
    {
        return;
    }
    if (row == OPTIONS_ROW_TEXT_SPEED)
    {
        label = "Text Spd:";
        value = App_GetTextSpeedLabel(textSpeedOption);
    }
    else
    {
        label = "Tutsil:";
        value = App_GetTutsilDifficultyLabel(tutsilDifficultyOption);
    }

    Video_DrawRect(
        APP_OPTIONS_ROW_X,
        s_rowY[row],
        APP_OPTIONS_ROW_W,
        APP_OPTIONS_ROW_H,
        APP_MENU_BG_COLOR);
    App_DrawOptionsRow(
        APP_OPTIONS_ROW_X,
        s_rowY[row],
        row == selectedRow,
        label,
        value);
}

static void App_DrawOptionsMenuDirtyRows(
    unsigned int dirtyRows,
    OptionsRow selectedRow,
    TextSpeed textSpeedOption,
    TutsilDifficulty tutsilDifficultyOption)
{
    int row;

    for (row = 0; row < OPTIONS_ROW_COUNT; row++)
    {
        if (dirtyRows & (1u << row))
        {
            App_DrawOptionsMenuRow(
                (OptionsRow)row,
                selectedRow,
                textSpeedOption,
                tutsilDifficultyOption);
        }
    }
}

static void App_DrawOptionsMenu(
    OptionsRow selectedRow,
    TextSpeed textSpeedOption,
    TutsilDifficulty tutsilDifficultyOption)
{
    int row;

    Video_Clear(APP_MENU_BG_COLOR);
    Video_DrawText(90, 36, "Options", RGB15(31, 31, 31));
    for (row = 0; row < OPTIONS_ROW_COUNT; row++)
    {
        App_DrawOptionsMenuRow(
            (OptionsRow)row,
            selectedRow,
            textSpeedOption,
            tutsilDifficultyOption);
    }
    Video_DrawText(58, 116, "A/L/R: Change", RGB15(31, 31, 0));
    Video_DrawText(88, 130, "B: Back", RGB15(31, 31, 0));
}

static CustomBattleOption App_GetCustomBattleMoveUp(CustomBattleOption option)
{
    switch (option)
    {
    case CUSTOM_BATTLE_OPTION_START_BATTLE:
        return CUSTOM_BATTLE_OPTION_ADD_ALLY;
    case CUSTOM_BATTLE_OPTION_REVIEW_BATTLE:
        return CUSTOM_BATTLE_OPTION_ADD_ENEMY;
    case CUSTOM_BATTLE_OPTION_SAVE_BATTLE:
        return CUSTOM_BATTLE_OPTION_START_BATTLE;
    case CUSTOM_BATTLE_OPTION_LOAD_BATTLE:
        return CUSTOM_BATTLE_OPTION_REVIEW_BATTLE;
    case CUSTOM_BATTLE_OPTION_DELETE_UNIT:
        return CUSTOM_BATTLE_OPTION_SAVE_BATTLE;
    case CUSTOM_BATTLE_OPTION_ADD_ALLY:
    case CUSTOM_BATTLE_OPTION_ADD_ENEMY:
        return CUSTOM_BATTLE_OPTION_DELETE_UNIT;
    default:
        return option;
    }
}

static CustomBattleOption App_GetCustomBattleMoveDown(CustomBattleOption option)
{
    switch (option)
    {
    case CUSTOM_BATTLE_OPTION_ADD_ALLY:
    case CUSTOM_BATTLE_OPTION_ADD_ENEMY:
        return CUSTOM_BATTLE_OPTION_START_BATTLE;
    case CUSTOM_BATTLE_OPTION_START_BATTLE:
    case CUSTOM_BATTLE_OPTION_REVIEW_BATTLE:
        return CUSTOM_BATTLE_OPTION_SAVE_BATTLE;
    case CUSTOM_BATTLE_OPTION_SAVE_BATTLE:
    case CUSTOM_BATTLE_OPTION_LOAD_BATTLE:
        return CUSTOM_BATTLE_OPTION_DELETE_UNIT;
    case CUSTOM_BATTLE_OPTION_DELETE_UNIT:
        return CUSTOM_BATTLE_OPTION_ADD_ALLY;
    default:
        return option;
    }
}

static CustomBattleOption App_GetCustomBattleMoveLeft(CustomBattleOption option)
{
    switch (option)
    {
    case CUSTOM_BATTLE_OPTION_ADD_ENEMY:
        return CUSTOM_BATTLE_OPTION_ADD_ALLY;
    case CUSTOM_BATTLE_OPTION_REVIEW_BATTLE:
        return CUSTOM_BATTLE_OPTION_START_BATTLE;
    case CUSTOM_BATTLE_OPTION_LOAD_BATTLE:
        return CUSTOM_BATTLE_OPTION_SAVE_BATTLE;
    default:
        return option;
    }
}

static CustomBattleOption App_GetCustomBattleMoveRight(CustomBattleOption option)
{
    switch (option)
    {
    case CUSTOM_BATTLE_OPTION_ADD_ALLY:
        return CUSTOM_BATTLE_OPTION_ADD_ENEMY;
    case CUSTOM_BATTLE_OPTION_START_BATTLE:
        return CUSTOM_BATTLE_OPTION_REVIEW_BATTLE;
    case CUSTOM_BATTLE_OPTION_SAVE_BATTLE:
        return CUSTOM_BATTLE_OPTION_LOAD_BATTLE;
    default:
        return option;
    }
}

#define CUSTOM_BATTLE_PORTRAIT_BOX_W 52
#define CUSTOM_BATTLE_PORTRAIT_BOX_H 34
#define CUSTOM_BATTLE_PORTRAIT_CROP_X 8
#define CUSTOM_BATTLE_PORTRAIT_CROP_Y 0
#define CUSTOM_BATTLE_PORTRAIT_CROP_W 48
#define CUSTOM_BATTLE_PORTRAIT_CROP_H 36

#define CUSTOM_BATTLE_ALLY_CARD_BOX_W 38
#define CUSTOM_BATTLE_ALLY_CARD_BOX_H 34
#define CUSTOM_BATTLE_ALLY_CARD_CROP_W 34
#define CUSTOM_BATTLE_ALLY_CARD_CROP_H 32
#define CUSTOM_BATTLE_ALLY_CARD_CROP_Y 0
#define CUSTOM_BATTLE_ALLY_CARD_NAME_Y 88

static const int s_customAllyCardCenterX[CUSTOM_ALLY_OPTION_COUNT] = {
    28,
    74,
    120,
    166,
    212
};

#define CUSTOM_BATTLE_ENEMY_CARD_BOX_W 44
#define CUSTOM_BATTLE_ENEMY_CARD_BOX_H 36
#define CUSTOM_BATTLE_ENEMY_CARD_CROP_W 40
#define CUSTOM_BATTLE_ENEMY_CARD_CROP_H 32
#define CUSTOM_BATTLE_ENEMY_CARD_CROP_Y 0
#define CUSTOM_BATTLE_ENEMY_CARD_NAME_Y 88

static const int s_customEnemyCardCenterX[CUSTOM_ENEMY_OPTION_COUNT] = {
    56,
    120,
    184
};

static void App_DrawCustomBattlePortraitBox(
    int boxX,
    int boxY,
    int boxW,
    int boxH,
    int selected,
    int srcBitmapW,
    int srcCropX,
    int srcCropY,
    int cropW,
    int cropH,
    const u16 *bitmap,
    u16 keyColor)
{
    int drawX;
    int drawY;
    u16 fillColor = RGB15(6, 6, 10);
    u16 borderColor = selected ? RGB15(31, 31, 0) : RGB15(18, 18, 24);

    Video_DrawRect(boxX, boxY, boxW, boxH, fillColor);
    Video_DrawRect(boxX, boxY, boxW, 1, borderColor);
    Video_DrawRect(boxX, boxY + boxH - 1, boxW, 1, borderColor);
    Video_DrawRect(boxX, boxY, 1, boxH, borderColor);
    Video_DrawRect(boxX + boxW - 1, boxY, 1, boxH, borderColor);

    if (!bitmap)
    {
        Video_DrawRect(boxX + 9, boxY + 8, boxW - 18, boxH - 16, RGB15(10, 10, 14));
        return;
    }

    drawX = boxX + (boxW - cropW) / 2;
    drawY = boxY + (boxH - cropH) / 2;
    Video_DrawBitmapKeyColorRegion(
        drawX,
        drawY,
        boxX + 1,
        boxY + 1,
        boxW - 2,
        boxH - 2,
        srcBitmapW,
        srcCropX,
        srcCropY,
        cropW,
        cropH,
        bitmap,
        keyColor);
}

static void App_DrawCustomBattleAllyCard(
    int centerX,
    CustomAllyOption option,
    int selected)
{
    int boxX = centerX - (CUSTOM_BATTLE_ALLY_CARD_BOX_W / 2);
    int boxY = 48;
    const char *label = App_GetCustomAllyLabel(option);
    int nameX = centerX - (Video_MeasureTextWidth(label) / 2);
    u16 nameColor = selected ? RGB15(31, 31, 0) : RGB15(31, 31, 31);
    int srcCropX;

    switch (option)
    {
    case CUSTOM_ALLY_OPTION_PROTAG:
        srcCropX = (CUSTOM_BATTLE_MENU_PROTAG_WIDTH - CUSTOM_BATTLE_ALLY_CARD_CROP_W) / 2;
        App_DrawCustomBattlePortraitBox(
            boxX,
            boxY,
            CUSTOM_BATTLE_ALLY_CARD_BOX_W,
            CUSTOM_BATTLE_ALLY_CARD_BOX_H,
            selected,
            CUSTOM_BATTLE_MENU_PROTAG_WIDTH,
            srcCropX,
            CUSTOM_BATTLE_ALLY_CARD_CROP_Y,
            CUSTOM_BATTLE_ALLY_CARD_CROP_W,
            CUSTOM_BATTLE_ALLY_CARD_CROP_H,
            protagonist_front_spriteBitmap,
            CUSTOM_BATTLE_MENU_SPRITE_KEY_COLOR);
        break;
    case CUSTOM_ALLY_OPTION_AKSIL:
        srcCropX = (CUSTOM_BATTLE_MENU_AKSIL_WIDTH - CUSTOM_BATTLE_ALLY_CARD_CROP_W) / 2;
        App_DrawCustomBattlePortraitBox(
            boxX,
            boxY,
            CUSTOM_BATTLE_ALLY_CARD_BOX_W,
            CUSTOM_BATTLE_ALLY_CARD_BOX_H,
            selected,
            CUSTOM_BATTLE_MENU_AKSIL_WIDTH,
            srcCropX,
            CUSTOM_BATTLE_ALLY_CARD_CROP_Y,
            CUSTOM_BATTLE_ALLY_CARD_CROP_W,
            CUSTOM_BATTLE_ALLY_CARD_CROP_H,
            aksil_front_spriteBitmap,
            CUSTOM_BATTLE_MENU_SPRITE_KEY_COLOR);
        break;
    case CUSTOM_ALLY_OPTION_MAREN:
        srcCropX = (CUSTOM_BATTLE_MENU_MAREN_WIDTH - CUSTOM_BATTLE_ALLY_CARD_CROP_W) / 2;
        App_DrawCustomBattlePortraitBox(
            boxX,
            boxY,
            CUSTOM_BATTLE_ALLY_CARD_BOX_W,
            CUSTOM_BATTLE_ALLY_CARD_BOX_H,
            selected,
            CUSTOM_BATTLE_MENU_MAREN_WIDTH,
            srcCropX,
            CUSTOM_BATTLE_ALLY_CARD_CROP_Y,
            CUSTOM_BATTLE_ALLY_CARD_CROP_W,
            CUSTOM_BATTLE_ALLY_CARD_CROP_H,
            maren_front_spriteBitmap,
            CUSTOM_BATTLE_MENU_SPRITE_KEY_COLOR);
        break;
    case CUSTOM_ALLY_OPTION_CONALL:
    case CUSTOM_ALLY_OPTION_REIKA:
    default:
        App_DrawCustomBattlePortraitBox(
            boxX,
            boxY,
            CUSTOM_BATTLE_ALLY_CARD_BOX_W,
            CUSTOM_BATTLE_ALLY_CARD_BOX_H,
            selected,
            0,
            0,
            CUSTOM_BATTLE_ALLY_CARD_CROP_Y,
            0,
            0,
            0,
            0);
        break;
    }

    Video_DrawText(nameX, CUSTOM_BATTLE_ALLY_CARD_NAME_Y, label, nameColor);
}

static void App_AdvanceCustomAllyOption(CustomAllyOption *option, int delta)
{
    int next = (int)*option + delta;

    if (next < 0)
    {
        next = CUSTOM_ALLY_OPTION_COUNT - 1;
    }
    else if (next >= CUSTOM_ALLY_OPTION_COUNT)
    {
        next = 0;
    }
    *option = (CustomAllyOption)next;
}

static void App_DrawCustomBattleEnemyCard(
    int centerX,
    CustomEnemyOption option,
    int selected)
{
    int boxX = centerX - (CUSTOM_BATTLE_ENEMY_CARD_BOX_W / 2);
    int boxY = 48;
    const char *label = App_GetCustomEnemySelectLabel(option);
    int nameX = centerX - (Video_MeasureTextWidth(label) / 2);
    u16 nameColor = selected ? RGB15(31, 31, 0) : RGB15(31, 31, 31);
    int srcCropX;

    switch (option)
    {
    case CUSTOM_ENEMY_OPTION_TUTSIL:
        srcCropX = (CUSTOM_BATTLE_MENU_TUTSIL_WIDTH - CUSTOM_BATTLE_ENEMY_CARD_CROP_W) / 2;
        App_DrawCustomBattlePortraitBox(
            boxX,
            boxY,
            CUSTOM_BATTLE_ENEMY_CARD_BOX_W,
            CUSTOM_BATTLE_ENEMY_CARD_BOX_H,
            selected,
            CUSTOM_BATTLE_MENU_TUTSIL_WIDTH,
            srcCropX,
            CUSTOM_BATTLE_ENEMY_CARD_CROP_Y,
            CUSTOM_BATTLE_ENEMY_CARD_CROP_W,
            CUSTOM_BATTLE_ENEMY_CARD_CROP_H,
            tutsil_front_spriteBitmap,
            CUSTOM_BATTLE_MENU_SPRITE_KEY_COLOR);
        break;
    case CUSTOM_ENEMY_OPTION_ARMOR_BADGER:
        srcCropX = (CUSTOM_BATTLE_MENU_BADGER_WIDTH - CUSTOM_BATTLE_ENEMY_CARD_CROP_W) / 2;
        App_DrawCustomBattlePortraitBox(
            boxX,
            boxY,
            CUSTOM_BATTLE_ENEMY_CARD_BOX_W,
            CUSTOM_BATTLE_ENEMY_CARD_BOX_H,
            selected,
            CUSTOM_BATTLE_MENU_BADGER_WIDTH,
            srcCropX,
            CUSTOM_BATTLE_ENEMY_CARD_CROP_Y,
            CUSTOM_BATTLE_ENEMY_CARD_CROP_W,
            CUSTOM_BATTLE_ENEMY_CARD_CROP_H,
            electric_armor_badger_spriteBitmap,
            CUSTOM_BATTLE_MENU_SPRITE_KEY_COLOR);
        break;
    case CUSTOM_ENEMY_OPTION_SENTIENT_CACTUS:
    default:
        App_DrawCustomBattlePortraitBox(
            boxX,
            boxY,
            CUSTOM_BATTLE_ENEMY_CARD_BOX_W,
            CUSTOM_BATTLE_ENEMY_CARD_BOX_H,
            selected,
            0,
            0,
            CUSTOM_BATTLE_ENEMY_CARD_CROP_Y,
            0,
            0,
            0,
            0);
        break;
    }

    Video_DrawText(nameX, CUSTOM_BATTLE_ENEMY_CARD_NAME_Y, label, nameColor);
}

static void App_AdvanceCustomEnemyOption(CustomEnemyOption *option, int delta)
{
    int next = (int)*option + delta;

    if (next < 0)
    {
        next = CUSTOM_ENEMY_OPTION_COUNT - 1;
    }
    else if (next >= CUSTOM_ENEMY_OPTION_COUNT)
    {
        next = 0;
    }
    *option = (CustomEnemyOption)next;
}

static void App_DrawCustomBattleMenuFormationSummary(void)
{
    int allyCount = 0;
    int enemyCount = 0;
    int slot;

    for (slot = 0; slot < BATTLE_PLAYER_SLOT_COUNT; slot++)
    {
        const CustomBattleFormationSlot *formationSlot = CustomBattleSetup_GetAllySlot(slot);

        if (formationSlot && formationSlot->occupied)
        {
            allyCount++;
        }
    }
    for (slot = 0; slot < BATTLE_ENEMY_SLOT_COUNT; slot++)
    {
        const CustomBattleFormationSlot *formationSlot = CustomBattleSetup_GetEnemySlot(slot);

        if (formationSlot && formationSlot->occupied)
        {
            enemyCount++;
        }
    }

    Video_DrawText(42, 84, "ALLIES ", RGB15(24, 24, 28));
    App_DrawNumberText(98, 84, allyCount, RGB15(24, 24, 28));

    Video_DrawText(138, 84, "ENEMIES ", RGB15(24, 24, 28));
    App_DrawNumberText(202, 84, enemyCount, RGB15(24, 24, 28));
}

static void App_DrawCustomBattleMenu(CustomBattleOption selectedOption)
{
    Video_Clear(RGB15(3, 3, 6));
    Video_DrawText(72, 18, "CUSTOM BATTLE", RGB15(31, 31, 31));

    App_DrawCustomBattlePortraitBox(
        34,
        36,
        CUSTOM_BATTLE_PORTRAIT_BOX_W,
        CUSTOM_BATTLE_PORTRAIT_BOX_H,
        0,
        CUSTOM_BATTLE_MENU_PROTAG_WIDTH,
        CUSTOM_BATTLE_PORTRAIT_CROP_X,
        CUSTOM_BATTLE_PORTRAIT_CROP_Y,
        CUSTOM_BATTLE_PORTRAIT_CROP_W,
        CUSTOM_BATTLE_PORTRAIT_CROP_H,
        protagonist_front_spriteBitmap,
        CUSTOM_BATTLE_MENU_SPRITE_KEY_COLOR);
    App_DrawCustomBattlePortraitBox(
        132,
        36,
        CUSTOM_BATTLE_PORTRAIT_BOX_W,
        CUSTOM_BATTLE_PORTRAIT_BOX_H,
        0,
        CUSTOM_BATTLE_MENU_TUTSIL_WIDTH,
        CUSTOM_BATTLE_PORTRAIT_CROP_X,
        CUSTOM_BATTLE_PORTRAIT_CROP_Y,
        CUSTOM_BATTLE_PORTRAIT_CROP_W,
        CUSTOM_BATTLE_PORTRAIT_CROP_H,
        tutsil_front_spriteBitmap,
        CUSTOM_BATTLE_MENU_SPRITE_KEY_COLOR);

    App_DrawCustomBattleOption(
        42,
        72,
        selectedOption == CUSTOM_BATTLE_OPTION_ADD_ALLY,
        "ADD ALLY");
    App_DrawCustomBattleOption(
        138,
        72,
        selectedOption == CUSTOM_BATTLE_OPTION_ADD_ENEMY,
        "ADD ENEMY");

    App_DrawCustomBattleMenuFormationSummary();

    App_DrawCustomBattleOption(
        34,
        104,
        selectedOption == CUSTOM_BATTLE_OPTION_START_BATTLE,
        "START BATTLE");
    App_DrawCustomBattleOption(
        126,
        104,
        selectedOption == CUSTOM_BATTLE_OPTION_REVIEW_BATTLE,
        "REVIEW BATTLE");
    App_DrawCustomBattleOption(
        34,
        120,
        selectedOption == CUSTOM_BATTLE_OPTION_SAVE_BATTLE,
        "SAVE BATTLE");
    App_DrawCustomBattleOption(
        126,
        120,
        selectedOption == CUSTOM_BATTLE_OPTION_LOAD_BATTLE,
        "LOAD BATTLE");
    App_DrawCustomBattleOption(
        74,
        136,
        selectedOption == CUSTOM_BATTLE_OPTION_DELETE_UNIT,
        "DELETE UNIT");

    Video_DrawText(92, 148, "B: BACK", RGB15(31, 31, 0));
}

static const char *App_GetCustomEnemyLabel(CustomEnemyOption option)
{
    switch (option)
    {
    case CUSTOM_ENEMY_OPTION_TUTSIL:
        return "TUTSIL";
    case CUSTOM_ENEMY_OPTION_ARMOR_BADGER:
        return "ARMOR BADGER";
    case CUSTOM_ENEMY_OPTION_SENTIENT_CACTUS:
    default:
        return "SENTIENT CACTUS";
    }
}

static const char *App_GetCustomEnemySelectLabel(CustomEnemyOption option)
{
    switch (option)
    {
    case CUSTOM_ENEMY_OPTION_TUTSIL:
        return "TUTSIL";
    case CUSTOM_ENEMY_OPTION_ARMOR_BADGER:
        return "BADGER";
    case CUSTOM_ENEMY_OPTION_SENTIENT_CACTUS:
    default:
        return "CACTUS";
    }
}

static const char *App_GetCustomAllyLabel(CustomAllyOption option)
{
    switch (option)
    {
    case CUSTOM_ALLY_OPTION_PROTAG:
        return "PROTAG";
    case CUSTOM_ALLY_OPTION_AKSIL:
        return "AKSIL";
    case CUSTOM_ALLY_OPTION_MAREN:
        return "MAREN";
    case CUSTOM_ALLY_OPTION_CONALL:
        return "CONALL";
    case CUSTOM_ALLY_OPTION_REIKA:
    default:
        return "REIKA";
    }
}

static int App_GetCustomAllyReady(CustomAllyOption option)
{
    return option == CUSTOM_ALLY_OPTION_PROTAG
        || option == CUSTOM_ALLY_OPTION_AKSIL
        || option == CUSTOM_ALLY_OPTION_MAREN;
}

static MemberTemplateId App_GetCustomAllyTemplateId(CustomAllyOption option)
{
    switch (option)
    {
    case CUSTOM_ALLY_OPTION_PROTAG:
        return MEMBER_TEMPLATE_PROTAGONIST;
    case CUSTOM_ALLY_OPTION_AKSIL:
        return MEMBER_TEMPLATE_AKSIL;
    case CUSTOM_ALLY_OPTION_MAREN:
        return MEMBER_TEMPLATE_MAREN;
    case CUSTOM_ALLY_OPTION_CONALL:
    case CUSTOM_ALLY_OPTION_REIKA:
    default:
        return MEMBER_TEMPLATE_NONE;
    }
}

static const MemberTemplate *App_GetCustomAllyTemplate(CustomAllyOption option)
{
    return MemberDatabase_Get(App_GetCustomAllyTemplateId(option));
}

static int App_GetCustomAllyLearnableMoveCount(CustomAllyOption option)
{
    switch (option)
    {
    case CUSTOM_ALLY_OPTION_PROTAG:
        return (int)(sizeof(s_protagLearnableMoves) / sizeof(s_protagLearnableMoves[0]));
    case CUSTOM_ALLY_OPTION_AKSIL:
        return (int)(sizeof(s_aksilLearnableMoves) / sizeof(s_aksilLearnableMoves[0]));
    case CUSTOM_ALLY_OPTION_MAREN:
        return (int)(sizeof(s_marenLearnableMoves) / sizeof(s_marenLearnableMoves[0]));
    default:
        return 0;
    }
}

static const MoveId *App_GetCustomAllyLearnableMoves(CustomAllyOption option)
{
    switch (option)
    {
    case CUSTOM_ALLY_OPTION_PROTAG:
        return s_protagLearnableMoves;
    case CUSTOM_ALLY_OPTION_AKSIL:
        return s_aksilLearnableMoves;
    case CUSTOM_ALLY_OPTION_MAREN:
        return s_marenLearnableMoves;
    default:
        return 0;
    }
}

static const char *App_GetMoveCategoryLabel(MoveCategory category)
{
    switch (category)
    {
    case MOVE_CATEGORY_SPECIAL:
        return "SPEC";
    case MOVE_CATEGORY_SUPPORT:
        return "SUPP";
    case MOVE_CATEGORY_STATUS:
        return "STAT";
    case MOVE_CATEGORY_DISRUPT:
        return "DISP";
    case MOVE_CATEGORY_PHYSICAL:
    default:
        return "PHYS";
    }
}

static const char *App_GetMoveDisplayName(MoveId moveId)
{
    const MoveData *move = MoveData_Get(moveId);

    if (!move || !move->name)
    {
        return "EMPTY";
    }
    return move->name;
}

static void App_CopyTruncatedText(char *dest, int destSize, const char *source)
{
    int i;

    if (!dest || destSize <= 0)
    {
        return;
    }
    dest[0] = '\0';
    if (!source)
    {
        return;
    }
    for (i = 0; i < destSize - 1 && source[i] != '\0'; i++)
    {
        dest[i] = source[i];
    }
    dest[i] = '\0';
}

static void App_InitCustomAllyPreviewMoves(MoveId *moveIds)
{
    int slot;

    if (!moveIds)
    {
        return;
    }
    for (slot = 0; slot < BATTLE_MEMBER_MOVE_SLOT_COUNT; slot++)
    {
        moveIds[slot] = MOVE_NONE;
    }
}

static void App_BuildAllyMoveSummary(
    const CustomBattleFormationSlot *slot,
    char *dest,
    int destSize)
{
    int moveSlot;
    int cursor = 0;
    int shown = 0;

    if (!dest || destSize <= 0)
    {
        return;
    }
    dest[0] = '\0';
    if (!slot)
    {
        return;
    }
    for (moveSlot = 0; moveSlot < BATTLE_MEMBER_EQUIPPED_MOVE_SLOT_COUNT && shown < 2; moveSlot++)
    {
        const char *name;
        int i;

        if (slot->moveIds[moveSlot] == MOVE_NONE)
        {
            continue;
        }
        name = App_GetMoveDisplayName(slot->moveIds[moveSlot]);
        if (shown > 0 && cursor < destSize - 4)
        {
            dest[cursor++] = ' ';
            dest[cursor++] = '/';
            dest[cursor++] = ' ';
        }
        for (i = 0; name[i] != '\0' && cursor < destSize - 1; i++)
        {
            dest[cursor++] = name[i];
        }
        shown++;
    }
    dest[cursor] = '\0';
    if (shown == 0)
    {
        App_CopyTruncatedText(dest, destSize, "(no moves)");
    }
}

static void App_DrawCustomBattleFormationReview(int reviewPage)
{
    int slot;

    Video_Clear(RGB15(3, 3, 6));
    Video_DrawText(62, 10, "CUSTOM FORMATION", RGB15(31, 31, 31));
    if (reviewPage == 0)
    {
        Video_DrawText(24, 24, "ALLIES", RGB15(31, 31, 0));
        for (slot = 0; slot < BATTLE_PLAYER_SLOT_COUNT; slot++)
        {
            const CustomBattleFormationSlot *formationSlot =
                CustomBattleSetup_GetAllySlot(slot);
            int y = 38 + (slot * 22);
            char moveSummary[40];

            App_DrawNumberText(20, y, slot, RGB15(31, 31, 31));
            if (formationSlot && formationSlot->occupied)
            {
                Video_DrawText(
                    34,
                    y,
                    App_GetMemberTemplateShortLabel(formationSlot->memberTemplateId),
                    RGB15(31, 31, 31));
                Video_DrawText(112, y, "L", RGB15(31, 31, 31));
                App_DrawNumberText(120, y, formationSlot->level, RGB15(31, 31, 31));
                App_BuildAllyMoveSummary(formationSlot, moveSummary, 40);
                Video_DrawText(34, y + 10, moveSummary, RGB15(24, 24, 28));
            }
            else
            {
                Video_DrawText(34, y, "EMPTY", RGB15(18, 18, 18));
            }
        }
        Video_DrawText(52, 150, "L/R: ENEMIES", RGB15(31, 31, 0));
    }
    else
    {
        Video_DrawText(24, 24, "ENEMIES", RGB15(31, 31, 0));
        for (slot = 0; slot < BATTLE_ENEMY_SLOT_COUNT; slot++)
        {
            const CustomBattleFormationSlot *formationSlot =
                CustomBattleSetup_GetEnemySlot(slot);
            int y = 38 + (slot * 18);

            App_DrawNumberText(20, y, slot, RGB15(31, 31, 31));
            if (formationSlot && formationSlot->occupied)
            {
                Video_DrawText(
                    34,
                    y,
                    App_GetMemberTemplateShortLabel(formationSlot->memberTemplateId),
                    RGB15(31, 31, 31));
                Video_DrawText(152, y, "L", RGB15(31, 31, 31));
                App_DrawNumberText(160, y, formationSlot->level, RGB15(31, 31, 31));
            }
            else
            {
                Video_DrawText(34, y, "EMPTY", RGB15(18, 18, 18));
            }
        }
        Video_DrawText(52, 150, "L/R: ALLIES", RGB15(31, 31, 0));
    }
    Video_DrawText(88, 156, "B: BACK", RGB15(31, 31, 0));
}

static int App_GetCustomEnemyReady(CustomEnemyOption option)
{
    return option == CUSTOM_ENEMY_OPTION_TUTSIL
        || option == CUSTOM_ENEMY_OPTION_ARMOR_BADGER;
}

static MemberTemplateId App_GetCustomEnemyTemplateId(CustomEnemyOption option)
{
    switch (option)
    {
    case CUSTOM_ENEMY_OPTION_TUTSIL:
        return MEMBER_TEMPLATE_TUTSIL;
    case CUSTOM_ENEMY_OPTION_ARMOR_BADGER:
        return MEMBER_TEMPLATE_ELECTRIC_ARMOR_BADGER;
    case CUSTOM_ENEMY_OPTION_SENTIENT_CACTUS:
    default:
        return MEMBER_TEMPLATE_NONE;
    }
}

static const MemberTemplate *App_GetCustomEnemyTemplate(CustomEnemyOption option)
{
    return MemberDatabase_Get(App_GetCustomEnemyTemplateId(option));
}

static const char *App_GetMemberTemplateShortLabel(MemberTemplateId memberTemplateId)
{
    switch (memberTemplateId)
    {
    case MEMBER_TEMPLATE_PROTAGONIST:
        return "PROTAG";
    case MEMBER_TEMPLATE_AKSIL:
        return "AKSIL";
    case MEMBER_TEMPLATE_MAREN:
        return "MAREN";
    case MEMBER_TEMPLATE_ELECTRIC_ARMOR_BADGER:
        return "ARMOR BADGER";
    case MEMBER_TEMPLATE_TUTSIL:
        return "TUTSIL";
    case MEMBER_TEMPLATE_TEST_DUMMY:
        return "DUMMY";
    case MEMBER_TEMPLATE_NONE:
    default:
        return "EMPTY";
    }
}

static void App_GetTemplateStatsAtLevel(
    const MemberTemplate *template,
    int level,
    StatCurveStats *outStats)
{
    StatCurveStats level50Stats;

    if (!outStats)
    {
        return;
    }
    outStats->hp = 0;
    outStats->attack = 0;
    outStats->defence = 0;
    outStats->speed = 0;
    if (!template)
    {
        return;
    }
    level50Stats.hp = (u16)template->level50Hp;
    level50Stats.attack = (u16)template->level50Attack;
    level50Stats.defence = (u16)template->level50Defence;
    level50Stats.speed = (u16)template->level50Speed;
    StatCurve_CalculateNaturalStatsAtLevel(&level50Stats, (u8)level, outStats);
}

static void App_DrawCustomBattleEnemySelect(CustomEnemyOption selectedEnemy)
{
    int i;

    Video_Clear(RGB15(3, 3, 6));
    Video_DrawText(84, 18, "ADD ENEMY", RGB15(31, 31, 31));
    for (i = 0; i < CUSTOM_ENEMY_OPTION_COUNT; i++)
    {
        App_DrawCustomBattleEnemyCard(
            s_customEnemyCardCenterX[i],
            (CustomEnemyOption)i,
            i == selectedEnemy);
    }
    Video_DrawText(54, 148, "A= OK   B= BACK", RGB15(31, 31, 0));
}

static void App_DrawCustomBattleAllySelect(CustomAllyOption selectedAlly)
{
    int i;

    Video_Clear(RGB15(3, 3, 6));
    Video_DrawText(90, 18, "ADD ALLY", RGB15(31, 31, 31));
    for (i = 0; i < CUSTOM_ALLY_OPTION_COUNT; i++)
    {
        App_DrawCustomBattleAllyCard(
            s_customAllyCardCenterX[i],
            (CustomAllyOption)i,
            i == selectedAlly);
    }
    Video_DrawText(54, 148, "A= OK   B= BACK", RGB15(31, 31, 0));
}

static void App_DrawCustomBattleAllyPreview(
    CustomAllyOption allyOption,
    int level,
    int showConfirm)
{
    const MemberTemplate *template;
    StatCurveStats stats;

    template = App_GetCustomAllyTemplate(allyOption);
    App_GetTemplateStatsAtLevel(template, level, &stats);

    Video_Clear(RGB15(3, 3, 6));
    Video_DrawText(54, 18, "ALLY LEVEL / STATS", RGB15(31, 31, 31));
    Video_DrawText(24, 42, App_GetCustomAllyLabel(allyOption), RGB15(31, 31, 0));
    Video_DrawText(24, 62, "LEVEL", RGB15(31, 31, 31));
    App_DrawNumberText(120, 62, level, RGB15(31, 31, 31));
    Video_DrawText(24, 78, "HP", RGB15(31, 31, 31));
    App_DrawNumberText(120, 78, stats.hp, RGB15(31, 31, 31));
    Video_DrawText(24, 94, "ATTACK", RGB15(31, 31, 31));
    App_DrawNumberText(120, 94, stats.attack, RGB15(31, 31, 31));
    Video_DrawText(24, 110, "DEFENCE", RGB15(31, 31, 31));
    App_DrawNumberText(120, 110, stats.defence, RGB15(31, 31, 31));
    Video_DrawText(24, 126, "SPEED", RGB15(31, 31, 31));
    App_DrawNumberText(120, 126, stats.speed, RGB15(31, 31, 31));
    Video_DrawText(46, 146, "L/R: LEVEL  A: OK", RGB15(31, 31, 0));
    Video_DrawText(88, 156, "B: BACK", RGB15(31, 31, 0));

    if (showConfirm)
    {
        Video_DrawRect(56, 48, 128, 42, RGB15(6, 6, 12));
        Video_DrawText(82, 56, "Looks Good?", RGB15(31, 31, 31));
        Video_DrawText(86, 68, "A Yes", RGB15(31, 31, 0));
        Video_DrawText(86, 80, "B No", RGB15(31, 31, 0));
    }
}

static void App_DrawCustomBattleAllyChooseMoves(
    CustomAllyOption allyOption,
    const MoveId *moveIds,
    int selectedIndex)
{
    int slot;

    Video_Clear(RGB15(3, 3, 6));
    Video_DrawText(76, 16, "CHOOSE MOVES", RGB15(31, 31, 31));
    Video_DrawText(24, 34, App_GetCustomAllyLabel(allyOption), RGB15(31, 31, 0));
    for (slot = 0; slot < BATTLE_MEMBER_EQUIPPED_MOVE_SLOT_COUNT; slot++)
    {
        int y = 52 + (slot * 16);
        int selected = selectedIndex == slot;

        Video_DrawText(
            24,
            y,
            selected ? "> " : "  ",
            selected ? RGB15(31, 31, 0) : RGB15(20, 20, 20));
        Video_DrawText(38, y, "SLOT", RGB15(31, 31, 31));
        App_DrawNumberText(70, y, slot + 1, RGB15(31, 31, 31));
        Video_DrawText(86, y, ":", RGB15(31, 31, 31));
        Video_DrawText(
            96,
            y,
            App_GetMoveDisplayName(moveIds ? moveIds[slot] : MOVE_NONE),
            selected ? RGB15(31, 31, 0) : RGB15(31, 31, 31));
    }
    Video_DrawText(
        24,
        104,
        selectedIndex == CUSTOM_ALLY_CHOOSE_MOVE_LOOKS_GOOD ? "> " : "  ",
        selectedIndex == CUSTOM_ALLY_CHOOSE_MOVE_LOOKS_GOOD
            ? RGB15(31, 31, 0)
            : RGB15(20, 20, 20));
    Video_DrawText(
        38,
        104,
        "LOOKS GOOD!",
        selectedIndex == CUSTOM_ALLY_CHOOSE_MOVE_LOOKS_GOOD
            ? RGB15(31, 31, 0)
            : RGB15(31, 31, 31));
    Video_DrawText(34, 126, "A: PICK / OK", RGB15(31, 31, 0));
    Video_DrawText(88, 136, "B: BACK", RGB15(31, 31, 0));
}

static void App_DrawCustomBattleAllyMoveList(
    CustomAllyOption allyOption,
    int editingMoveSlot,
    int selectedMoveIndex)
{
    const MoveId *learnableMoves;
    int moveCount;
    int scrollOffset;
    int visibleRow;
    const MoveData *hoveredMove = 0;
    char descriptionLine[34];

    learnableMoves = App_GetCustomAllyLearnableMoves(allyOption);
    moveCount = App_GetCustomAllyLearnableMoveCount(allyOption);
    if (learnableMoves && selectedMoveIndex >= 0 && selectedMoveIndex < moveCount)
    {
        hoveredMove = MoveData_Get(learnableMoves[selectedMoveIndex]);
    }

    scrollOffset = 0;
    if (moveCount > CUSTOM_ALLY_MOVE_LIST_VISIBLE_ROWS)
    {
        if (selectedMoveIndex < 0)
        {
            scrollOffset = 0;
        }
        else
        {
            scrollOffset = selectedMoveIndex - (CUSTOM_ALLY_MOVE_LIST_VISIBLE_ROWS - 1);
            if (scrollOffset < 0)
            {
                scrollOffset = 0;
            }
            if (scrollOffset > moveCount - CUSTOM_ALLY_MOVE_LIST_VISIBLE_ROWS)
            {
                scrollOffset = moveCount - CUSTOM_ALLY_MOVE_LIST_VISIBLE_ROWS;
            }
        }
    }

    Video_Clear(RGB15(3, 3, 6));
    Video_DrawText(88, 12, "PICK MOVE", RGB15(31, 31, 31));
    Video_DrawText(24, 26, "SLOT", RGB15(31, 31, 31));
    App_DrawNumberText(56, 26, editingMoveSlot + 1, RGB15(31, 31, 0));
    if (moveCount > CUSTOM_ALLY_MOVE_LIST_VISIBLE_ROWS)
    {
        Video_DrawText(170, 26, "UP/DN", RGB15(20, 20, 24));
    }
    for (visibleRow = 0; visibleRow < CUSTOM_ALLY_MOVE_LIST_VISIBLE_ROWS; visibleRow++)
    {
        int moveIndex = scrollOffset + visibleRow;
        const MoveData *move;
        int y;
        int selected;

        if (moveIndex >= moveCount)
        {
            break;
        }
        move = MoveData_Get(learnableMoves[moveIndex]);
        y = CUSTOM_ALLY_MOVE_LIST_FIRST_Y + (visibleRow * CUSTOM_ALLY_MOVE_LIST_ROW_HEIGHT);
        selected = moveIndex == selectedMoveIndex;

        Video_DrawText(
            18,
            y,
            selected ? "> " : "  ",
            selected ? RGB15(31, 31, 0) : RGB15(20, 20, 20));
        if (move)
        {
            const char *elementLabel = ElementDatabase_GetShortName(move->element);

            Video_DrawText(32, y, elementLabel ? elementLabel : "---", RGB15(31, 31, 31));
            Video_DrawText(58, y, move->name ? move->name : "?", RGB15(31, 31, 31));
            Video_DrawText(150, y, App_GetMoveCategoryLabel(move->category), RGB15(31, 31, 31));
            Video_DrawText(182, y, "SC", RGB15(31, 31, 31));
            App_DrawNumberText(198, y, move->maxSc, RGB15(31, 31, 31));
            if (selected)
            {
                Video_DrawText(32, y + 10, "LP", RGB15(31, 31, 31));
                App_DrawNumberText(48, y + 10, move->optionalLpCost, RGB15(31, 31, 31));
            }
        }
    }
    App_CopyTruncatedText(
        descriptionLine,
        34,
        hoveredMove && hoveredMove->description ? hoveredMove->description : "");
    Video_DrawText(18, 124, descriptionLine, RGB15(24, 24, 28));
    Video_DrawText(88, 146, "A: ASSIGN", RGB15(31, 31, 0));
    Video_DrawText(88, 156, "B: BACK", RGB15(31, 31, 0));
}

static void App_DrawCustomBattleAllyPlace(
    CustomAllyOption allyOption,
    int level,
    int selectedSlot)
{
    int slot;

    Video_Clear(RGB15(3, 3, 6));
    Video_DrawText(72, 18, "PLACE ALLY", RGB15(31, 31, 31));
    Video_DrawText(26, 36, App_GetCustomAllyLabel(allyOption), RGB15(31, 31, 0));
    Video_DrawText(168, 36, "LV", RGB15(31, 31, 31));
    App_DrawNumberText(190, 36, level, RGB15(31, 31, 31));
    Video_DrawText(78, 56, "ALLY TILES", RGB15(31, 31, 31));
    for (slot = 0; slot < BATTLE_PLAYER_SLOT_COUNT; slot++)
    {
        int x = 18 + (slot * 44);
        const CustomBattleFormationSlot *formationSlot =
            CustomBattleSetup_GetAllySlot(slot);

        Video_DrawRect(
            x,
            82,
            36,
            22,
            selectedSlot == slot ? RGB15(10, 10, 18) : RGB15(6, 6, 12));
        Video_DrawText(x + 4, 86, selectedSlot == slot ? ">" : " ", RGB15(31, 31, 0));
        App_DrawNumberText(x + 12, 86, slot, RGB15(31, 31, 31));
        if (formationSlot && formationSlot->occupied)
        {
            Video_DrawText(x + 4, 94, "USED", RGB15(31, 14, 10));
        }
        else
        {
            Video_DrawText(x + 4, 94, "OPEN", RGB15(10, 31, 10));
        }
    }
    Video_DrawText(34, 138, "L/R: SLOT  A: PLACE", RGB15(31, 31, 0));
    Video_DrawText(88, 152, "B: BACK", RGB15(31, 31, 0));
}

static void App_DrawDeleteUnitRow(
    int y,
    int isEnemyRow,
    int selectedIsEnemyRow,
    int selectedSlot)
{
    int slot;

    Video_DrawText(22, y, isEnemyRow ? "ENEMY" : "ALLY", RGB15(31, 31, 31));
    for (slot = 0; slot < 5; slot++)
    {
        int x = 22 + (slot * 44);
        const CustomBattleFormationSlot *formationSlot = isEnemyRow
            ? CustomBattleSetup_GetEnemySlot(slot)
            : CustomBattleSetup_GetAllySlot(slot);
        int selected = selectedIsEnemyRow == isEnemyRow && selectedSlot == slot;

        Video_DrawRect(
            x,
            y + 12,
            40,
            34,
            selected ? RGB15(10, 10, 18) : RGB15(6, 6, 12));
        Video_DrawText(x + 4, y + 16, selected ? ">" : " ", RGB15(31, 31, 0));
        App_DrawNumberText(x + 12, y + 16, slot, RGB15(31, 31, 31));
        Video_DrawText(x + 20, y + 16, ":", RGB15(31, 31, 31));
        if (formationSlot && formationSlot->occupied)
        {
            Video_DrawText(
                x + 4,
                y + 26,
                App_GetMemberTemplateShortLabel(formationSlot->memberTemplateId),
                RGB15(31, 31, 31));
            Video_DrawText(x + 4, y + 36, "L", RGB15(31, 31, 31));
            App_DrawNumberText(x + 12, y + 36, formationSlot->level, RGB15(31, 31, 31));
        }
        else
        {
            Video_DrawText(x + 4, y + 30, "EMPTY", RGB15(18, 18, 18));
        }
    }
}

static void App_DrawSaveSlotSummaryText(int x, int y, int saveSlot, u16 color)
{
    int allyCount = 0;
    int enemyCount = 0;

    if (!CustomBattleSetup_IsSaveSlotOccupied(saveSlot))
    {
        Video_DrawText(x, y, "EMPTY", color);
        return;
    }
    CustomBattleSetup_GetSaveSlotSummary(saveSlot, &allyCount, &enemyCount);
    Video_DrawText(x, y, "A", color);
    App_DrawNumberText(x + 8, y, allyCount, color);
    Video_DrawText(x + 20, y, "E", color);
    App_DrawNumberText(x + 28, y, enemyCount, color);
}

static void App_DrawCustomBattleSave(int selectedSaveSlot, int showConfirm)
{
    int slot;

    Video_Clear(RGB15(3, 3, 6));
    Video_DrawText(76, 18, "SAVE BATTLE", RGB15(31, 31, 31));
    for (slot = 0; slot < CUSTOM_BATTLE_SAVE_SLOT_COUNT; slot++)
    {
        int y = 52 + (slot * 22);
        int selected = slot == selectedSaveSlot;

        Video_DrawText(
            34,
            y,
            selected ? "> " : "  ",
            selected ? RGB15(31, 31, 0) : RGB15(20, 20, 20));
        Video_DrawText(48, y, "SLOT", RGB15(31, 31, 31));
        App_DrawNumberText(80, y, slot + 1, RGB15(31, 31, 31));
        Video_DrawText(96, y, ":", RGB15(31, 31, 31));
        App_DrawSaveSlotSummaryText(
            108,
            y,
            slot,
            selected ? RGB15(31, 31, 0) : RGB15(31, 31, 31));
    }
    Video_DrawText(88, 146, "A: SAVE", RGB15(31, 31, 0));
    Video_DrawText(88, 156, "B: BACK", RGB15(31, 31, 0));

    if (showConfirm)
    {
        Video_DrawRect(56, 56, 128, 42, RGB15(6, 6, 12));
        Video_DrawText(86, 64, "Save here?", RGB15(31, 31, 31));
        Video_DrawText(86, 76, "A Yes", RGB15(31, 31, 0));
        Video_DrawText(86, 88, "B No", RGB15(31, 31, 0));
    }
}

static void App_DrawCustomBattleLoad(int selectedSaveSlot, int showConfirm)
{
    int slot;

    Video_Clear(RGB15(3, 3, 6));
    Video_DrawText(76, 18, "LOAD BATTLE", RGB15(31, 31, 31));
    for (slot = 0; slot < CUSTOM_BATTLE_SAVE_SLOT_COUNT; slot++)
    {
        int y = 52 + (slot * 22);
        int selected = slot == selectedSaveSlot;

        Video_DrawText(
            34,
            y,
            selected ? "> " : "  ",
            selected ? RGB15(31, 31, 0) : RGB15(20, 20, 20));
        Video_DrawText(48, y, "SLOT", RGB15(31, 31, 31));
        App_DrawNumberText(80, y, slot + 1, RGB15(31, 31, 31));
        Video_DrawText(96, y, ":", RGB15(31, 31, 31));
        App_DrawSaveSlotSummaryText(
            108,
            y,
            slot,
            selected ? RGB15(31, 31, 0) : RGB15(31, 31, 31));
    }
    Video_DrawText(88, 146, "A: LOAD", RGB15(31, 31, 0));
    Video_DrawText(88, 156, "B: BACK", RGB15(31, 31, 0));

    if (showConfirm)
    {
        Video_DrawRect(56, 56, 128, 42, RGB15(6, 6, 12));
        Video_DrawText(82, 64, "Load battle?", RGB15(31, 31, 31));
        Video_DrawText(86, 76, "A Yes", RGB15(31, 31, 0));
        Video_DrawText(86, 88, "B No", RGB15(31, 31, 0));
    }
}

static void App_DrawCustomBattleDelete(
    int selectedDeleteIsEnemy,
    int selectedDeleteSlot,
    int showConfirm)
{
    Video_Clear(RGB15(3, 3, 6));
    Video_DrawText(76, 12, "DELETE UNIT", RGB15(31, 31, 31));
    App_DrawDeleteUnitRow(28, 0, selectedDeleteIsEnemy, selectedDeleteSlot);
    App_DrawDeleteUnitRow(84, 1, selectedDeleteIsEnemy, selectedDeleteSlot);
    Video_DrawText(42, 148, "A: DELETE", RGB15(31, 31, 0));
    Video_DrawText(142, 148, "B: BACK", RGB15(31, 31, 0));

    if (showConfirm)
    {
        Video_DrawRect(56, 56, 128, 42, RGB15(6, 6, 12));
        Video_DrawText(82, 64, "Delete unit?", RGB15(31, 31, 31));
        Video_DrawText(86, 76, "A Yes", RGB15(31, 31, 0));
        Video_DrawText(86, 88, "B No", RGB15(31, 31, 0));
    }
}

static void App_DrawCustomBattleEnemyPreview(
    CustomEnemyOption enemyOption,
    int level,
    int showConfirm)
{
    const MemberTemplate *template;
    StatCurveStats stats;

    template = App_GetCustomEnemyTemplate(enemyOption);
    App_GetTemplateStatsAtLevel(template, level, &stats);

    Video_Clear(RGB15(3, 3, 6));
    Video_DrawText(46, 18, "ENEMY LEVEL / STATS", RGB15(31, 31, 31));
    Video_DrawText(24, 42, App_GetCustomEnemyLabel(enemyOption), RGB15(31, 31, 0));
    Video_DrawText(24, 62, "LEVEL", RGB15(31, 31, 31));
    App_DrawNumberText(120, 62, level, RGB15(31, 31, 31));
    Video_DrawText(24, 78, "HP", RGB15(31, 31, 31));
    App_DrawNumberText(120, 78, stats.hp, RGB15(31, 31, 31));
    Video_DrawText(24, 94, "ATTACK", RGB15(31, 31, 31));
    App_DrawNumberText(120, 94, stats.attack, RGB15(31, 31, 31));
    Video_DrawText(24, 110, "DEFENCE", RGB15(31, 31, 31));
    App_DrawNumberText(120, 110, stats.defence, RGB15(31, 31, 31));
    Video_DrawText(24, 126, "SPEED", RGB15(31, 31, 31));
    App_DrawNumberText(120, 126, stats.speed, RGB15(31, 31, 31));
    Video_DrawText(46, 146, "L/R: LEVEL  A: OK", RGB15(31, 31, 0));
    Video_DrawText(88, 156, "B: BACK", RGB15(31, 31, 0));

    if (showConfirm)
    {
        Video_DrawRect(56, 48, 128, 42, RGB15(6, 6, 12));
        Video_DrawText(82, 56, "Looks Good?", RGB15(31, 31, 31));
        Video_DrawText(86, 68, "A Yes", RGB15(31, 31, 0));
        Video_DrawText(86, 80, "B No", RGB15(31, 31, 0));
    }
}

static void App_DrawCustomBattleEnemyPlace(
    CustomEnemyOption enemyOption,
    int level,
    int selectedSlot)
{
    int slot;

    Video_Clear(RGB15(3, 3, 6));
    Video_DrawText(64, 18, "PLACE ENEMY", RGB15(31, 31, 31));
    Video_DrawText(26, 36, App_GetCustomEnemyLabel(enemyOption), RGB15(31, 31, 0));
    Video_DrawText(168, 36, "LV", RGB15(31, 31, 31));
    App_DrawNumberText(190, 36, level, RGB15(31, 31, 31));
    Video_DrawText(70, 56, "ENEMY TILES", RGB15(31, 31, 31));
    for (slot = 0; slot < BATTLE_ENEMY_SLOT_COUNT; slot++)
    {
        int x = 18 + (slot * 44);
        const CustomBattleFormationSlot *formationSlot =
            CustomBattleSetup_GetEnemySlot(slot);

        Video_DrawRect(
            x,
            82,
            36,
            22,
            selectedSlot == slot ? RGB15(10, 10, 18) : RGB15(6, 6, 12));
        Video_DrawText(x + 4, 86, selectedSlot == slot ? ">" : " ", RGB15(31, 31, 0));
        App_DrawNumberText(x + 12, 86, slot, RGB15(31, 31, 31));
        if (formationSlot && formationSlot->occupied)
        {
            Video_DrawText(x + 4, 94, "USED", RGB15(31, 14, 10));
        }
        else
        {
            Video_DrawText(x + 4, 94, "OPEN", RGB15(10, 31, 10));
        }
    }
    Video_DrawText(34, 138, "L/R: SLOT  A: PLACE", RGB15(31, 31, 0));
    Video_DrawText(88, 152, "B: BACK", RGB15(31, 31, 0));
}

static void App_DrawCustomBattleMessage(
    CustomBattleOption selectedOption,
    AppMode returnMode,
    CustomAllyOption allyOption,
    CustomEnemyOption enemyOption,
    int selectedDeleteIsEnemy,
    int level,
    int selectedSlot,
    const char *message)
{
    if (returnMode == APP_MODE_CUSTOM_BATTLE_DELETE)
    {
        App_DrawCustomBattleDelete(selectedDeleteIsEnemy, selectedSlot, 0);
    }
    else if (returnMode == APP_MODE_CUSTOM_BATTLE_SAVE)
    {
        App_DrawCustomBattleSave(selectedSlot, 0);
    }
    else if (returnMode == APP_MODE_CUSTOM_BATTLE_LOAD)
    {
        App_DrawCustomBattleLoad(selectedSlot, 0);
    }
    else if (returnMode == APP_MODE_CUSTOM_BATTLE_ALLY_SELECT)
    {
        App_DrawCustomBattleAllySelect(allyOption);
    }
    else if (returnMode == APP_MODE_CUSTOM_BATTLE_ALLY_PLACE)
    {
        App_DrawCustomBattleAllyPlace(allyOption, level, selectedSlot);
    }
    else if (returnMode == APP_MODE_CUSTOM_BATTLE_ENEMY_SELECT)
    {
        App_DrawCustomBattleEnemySelect(enemyOption);
    }
    else if (returnMode == APP_MODE_CUSTOM_BATTLE_ENEMY_PLACE)
    {
        App_DrawCustomBattleEnemyPlace(enemyOption, level, selectedSlot);
    }
    else
    {
        App_DrawCustomBattleMenu(selectedOption);
    }
    Video_DrawRect(52, 76, 136, 22, RGB15(6, 6, 12));
    Video_DrawText(64, 84, message ? message : "Not ready yet", RGB15(31, 31, 31));
    Video_DrawText(74, 152, "A/B: BACK", RGB15(31, 31, 0));
}

int main(void)
{
    AppMode appMode;
    MenuOption selectedMenuOption;
    MenuOption battleReturnMenuOption;
    OptionsRow selectedOptionsRow;
    CustomBattleOption selectedCustomBattleOption;
    CustomAllyOption selectedCustomAllyOption;
    CustomAllyOption previewCustomAllyOption;
    CustomEnemyOption selectedCustomEnemyOption;
    CustomEnemyOption previewCustomEnemyOption;
    int previewCustomAllyLevel;
    int previewCustomEnemyLevel;
    MoveId previewCustomAllyMoveIds[BATTLE_MEMBER_MOVE_SLOT_COUNT];
    int selectedAllyChooseMoveIndex;
    int editingAllyMoveSlot;
    int selectedAllyLearnableMoveIndex;
    int selectedDeleteIsEnemy;
    int selectedDeleteSlot;
    int selectedSaveSlot;
    int customBattleReviewPage;
    int selectedEnemyPlacementSlot;
    int selectedAllyPlacementSlot;
    AppMode customBattleMessageReturnMode;
    const char *customBattleMessage;
    int menuNeedsDraw;
    unsigned int mainMenuDirtyRows;
    unsigned int optionsMenuDirtyRows;
    int battleFinishedReadyForReturn;
    BattleReturnTarget battleReturnTarget;
    TextSpeed textSpeedOption;
    TutsilDifficulty tutsilDifficultyOption;
    AppLevelRepeatState allyLevelRepeatState;
    AppLevelRepeatState enemyLevelRepeatState;

    System_Init();
    Input_Init();
    Video_Init();

    appMode = APP_MODE_MAIN_MENU;
    selectedMenuOption = MENU_OPTION_PETALBURG_SHOWDOWN;
    battleReturnMenuOption = MENU_OPTION_PETALBURG_SHOWDOWN;
    selectedOptionsRow = OPTIONS_ROW_TEXT_SPEED;
    selectedCustomBattleOption = CUSTOM_BATTLE_OPTION_ADD_ALLY;
    selectedCustomAllyOption = CUSTOM_ALLY_OPTION_PROTAG;
    previewCustomAllyOption = CUSTOM_ALLY_OPTION_PROTAG;
    selectedCustomEnemyOption = CUSTOM_ENEMY_OPTION_TUTSIL;
    previewCustomEnemyOption = CUSTOM_ENEMY_OPTION_TUTSIL;
    previewCustomAllyLevel = 1;
    previewCustomEnemyLevel = 1;
    App_InitCustomAllyPreviewMoves(previewCustomAllyMoveIds);
    selectedAllyChooseMoveIndex = 0;
    editingAllyMoveSlot = 0;
    selectedAllyLearnableMoveIndex = 0;
    selectedDeleteIsEnemy = 0;
    selectedDeleteSlot = 0;
    selectedSaveSlot = 0;
    customBattleReviewPage = 0;
    selectedAllyPlacementSlot = 0;
    selectedEnemyPlacementSlot = 0;
    customBattleMessageReturnMode = APP_MODE_CUSTOM_BATTLE;
    customBattleMessage = "Not ready yet";
    menuNeedsDraw = 1;
    mainMenuDirtyRows = 0;
    optionsMenuDirtyRows = 0;
    battleFinishedReadyForReturn = 0;
    battleReturnTarget = BATTLE_RETURN_MAIN_MENU;
    textSpeedOption = TEXT_SPEED_NORMAL;
    tutsilDifficultyOption = TUTSIL_DIFFICULTY_NORMAL;
    App_ResetLevelRepeatState(&allyLevelRepeatState);
    App_ResetLevelRepeatState(&enemyLevelRepeatState);
    CustomBattleSetup_ResetCurrentFormation();

    while (1)
    {
        AppMode appModeAtFrameStart;

        System_WaitForVBlank();
        Input_Update();
        appModeAtFrameStart = appMode;

        if (appMode == APP_MODE_MAIN_MENU)
        {
            if (menuNeedsDraw)
            {
                App_DrawMainMenu(selectedMenuOption);
                menuNeedsDraw = 0;
                mainMenuDirtyRows = 0;
            }
            else if (mainMenuDirtyRows)
            {
                App_DrawMainMenuDirtyRows(mainMenuDirtyRows, selectedMenuOption);
                mainMenuDirtyRows = 0;
            }
            if (Input_JustPressed(KEY_UP))
            {
                MenuOption previousMenuOption = selectedMenuOption;

                if (selectedMenuOption == MENU_OPTION_PETALBURG_SHOWDOWN)
                {
                    selectedMenuOption = (MenuOption)(MENU_OPTION_COUNT - 1);
                }
                else
                {
                    selectedMenuOption = (MenuOption)(selectedMenuOption - 1);
                }
                mainMenuDirtyRows |=
                    (1u << previousMenuOption) | (1u << selectedMenuOption);
            }
            else if (Input_JustPressed(KEY_DOWN))
            {
                MenuOption previousMenuOption = selectedMenuOption;

                if (selectedMenuOption == (MenuOption)(MENU_OPTION_COUNT - 1))
                {
                    selectedMenuOption = MENU_OPTION_PETALBURG_SHOWDOWN;
                }
                else
                {
                    selectedMenuOption = (MenuOption)(selectedMenuOption + 1);
                }
                mainMenuDirtyRows |=
                    (1u << previousMenuOption) | (1u << selectedMenuOption);
            }
            if (Input_JustPressed(KEY_A))
            {
                if (selectedMenuOption == MENU_OPTION_PETALBURG_SHOWDOWN)
                {
                    App_StartPetalburgShowdown(
                        BATTLE_RETURN_MAIN_MENU,
                        tutsilDifficultyOption,
                        textSpeedOption,
                        BORDER_COLOR_BLUE,
                        &battleReturnTarget);
                    battleReturnMenuOption = MENU_OPTION_PETALBURG_SHOWDOWN;
                    battleFinishedReadyForReturn = 0;
                    appMode = APP_MODE_BATTLE;
                }
                else if (selectedMenuOption == MENU_OPTION_DUMMY_TEST_BATTLE)
                {
                    Video_EnterMode3Bitmap();
                    Battle_InitWithDemoAndSettings(
                        BATTLE_DEMO_DUMMY_TEST,
                        TUTSIL_DIFFICULTY_NORMAL,
                        textSpeedOption,
                        BORDER_COLOR_BLUE);
                    battleReturnTarget = BATTLE_RETURN_MAIN_MENU;
                    battleReturnMenuOption = MENU_OPTION_DUMMY_TEST_BATTLE;
                    battleFinishedReadyForReturn = 0;
                    appMode = APP_MODE_BATTLE;
                }
                else if (selectedMenuOption == MENU_OPTION_OVERWORLD_TEST)
                {
                    Overworld_Init();
                    appMode = APP_MODE_OVERWORLD;
                }
                else if (selectedMenuOption == MENU_OPTION_DIALOGUE_DEMO)
                {
                    DialogueDemo_Enter(textSpeedOption);
                    appMode = APP_MODE_DIALOGUE_DEMO;
                }
                else if (selectedMenuOption == MENU_OPTION_CUSTOM_BATTLE)
                {
                    appMode = APP_MODE_CUSTOM_BATTLE;
                    selectedCustomBattleOption = CUSTOM_BATTLE_OPTION_ADD_ALLY;
                    menuNeedsDraw = 1;
                }
                else
                {
                    appMode = APP_MODE_OPTIONS;
                    selectedOptionsRow = OPTIONS_ROW_TEXT_SPEED;
                    menuNeedsDraw = 1;
                }
            }
        }
        else if (appMode == APP_MODE_OVERWORLD)
        {
            if (Dialogue_IsActive())
            {
                int startCancel = Input_JustPressed(KEY_START);

                /* A advances; B held reveals text, B edge also advances at prompt. */
                Dialogue_Update(
                    Input_JustPressed(KEY_A) || Input_JustPressed(KEY_B),
                    Input_IsPressed(KEY_B),
                    startCancel);
                (void)Dialogue_TakeResult();
                /* START cancels dialogue only; does not open the main menu. */
            }
            else
            {
                int overworldResult = Overworld_Update();

                if (overworldResult == OVERWORLD_UPDATE_EXIT_MENU)
                {
                    Overworld_Shutdown();
                    Video_EnterMode3Bitmap();
                    appMode = APP_MODE_MAIN_MENU;
                    selectedMenuOption = MENU_OPTION_OVERWORLD_TEST;
                    menuNeedsDraw = 1;
                    mainMenuDirtyRows = 0;
                    optionsMenuDirtyRows = 0;
                }
                else if (overworldResult == OVERWORLD_UPDATE_START_DIALOGUE)
                {
                    (void)App_StartFieldDialogue(textSpeedOption);
                }
                else if (overworldResult == OVERWORLD_UPDATE_ENTER_BATTLE)
                {
                    /* Player/camera/latch remain in overworld statics for resume. */
                    App_StartPetalburgShowdown(
                        BATTLE_RETURN_OVERWORLD,
                        tutsilDifficultyOption,
                        textSpeedOption,
                        BORDER_COLOR_BLUE,
                        &battleReturnTarget);
                    battleFinishedReadyForReturn = 0;
                    appMode = APP_MODE_BATTLE;
                }
            }
        }
        else if (appMode == APP_MODE_DIALOGUE_DEMO)
        {
            if (DialogueDemo_Update())
            {
                DialogueDemo_Exit();
                Video_EnterMode3Bitmap();
                appMode = APP_MODE_MAIN_MENU;
                selectedMenuOption = MENU_OPTION_DIALOGUE_DEMO;
                menuNeedsDraw = 1;
                mainMenuDirtyRows = 0;
                optionsMenuDirtyRows = 0;
            }
        }
        else if (appMode == APP_MODE_CUSTOM_BATTLE)
        {
            if (menuNeedsDraw)
            {
                App_DrawCustomBattleMenu(selectedCustomBattleOption);
                menuNeedsDraw = 0;
            }
            if (Input_JustPressed(KEY_UP))
            {
                selectedCustomBattleOption = App_GetCustomBattleMoveUp(
                    selectedCustomBattleOption);
                menuNeedsDraw = 1;
            }
            else if (Input_JustPressed(KEY_DOWN))
            {
                selectedCustomBattleOption = App_GetCustomBattleMoveDown(
                    selectedCustomBattleOption);
                menuNeedsDraw = 1;
            }
            else if (Input_JustPressed(KEY_LEFT))
            {
                selectedCustomBattleOption = App_GetCustomBattleMoveLeft(
                    selectedCustomBattleOption);
                menuNeedsDraw = 1;
            }
            else if (Input_JustPressed(KEY_RIGHT))
            {
                selectedCustomBattleOption = App_GetCustomBattleMoveRight(
                    selectedCustomBattleOption);
                menuNeedsDraw = 1;
            }
            else if (Input_JustPressed(KEY_A))
            {
                if (selectedCustomBattleOption == CUSTOM_BATTLE_OPTION_START_BATTLE)
                {
                    Video_EnterMode3Bitmap();
                    Battle_InitWithCustomFormationAndSettings(
                        CustomBattleSetup_GetCurrentFormationConst(),
                        textSpeedOption,
                        BORDER_COLOR_BLUE);
                    battleReturnTarget = BATTLE_RETURN_MAIN_MENU;
                    battleReturnMenuOption = MENU_OPTION_CUSTOM_BATTLE;
                    battleFinishedReadyForReturn = 0;
                    appMode = APP_MODE_BATTLE;
                }
                else if (selectedCustomBattleOption == CUSTOM_BATTLE_OPTION_ADD_ENEMY)
                {
                    selectedCustomEnemyOption = CUSTOM_ENEMY_OPTION_TUTSIL;
                    appMode = APP_MODE_CUSTOM_BATTLE_ENEMY_SELECT;
                    menuNeedsDraw = 1;
                }
                else if (selectedCustomBattleOption == CUSTOM_BATTLE_OPTION_ADD_ALLY)
                {
                    selectedCustomAllyOption = CUSTOM_ALLY_OPTION_PROTAG;
                    appMode = APP_MODE_CUSTOM_BATTLE_ALLY_SELECT;
                    menuNeedsDraw = 1;
                }
                else if (selectedCustomBattleOption == CUSTOM_BATTLE_OPTION_DELETE_UNIT)
                {
                    selectedDeleteIsEnemy = 0;
                    selectedDeleteSlot = 0;
                    appMode = APP_MODE_CUSTOM_BATTLE_DELETE;
                    menuNeedsDraw = 1;
                }
                else if (selectedCustomBattleOption == CUSTOM_BATTLE_OPTION_REVIEW_BATTLE)
                {
                    customBattleReviewPage = 0;
                    appMode = APP_MODE_CUSTOM_BATTLE_REVIEW;
                    menuNeedsDraw = 1;
                }
                else if (selectedCustomBattleOption == CUSTOM_BATTLE_OPTION_SAVE_BATTLE)
                {
                    selectedSaveSlot = 0;
                    appMode = APP_MODE_CUSTOM_BATTLE_SAVE;
                    menuNeedsDraw = 1;
                }
                else if (selectedCustomBattleOption == CUSTOM_BATTLE_OPTION_LOAD_BATTLE)
                {
                    selectedSaveSlot = 0;
                    appMode = APP_MODE_CUSTOM_BATTLE_LOAD;
                    menuNeedsDraw = 1;
                }
                else
                {
                    customBattleMessage = "Not ready yet";
                    customBattleMessageReturnMode = APP_MODE_CUSTOM_BATTLE;
                    appMode = APP_MODE_CUSTOM_BATTLE_MESSAGE;
                    menuNeedsDraw = 1;
                }
            }
            if (Input_JustPressed(KEY_B) || Input_JustPressed(KEY_START))
            {
                appMode = APP_MODE_MAIN_MENU;
                selectedMenuOption = MENU_OPTION_CUSTOM_BATTLE;
                menuNeedsDraw = 1;
            }
        }
        else if (appMode == APP_MODE_CUSTOM_BATTLE_REVIEW)
        {
            if (menuNeedsDraw)
            {
                App_DrawCustomBattleFormationReview(customBattleReviewPage);
                menuNeedsDraw = 0;
            }
            if (Input_JustPressed(KEY_LEFT) || Input_JustPressed(KEY_RIGHT))
            {
                customBattleReviewPage = customBattleReviewPage == 0 ? 1 : 0;
                menuNeedsDraw = 1;
            }
            if (Input_JustPressed(KEY_B)
                || Input_JustPressed(KEY_START)
                || Input_JustPressed(KEY_A))
            {
                appMode = APP_MODE_CUSTOM_BATTLE;
                menuNeedsDraw = 1;
            }
        }
        else if (appMode == APP_MODE_CUSTOM_BATTLE_DELETE)
        {
            if (menuNeedsDraw)
            {
                App_DrawCustomBattleDelete(selectedDeleteIsEnemy, selectedDeleteSlot, 0);
                menuNeedsDraw = 0;
            }
            if (Input_JustPressed(KEY_UP) || Input_JustPressed(KEY_DOWN))
            {
                selectedDeleteIsEnemy = !selectedDeleteIsEnemy;
                menuNeedsDraw = 1;
            }
            else if (Input_JustPressed(KEY_LEFT))
            {
                if (selectedDeleteSlot == 0)
                {
                    selectedDeleteSlot = BATTLE_PLAYER_SLOT_COUNT - 1;
                }
                else
                {
                    selectedDeleteSlot--;
                }
                menuNeedsDraw = 1;
            }
            else if (Input_JustPressed(KEY_RIGHT))
            {
                if (selectedDeleteSlot == BATTLE_PLAYER_SLOT_COUNT - 1)
                {
                    selectedDeleteSlot = 0;
                }
                else
                {
                    selectedDeleteSlot++;
                }
                menuNeedsDraw = 1;
            }
            else if (Input_JustPressed(KEY_A))
            {
                int occupied = selectedDeleteIsEnemy
                    ? CustomBattleSetup_IsEnemySlotOccupied(selectedDeleteSlot)
                    : CustomBattleSetup_IsAllySlotOccupied(selectedDeleteSlot);

                if (!occupied)
                {
                    customBattleMessage = "Slot empty";
                    customBattleMessageReturnMode = APP_MODE_CUSTOM_BATTLE_DELETE;
                    appMode = APP_MODE_CUSTOM_BATTLE_MESSAGE;
                }
                else
                {
                    appMode = APP_MODE_CUSTOM_BATTLE_DELETE_CONFIRM;
                }
                menuNeedsDraw = 1;
            }
            if (Input_JustPressed(KEY_B))
            {
                appMode = APP_MODE_CUSTOM_BATTLE;
                menuNeedsDraw = 1;
            }
        }
        else if (appMode == APP_MODE_CUSTOM_BATTLE_DELETE_CONFIRM)
        {
            if (menuNeedsDraw)
            {
                App_DrawCustomBattleDelete(selectedDeleteIsEnemy, selectedDeleteSlot, 1);
                menuNeedsDraw = 0;
            }
            if (Input_JustPressed(KEY_A))
            {
                if (selectedDeleteIsEnemy)
                {
                    CustomBattleSetup_ClearEnemySlot(selectedDeleteSlot);
                }
                else
                {
                    CustomBattleSetup_ClearAllySlot(selectedDeleteSlot);
                }
                customBattleMessage = "Unit deleted";
                customBattleMessageReturnMode = APP_MODE_CUSTOM_BATTLE_DELETE;
                appMode = APP_MODE_CUSTOM_BATTLE_MESSAGE;
                menuNeedsDraw = 1;
            }
            if (Input_JustPressed(KEY_B))
            {
                appMode = APP_MODE_CUSTOM_BATTLE_DELETE;
                menuNeedsDraw = 1;
            }
        }
        else if (appMode == APP_MODE_CUSTOM_BATTLE_SAVE)
        {
            if (menuNeedsDraw)
            {
                App_DrawCustomBattleSave(selectedSaveSlot, 0);
                menuNeedsDraw = 0;
            }
            if (Input_JustPressed(KEY_UP))
            {
                if (selectedSaveSlot == 0)
                {
                    selectedSaveSlot = CUSTOM_BATTLE_SAVE_SLOT_COUNT - 1;
                }
                else
                {
                    selectedSaveSlot--;
                }
                menuNeedsDraw = 1;
            }
            else if (Input_JustPressed(KEY_DOWN))
            {
                if (selectedSaveSlot == CUSTOM_BATTLE_SAVE_SLOT_COUNT - 1)
                {
                    selectedSaveSlot = 0;
                }
                else
                {
                    selectedSaveSlot++;
                }
                menuNeedsDraw = 1;
            }
            else if (Input_JustPressed(KEY_A))
            {
                appMode = APP_MODE_CUSTOM_BATTLE_SAVE_CONFIRM;
                menuNeedsDraw = 1;
            }
            if (Input_JustPressed(KEY_B))
            {
                appMode = APP_MODE_CUSTOM_BATTLE;
                menuNeedsDraw = 1;
            }
        }
        else if (appMode == APP_MODE_CUSTOM_BATTLE_SAVE_CONFIRM)
        {
            if (menuNeedsDraw)
            {
                App_DrawCustomBattleSave(selectedSaveSlot, 1);
                menuNeedsDraw = 0;
            }
            if (Input_JustPressed(KEY_A))
            {
                if (CustomBattleSetup_SaveCurrentToSlot(selectedSaveSlot))
                {
                    customBattleMessage = "Battle saved";
                    customBattleMessageReturnMode = APP_MODE_CUSTOM_BATTLE;
                    appMode = APP_MODE_CUSTOM_BATTLE_MESSAGE;
                }
                else
                {
                    customBattleMessage = "Not ready yet";
                    customBattleMessageReturnMode = APP_MODE_CUSTOM_BATTLE_SAVE;
                    appMode = APP_MODE_CUSTOM_BATTLE_MESSAGE;
                }
                menuNeedsDraw = 1;
            }
            if (Input_JustPressed(KEY_B))
            {
                appMode = APP_MODE_CUSTOM_BATTLE_SAVE;
                menuNeedsDraw = 1;
            }
        }
        else if (appMode == APP_MODE_CUSTOM_BATTLE_LOAD)
        {
            if (menuNeedsDraw)
            {
                App_DrawCustomBattleLoad(selectedSaveSlot, 0);
                menuNeedsDraw = 0;
            }
            if (Input_JustPressed(KEY_UP))
            {
                if (selectedSaveSlot == 0)
                {
                    selectedSaveSlot = CUSTOM_BATTLE_SAVE_SLOT_COUNT - 1;
                }
                else
                {
                    selectedSaveSlot--;
                }
                menuNeedsDraw = 1;
            }
            else if (Input_JustPressed(KEY_DOWN))
            {
                if (selectedSaveSlot == CUSTOM_BATTLE_SAVE_SLOT_COUNT - 1)
                {
                    selectedSaveSlot = 0;
                }
                else
                {
                    selectedSaveSlot++;
                }
                menuNeedsDraw = 1;
            }
            else if (Input_JustPressed(KEY_A))
            {
                if (!CustomBattleSetup_IsSaveSlotOccupied(selectedSaveSlot))
                {
                    customBattleMessage = "Slot empty";
                    customBattleMessageReturnMode = APP_MODE_CUSTOM_BATTLE_LOAD;
                    appMode = APP_MODE_CUSTOM_BATTLE_MESSAGE;
                }
                else
                {
                    appMode = APP_MODE_CUSTOM_BATTLE_LOAD_CONFIRM;
                }
                menuNeedsDraw = 1;
            }
            if (Input_JustPressed(KEY_B))
            {
                appMode = APP_MODE_CUSTOM_BATTLE;
                menuNeedsDraw = 1;
            }
        }
        else if (appMode == APP_MODE_CUSTOM_BATTLE_LOAD_CONFIRM)
        {
            if (menuNeedsDraw)
            {
                App_DrawCustomBattleLoad(selectedSaveSlot, 1);
                menuNeedsDraw = 0;
            }
            if (Input_JustPressed(KEY_A))
            {
                if (CustomBattleSetup_LoadSlotToCurrent(selectedSaveSlot))
                {
                    customBattleMessage = "Battle loaded";
                    customBattleMessageReturnMode = APP_MODE_CUSTOM_BATTLE;
                    appMode = APP_MODE_CUSTOM_BATTLE_MESSAGE;
                }
                else
                {
                    customBattleMessage = "Slot empty";
                    customBattleMessageReturnMode = APP_MODE_CUSTOM_BATTLE_LOAD;
                    appMode = APP_MODE_CUSTOM_BATTLE_MESSAGE;
                }
                menuNeedsDraw = 1;
            }
            if (Input_JustPressed(KEY_B))
            {
                appMode = APP_MODE_CUSTOM_BATTLE_LOAD;
                menuNeedsDraw = 1;
            }
        }
        else if (appMode == APP_MODE_CUSTOM_BATTLE_ALLY_SELECT)
        {
            if (menuNeedsDraw)
            {
                App_DrawCustomBattleAllySelect(selectedCustomAllyOption);
                menuNeedsDraw = 0;
            }
            if (Input_JustPressed(KEY_LEFT) || Input_JustPressed(KEY_UP))
            {
                App_AdvanceCustomAllyOption(&selectedCustomAllyOption, -1);
                menuNeedsDraw = 1;
            }
            else if (Input_JustPressed(KEY_RIGHT) || Input_JustPressed(KEY_DOWN))
            {
                App_AdvanceCustomAllyOption(&selectedCustomAllyOption, 1);
                menuNeedsDraw = 1;
            }
            else if (Input_JustPressed(KEY_A))
            {
                const MemberTemplate *template = App_GetCustomAllyTemplate(selectedCustomAllyOption);

                if (!App_GetCustomAllyReady(selectedCustomAllyOption) || !template)
                {
                    customBattleMessage = "Not ready yet";
                    customBattleMessageReturnMode = APP_MODE_CUSTOM_BATTLE_ALLY_SELECT;
                    appMode = APP_MODE_CUSTOM_BATTLE_MESSAGE;
                }
                else
                {
                    previewCustomAllyOption = selectedCustomAllyOption;
                    previewCustomAllyLevel = template->startingLevel;
                    if (previewCustomAllyLevel < BATTLE_MEMBER_LEVEL_MIN)
                    {
                        previewCustomAllyLevel = BATTLE_MEMBER_LEVEL_MIN;
                    }
                    if (previewCustomAllyLevel > STAT_CURVE_NATURAL_LEVEL_MAX)
                    {
                        previewCustomAllyLevel = STAT_CURVE_NATURAL_LEVEL_MAX;
                    }
                    App_ResetLevelRepeatState(&allyLevelRepeatState);
                    appMode = APP_MODE_CUSTOM_BATTLE_ALLY_PREVIEW;
                }
                menuNeedsDraw = 1;
            }
            if (Input_JustPressed(KEY_B))
            {
                appMode = APP_MODE_CUSTOM_BATTLE;
                menuNeedsDraw = 1;
            }
        }
        else if (appMode == APP_MODE_CUSTOM_BATTLE_ALLY_PREVIEW)
        {
            if (App_TickLevelHoldRepeat(
                    &allyLevelRepeatState,
                    &previewCustomAllyLevel,
                    BATTLE_MEMBER_LEVEL_MIN,
                    STAT_CURVE_NATURAL_LEVEL_MAX))
            {
                menuNeedsDraw = 1;
            }
            if (Input_JustPressed(KEY_A))
            {
                appMode = APP_MODE_CUSTOM_BATTLE_ALLY_CONFIRM;
                menuNeedsDraw = 1;
            }
            if (Input_JustPressed(KEY_B))
            {
                App_ResetLevelRepeatState(&allyLevelRepeatState);
                appMode = APP_MODE_CUSTOM_BATTLE_ALLY_SELECT;
                menuNeedsDraw = 1;
            }
            if (menuNeedsDraw && appMode == appModeAtFrameStart)
            {
                App_DrawCustomBattleAllyPreview(
                    previewCustomAllyOption,
                    previewCustomAllyLevel,
                    0);
                menuNeedsDraw = 0;
            }
        }
        else if (appMode == APP_MODE_CUSTOM_BATTLE_ALLY_CONFIRM)
        {
            if (Input_JustPressed(KEY_A))
            {
                App_InitCustomAllyPreviewMoves(previewCustomAllyMoveIds);
                selectedAllyChooseMoveIndex = 0;
                appMode = APP_MODE_CUSTOM_BATTLE_ALLY_CHOOSE_MOVES;
                menuNeedsDraw = 1;
            }
            if (Input_JustPressed(KEY_B))
            {
                appMode = APP_MODE_CUSTOM_BATTLE_ALLY_PREVIEW;
                menuNeedsDraw = 1;
            }
            if (menuNeedsDraw && appMode == appModeAtFrameStart)
            {
                App_DrawCustomBattleAllyPreview(
                    previewCustomAllyOption,
                    previewCustomAllyLevel,
                    1);
                menuNeedsDraw = 0;
            }
        }
        else if (appMode == APP_MODE_CUSTOM_BATTLE_ALLY_CHOOSE_MOVES)
        {
            if (Input_JustPressed(KEY_UP))
            {
                if (selectedAllyChooseMoveIndex == 0)
                {
                    selectedAllyChooseMoveIndex = CUSTOM_ALLY_CHOOSE_MOVE_LOOKS_GOOD;
                }
                else
                {
                    selectedAllyChooseMoveIndex--;
                }
                menuNeedsDraw = 1;
            }
            else if (Input_JustPressed(KEY_DOWN))
            {
                if (selectedAllyChooseMoveIndex == CUSTOM_ALLY_CHOOSE_MOVE_LOOKS_GOOD)
                {
                    selectedAllyChooseMoveIndex = 0;
                }
                else
                {
                    selectedAllyChooseMoveIndex++;
                }
                menuNeedsDraw = 1;
            }
            else if (Input_JustPressed(KEY_A))
            {
                if (selectedAllyChooseMoveIndex == CUSTOM_ALLY_CHOOSE_MOVE_LOOKS_GOOD)
                {
                    selectedAllyPlacementSlot = BATTLE_CENTER_SLOT;
                    appMode = APP_MODE_CUSTOM_BATTLE_ALLY_PLACE;
                }
                else
                {
                    editingAllyMoveSlot = selectedAllyChooseMoveIndex;
                    selectedAllyLearnableMoveIndex = 0;
                    appMode = APP_MODE_CUSTOM_BATTLE_ALLY_MOVE_LIST;
                }
                menuNeedsDraw = 1;
            }
            if (Input_JustPressed(KEY_B))
            {
                appMode = APP_MODE_CUSTOM_BATTLE_ALLY_PREVIEW;
                menuNeedsDraw = 1;
            }
            if (menuNeedsDraw && appMode == appModeAtFrameStart)
            {
                App_DrawCustomBattleAllyChooseMoves(
                    previewCustomAllyOption,
                    previewCustomAllyMoveIds,
                    selectedAllyChooseMoveIndex);
                menuNeedsDraw = 0;
            }
        }
        else if (appMode == APP_MODE_CUSTOM_BATTLE_ALLY_MOVE_LIST)
        {
            int moveCount = App_GetCustomAllyLearnableMoveCount(previewCustomAllyOption);
            const MoveId *learnableMoves = App_GetCustomAllyLearnableMoves(previewCustomAllyOption);

            if (moveCount > 0)
            {
                if (Input_JustPressed(KEY_UP))
                {
                    if (selectedAllyLearnableMoveIndex == 0)
                    {
                        selectedAllyLearnableMoveIndex = moveCount - 1;
                    }
                    else
                    {
                        selectedAllyLearnableMoveIndex--;
                    }
                    menuNeedsDraw = 1;
                }
                else if (Input_JustPressed(KEY_DOWN))
                {
                    if (selectedAllyLearnableMoveIndex == moveCount - 1)
                    {
                        selectedAllyLearnableMoveIndex = 0;
                    }
                    else
                    {
                        selectedAllyLearnableMoveIndex++;
                    }
                    menuNeedsDraw = 1;
                }
                else if (Input_JustPressed(KEY_A))
                {
                    if (learnableMoves
                        && selectedAllyLearnableMoveIndex >= 0
                        && selectedAllyLearnableMoveIndex < moveCount)
                    {
                        previewCustomAllyMoveIds[editingAllyMoveSlot] =
                            learnableMoves[selectedAllyLearnableMoveIndex];
                    }
                    appMode = APP_MODE_CUSTOM_BATTLE_ALLY_CHOOSE_MOVES;
                    menuNeedsDraw = 1;
                }
            }
            if (Input_JustPressed(KEY_B))
            {
                appMode = APP_MODE_CUSTOM_BATTLE_ALLY_CHOOSE_MOVES;
                menuNeedsDraw = 1;
            }
            if (menuNeedsDraw && appMode == appModeAtFrameStart)
            {
                App_DrawCustomBattleAllyMoveList(
                    previewCustomAllyOption,
                    editingAllyMoveSlot,
                    selectedAllyLearnableMoveIndex);
                menuNeedsDraw = 0;
            }
        }
        else if (appMode == APP_MODE_CUSTOM_BATTLE_ALLY_PLACE)
        {
            if (Input_JustPressed(KEY_LEFT))
            {
                if (selectedAllyPlacementSlot == 0)
                {
                    selectedAllyPlacementSlot = BATTLE_PLAYER_SLOT_COUNT - 1;
                }
                else
                {
                    selectedAllyPlacementSlot--;
                }
                menuNeedsDraw = 1;
            }
            else if (Input_JustPressed(KEY_RIGHT))
            {
                if (selectedAllyPlacementSlot == BATTLE_PLAYER_SLOT_COUNT - 1)
                {
                    selectedAllyPlacementSlot = 0;
                }
                else
                {
                    selectedAllyPlacementSlot++;
                }
                menuNeedsDraw = 1;
            }
            else if (Input_JustPressed(KEY_A))
            {
                if (CustomBattleSetup_IsAllySlotOccupied(selectedAllyPlacementSlot))
                {
                    customBattleMessage = "Slot occupied";
                    customBattleMessageReturnMode = APP_MODE_CUSTOM_BATTLE_ALLY_PLACE;
                    appMode = APP_MODE_CUSTOM_BATTLE_MESSAGE;
                }
                else if (CustomBattleSetup_SetAllySlotWithMoves(
                             selectedAllyPlacementSlot,
                             App_GetCustomAllyTemplateId(previewCustomAllyOption),
                             previewCustomAllyLevel,
                             previewCustomAllyMoveIds))
                {
                    appMode = APP_MODE_CUSTOM_BATTLE;
                }
                else
                {
                    customBattleMessage = "Not ready yet";
                    customBattleMessageReturnMode = APP_MODE_CUSTOM_BATTLE_ALLY_PLACE;
                    appMode = APP_MODE_CUSTOM_BATTLE_MESSAGE;
                }
                menuNeedsDraw = 1;
            }
            if (Input_JustPressed(KEY_B))
            {
                appMode = APP_MODE_CUSTOM_BATTLE_ALLY_CHOOSE_MOVES;
                menuNeedsDraw = 1;
            }
            if (menuNeedsDraw && appMode == appModeAtFrameStart)
            {
                App_DrawCustomBattleAllyPlace(
                    previewCustomAllyOption,
                    previewCustomAllyLevel,
                    selectedAllyPlacementSlot);
                menuNeedsDraw = 0;
            }
        }
        else if (appMode == APP_MODE_CUSTOM_BATTLE_ENEMY_SELECT)
        {
            if (menuNeedsDraw)
            {
                App_DrawCustomBattleEnemySelect(selectedCustomEnemyOption);
                menuNeedsDraw = 0;
            }
            if (Input_JustPressed(KEY_LEFT) || Input_JustPressed(KEY_UP))
            {
                App_AdvanceCustomEnemyOption(&selectedCustomEnemyOption, -1);
                menuNeedsDraw = 1;
            }
            else if (Input_JustPressed(KEY_RIGHT) || Input_JustPressed(KEY_DOWN))
            {
                App_AdvanceCustomEnemyOption(&selectedCustomEnemyOption, 1);
                menuNeedsDraw = 1;
            }
            else if (Input_JustPressed(KEY_A))
            {
                const MemberTemplate *template = App_GetCustomEnemyTemplate(selectedCustomEnemyOption);

                if (!App_GetCustomEnemyReady(selectedCustomEnemyOption) || !template)
                {
                    customBattleMessage = "Not ready yet";
                    customBattleMessageReturnMode = APP_MODE_CUSTOM_BATTLE_ENEMY_SELECT;
                    appMode = APP_MODE_CUSTOM_BATTLE_MESSAGE;
                }
                else
                {
                    previewCustomEnemyOption = selectedCustomEnemyOption;
                    previewCustomEnemyLevel = template->startingLevel;
                    if (previewCustomEnemyLevel < BATTLE_MEMBER_LEVEL_MIN)
                    {
                        previewCustomEnemyLevel = BATTLE_MEMBER_LEVEL_MIN;
                    }
                    if (previewCustomEnemyLevel > STAT_CURVE_NATURAL_LEVEL_MAX)
                    {
                        previewCustomEnemyLevel = STAT_CURVE_NATURAL_LEVEL_MAX;
                    }
                    App_ResetLevelRepeatState(&enemyLevelRepeatState);
                    appMode = APP_MODE_CUSTOM_BATTLE_ENEMY_PREVIEW;
                }
                menuNeedsDraw = 1;
            }
            if (Input_JustPressed(KEY_B))
            {
                appMode = APP_MODE_CUSTOM_BATTLE;
                menuNeedsDraw = 1;
            }
        }
        else if (appMode == APP_MODE_CUSTOM_BATTLE_ENEMY_PREVIEW)
        {
            if (App_TickLevelHoldRepeat(
                    &enemyLevelRepeatState,
                    &previewCustomEnemyLevel,
                    BATTLE_MEMBER_LEVEL_MIN,
                    STAT_CURVE_NATURAL_LEVEL_MAX))
            {
                menuNeedsDraw = 1;
            }
            if (Input_JustPressed(KEY_A))
            {
                appMode = APP_MODE_CUSTOM_BATTLE_ENEMY_CONFIRM;
                menuNeedsDraw = 1;
            }
            if (Input_JustPressed(KEY_B))
            {
                App_ResetLevelRepeatState(&enemyLevelRepeatState);
                appMode = APP_MODE_CUSTOM_BATTLE_ENEMY_SELECT;
                menuNeedsDraw = 1;
            }
            if (menuNeedsDraw && appMode == appModeAtFrameStart)
            {
                App_DrawCustomBattleEnemyPreview(
                    previewCustomEnemyOption,
                    previewCustomEnemyLevel,
                    0);
                menuNeedsDraw = 0;
            }
        }
        else if (appMode == APP_MODE_CUSTOM_BATTLE_ENEMY_CONFIRM)
        {
            if (Input_JustPressed(KEY_A))
            {
                selectedEnemyPlacementSlot = BATTLE_CENTER_SLOT;
                appMode = APP_MODE_CUSTOM_BATTLE_ENEMY_PLACE;
                menuNeedsDraw = 1;
            }
            if (Input_JustPressed(KEY_B))
            {
                appMode = APP_MODE_CUSTOM_BATTLE_ENEMY_PREVIEW;
                menuNeedsDraw = 1;
            }
            if (menuNeedsDraw && appMode == appModeAtFrameStart)
            {
                App_DrawCustomBattleEnemyPreview(
                    previewCustomEnemyOption,
                    previewCustomEnemyLevel,
                    1);
                menuNeedsDraw = 0;
            }
        }
        else if (appMode == APP_MODE_CUSTOM_BATTLE_ENEMY_PLACE)
        {
            if (Input_JustPressed(KEY_LEFT))
            {
                if (selectedEnemyPlacementSlot == 0)
                {
                    selectedEnemyPlacementSlot = BATTLE_ENEMY_SLOT_COUNT - 1;
                }
                else
                {
                    selectedEnemyPlacementSlot--;
                }
                menuNeedsDraw = 1;
            }
            else if (Input_JustPressed(KEY_RIGHT))
            {
                if (selectedEnemyPlacementSlot == BATTLE_ENEMY_SLOT_COUNT - 1)
                {
                    selectedEnemyPlacementSlot = 0;
                }
                else
                {
                    selectedEnemyPlacementSlot++;
                }
                menuNeedsDraw = 1;
            }
            else if (Input_JustPressed(KEY_A))
            {
                if (CustomBattleSetup_IsEnemySlotOccupied(selectedEnemyPlacementSlot))
                {
                    customBattleMessage = "Slot occupied";
                    customBattleMessageReturnMode = APP_MODE_CUSTOM_BATTLE_ENEMY_PLACE;
                    appMode = APP_MODE_CUSTOM_BATTLE_MESSAGE;
                }
                else if (CustomBattleSetup_SetEnemySlot(
                             selectedEnemyPlacementSlot,
                             App_GetCustomEnemyTemplateId(previewCustomEnemyOption),
                             previewCustomEnemyLevel))
                {
                    appMode = APP_MODE_CUSTOM_BATTLE;
                }
                else
                {
                    customBattleMessage = "Not ready yet";
                    customBattleMessageReturnMode = APP_MODE_CUSTOM_BATTLE_ENEMY_PLACE;
                    appMode = APP_MODE_CUSTOM_BATTLE_MESSAGE;
                }
                menuNeedsDraw = 1;
            }
            if (Input_JustPressed(KEY_B))
            {
                appMode = APP_MODE_CUSTOM_BATTLE_ENEMY_PREVIEW;
                menuNeedsDraw = 1;
            }
            if (menuNeedsDraw && appMode == appModeAtFrameStart)
            {
                App_DrawCustomBattleEnemyPlace(
                    previewCustomEnemyOption,
                    previewCustomEnemyLevel,
                    selectedEnemyPlacementSlot);
                menuNeedsDraw = 0;
            }
        }
        else if (appMode == APP_MODE_CUSTOM_BATTLE_MESSAGE)
        {
            if (menuNeedsDraw)
            {
                App_DrawCustomBattleMessage(
                    selectedCustomBattleOption,
                    customBattleMessageReturnMode,
                    previewCustomAllyOption,
                    previewCustomEnemyOption,
                    selectedDeleteIsEnemy,
                    customBattleMessageReturnMode == APP_MODE_CUSTOM_BATTLE_ALLY_SELECT ||
                            customBattleMessageReturnMode == APP_MODE_CUSTOM_BATTLE_ALLY_PLACE
                        ? previewCustomAllyLevel
                        : previewCustomEnemyLevel,
                    customBattleMessageReturnMode == APP_MODE_CUSTOM_BATTLE_DELETE
                        ? selectedDeleteSlot
                        : customBattleMessageReturnMode == APP_MODE_CUSTOM_BATTLE_SAVE
                            || customBattleMessageReturnMode == APP_MODE_CUSTOM_BATTLE_LOAD
                        ? selectedSaveSlot
                        : customBattleMessageReturnMode == APP_MODE_CUSTOM_BATTLE_ALLY_PLACE
                        ? selectedAllyPlacementSlot
                        : selectedEnemyPlacementSlot,
                    customBattleMessage);
                menuNeedsDraw = 0;
            }
            if (Input_JustPressed(KEY_A)
                || Input_JustPressed(KEY_B)
                || Input_JustPressed(KEY_START))
            {
                appMode = customBattleMessageReturnMode;
                menuNeedsDraw = 1;
            }
        }
        else if (appMode == APP_MODE_OPTIONS)
        {
            if (menuNeedsDraw)
            {
                App_DrawOptionsMenu(
                    selectedOptionsRow,
                    textSpeedOption,
                    tutsilDifficultyOption);
                menuNeedsDraw = 0;
                optionsMenuDirtyRows = 0;
            }
            else if (optionsMenuDirtyRows)
            {
                App_DrawOptionsMenuDirtyRows(
                    optionsMenuDirtyRows,
                    selectedOptionsRow,
                    textSpeedOption,
                    tutsilDifficultyOption);
                optionsMenuDirtyRows = 0;
            }
            if (Input_JustPressed(KEY_UP))
            {
                OptionsRow previousOptionsRow = selectedOptionsRow;

                if (selectedOptionsRow == OPTIONS_ROW_TEXT_SPEED)
                {
                    selectedOptionsRow = (OptionsRow)(OPTIONS_ROW_COUNT - 1);
                }
                else
                {
                    selectedOptionsRow = (OptionsRow)(selectedOptionsRow - 1);
                }
                optionsMenuDirtyRows |=
                    (1u << previousOptionsRow) | (1u << selectedOptionsRow);
            }
            else if (Input_JustPressed(KEY_DOWN))
            {
                OptionsRow previousOptionsRow = selectedOptionsRow;

                if (selectedOptionsRow == (OptionsRow)(OPTIONS_ROW_COUNT - 1))
                {
                    selectedOptionsRow = OPTIONS_ROW_TEXT_SPEED;
                }
                else
                {
                    selectedOptionsRow = (OptionsRow)(selectedOptionsRow + 1);
                }
                optionsMenuDirtyRows |=
                    (1u << previousOptionsRow) | (1u << selectedOptionsRow);
            }
            else if (Input_JustPressed(KEY_LEFT))
            {
                if (selectedOptionsRow == OPTIONS_ROW_TEXT_SPEED)
                {
                    textSpeedOption = (TextSpeed)(
                        (textSpeedOption + TEXT_SPEED_COUNT - 1) % TEXT_SPEED_COUNT);
                }
                else
                {
                    tutsilDifficultyOption = (TutsilDifficulty)(
                        (tutsilDifficultyOption + TUTSIL_DIFFICULTY_COUNT - 1)
                        % TUTSIL_DIFFICULTY_COUNT);
                }
                optionsMenuDirtyRows |= (1u << selectedOptionsRow);
            }
            else if (Input_JustPressed(KEY_RIGHT) || Input_JustPressed(KEY_A))
            {
                if (selectedOptionsRow == OPTIONS_ROW_TEXT_SPEED)
                {
                    textSpeedOption = (TextSpeed)(
                        (textSpeedOption + 1) % TEXT_SPEED_COUNT);
                }
                else
                {
                    tutsilDifficultyOption = (TutsilDifficulty)(
                        (tutsilDifficultyOption + 1) % TUTSIL_DIFFICULTY_COUNT);
                }
                optionsMenuDirtyRows |= (1u << selectedOptionsRow);
            }
            if (Input_JustPressed(KEY_B))
            {
                appMode = APP_MODE_MAIN_MENU;
                selectedMenuOption = MENU_OPTION_OPTIONS;
                menuNeedsDraw = 1;
            }
        }
        else
        {
            Battle_Update();
            Battle_Draw();

            if (Battle_IsFinished())
            {
                if (battleFinishedReadyForReturn
                    && (Input_JustPressed(KEY_A)
                        || Input_JustPressed(KEY_B)
                        || Input_JustPressed(KEY_START)))
                {
                    if (battleReturnTarget == BATTLE_RETURN_OVERWORLD)
                    {
                        Overworld_ResumeAfterBattle();
                        battleFinishedReadyForReturn = 0;
                        battleReturnTarget = BATTLE_RETURN_MAIN_MENU;
                        appMode = APP_MODE_OVERWORLD;
                    }
                    else
                    {
                        appMode = APP_MODE_MAIN_MENU;
                        selectedMenuOption = battleReturnMenuOption;
                        menuNeedsDraw = 1;
                        battleFinishedReadyForReturn = 0;
                    }
                }
                else
                {
                    battleFinishedReadyForReturn = 1;
                }
            }
            else
            {
                battleFinishedReadyForReturn = 0;
            }
        }
    }

    return 0;
}
