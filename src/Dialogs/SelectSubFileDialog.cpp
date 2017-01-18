#include "Dialogs/Dialogs.h"
#include "General/Utils.h"
#include "resource.h"
using namespace Alamo;
using namespace std;

namespace Dialogs
{

struct SELECTFILEPARAM
{
	const MegaFile* megaFile;
    SSF_CALLBACK    callback;
	void*           userData;
};

// Dialog box window procedure for "Select file" dialog
INT_PTR CALLBACK SelectFileDlgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_INITDIALOG:
		{
			HWND hList = GetDlgItem(hWnd, IDC_LIST1);
        	SELECTFILEPARAM* sfp = (SELECTFILEPARAM*)lParam;
			for (unsigned int i = 0; i < sfp->megaFile->GetNumFiles(); i++)
			{
                const string& name = sfp->megaFile->GetFilename(i);
				ptr<IFile> f = sfp->megaFile->GetFile(i);
                if (sfp->callback == NULL || sfp->callback(name, f, sfp->userData))
			    {
                    wstring wname = AnsiToWide(name);
				    int index = (int)SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)wname.c_str() );
				    SendMessage(hList, LB_SETITEMDATA, index, i );
			    }
			}
			SetFocus( hList );
			return FALSE;
		}

		case WM_COMMAND:
		{
			WORD code = HIWORD(wParam);
			WORD id   = LOWORD(wParam);
			if (code == LBN_DBLCLK || code == BN_CLICKED)
			{
				int index = -1;
				if ((code == BN_CLICKED && id == IDOK) || (code == LBN_DBLCLK && id == IDC_LIST1))
				{
					HWND hList = GetDlgItem(hWnd, IDC_LIST1);
					index = (int)SendMessage(hList, LB_GETCURSEL, 0, 0 );
					if (index == LB_ERR)
					{
						return TRUE;
					}
					index = (int)SendMessage(hList, LB_GETITEMDATA, index, 0 );
				}
				EndDialog( hWnd, index );
			}
			return TRUE;
		}
	}
	return FALSE;
}

int ShowSelectSubFileDialog(HWND hWndParent, ptr<MegaFile> meg, SSF_CALLBACK callback, void* userdata)
{
    SELECTFILEPARAM sfp;
	sfp.megaFile = meg;
	sfp.callback = callback;
    sfp.userData = userdata;

	return (int)DialogBoxParam(NULL, MAKEINTRESOURCE(IDD_SELECT_SUBFILE), hWndParent, SelectFileDlgProc, (LPARAM)&sfp );
}

}