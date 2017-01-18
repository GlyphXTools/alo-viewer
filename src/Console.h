#ifndef CONSOLE_H
#define CONSOLE_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

bool RegisterConsoleWindow(HINSTANCE hInstance);
HWND CreateConsoleWindow(HWND hWndParent, bool visible);
void ShowConsoleWindow(HWND hWnd, bool show);
void UnregisterConsoleWindow(HINSTANCE hInstance);

#endif