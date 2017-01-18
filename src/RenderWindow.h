#ifndef RENDERWINDOW_H
#define RENDERWINDOW_H

#include "RenderEngine/RenderEngine.h"
#include "Games.h"

bool RegisterRenderWindow(HINSTANCE hInstance);
HWND CreateRenderWindow(HWND hWndParent, GameID game, const Alamo::Environment& env, int x, int y, int w, int h);
Alamo::IRenderEngine* GetRenderEngine(HWND hWnd);
void SetRenderWindowOptions(HWND hWnd, const Alamo::RenderOptions& options);
void UnregisterRenderWindow(HINSTANCE hInstance);
void SetRenderWindowGameEngine(HWND hWnd, GameID game);

#endif