#ifndef UIELEMENTS_H
#define UIELEMENTS_H

#include <windows.h>
#include <string>
#include "General/Exceptions.h"
#include "General/3DTypes.h"
#include "resource.h"

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

static float GetUIFloat(HWND hDlg, int nIDDlgItem)
{
	SPINNER_INFO si;
	si.IsFloat = true;
	si.Mask    = SPIF_VALUE;
	Spinner_GetInfo(GetDlgItem(hDlg, nIDDlgItem), &si);
	return si.f.Value;
}

static void SetUIFloat(HWND hDlg, int nIDDlgItem, float f)
{
	SPINNER_INFO si;
	si.IsFloat = true;
	si.Mask    = SPIF_VALUE;
    si.f.Value = f;
	Spinner_SetInfo(GetDlgItem(hDlg, nIDDlgItem), &si);
}

/*
 * ColorButton control
 */

// Messages
#define CBN_CHANGE		(WM_APP + 3)	// Color button value has changed. Sent as WM_COMMAND.

void ColorButton_SetColor(HWND hWnd, const Alamo::Color& color);
Alamo::Color ColorButton_GetColor(HWND hWnd);
void ColorButton_SetSelected(HWND hWnd, bool selected);

/* 
 * Global UI functions
 */
bool UI_Initialize( HINSTANCE hInstance );
void UI_Uninitialize( HINSTANCE hInstance );
#endif