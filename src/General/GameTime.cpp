#include "GameTime.h"
#include <windows.h>
namespace Alamo
{
float g_PreviousGameTime = 0.0f;
float g_CurrentGameTime  = 0.0f;
float g_CurrentGameSpeed = 1.0f;

void UpdateGameTime()
{
    static DWORD prev = 0;
    if (prev == 0)
    {
        prev = GetTickCount();
    }
    DWORD cur = GetTickCount();
    g_PreviousGameTime = g_CurrentGameTime;
    g_CurrentGameTime += g_CurrentGameSpeed * (cur - prev) / 1000.0f;
    prev = cur;
}

void ResetGameTime()
{
    g_CurrentGameTime  = 0.0f;
    g_PreviousGameTime = 0.0f;
}

void SetGameTimeSpeed(float speed)
{
    g_CurrentGameSpeed = speed;
}

float GetGameTimeSpeed()
{
    return g_CurrentGameSpeed;
}

}