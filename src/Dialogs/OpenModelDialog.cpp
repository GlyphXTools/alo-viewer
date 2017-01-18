#include "Dialogs/Dialogs.h"
#include "General/Utils.h"
#include "General/WinUtils.h"
#include "resource.h"
#include <commdlg.h>
using namespace Alamo;
using namespace std;

static bool ModelFilter(const string& name, ptr<IFile> f, void* param)
{
    // Check extension
    string::size_type ofs = name.find_last_of(".");
    return (ofs != string::npos) && (Uppercase(name.substr(ofs + 1)) == "ALO");
}

ptr<Model> Dialogs::ShowOpenModelDialog(HWND hWndParent, wstring* filename, ptr<MegaFile>& meg)
{
	ptr<Model> model = NULL;
#ifdef NDEBUG
    // In debug mode the IDE should catch this
	try
#endif
	{
        HINSTANCE hInstance = (HINSTANCE)(LONG_PTR)GetWindowLongPtr(hWndParent, GWLP_HINSTANCE);
	    TCHAR filebuf[MAX_PATH];
	    filebuf[0] = '\0';

        wstring filter = LoadString(IDS_FILES_ALAMO) + wstring(L" (*.alo, *.meg)\0*.ALO; *.MEG\0", 29)
                       + LoadString(IDS_FILES_ALL)   + wstring(L" (*.*)\0*.*\0", 11);

	    OPENFILENAME ofn;
	    memset(&ofn, 0, sizeof(OPENFILENAME));
	    ofn.lStructSize  = sizeof(OPENFILENAME);
	    ofn.hwndOwner    = hWndParent;
	    ofn.hInstance    = hInstance;
        ofn.lpstrFilter  = filter.c_str();
	    ofn.nFilterIndex = 0;
	    ofn.lpstrFile    = filebuf;
	    ofn.nMaxFile     = MAX_PATH;
	    ofn.Flags        = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
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
                    file       = meg->GetFile(index);
                    *filename += L"|" + AnsiToWide(meg->GetFilename(index));
                }
			}
			
            if (file != NULL)
			{
				// Load the model file
				model = new Model(file);
			}
		}
	}
#ifdef NDEBUG
	catch (...)
	{
        wstring error = LoadString(IDS_ERR_UNABLE_TO_OPEN_MODEL);
		MessageBox(NULL, error.c_str(), NULL, MB_OK | MB_ICONHAND );
		model = NULL;
	}
#endif
	return model;
}