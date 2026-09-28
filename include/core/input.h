#ifndef INPUT_H
#define INPUT_H

#include "core/gba_types.h"

void Input_Init(void);
void Input_Update(void);
int Input_IsPressed(u16 key);
int Input_JustPressed(u16 key);

#endif
