#ifndef BATTLE_PACING_H
#define BATTLE_PACING_H

void BattlePacing_Reset(void);
int BattlePacing_IsWaiting(void);
void BattlePacing_StartWait(int frames);
void BattlePacing_Update(void);

#endif
