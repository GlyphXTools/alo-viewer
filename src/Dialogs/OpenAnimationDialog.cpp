#include "Dialogs/Dialogs.h"
#include "General/Utils.h"
#include "General/WinUtils.h"
#include "resource.h"
#include <commdlg.h>
#include <sstream>
using namespace Alamo;
using namespace std;

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

ptr<Animation> Dialogs::ShowOpenAnimationDialog(HWND hWndParent, GameMod mod, ptr<Model> model, wstring* filename)
{
	ptr<Animation> anim = NULL;

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
            *filename = filebuf;
			wstring ext = Uppercase(&filebuf[ofn.nFileExtension]);
			ptr<IFile> file = new PhysicalFile(filebuf);
			if (ext == L"MEG")
			{
				// Load it through a MegaFile
				ptr<MegaFile> meg = new MegaFile( file );
                int index = Dialogs::ShowSelectSubFileDialog(hWndParent, meg, AnimationFilter, (Model*)model);
                if (index >= 0)
                {
                    file       = meg->GetFile(index);
                    *filename += L"|" + AnsiToWide(meg->GetFilename(index));
                }
			}
			
            if (file != NULL)
			{
				// Load the model file
				anim = new Animation(file, *model);
			}
		}
	}
#ifdef NDEBUG
	catch (...)
	{
        wstring error = LoadString(IDS_ERR_UNABLE_TO_OPEN_ANIMATION);
		MessageBox(NULL, error.c_str(), NULL, MB_OK | MB_ICONHAND );
		anim = NULL;
	}
#endif
	return anim;
}