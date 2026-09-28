#ifndef RANDOM_H
#define RANDOM_H

#include "core/gba_types.h"

void Random_Init(u32 seed);
u32 Random_Next(void);
int Random_Range(int min, int max);
int Random_Percent(int chancePercent);
int Random_Permille(int chancePermille);

#endif
