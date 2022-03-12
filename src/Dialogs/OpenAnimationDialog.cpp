#include "Dialogs/Dialogs.h"
#include "General/Utils.h"
#include "General/WinUtils.h"
#include "resource.h"
#include <commdlg.h>
#include <sstream>
using namespace Alamo;
using namespace std;
using namespace Dialogs;

static bool AnimationFilter(const string& name, ptr<IFile> f, void* param)
{
    // Check extension
    string::size_type ofs = name.find_last_of(".");
    if (ofs != string::npos && Uppercase(name.substr(ofs + 1)) == "ALA")
    {
        // Check compatability
        try
        {
            const Model* model = (const Model*)param;
            ptr<Animation> anim = new Animation(f, *model, true);
            return true;
        }
        catch (...)
        {
        }
    }
    return false;
}

ptr<ANIMATION_INFO> Dialogs::ShowOpenAnimationDialog(HWND hWndParent, GameMod mod, ptr<Model> model)
{
	ptr<ANIMATION_INFO> pai = new ANIMATION_INFO;
#ifdef NDEBUG
    // In debug mode, we want to trap the exception in the debugger
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

		wstring filter = LoadString(IDS_FILES_ALAMO) + wstring(L" (*.ala, *.meg)\0*.ALA; *.MEG\0", 29)
                       + LoadString(IDS_FILES_ALL)   + wstring(L" (*.*)\0*.*\0", 11);

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
		ofn.Flags           = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
		if (GetOpenFileName( &ofn ) != 0)
		{
			pai->filename = filebuf;
			wstring ext = Uppercase(&filebuf[ofn.nFileExtension]);
			pai->file = new PhysicalFile(filebuf);
			if (ext == L"MEG")
			{
				// Load it through a MegaFile
				ptr<MegaFile> meg = new MegaFile(pai->file);
                int index = Dialogs::ShowSelectSubFileDialog(hWndParent, meg, AnimationFilter, (Model*)model);
                if (index >= 0)
                {
					pai->file      = meg->GetFile(index);
					pai->filename += L"|" + AnsiToWide(meg->GetFilename(index));
                }
			}
			
            if (pai->file != NULL)
			{
				// Load the model file
				pai->animation = new Animation(pai->file, *model);
			}
		}
	}
#ifdef NDEBUG
	catch (...)
	{
        wstring error = LoadString(IDS_ERR_UNABLE_TO_OPEN_ANIMATION);
		MessageBox(NULL, error.c_str(), NULL, MB_OK | MB_ICONHAND );
		pai->animation = NULL;
	}
#endif
	
	wchar_t fName[_MAX_FNAME];
	if (_wsplitpath_s(pai->filename.c_str(), NULL, 0, NULL, 0, fName, _MAX_FNAME, NULL, 0) == 0)
	{
		pai->name = fName;
	}
	else
	{
		pai->name = L"Failed to get animation name";
	}
	return pai;
}