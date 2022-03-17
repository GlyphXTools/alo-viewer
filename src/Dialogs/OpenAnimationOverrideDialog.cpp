#include "Dialogs/Dialogs.h"
#include "General/Utils.h"
#include "General/WinUtils.h"
#include "resource.h"
#include <commdlg.h>
#include <sstream>
using namespace Alamo;
using namespace std;
using namespace Dialogs;

static bool ModelFilter(const string& name, ptr<IFile> f, void* param)
{
    // Check extension
    string::size_type ofs = name.find_last_of(".");
    return (ofs != string::npos) && (Uppercase(name.substr(ofs + 1)) == "ALO");
}

bool Dialogs::ShowOpenAnimationOverrideDialog(HWND hWndParent, GameMod mod, wstring* filename, ptr<MegaFile>& meg)
{
#ifdef NDEBUG
    // In debug mode the IDE should catch this
	try
#endif
	{
        HINSTANCE hInstance = (HINSTANCE)(LONG_PTR)GetWindowLongPtr(hWndParent, GWLP_HINSTANCE);
	    TCHAR filebuf[MAX_PATH];
	    filebuf[0] = '\0';

		wstring initialDir = mod.GetBaseDir() + L"\\Data";
		if (!mod.IsBaseGame()) {
			initialDir += L"\\Art\\Models";
		}

        wstring filter = LoadString(IDS_FILES_ALAMO) + wstring(L" (*.alo, *.meg)\0*.ALO; *.MEG\0", 29)
                       + LoadString(IDS_FILES_ALL)   + wstring(L" (*.*)\0*.*\0", 11);
		wstring title  = LoadString(IDS_FILES_OPENANIMATIONOVERRIDE);

	    OPENFILENAME ofn;
	    memset(&ofn, 0, sizeof(OPENFILENAME));
	    ofn.lStructSize     = sizeof(OPENFILENAME);
	    ofn.hwndOwner       = hWndParent;
	    ofn.hInstance       = hInstance;
        ofn.lpstrFilter     = filter.c_str();
	    ofn.nFilterIndex    = 0;
	    ofn.lpstrFile       = filebuf;
	    ofn.nMaxFile        = MAX_PATH;
		ofn.lpstrInitialDir = initialDir.c_str();
		ofn.lpstrTitle      = title.c_str();
	    ofn.Flags           = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
		if (GetOpenFileName( &ofn ) != 0)
		{
            *filename = filebuf;
			wstring ext = Uppercase(&filebuf[ofn.nFileExtension]);
			ptr<IFile> file = new PhysicalFile(filebuf);
			if (ext == L"MEG")
			{
				// Load it through a MegaFile
				meg = new MegaFile( file );
                int index = Dialogs::ShowSelectSubFileDialog(hWndParent, meg, ModelFilter, NULL);
                if (index >= 0)
                {
                    *filename += L"|" + AnsiToWide(meg->GetFilename(index));
                }
			}
		}
	}
#ifdef NDEBUG
	catch (...)
	{
        wstring error = LoadString(IDS_ERR_UNABLE_TO_OPEN_MODEL);
		MessageBox(NULL, error.c_str(), NULL, MB_OK | MB_ICONHAND );
		return false;
	}
#endif
	return true;
}