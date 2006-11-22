#include "UI.h"
#include <sstream>
#include <iomanip>
using namespace std;

enum DragType
{
	NONE,
	HOLDING_UP,
	HOLDING_DOWN,
	DRAGGING
};

namespace SystemInfo
{
	HCURSOR  hSpinCursor;
	HCURSOR  hNormalCursor;
	int      spinnerWidth;
	int      holdDelay;
	int      holdSpeed;
};

struct SpinnerControl
{
	SPINNER_INFO info;
	HWND     hSpinner;
	HWND     hEdit;
	UINT_PTR hTimer;
	WNDPROC  EditWindowProc;

	int      dragStartY;
	float	 dragStartValueF;
	long	 dragStartValueI;

	DragType dragType;
};

static void Spinner_Paint(HWND hWnd, SpinnerControl* control)
{
	PAINTSTRUCT ps;
	HDC hDC = BeginPaint(hWnd, &ps);

	RECT rect;
	GetClientRect(hWnd, &rect);

	int styleUp = (control->dragType == HOLDING_UP   || control->dragType == DRAGGING) ? DFCS_PUSHED : 0;
	int styleDn = (control->dragType == HOLDING_DOWN || control->dragType == DRAGGING) ? DFCS_PUSHED : 0;
	
	// Render up-down buttons
	rect.left = max(0, rect.right - SystemInfo::spinnerWidth) + 1;
	rect.top  = rect.bottom - rect.bottom / 2;
	DrawFrameControl(hDC, &rect, DFC_SCROLL, styleDn | DFCS_SCROLLDOWN);
	rect.bottom /= 2;
	rect.top     = 0;
	DrawFrameControl(hDC, &rect, DFC_SCROLL, styleUp | DFCS_SCROLLUP);

	EndPaint(hWnd, &ps);
}

static void Spinner_Update(SpinnerControl* control)
{
	stringstream ss;
	if (control->info.IsFloat)
	{
		control->info.f.Value = max(min(control->info.f.Value, control->info.f.MaxValue), control->info.f.MinValue);
		ss << fixed << setprecision(2) << control->info.f.Value;
	}
	else
	{
		control->info.i.Value = max(min(control->info.i.Value, control->info.i.MaxValue), control->info.i.MinValue);
		ss << control->info.i.Value;
	}
	SetWindowText(control->hEdit, ss.str().c_str());
}

static void Spinner_Redraw(HWND hWnd, SpinnerControl* control)
{
	RECT size;
	GetClientRect(hWnd, &size);
	size.left = size.right - SystemInfo::spinnerWidth;
	RedrawWindow(hWnd, &size, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
}

static LRESULT CALLBACK SpinnerEditWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	SpinnerControl* control = (SpinnerControl*)(LONG_PTR)GetWindowLongPtr(hWnd,GWL_USERDATA);
	
	// Here we can trap certain message, otherwise sent to the edit control
	switch (uMsg)
	{
		case WM_KEYUP:
			if (wParam == VK_UP || wParam == VK_DOWN)
			{
				return 0;
			}
			break;
		
		case WM_CHAR:
		{
			// First-level filter: Ignore everything but digits and periods and minus (if the number is a float)
			char c = (char)wParam;
			if (wParam > 127 || (!iscntrl(c) && !isdigit(c) && ((c != '.' && c != '-') || !control->info.IsFloat)))
			{
				return 0;
			}

			if (wParam == '.')
			{
				// Check to see if it already contains a period
				TCHAR text[256];
				GetWindowText(hWnd, text, 256);
				if (strchr(text,'.') != NULL)
				{
					return 0;
				}
			}
			break;
		}

		case WM_KEYDOWN:
			if (wParam == VK_UP)
			{
				if (control->info.IsFloat)
					control->info.f.Value += control->info.f.Increment;
				else
					control->info.i.Value += control->info.i.Increment;
				Spinner_Update(control);
				return 0;
			}

			if (wParam == VK_DOWN)
			{
				if (control->info.IsFloat)
					control->info.f.Value -= control->info.f.Increment;
				else
					control->info.i.Value -= control->info.i.Increment;
				Spinner_Update(control);
				return 0;
			}
	}

	// Pass message on to edit control
	return control->EditWindowProc(hWnd, uMsg, wParam, lParam);
}

static LRESULT CALLBACK SpinnerWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	SpinnerControl* control = (SpinnerControl*)(LONG_PTR)GetWindowLongPtr(hWnd,GWL_USERDATA);
	switch (uMsg)
	{
		case WM_CREATE:
		{
			CREATESTRUCT* pcs = (CREATESTRUCT*)lParam;
			RECT size;
			GetClientRect(hWnd, &size);

			control = new SpinnerControl;
			control->hSpinner = hWnd;
			control->info.IsFloat = false;
			control->info.i.Value     = 0;
			control->info.i.MinValue  = 0;
			control->info.i.MaxValue  = 100;
			control->info.i.Increment = 1;

			control->dragType     = NONE;

			LONG style = pcs->style & WS_DISABLED;
			if ((control->hEdit = CreateWindow("EDIT", "0", style | WS_CHILD | WS_VISIBLE | ES_RIGHT | ES_AUTOHSCROLL | WS_TABSTOP,
				0, 0, size.right - SystemInfo::spinnerWidth, size.bottom, hWnd, NULL, pcs->hInstance, NULL)) == NULL)
			{
				delete control;
				return -1;
			}

			// We hijack the edit control's message window because we need to trap certain messages.
			control->EditWindowProc = (WNDPROC)(LONG_PTR)GetWindowLong(control->hEdit, GWL_WNDPROC);
			SetWindowLong(control->hEdit, GWL_USERDATA, (LONG)(LONG_PTR)control);
			SetWindowLong(control->hEdit, GWL_WNDPROC,  (LONG)(LONG_PTR)SpinnerEditWindowProc);
			SetWindowLong(hWnd, GWL_USERDATA, (LONG)(LONG_PTR)control);
			break;
		}

		case WM_DESTROY:
			DestroyWindow(control->hEdit);
			delete control;
			break;

		case WM_COMMAND:
			if (control != NULL && (HWND)lParam == control->hEdit)
			{
				// Notification from edit control
				switch (HIWORD(wParam))
				{
					case EN_CHANGE:
					{
						// Forward to parent
						SendMessage(GetParent(hWnd), WM_COMMAND, (WPARAM)MAKELONG(GetDlgCtrlID(hWnd), SN_CHANGE), (LPARAM)hWnd );
						break;
					}

					case EN_UPDATE:
					{
						// Check new text to make sure
						TCHAR text[256];
						GetWindowText(control->hEdit, text, 256);
						wstringstream ss;
						ss << text;
						if (control->info.IsFloat)
						{
							float val;
							ss >> val;
							control->info.f.Value = val;
						}
						else
						{
							long val;
							ss >> val;
							control->info.i.Value = val;
						}
						break;
					}

					case EN_KILLFOCUS:
						// Forward to parent
						SendMessage(GetParent(hWnd), WM_COMMAND, (WPARAM)MAKELONG(GetDlgCtrlID(hWnd), SN_KILLFOCUS), (LPARAM)hWnd );
						Spinner_Update(control);
						break;
				}
			}
			break;
		
		case WM_ENABLE:
			EnableWindow(control->hEdit, (BOOL)wParam);
			break;

		case WM_LBUTTONUP:
		{
			KillTimer(hWnd, control->hTimer);
			ReleaseCapture();
			control->dragType = NONE;

			if (control->dragType == DRAGGING)
			{
				SetCursor(SystemInfo::hNormalCursor);
			}

			Spinner_Redraw(hWnd, control);
			break;
		}

		case WM_MOUSEMOVE:
		{
			if (control->dragType != NONE)
			{
				int y = (short)HIWORD(lParam);
				if (control->dragType != DRAGGING)
				{
					KillTimer(hWnd, control->hTimer);
					control->dragType   = DRAGGING;
					control->dragStartY = y;
					if (control->info.IsFloat)
						control->dragStartValueF = control->info.f.Value;
					else
						control->dragStartValueI = control->info.i.Value;
					SetCursor(SystemInfo::hSpinCursor);
					Spinner_Redraw(hWnd, control);
				}

				long amount = control->dragStartY - y;
				if (control->info.IsFloat)
					control->info.f.Value = control->dragStartValueF + amount * control->info.f.Increment;
				else
					control->info.i.Value = control->dragStartValueI + amount * control->info.i.Increment;
				Spinner_Update(control);
			}
			break;
		}

		case WM_LBUTTONDOWN:
		{
			SetFocus(control->hEdit);
			SetCapture(hWnd);

			RECT size;
			GetClientRect(hWnd, &size);
			control->dragType = (HIWORD(lParam) < size.bottom / 2) ? HOLDING_UP : HOLDING_DOWN;
			control->hTimer   = SetTimer(hWnd, 1, SystemInfo::holdDelay, NULL);

			Spinner_Redraw(hWnd, control);
			wParam = 0;
		}
		
		case WM_TIMER:
		{
			if (wParam == 1)
			{
				// Initial hold delay has passed
				KillTimer(hWnd, control->hTimer);
				control->hTimer = SetTimer(hWnd, 2, 1000 / SystemInfo::holdSpeed, NULL);
			}

			// Time step has passed, increase
			long sign = (control->dragType == HOLDING_UP) ? 1 : -1;
			if (control->info.IsFloat)
				control->info.f.Value = control->info.f.Value + sign * control->info.f.Increment;
			else
				control->info.i.Value = control->info.i.Value + sign * control->info.i.Increment;
			Spinner_Update(control);
			break;
		}

		case WM_SETFOCUS:
			SetFocus(control->hEdit);
			break;

		case WM_SETFONT:
			SendMessage(control->hEdit, WM_SETFONT, wParam, lParam);
			break;

		case WM_PAINT:
			Spinner_Paint(hWnd, control);
			break;
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

void Spinner_SetInfo(HWND hWnd, const SPINNER_INFO* psi)
{
	SpinnerControl* control = (SpinnerControl*)(LONG_PTR)GetWindowLongPtr(hWnd,GWL_USERDATA);
	if (control != NULL)
	{
		control->info.IsFloat = psi->IsFloat;
		if (psi->IsFloat)
		{
			if (psi->Mask & SPIF_RANGE)   { control->info.f.MinValue = psi->f.MinValue; control->info.f.MaxValue = psi->f.MaxValue; }
			if (psi->Mask & SPIF_INCREMENT) control->info.f.Increment = psi->f.Increment;
			if (psi->Mask & SPIF_VALUE)	    control->info.f.Value = psi->f.Value;
		}
		else
		{
			if (psi->Mask & SPIF_RANGE)   { control->info.i.MinValue = psi->i.MinValue; control->info.i.MaxValue = psi->i.MaxValue; }
			if (psi->Mask & SPIF_INCREMENT) control->info.i.Increment = psi->i.Increment;
			if (psi->Mask & SPIF_VALUE)	    control->info.i.Value = psi->i.Value;
		}
		Spinner_Update(control);
		Spinner_Redraw(hWnd, control);
	}
}

void Spinner_GetInfo(HWND hWnd, SPINNER_INFO* psi)
{
	const SpinnerControl* control = (SpinnerControl*)(LONG_PTR)GetWindowLongPtr(hWnd,GWL_USERDATA);
	if (control != NULL)
	{
		psi->IsFloat = control->info.IsFloat;
		if (control->info.IsFloat)
		{
			if (psi->Mask & SPIF_VALUE)	    psi->f.Value = control->info.f.Value;
			if (psi->Mask & SPIF_RANGE)   { psi->f.MinValue = control->info.f.MinValue; psi->f.MaxValue = control->info.f.MaxValue; }
			if (psi->Mask & SPIF_INCREMENT) psi->f.Increment = control->info.f.Increment;
		}
		else
		{
			if (psi->Mask & SPIF_VALUE)	   psi->i.Value = control->info.i.Value;
			if (psi->Mask & SPIF_RANGE)   { psi->i.MinValue = control->info.i.MinValue; psi->i.MaxValue = control->info.i.MaxValue; }
			if (psi->Mask & SPIF_INCREMENT) psi->i.Increment = control->info.i.Increment;
		}
	}
}

bool Spinner_Initialize( HINSTANCE hInstance )
{
	// Get system settings info
	SystemInfo::spinnerWidth = GetSystemMetrics(SM_CXVSCROLL);
	SystemParametersInfo(SPI_GETKEYBOARDDELAY, 0, &SystemInfo::holdDelay, 0 );
	SystemParametersInfo(SPI_GETKEYBOARDSPEED, 0, &SystemInfo::holdSpeed, 0 );
	SystemInfo::holdDelay = (SystemInfo::holdDelay + 1) * 250;	// To milliseconds
	SystemInfo::holdSpeed = SystemInfo::holdSpeed + 3;			// Repetitions per second (sort of)
	SystemInfo::hNormalCursor = LoadCursor(NULL, IDC_ARROW);
	SystemInfo::hSpinCursor   = LoadCursor(NULL, IDC_SIZENS);

	WNDCLASSEX wcx;
	wcx.cbSize        = sizeof(WNDCLASSEX);
	wcx.style         = CS_HREDRAW | CS_VREDRAW;
	wcx.lpfnWndProc   = SpinnerWindowProc;
	wcx.cbClsExtra    = 0;
	wcx.cbWndExtra    = 0;
	wcx.hInstance     = hInstance;
	wcx.hIcon         = NULL;
	wcx.hCursor       = SystemInfo::hNormalCursor;
	wcx.hbrBackground = NULL;
	wcx.lpszMenuName  = NULL;
	wcx.lpszClassName = "Spinner";
	wcx.hIconSm       = NULL;
	
	if (!RegisterClassEx(&wcx))
	{
		return false;
	}

	return true;
}
