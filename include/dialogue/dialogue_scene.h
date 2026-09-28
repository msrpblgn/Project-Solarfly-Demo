#ifndef DIALOGUE_SCENE_H
#define DIALOGUE_SCENE_H

#include "battle_ui/battle_message_queue.h"
#include "core/gba_types.h"
#include "dialogue/dialogue_portrait_controller.h"
#include "dialogue/dialogue_text_printer.h"

#define DIALOGUE_SCENE_MAX_COMMANDS 48
#define DIALOGUE_SCENE_MAX_IMMEDIATE_STEPS 32

#define DIALOGUE_EXPRESSION_NEUTRAL 0

#define DIALOGUE_EVENT_TEST_COMPLETE 1

typedef enum DialogueSceneOp {
    DIALOGUE_SCENE_OP_ENTER_PAIR = 1,
    DIALOGUE_SCENE_OP_ENTER,
    DIALOGUE_SCENE_OP_EXIT,
    DIALOGUE_SCENE_OP_EXIT_BOTH,
    DIALOGUE_SCENE_OP_REPLACE,
    DIALOGUE_SCENE_OP_FOCUS,
    DIALOGUE_SCENE_OP_SAY,
    DIALOGUE_SCENE_OP_SET_EXPRESSION,
    DIALOGUE_SCENE_OP_WAIT_FRAMES,
    DIALOGUE_SCENE_OP_EVENT,
    DIALOGUE_SCENE_OP_TECH_REPLACE,
    DIALOGUE_SCENE_OP_END
} DialogueSceneOp;

/*
 * Scene command (ROM). Text streams are separate DialogueTextCommand arrays.
 * SAY blocks until text-level END, then one fresh A press.
 * Text-level WAIT/END are not scene WAIT_FRAMES/END.
 */
typedef struct DialogueSceneCommand {
    u8 op;
    u8 side; /* DialoguePortraitSide */
    u8 actor; /* DialoguePortraitActor */
    u8 arg; /* active flag, expression, wait frames, event id, leftActive */
    const DialogueTextCommand *textCommands;
    u16 textCommandCount;
} DialogueSceneCommand;

typedef struct DialogueScene {
    const char *id;
    const DialogueSceneCommand *commands;
    int commandCount;
} DialogueScene;

#define DIALOGUE_SCENE_CMD_ENTER_PAIR(leftActive) \
    { DIALOGUE_SCENE_OP_ENTER_PAIR, 0, 0, (u8)(leftActive), 0, 0 }
#define DIALOGUE_SCENE_CMD_ENTER(side, actor, asActive) \
    { DIALOGUE_SCENE_OP_ENTER, (u8)(side), (u8)(actor), (u8)(asActive), 0, 0 }
#define DIALOGUE_SCENE_CMD_EXIT(side) \
    { DIALOGUE_SCENE_OP_EXIT, (u8)(side), 0, 0, 0, 0 }
#define DIALOGUE_SCENE_CMD_EXIT_BOTH() \
    { DIALOGUE_SCENE_OP_EXIT_BOTH, 0, 0, 0, 0, 0 }
#define DIALOGUE_SCENE_CMD_REPLACE(side, actor) \
    { DIALOGUE_SCENE_OP_REPLACE, (u8)(side), (u8)(actor), 0, 0, 0 }
#define DIALOGUE_SCENE_CMD_FOCUS(side) \
    { DIALOGUE_SCENE_OP_FOCUS, (u8)(side), 0, 0, 0, 0 }
#define DIALOGUE_SCENE_CMD_SAY(side, actor, cmds, count) \
    { DIALOGUE_SCENE_OP_SAY, (u8)(side), (u8)(actor), 0, (cmds), (u16)(count) }
#define DIALOGUE_SCENE_CMD_SET_EXPRESSION(side, expr) \
    { DIALOGUE_SCENE_OP_SET_EXPRESSION, (u8)(side), 0, (u8)(expr), 0, 0 }
#define DIALOGUE_SCENE_CMD_WAIT_FRAMES(n) \
    { DIALOGUE_SCENE_OP_WAIT_FRAMES, 0, 0, (u8)(n), 0, 0 }
#define DIALOGUE_SCENE_CMD_EVENT(id) \
    { DIALOGUE_SCENE_OP_EVENT, 0, 0, (u8)(id), 0, 0 }
#define DIALOGUE_SCENE_CMD_TECH_REPLACE() \
    { DIALOGUE_SCENE_OP_TECH_REPLACE, 0, 0, 0, 0, 0 }
#define DIALOGUE_SCENE_CMD_END() \
    { DIALOGUE_SCENE_OP_END, 0, 0, 0, 0, 0 }

const DialogueScene *DialogueScene_GetSharedConversation(void);
const DialogueScene *DialogueScene_GetReplacementTest(void);

int DialogueScene_Validate(
    const DialogueScene *scene,
    char *error,
    int errorSize);

#endif
