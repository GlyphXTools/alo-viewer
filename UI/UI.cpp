#include "UI.h"

bool Spinner_Initialize(HINSTANCE hInstance);
bool ColorButton_Initialize(HINSTANCE hInstance);

bool UI_Initialize( HINSTANCE hInstance )
{
	CoInitializeEx(NULL, COINIT_APARTMENTTHREADED );
	InitCommonControls();
	return (
			Spinner_Initialize(hInstance) &&
			ColorButton_Initialize(hInstance)
			);
}
