#ifndef UIELEMENTS_H
#define UIELEMENTS_H

#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0400
#include <windows.h>
#include <commctrl.h>
#include <afxres.h>
#include <string>
#include "../exceptions.h"
#include "../resource.h"
#include "../types.h"

/*
 * Spinner control
 */

// Spinner Flags
static const int SPIF_VALUE     = 1;
static const int SPIF_RANGE     = 2;
static const int SPIF_INCREMENT = 4;
static const int SPIF_ALL       = SPIF_VALUE | SPIF_RANGE | SPIF_INCREMENT;

// Messages
#define SN_CHANGE		(WM_APP + 1)	// Spinner value has changed. Sent as WM_COMMAND.
#define SN_KILLFOCUS	(WM_APP + 2)	// Spinner has lost focus. Sent as WM_COMMAND.

// Structure with spinner info
struct SPINNER_INFO
{
	int  Mask;
	bool IsFloat;
	union
	{
		struct
		{
			float Value;
			float MaxValue;
			float MinValue;
			float Increment;
		} f;
		struct
		{
			long Value;
			long MaxValue;
			long MinValue;
			long Increment;
		} i;
	};
};

void Spinner_SetInfo(HWND hWnd, const SPINNER_INFO* psi);
void Spinner_GetInfo(HWND hWnd, SPINNER_INFO* psi);

/*
 * ColorButton control
 */

// Messages
#define CBN_CHANGE		(WM_APP + 3)	// Color button value has changed. Sent as WM_COMMAND.

void ColorButton_SetColor(HWND hWnd, COLORREF color);
COLORREF ColorButton_GetColor(HWND hWnd);

/* 
 * Global UI functions
 */
bool UI_Initialize( HINSTANCE hInstance );

#endif