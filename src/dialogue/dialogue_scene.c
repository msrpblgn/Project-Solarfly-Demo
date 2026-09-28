#include "dialogue/dialogue_scene.h"

#include "core/video.h"

static const DialogueTextCommand s_textAksilChallenge[] = {
    DIALOGUE_CMD_TEXT("YOU THOUGHT THAT WOULD WORK?"),
    DIALOGUE_CMD_NEWLINE(),
    DIALOGUE_CMD_TEXT("COME ON. ONE MORE TRY."),
    DIALOGUE_CMD_END()
};

static const DialogueTextCommand s_textProtagonistReply[] = {
    DIALOGUE_CMD_TEXT("IT ALMOST DID."),
    DIALOGUE_CMD_END()
};

static const DialogueTextCommand s_textAksilPacing[] = {
    DIALOGUE_CMD_TEXT("WATCH THIS."),
    DIALOGUE_CMD_PAUSE(30),
    DIALOGUE_CMD_WAIT(),
    DIALOGUE_CMD_NEWLINE(),
    DIALOGUE_CMD_TEXT("SAME PAGE AFTER THE WAIT."),
    DIALOGUE_CMD_PAGE(),
    DIALOGUE_CMD_SPEED(TEXT_SPEED_SLOW),
    DIALOGUE_CMD_TEXT("SLOW. "),
    DIALOGUE_CMD_SPEED(TEXT_SPEED_FAST),
    DIALOGUE_CMD_TEXT("FAST. "),
    DIALOGUE_CMD_SPEED(DIALOGUE_TEXT_SPEED_RESTORE_BASE),
    DIALOGUE_CMD_TEXT("BASE."),
    DIALOGUE_CMD_NEWLINE(),
    DIALOGUE_CMD_SFX(DIALOGUE_TEXT_SFX_CUE_DEMO),
    DIALOGUE_CMD_COLOR(DIALOGUE_TEXT_COLOR_EMPHASIS),
    DIALOGUE_CMD_TEXT("ONE CUE. "),
    DIALOGUE_CMD_COLOR(DIALOGUE_TEXT_COLOR_DEFAULT),
    DIALOGUE_CMD_TEXT("THEN DONE."),
    DIALOGUE_CMD_END()
};

static const DialogueSceneCommand s_sharedConversationCommands[] = {
    DIALOGUE_SCENE_CMD_ENTER_PAIR(0),
    DIALOGUE_SCENE_CMD_SAY(
        DIALOGUE_PORTRAIT_SIDE_RIGHT,
        DIALOGUE_PORTRAIT_ACTOR_AKSIL,
        s_textAksilChallenge,
        (int)(sizeof(s_textAksilChallenge) / sizeof(s_textAksilChallenge[0]))),
    DIALOGUE_SCENE_CMD_SAY(
        DIALOGUE_PORTRAIT_SIDE_LEFT,
        DIALOGUE_PORTRAIT_ACTOR_PROTAGONIST,
        s_textProtagonistReply,
        (int)(sizeof(s_textProtagonistReply) / sizeof(s_textProtagonistReply[0]))),
    DIALOGUE_SCENE_CMD_SAY(
        DIALOGUE_PORTRAIT_SIDE_RIGHT,
        DIALOGUE_PORTRAIT_ACTOR_AKSIL,
        s_textAksilPacing,
        (int)(sizeof(s_textAksilPacing) / sizeof(s_textAksilPacing[0]))),
    DIALOGUE_SCENE_CMD_EVENT(DIALOGUE_EVENT_TEST_COMPLETE),
    DIALOGUE_SCENE_CMD_EXIT_BOTH(),
    DIALOGUE_SCENE_CMD_END()
};

static const DialogueScene s_sharedConversation = {
    "shared_conversation",
    s_sharedConversationCommands,
    (int)(sizeof(s_sharedConversationCommands) / sizeof(s_sharedConversationCommands[0]))
};

static const DialogueSceneCommand s_replacementTestCommands[] = {
    DIALOGUE_SCENE_CMD_ENTER_PAIR(0),
    DIALOGUE_SCENE_CMD_TECH_REPLACE(),
    DIALOGUE_SCENE_CMD_END()
};

static const DialogueScene s_replacementTest = {
    "replacement_test",
    s_replacementTestCommands,
    (int)(sizeof(s_replacementTestCommands) / sizeof(s_replacementTestCommands[0]))
};

const DialogueScene *DialogueScene_GetSharedConversation(void)
{
    return &s_sharedConversation;
}

const DialogueScene *DialogueScene_GetReplacementTest(void)
{
    return &s_replacementTest;
}

static void DialogueScene_WriteError(char *error, int errorSize, const char *message)
{
    int i;

    if (!error || errorSize <= 0)
    {
        return;
    }
    if (!message)
    {
        error[0] = '\0';
        return;
    }
    for (i = 0; i < errorSize - 1 && message[i] != '\0'; i++)
    {
        error[i] = message[i];
    }
    error[i] = '\0';
}

int DialogueScene_Validate(const DialogueScene *scene, char *error, int errorSize)
{
    int i;
    int sawEnd = 0;
    int occupied[2];
    u8 actors[2];
    char textError[48];

    occupied[0] = 0;
    occupied[1] = 0;
    actors[0] = (u8)DIALOGUE_PORTRAIT_ACTOR_NONE;
    actors[1] = (u8)DIALOGUE_PORTRAIT_ACTOR_NONE;

    if (!scene || !scene->commands || scene->commandCount <= 0)
    {
        DialogueScene_WriteError(error, errorSize, "scene empty");
        return 0;
    }
    if (scene->commandCount > DIALOGUE_SCENE_MAX_COMMANDS)
    {
        DialogueScene_WriteError(error, errorSize, "too many scene commands");
        return 0;
    }

    for (i = 0; i < scene->commandCount; i++)
    {
        const DialogueSceneCommand *cmd = &scene->commands[i];

        if (sawEnd)
        {
            DialogueScene_WriteError(error, errorSize, "commands after END");
            return 0;
        }

        switch (cmd->op)
        {
        case DIALOGUE_SCENE_OP_ENTER_PAIR:
            if (occupied[0] || occupied[1])
            {
                DialogueScene_WriteError(error, errorSize, "ENTER_PAIR into occupied");
                return 0;
            }
            occupied[0] = 1;
            occupied[1] = 1;
            actors[0] = (u8)DIALOGUE_PORTRAIT_ACTOR_PROTAGONIST;
            actors[1] = (u8)DIALOGUE_PORTRAIT_ACTOR_AKSIL;
            break;
        case DIALOGUE_SCENE_OP_ENTER:
            if (cmd->side > DIALOGUE_PORTRAIT_SIDE_RIGHT
                || cmd->actor == DIALOGUE_PORTRAIT_ACTOR_NONE
                || cmd->actor > DIALOGUE_PORTRAIT_ACTOR_AKSIL)
            {
                DialogueScene_WriteError(error, errorSize, "invalid ENTER args");
                return 0;
            }
            if (occupied[cmd->side])
            {
                DialogueScene_WriteError(error, errorSize, "ENTER into occupied side");
                return 0;
            }
            occupied[cmd->side] = 1;
            actors[cmd->side] = cmd->actor;
            break;
        case DIALOGUE_SCENE_OP_EXIT:
            if (cmd->side > DIALOGUE_PORTRAIT_SIDE_RIGHT || !occupied[cmd->side])
            {
                DialogueScene_WriteError(error, errorSize, "invalid EXIT");
                return 0;
            }
            occupied[cmd->side] = 0;
            actors[cmd->side] = (u8)DIALOGUE_PORTRAIT_ACTOR_NONE;
            break;
        case DIALOGUE_SCENE_OP_EXIT_BOTH:
            occupied[0] = 0;
            occupied[1] = 0;
            actors[0] = (u8)DIALOGUE_PORTRAIT_ACTOR_NONE;
            actors[1] = (u8)DIALOGUE_PORTRAIT_ACTOR_NONE;
            break;
        case DIALOGUE_SCENE_OP_REPLACE:
            if (cmd->side > DIALOGUE_PORTRAIT_SIDE_RIGHT
                || !occupied[cmd->side]
                || cmd->actor == DIALOGUE_PORTRAIT_ACTOR_NONE)
            {
                DialogueScene_WriteError(error, errorSize, "invalid REPLACE");
                return 0;
            }
            actors[cmd->side] = cmd->actor;
            break;
        case DIALOGUE_SCENE_OP_FOCUS:
            if (cmd->side > DIALOGUE_PORTRAIT_SIDE_RIGHT || !occupied[cmd->side])
            {
                DialogueScene_WriteError(error, errorSize, "invalid FOCUS");
                return 0;
            }
            break;
        case DIALOGUE_SCENE_OP_SAY:
            if (cmd->side > DIALOGUE_PORTRAIT_SIDE_RIGHT
                || !occupied[cmd->side]
                || cmd->actor == DIALOGUE_PORTRAIT_ACTOR_NONE
                || actors[cmd->side] != cmd->actor
                || !cmd->textCommands
                || cmd->textCommandCount == 0)
            {
                DialogueScene_WriteError(error, errorSize, "invalid SAY");
                return 0;
            }
            if (!DialogueTextPrinter_ValidateCommands(
                    cmd->textCommands,
                    (int)cmd->textCommandCount,
                    textError,
                    (int)sizeof(textError)))
            {
                DialogueScene_WriteError(error, errorSize, "invalid SAY text");
                return 0;
            }
            break;
        case DIALOGUE_SCENE_OP_SET_EXPRESSION:
            if (cmd->side > DIALOGUE_PORTRAIT_SIDE_RIGHT
                || !occupied[cmd->side]
                || cmd->arg != DIALOGUE_EXPRESSION_NEUTRAL)
            {
                DialogueScene_WriteError(error, errorSize, "invalid expression");
                return 0;
            }
            break;
        case DIALOGUE_SCENE_OP_WAIT_FRAMES:
            break;
        case DIALOGUE_SCENE_OP_EVENT:
            if (cmd->arg == 0)
            {
                DialogueScene_WriteError(error, errorSize, "invalid event id");
                return 0;
            }
            break;
        case DIALOGUE_SCENE_OP_TECH_REPLACE:
            if (!occupied[0] || !occupied[1])
            {
                DialogueScene_WriteError(error, errorSize, "TECH_REPLACE needs both");
                return 0;
            }
            break;
        case DIALOGUE_SCENE_OP_END:
            sawEnd = 1;
            break;
        default:
            DialogueScene_WriteError(error, errorSize, "unknown scene opcode");
            return 0;
        }
    }

    if (!sawEnd)
    {
        DialogueScene_WriteError(error, errorSize, "missing scene END");
        return 0;
    }

    DialogueScene_WriteError(error, errorSize, "");
    return 1;
}
