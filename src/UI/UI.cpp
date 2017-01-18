#include "UI.h"
#include <commctrl.h>
#include <afxres.h>

bool Spinner_Initialize(HINSTANCE hInstance);
void Spinner_Uninitialize(HINSTANCE hInstance);

bool ColorButton_Initialize(HINSTANCE hInstance);
void ColorButton_Uninitialize(HINSTANCE hInstance);

bool UI_Initialize( HINSTANCE hInstance )
{
	CoInitializeEx(NULL, COINIT_APARTMENTTHREADED );
	InitCommonControls();
	return (
			Spinner_Initialize(hInstance) &&
			ColorButton_Initialize(hInstance)
			);
}

void UI_Uninitialize( HINSTANCE hInstance )
{
    ColorButton_Uninitialize(hInstance);
	Spinner_Uninitialize(hInstance);
}
