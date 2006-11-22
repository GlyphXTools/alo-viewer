#include "UI.h"
#include <commdlg.h>

struct ColorButtonControl
{
	UINT     msgColorOkString;
	HWND     hWnd;
	COLORREF color;
};

static ColorButtonControl* activeDialog = NULL;

static UINT APIENTRY CustomHookProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (activeDialog != NULL)
	{
		// Read the Red, Green and Blue edit boxes
		char text1[16], text2[16], text3[16];
		GetWindowText(GetDlgItem(hWnd, 0x2c2), text1, 16);
		GetWindowText(GetDlgItem(hWnd, 0x2c3), text2, 16);
		GetWindowText(GetDlgItem(hWnd, 0x2c4), text3, 16);
		
		// Convert to COLORREF
		char* endptr;
		COLORREF color = RGB(
			strtoul(text1, &endptr, 10),
			strtoul(text2, &endptr, 10),
			strtoul(text3, &endptr, 10) );

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
	ColorButtonControl* control = (ColorButtonControl*)(LONG_PTR)GetWindowLongPtr(hWnd,GWL_USERDATA);
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
			SetWindowLong(hWnd, GWL_USERDATA, (LONG)(LONG_PTR)control);
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

			HPEN   hPen   = CreatePen(PS_SOLID, 1, RGB(0,0,0));
			HBRUSH hBrush = CreateSolidBrush( control->color);
			SelectObject(hDC, hPen);
			SelectObject(hDC, hBrush);
			Rectangle(hDC, 0, 0, client.right, client.bottom);
			DeleteObject(hBrush);
			DeleteObject(hPen);

			EndPaint(hWnd, &ps);
			break;
		}
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

void ColorButton_SetColor(HWND hWnd, COLORREF color)
{
	ColorButtonControl* control = (ColorButtonControl*)(LONG_PTR)GetWindowLongPtr(hWnd,GWL_USERDATA);
	if (control != NULL)
	{
		control->color = color;
		RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
	}
}

COLORREF ColorButton_GetColor(HWND hWnd)
{
	const ColorButtonControl* control = (ColorButtonControl*)(LONG_PTR)GetWindowLongPtr(hWnd,GWL_USERDATA);
	if (control != NULL)
	{
		return control->color;
	}
	return RGB(0,0,0);
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
	wcx.lpszClassName = "ColorButton";
	wcx.hIconSm       = NULL;
	
	if (!RegisterClassEx(&wcx))
	{
		return false;
	}

	return true;
}
