#ifndef WINUTILS_H
#define WINUTILS_H

#include <string>

// Windows-specific utility functions
namespace Alamo
{
std::wstring GetWindowText(HWND hWnd);
void GetParentRect(HWND hWnd, RECT* pRect);
std::wstring LoadString(UINT id, ...);
BOOL ScreenToClient(HWND hWnd, LPRECT rect);
}

#endif