#include "core/input.h"

static u16 s_keysHeld;
static u16 s_keysPressed;

void Input_Init(void)
{
    s_keysHeld = 0;
    s_keysPressed = 0;
}

void Input_Update(void)
{
    u16 keys = ~REG_KEYINPUT & KEY_MASK;
    s_keysPressed = keys & ~s_keysHeld;
    s_keysHeld = keys;
}

int Input_IsPressed(u16 key)
{
    return (s_keysHeld & key) != 0;
}

int Input_JustPressed(u16 key)
{
    return (s_keysPressed & key) != 0;
}
