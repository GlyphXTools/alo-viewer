#include "Dialogs/Dialogs.h"
#include "General/Utils.h"
#include "General/WinUtils.h"
#include "config.h"
#include "resource.h"
#include <sstream>
using namespace std;
using namespace Alamo;

INT_PTR CALLBACK AboutProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            HWND hVersion = GetDlgItem(hWnd, IDC_VERSION);
            wstring text = GetWindowText(hVersion);
            text = FormatString(text.c_str(), Config::VERSION_MAJOR, Config::VERSION_MINOR);
            SetWindowText(hVersion, text.c_str());
            
            HWND hBuildDate = GetDlgItem(hWnd, IDC_BUILDDATE);
            text = GetWindowText(hBuildDate);
            const char* s = __DATE__;
            text = FormatString(text.c_str(), s);
            SetWindowText(hBuildDate, text.c_str());

            wstring copyright = LoadString(IDS_EXPAT_COPYRIGHT);
            SetWindowText(GetDlgItem(hWnd, IDC_EXPAT_COPYRIGHT), copyright.c_str());

            copyright = LoadString(IDS_NLOHMANNJSON_COPYRIGHT);
            SetWindowText(GetDlgItem(hWnd, IDC_NLOHMANNJSON_COPYRIGHT), copyright.c_str());

            wstring disclaimer = LoadString(IDS_DISCLAIMER);
            SetWindowText(GetDlgItem(hWnd, IDC_DISCLAIMER), disclaimer.c_str());

            return TRUE;
        }

        case WM_COMMAND:
		{
			WORD code = HIWORD(wParam);
			WORD id   = LOWORD(wParam);
			if (code == BN_CLICKED && id == IDOK || id == IDCANCEL)
			{
                EndDialog(hWnd, 0);
            }
            break;
        }
    }
    return FALSE;
}

void Dialogs::ShowAboutDialog(HWND hWndParent)
{
    DialogBox(NULL, MAKEINTRESOURCE(IDD_ABOUT), hWndParent, AboutProc);
}
