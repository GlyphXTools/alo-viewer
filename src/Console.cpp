#include "Console.h"
#include "General/Log.h"
#include "General/Utils.h"
#include <iomanip>
#include <sstream>
using namespace std;
using namespace Alamo;

struct ConsoleInfo
{
    HWND   hWnd;
    HWND   hEdit;
    size_t callback;
};

void LogCallback(const vector<Log::Line>& newlines, void* data)
{
    ConsoleInfo* info = (ConsoleInfo*)data;

    stringstream text;
    bool hasErrors = false;
    for (size_t i = 0; i < newlines.size(); i++)
    {
        const Log::Line& line = newlines[i];
        const tm* t = localtime(&line.time);

        text << setfill('0') << "[" << setw(2) << t->tm_hour << ":" << setw(2) << t->tm_min << "] " << line.text << "\r\n";
        hasErrors |= (line.type == Log::LT_ERROR);
    }

    int len = GetWindowTextLength(info->hEdit);
    wstring wtext(len, '\0');
    GetWindowText(info->hEdit, (LPWSTR)wtext.c_str(), len + 1);
    wtext += AnsiToWide(text.str());

    SetWindowText(info->hEdit, wtext.c_str());
    SendMessage(info->hEdit, WM_VSCROLL, SB_BOTTOM, 0);

    if (hasErrors)
    {
        ShowConsoleWindow(info->hWnd, true);
    }
}

static LRESULT CALLBACK ConsoleWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    ConsoleInfo* info = (ConsoleInfo*)(LONG_PTR)GetWindowLongPtr(hWnd, GWLP_USERDATA);
    switch (uMsg)
    {
        case WM_CREATE:
        {
            CREATESTRUCT* pcs = (CREATESTRUCT*)lParam;
            info = new ConsoleInfo;
            info->hWnd = hWnd;
            SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG)(LONG_PTR)info);

            if ((info->hEdit = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_VSCROLL | WS_CHILD | WS_VISIBLE | ES_READONLY | ES_MULTILINE, 0, 0, pcs->cx, pcs->cy, hWnd, NULL, pcs->hInstance, NULL)) == NULL)
            {
                delete info;
                return -1;
            }

            // Set font
            SendMessage(info->hEdit, WM_SETFONT, (WPARAM)GetStockObject(ANSI_FIXED_FONT), FALSE); 

            info->callback = Log::RegisterCallback(LogCallback, info);
            break;
        }

        case WM_DESTROY:
            Log::UnregisterCallback(info->callback);
            delete info;
            break;

        case WM_SIZE:
            MoveWindow(info->hEdit, 0, 4, LOWORD(lParam), HIWORD(lParam) - 4, TRUE);
            break;
    }
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

void ShowConsoleWindow(HWND hWnd, bool show)
{
    ConsoleInfo* info = (ConsoleInfo*)(LONG_PTR)GetWindowLongPtr(hWnd, GWLP_USERDATA);
    if (info != NULL)
    {
        ShowWindow(hWnd, show ? SW_SHOW : SW_HIDE);
        
        // Trigger a WM_SIZE for the parent, it will reposition its child controls
        HWND hWndParent = GetParent(hWnd);
        RECT size;
        GetClientRect(hWndParent, &size);
        SendMessage(hWndParent, WM_SIZE, SIZE_RESTORED, MAKELONG(size.right, size.bottom));
    }
}

HWND CreateConsoleWindow(HWND hWndParent, bool visible)
{
    HINSTANCE hInstance = (HINSTANCE)(LONG_PTR)GetWindowLongPtr(hWndParent, GWLP_HINSTANCE);
    return CreateWindowEx(0, L"AloConsoleWindow", NULL, WS_CHILD | (visible ? WS_VISIBLE : 0), 0, 0, 1, 90, hWndParent, NULL, hInstance, NULL);
}

bool RegisterConsoleWindow(HINSTANCE hInstance)
{
	WNDCLASSEX wcx;
	wcx.cbSize        = sizeof wcx;
    wcx.style		  = CS_HREDRAW | CS_VREDRAW;
    wcx.lpfnWndProc	  = ConsoleWindowProc;
    wcx.cbClsExtra	  = 0;
    wcx.cbWndExtra    = 0;
    wcx.hInstance     = hInstance;
    wcx.hIcon         = NULL;
    wcx.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wcx.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wcx.lpszMenuName  = NULL;
    wcx.lpszClassName = L"AloConsoleWindow";
    wcx.hIconSm       = NULL;
	return (RegisterClassEx(&wcx) != 0);
}

void UnregisterConsoleWindow(HINSTANCE hInstance)
{
    UnregisterClass(L"AloConsoleWindow", hInstance);
}
