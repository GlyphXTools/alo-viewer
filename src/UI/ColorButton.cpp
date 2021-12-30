#include "UI.h"
#include <commdlg.h>
using namespace Alamo;

struct ColorButtonControl
{
	UINT     msgColorOkString;
	HWND     hWnd;
	COLORREF color;
	bool     isSelected;
};

static ColorButtonControl* activeDialog = NULL;

static UINT_PTR APIENTRY CustomHookProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (activeDialog != NULL)
	{
		// Read the Red, Green and Blue edit boxes
		TCHAR text1[16], text2[16], text3[16];
		GetWindowText(GetDlgItem(hWnd, 0x2c2), text1, 16);
		GetWindowText(GetDlgItem(hWnd, 0x2c3), text2, 16);
		GetWindowText(GetDlgItem(hWnd, 0x2c4), text3, 16);
		
		// Convert to COLORREF
		TCHAR* endptr;
		COLORREF color = RGB(
			wcstoul(text1, &endptr, 10),
			wcstoul(text2, &endptr, 10),
			wcstoul(text3, &endptr, 10) );

		// Notify on change
		if (activeDialog->color != color)
		{
			activeDialog->color = color;
			RedrawWindow(activeDialog->hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
			SendMessage(GetParent(activeDialog->hWnd), WM_COMMAND, (WPARAM)MAKELONG(GetDlgCtrlID(activeDialog->hWnd), CBN_CHANGE), (LPARAM)activeDialog->hWnd );
		}

		if (uMsg == activeDialog->msgColorOkString)
		{
			// Clear the dialog to avoid setting the color on cleanups
			activeDialog = NULL;
		}
	}

	return 0;
}

static LRESULT CALLBACK ColorButtonWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	ColorButtonControl* control = (ColorButtonControl*)(LONG_PTR)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	switch (uMsg)
	{
		case WM_CREATE:
		{
			CREATESTRUCT* pcs = (CREATESTRUCT*)lParam;
			RECT size;
			GetClientRect(hWnd, &size);

			control = new ColorButtonControl;
			control->msgColorOkString = RegisterWindowMessage(COLOROKSTRING);
			control->hWnd  = hWnd;
			control->color = RGB(0,0,0);
			control->isSelected = false;
			SetWindowLong(hWnd, GWLP_USERDATA, (LONG)(LONG_PTR)control);
			break;
		}

		case WM_DESTROY:
			delete control;
			break;

		case WM_LBUTTONDOWN:
			SetCapture(hWnd);
			break;

		case WM_LBUTTONUP:
		{
			ReleaseCapture();
			static COLORREF CustomColors[16] = {0};

			// Show the color chooser
			CHOOSECOLOR cc;
			ZeroMemory(&cc, sizeof(CHOOSECOLOR));
			cc.lStructSize  = sizeof(CHOOSECOLOR);
			cc.lpCustColors = CustomColors;
			cc.hwndOwner    = hWnd;
			cc.rgbResult    = control->color;
			cc.lpfnHook     = CustomHookProc;
			cc.Flags        = CC_ANYCOLOR | CC_FULLOPEN | CC_RGBINIT | CC_ENABLEHOOK;
			COLORREF original = control->color;
			activeDialog = control;
			if (ChooseColor(&cc))
			{
				activeDialog = NULL;
			}
			else
			{
				activeDialog = NULL;
				cc.rgbResult = original;
			}
			control->color = cc.rgbResult;
			RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
			SendMessage(GetParent(hWnd), WM_COMMAND, (WPARAM)MAKELONG(GetDlgCtrlID(hWnd), CBN_CHANGE), (LPARAM)hWnd );
			break;
		}
		
		case WM_PAINT:
		{
			RECT client;
			GetClientRect(hWnd, &client);

			PAINTSTRUCT ps;
			HDC hDC = BeginPaint(hWnd, &ps);

			HPEN   hPen;
			if (control->isSelected) {
				hPen = CreatePen(PS_SOLID, 5, GetSysColor(COLOR_HIGHLIGHT));
			}
			else {
				hPen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
			}

			COLORREF oldBkColor = SetBkColor(hDC, control->color);
			int oldBkMode = SetBkMode(hDC, OPAQUE);
			LOGBRUSH lb = {
				BS_HATCHED,
				RGB(128, 128, 128),
				HS_BDIAGONAL,
			};

			HBRUSH hBrush = CreateBrushIndirect(&lb);

			SelectObject(hDC, hPen);
			SelectObject(hDC, hBrush);
			Rectangle(hDC, 0, 0, client.right, client.bottom);
			DeleteObject(hBrush);
			DeleteObject(hPen);
			SetBkColor(hDC, oldBkColor);
			SetBkMode(hDC, oldBkMode);

			EndPaint(hWnd, &ps);
			break;
		}
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

void ColorButton_SetColor(HWND hWnd, const Color& color)
{
	ColorButtonControl* control = (ColorButtonControl*)(LONG_PTR)GetWindowLongPtr(hWnd,GWLP_USERDATA);
	if (control != NULL)
	{
		control->color = RGB(
            (int)(color.r * color.a * 255),
            (int)(color.g * color.a * 255),
            (int)(color.b * color.a * 255) );
		RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
	}
}

Color ColorButton_GetColor(HWND hWnd)
{
	const ColorButtonControl* control = (ColorButtonControl*)(LONG_PTR)GetWindowLongPtr(hWnd,GWLP_USERDATA);
	if (control != NULL)
	{
   		return Color(
            GetRValue(control->color) / 255.0f,
            GetGValue(control->color) / 255.0f,
            GetBValue(control->color) / 255.0f, 1.0f);
	}
	return Color(0,0,0,0);
}

void ColorButton_SetSelected(HWND hWnd, bool selected)
{
	ColorButtonControl* control = (ColorButtonControl*)(LONG_PTR)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	if (control != NULL)
	{
		control->isSelected = selected;
		RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
	}
}

bool ColorButton_Initialize(HINSTANCE hInstance)
{
	WNDCLASSEX wcx;
	wcx.cbSize        = sizeof(WNDCLASSEX);
	wcx.style         = CS_HREDRAW | CS_VREDRAW;
	wcx.lpfnWndProc   = ColorButtonWindowProc;
	wcx.cbClsExtra    = 0;
	wcx.cbWndExtra    = 0;
	wcx.hInstance     = hInstance;
	wcx.hIcon         = NULL;
	wcx.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wcx.hbrBackground = NULL;
	wcx.lpszMenuName  = NULL;
	wcx.lpszClassName = L"ColorButton";
	wcx.hIconSm       = NULL;
	
	if (!RegisterClassEx(&wcx))
	{
		return false;
	}

	return true;
}

void ColorButton_Uninitialize(HINSTANCE hInstance)
{
    UnregisterClass(L"ColorButton", hInstance);
}