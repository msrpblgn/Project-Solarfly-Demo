#ifndef BATTLE_MESSAGE_QUEUE_H
#define BATTLE_MESSAGE_QUEUE_H

#include "battle_ui/hud_layout.h"

#define BATTLE_MESSAGE_QUEUE_CAPACITY 10
#define BATTLE_MESSAGE_MAX_LENGTH 48

#define BATTLE_MESSAGE_HOLD_FRAMES_SLOW 90
#define BATTLE_MESSAGE_HOLD_FRAMES_NORMAL 60
#define BATTLE_MESSAGE_HOLD_FRAMES_FAST 30

typedef enum BattleMessagePriority {
    BATTLE_MESSAGE_PRIORITY_LOW = 0,
    BATTLE_MESSAGE_PRIORITY_NORMAL,
    BATTLE_MESSAGE_PRIORITY_HIGH
} BattleMessagePriority;

typedef enum TextSpeed {
    TEXT_SPEED_SLOW = 0,
    TEXT_SPEED_NORMAL,
    TEXT_SPEED_FAST,
    TEXT_SPEED_COUNT
} TextSpeed;

void BattleMessageQueue_Init(void);
void BattleMessageQueue_SetTextSpeed(TextSpeed textSpeed);
void BattleMessageQueue_Enqueue(const char *message);
void BattleMessageQueue_EnqueuePriority(const char *message, BattleMessagePriority priority);
void BattleMessageQueue_Update(void);
void BattleMessageQueue_Draw(void);
int BattleMessageQueue_IsBusy(void);
int BattleMessageQueue_TakeDisplayChange(void);

#endif
