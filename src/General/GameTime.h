#ifndef GAMETIME_H
#define GAMETIME_H

namespace Alamo
{
extern float g_CurrentGameTime;
extern float g_PreviousGameTime;

inline float GetGameTime()
{
    return g_CurrentGameTime;
}

inline float GetPreviousGameTime()
{
    return g_PreviousGameTime;
}

void UpdateGameTime();
void ResetGameTime();
void SetGameTimeSpeed(float speed);
float GetGameTimeSpeed();
}
#endif