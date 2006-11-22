#include <iostream>
#include <algorithm>
#include <sstream>
#include "model.h"
#include "mesh.h"
#include "engine.h"
#include "exceptions.h"
#include "dialogs.h"
#include "log.h"
#include "UI/UI.h"
using namespace std;

#include <shlobj.h>
#include <shlwapi.h>
#include <shellapi.h>
#include <afxres.h>
#include <commdlg.h>
#include <richedit.h>
#include "resource.h"

// Show up to this amount of files in the File menu
static const int NUM_HISTORY_ITEMS = 9;

class TextureManager : public ITextureManager
{
	typedef map<string,IDirect3DTexture9*> TextureMap;

	TextureMap			textures;
	string				basePath;
	IFileManager*		fileManager;
	IDirect3DTexture9*  pDefaultTexture;

	IDirect3DTexture9* load(IDirect3DDevice9* pDevice, const string& filename)
	{
		TextureMap::iterator p = textures.find(filename);
		if (p != textures.end())
		{
			// Texture has already been loaded
			return p->second;
		}

		IDirect3DTexture9* pTexture = NULL;
		File* file = fileManager->getFile( basePath + filename );
		if (file != NULL)
		{
			unsigned long size = file->getSize();
			char* data = new char[ size ];
			file->read( (void*)data, size );
			if (D3DXCreateTextureFromFileInMemory( pDevice, (void*)data, size, &pTexture ) != D3D_OK)
			{
				Log::Write("Unable to load texture %s\n", filename.c_str());
				pTexture = NULL;
			}
			delete[] data;
		}
		return pTexture;
	}

public:
	// Invalidate all entries (i.e. clear them)
	void invalidateAll()
	{
		for (TextureMap::iterator i = textures.begin(); i != textures.end(); i++)
		{
			i->second->Release();
		}
		textures.clear();
		if (pDefaultTexture != NULL)
		{
			pDefaultTexture->Release();
			pDefaultTexture = NULL;
		}
	}

	IDirect3DTexture9* getTexture(IDirect3DDevice9* pDevice, string filename)
	{
		transform(filename.begin(), filename.end(), filename.begin(), toupper);

		IDirect3DTexture9* pTexture = load(pDevice, filename);
		if (pTexture == NULL)
		{
			size_t pos;
			string name = filename;
			if ((pos = filename.rfind('.')) != string::npos)
			{
				name = name.substr(0, pos) + ".DDS";
			}
		
			pTexture = load(pDevice, name);
			if (pTexture == NULL)
			{
				// Load and return default placeholder texture
				if (pDefaultTexture == NULL)
				{
					D3DXCreateTextureFromResource( pDevice, GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_MISSING), &pDefaultTexture );
				}

				if (pDefaultTexture != NULL)
				{
					pTexture = pDefaultTexture;
					pDefaultTexture->AddRef();
				}
			}
		}

		if (pTexture != NULL)
		{
			textures.insert(make_pair(filename, pTexture));
		}

		return pTexture;
	}

	TextureManager(IFileManager* fileManager, const string& basePath)
	{
		this->basePath		  = basePath;
		this->fileManager	  = fileManager;
		this->pDefaultTexture = NULL;
	}

	~TextureManager()
	{
		invalidateAll();
	}
};

class EffectInclude : public ID3DXInclude
{
	string		   basePath;
	IFileManager*  fileManager;

public:
	STDMETHOD(Open)(D3DXINCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID * ppData, UINT * pBytes)
	{
		try
		{
			File* file = fileManager->getFile(basePath + pFileName);
			if (file != NULL)
			{
				*pBytes = file->getSize();
				char* data = new char[*pBytes];
				file->read(data, *pBytes);
				*ppData = data;
				return S_OK;
			}
		}
		catch (...)
		{
		}
		return E_FAIL;
	}
	
	STDMETHOD(Close)(LPCVOID pData)
	{
		delete[] (char*)pData;
		return S_OK;
	}

	EffectInclude(IFileManager* fileManager, const string& basePath)
	{
		this->fileManager = fileManager;
		this->basePath    = basePath;
	}
};

class EffectManager : public IEffectManager
{
	typedef map<string,ID3DXEffect*> EffectMap;

	EffectInclude  include;

	EffectMap      effects;
	string		   basePath;
	IFileManager*  fileManager;
	ID3DXEffect*   pDefaultEffect;

	ID3DXEffect* load(IDirect3DDevice9* pDevice, const string& filename)
	{
		EffectMap::iterator p = effects.find(filename);
		if (p != effects.end())
		{
			// Effect has already been loaded
			return p->second;
		}
		
		// Effect has not yet been loaded; load it
		ID3DXEffect* pEffect = NULL;
		File* file = fileManager->getFile( basePath + filename );
		if (file != NULL)
		{
			unsigned long size = file->getSize();
			char* data = new char[ size ];
			file->read( (void*)data, size );
			
			HRESULT hErr;
			ID3DXBuffer* errors;
			if ((hErr = D3DXCreateEffect(pDevice, data, size, NULL, &include, 0, NULL, &pEffect, &errors)) != D3D_OK)
			{
				Log::Write("Unable to load shader %s\nErrors:\n%.*s\n", filename.c_str(), errors->GetBufferSize(), errors->GetBufferPointer());
				errors->Release();
				pEffect = NULL;
			}
			delete[] data;
		}

		return pEffect;
	}

public:

	// Invalidate all entries (i.e. clear them)
	void invalidateAll()
	{
		for (EffectMap::iterator i = effects.begin(); i != effects.end(); i++)
		{
			i->second->Release();
		}
		effects.clear();
		if (pDefaultEffect != NULL)
		{
			pDefaultEffect->Release();
			pDefaultEffect = NULL;
		}
	}

	ID3DXEffect* getEffect(IDirect3DDevice9* pDevice, string filename)
	{
		transform(filename.begin(), filename.end(), filename.begin(), toupper);
		ID3DXEffect* pEffect = load(pDevice, filename);
		if (pEffect == NULL)
		{
			string name = filename;
			size_t pos  = name.rfind(".");
			if (pos != string::npos)
			{
				name = name.substr(0, pos) + ".FXO";
			}

			pEffect = load(pDevice, name);
			if (pEffect == NULL)
			{
				// Load and return default placeholder texture
				if (pDefaultEffect == NULL)
				{
					HRSRC   hRsrc = FindResource(NULL, MAKEINTRESOURCE(IDS_DEFAULT), MAKEINTRESOURCE(300));
					HGLOBAL hGlobal;
					LPVOID  data;
					if (hRsrc != NULL && (hGlobal = LoadResource(NULL, hRsrc)) != NULL && (data = LockResource(hGlobal)) != NULL)
					{
						D3DXCreateEffect(pDevice, data, SizeofResource(NULL, hRsrc), NULL, NULL, 0, NULL, &pDefaultEffect, NULL);
					}
				}
				
				if (pDefaultEffect != NULL)
				{
					pEffect = pDefaultEffect;
					pDefaultEffect->AddRef();
				}
			}
		}

		if (pEffect != NULL)
		{
			effects.insert(make_pair(filename, pEffect));
		}

		return pEffect;
	}

	EffectManager(IFileManager* fileManager, const string& basePath) : include(fileManager, basePath)
	{
		this->basePath       = basePath;
		this->fileManager    = fileManager;
		this->pDefaultEffect = NULL;
	}

	~EffectManager()
	{
		invalidateAll();
	}
};

static void SetWindowTitle( ApplicationInfo* info, const string& filename )
{
	int len = GetWindowTextLength(info->hMainWnd) + 1;
	char* buffer = new char[len];
	GetWindowText(info->hMainWnd, buffer, len);
	string text = buffer;
	delete[] buffer;
	
	size_t pos = text.find(" - ");
	if (pos != string::npos)
	{
		text = text.substr(0,pos);
	}
	
	if (filename != "")
	{
		text = text + " - " + filename;
	}

	SetWindowText(info->hMainWnd, text.c_str());
}

static void GetHistory(map<ULONGLONG, string>& history)
{
	history.clear();

	HKEY hKey;
	if (RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\AloViewer", 0, KEY_READ | KEY_WRITE, &hKey) == ERROR_SUCCESS)
	{
		LONG error;
		for (int i = 0;; i++)
		{
			char  name[256] = {'\0'};
			DWORD length = 255;
			DWORD type, size;
			if ((error = RegEnumValue(hKey, i, name, &length, NULL, &type, NULL, &size)) != ERROR_SUCCESS)
			{
				break;
			}

			if (type == REG_BINARY && size == sizeof(FILETIME))
			{
				FILETIME filetime;
				if (RegQueryValueEx(hKey, name, NULL, &type, (BYTE*)&filetime, &size) != ERROR_SUCCESS)
				{
					break;
				}
				ULARGE_INTEGER largeint;
				largeint.LowPart  = filetime.dwLowDateTime;
				largeint.HighPart = filetime.dwHighDateTime;
				history.insert(make_pair(largeint.QuadPart, name));
			}
		}

		if (error == ERROR_NO_MORE_ITEMS)
		{
			// Graceful loop end, now delete everything older than the X-th oldest item
			map<ULONGLONG,string>::const_reverse_iterator p = history.rbegin();
			for (int j = 0; p != history.rend() && j < NUM_HISTORY_ITEMS; p++, j++);

			// Now start deleting
			for (; p != history.rend(); p++)
			{
				RegDeleteValue(hKey, p->second.c_str());
			}
		}

		RegCloseKey(hKey);
	}
}

// Adds the Alo Viewer history to the file menu
static bool AppendHistory(ApplicationInfo* info)
{
	// Get the history (timestamp, filename pairs)
	GetHistory(info->history);

	HMENU hMenu = GetMenu(info->hMainWnd);
	hMenu = GetSubMenu(hMenu, 0);

	MENUITEMINFO mii;
	mii.cbSize = sizeof(MENUITEMINFO);
	mii.fMask  = MIIM_TYPE;
	mii.cch    = 0;

	// Find the first seperator
	int i = 0;
	do {
		if (!GetMenuItemInfo(hMenu, i++, true, &mii))
		{
			return false;
		}
	} while (mii.fType != MFT_SEPARATOR);

	// Delete everything after it until only Exit's left
	mii.fMask = MIIM_ID;
	while (GetMenuItemInfo(hMenu, i, true, &mii) && mii.wID != ID_FILE_EXIT)
	{
		DeleteMenu(hMenu, i, MF_BYPOSITION);
	}

	if (!info->history.empty())
	{
		int j = 0;
		for (map<ULONGLONG,string>::const_reverse_iterator p = info->history.rbegin(); p != info->history.rend() && j < NUM_HISTORY_ITEMS; p++, i++, j++)
		{
			string name = p->second.c_str();

			HDC hDC = GetDC(info->hMainWnd);
			SelectObject(hDC, GetStockObject(DEFAULT_GUI_FONT));
			PathCompactPath(hDC, (LPSTR)name.c_str(), 400);
			ReleaseDC(info->hMainWnd, hDC);

			if (j < 9)
			{
				name = string("&") + (char)('1' + j) + " " + name;
			}
			InsertMenu(hMenu, i, MF_BYPOSITION | MF_STRING, ID_FILE_HISTORY_0 + j, name.c_str());
		}

		// Finally, add the seperator
		InsertMenu(hMenu, i, MF_BYPOSITION | MF_SEPARATOR, 0, 0);
	}
	return true;
}

// Adds this filename to the history
static void AddToHistory(const string& name)
{
	// Get the current date & time
	FILETIME   filetime;
	SYSTEMTIME systime;
	GetSystemTime(&systime);
	SystemTimeToFileTime(&systime, &filetime);

	HKEY hKey;
	if (RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\AloViewer", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS)
	{
		RegSetValueEx(hKey, name.c_str(), 0, REG_BINARY, (BYTE*)&filetime, sizeof(FILETIME));
		RegCloseKey(hKey);
	}
}

// Initializes the list box with mesh names when a model has been loaded
static void onModelLoaded( ApplicationInfo* info, Model* model )
{
	// Clear the list box
	SendMessage(info->hListBox, LB_RESETCONTENT, 0, 0);

	delete info->model;
	info->model = model;
	if (info->engine != NULL)
	{
		info->engine->setModel( model );
	}

	if (model == NULL)
	{
		SetWindowTitle(info, "");
	}
	else
	{
		// Add it to the history
		AddToHistory(model->getName());
		AppendHistory(info);

		SetWindowTitle(info, model->getName());

		// Add meshes to listbox
		for (unsigned int i = 0; i < model->getNumMeshes(); i++)
		{
			const Mesh* pMesh = model->getMesh(i);
			SendMessage(info->hListBox, LB_ADDSTRING, 0, (LPARAM)pMesh->getName().c_str());

			bool visible = !pMesh->isHidden();
			int bone = model->getConnection(i);
			if (bone >= 0)
			{
				visible = visible && model->getBone(bone)->visible;
			}

			if (visible)
			{
				SendMessage(info->hListBox, LB_SETSEL, TRUE, i);
				info->engine->enableMesh(i, true);
			}
		}
	}

	// Enable model menu
	HMENU hMenu = GetMenu(info->hMainWnd);
	EnableMenuItem(hMenu, 1, MF_BYPOSITION | ((model == NULL) ? MF_DISABLED : MF_ENABLED));
	DrawMenuBar(info->hMainWnd);

	// Enable Load animation menu item
	EnableMenuItem(GetSubMenu(hMenu, 0), 1, MF_BYPOSITION | ((model == NULL) ? MF_DISABLED : MF_ENABLED));

	// Reset animation controls
	SendMessage(info->hTimeSlider, TBM_SETPOS,   TRUE, 0);
	SendMessage(info->hTimeSlider, TBM_SETRANGE, TRUE, MAKELONG(0,1));
	EnableWindow(info->hTimeSlider, FALSE);
	EnableWindow(info->hPlayButton, FALSE);
	SendMessage(info->hPlayButton, BM_SETIMAGE, IMAGE_ICON, (LPARAM)LoadIcon(info->hInstance, MAKEINTRESOURCE(IDI_ICON_PLAY)));
	info->playing = false;

	// Redraw render window
	InvalidateRect(info->hRenderWnd, NULL, FALSE );
	UpdateWindow(info->hRenderWnd);
}

static void onAnimationLoaded( ApplicationInfo* info, Animation* anim )
{
	delete info->anim;
	info->anim = anim;
	if (info->engine != NULL)
	{
		info->engine->applyAnimation(anim);
	}

	if (anim != NULL)
	{
		// Add it to the history
		AddToHistory(anim->getName());
		AppendHistory(info);

		// Reset animation controls
		unsigned int numFrames = info->anim->getNumFrames();
		SendMessage(info->hTimeSlider, TBM_SETRANGE, TRUE, MAKELONG(0, numFrames - 1));
		SendMessage(info->hTimeSlider, TBM_SETPOS,   TRUE, 0);
		EnableWindow(info->hTimeSlider, TRUE);
		EnableWindow(info->hPlayButton, TRUE);
		SendMessage(info->hPlayButton, BM_SETIMAGE, IMAGE_ICON, (LPARAM)LoadIcon(info->hInstance, MAKEINTRESOURCE(IDI_ICON_PLAY)));
		info->playing = false;

		// Redraw render window
		InvalidateRect(info->hRenderWnd, NULL, FALSE );
		UpdateWindow(info->hRenderWnd);
	}
}

static bool LoadModel( ApplicationInfo* info, const string& filename )
{
	try
	{
		Model* model = NULL;
		size_t pos = filename.find_first_of("|");
		if (pos != string::npos)
		{
			// It's an MEG:Filename pair
			File* file = new PhysicalFile(filename.substr(0, pos));
			MegaFile megaFile(file);
			file->release();
			file = megaFile.getFile(filename.substr(pos + 1));
			model = new Model(file);
			file->release();
		}
		else
		{
			// It's a regular files
			File* file = new PhysicalFile(filename);
			model = new Model(file);
			file->release();
		}
		onModelLoaded(info, model);
		return true;
	}
	catch (...)
	{
		return false;
	}
}

static bool LoadAnimation( ApplicationInfo* info, const string& filename )
{
	try
	{
		Animation* anim = NULL;
		size_t pos = filename.find_first_of("|");
		if (pos != string::npos)
		{
			// It's an MEG:Filename pair
			File* file = new PhysicalFile(filename.substr(0, pos));
			MegaFile megaFile(file);
			file->release();
			file = megaFile.getFile(filename.substr(pos + 1));
			anim = new Animation(file);
			file->release();
		}
		else
		{
			// It's a regular files
			File* file = new PhysicalFile(filename);
			anim = new Animation(file);
			file->release();
		}
		onAnimationLoaded(info, anim);
		return true;
	}
	catch (...)
	{
		return false;
	}
}

static void LoadFile(ApplicationInfo* info, const string& filename)
{
	const char* ext = PathFindExtension(filename.c_str());
	if (*ext != '\0')
	{
		if (_stricmp(ext,".alo") == 0)
		{
			if (!LoadModel(info, filename))
			{
				MessageBox(NULL, "Unable to open the specified model", NULL, MB_OK | MB_ICONHAND );
			}
		}
		else if (_stricmp(ext,".ala") == 0)
		{
			if (info->model != NULL && !LoadAnimation(info, filename))
			{
				MessageBox(NULL, "Unable to open the specified animation", NULL, MB_OK | MB_ICONHAND );
			}
		}
	}
}

static void OpenHistoryFile(ApplicationInfo* info, int idx)
{
	// Find the correct entry
	map<ULONGLONG, string>::const_reverse_iterator p = info->history.rbegin();
	for (int j = 0; p != info->history.rend() && idx > 0; idx--, p++);
	if (p != info->history.rend())
	{
		LoadFile(info, p->second);
	}
}

static float GetSpinnerFloat(HWND hWnd)
{
	SPINNER_INFO si;
	si.Mask = SPIF_VALUE;
	Spinner_GetInfo(hWnd, &si);
	return si.f.Value;
};

// Reads light parameters from the UI spinners and color buttons
LIGHT ReadLight(HWND hWnd, int idIntensity, int idZAngle, int idTilt, int idColor, int idSpecular = -1)
{
	float    intensity = GetSpinnerFloat(GetDlgItem(hWnd, idIntensity));
	float    zangle    = D3DXToRadian(GetSpinnerFloat(GetDlgItem(hWnd, idZAngle)));
	float    tilt      = D3DXToRadian(GetSpinnerFloat(GetDlgItem(hWnd, idTilt)));
	COLORREF diffuse   = ColorButton_GetColor(GetDlgItem(hWnd, idColor));
	COLORREF specular  = ColorButton_GetColor(GetDlgItem(hWnd, idSpecular));

	LIGHT light = {
		D3DXVECTOR4(intensity * GetRValue(diffuse ) / 255.0f, intensity * GetGValue(diffuse ) / 255.0f, intensity * GetBValue(diffuse)  / 255.0f, 1.0f),
		D3DXVECTOR4(intensity * GetRValue(specular) / 255.0f, intensity * GetGValue(specular) / 255.0f, intensity * GetBValue(specular) / 255.0f, 1.0f),
		D3DXVECTOR3(cos(zangle)*cos(tilt), sin(zangle)*cos(tilt), sin(tilt))
	};
	return light;
}

static BOOL CALLBACK SettingsDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	ApplicationInfo* info = (ApplicationInfo*)(LONG_PTR)GetWindowLongPtr(hWnd, GWL_USERDATA);

	switch (uMsg)
	{
	case WM_INITDIALOG:
	{
		info = (ApplicationInfo*)lParam;
		SetWindowLongPtr(hWnd, GWL_USERDATA, (LONG)(LONG_PTR)info);

		SPINNER_INFO si;
		si.IsFloat = true;
		si.Mask = SPIF_ALL;

		si.f.Increment = 0.01f; si.f.MinValue = 0.0f; si.f.Value = 0.5f; 
		si.f.MaxValue = 0.75; Spinner_SetInfo(GetDlgItem(hWnd, IDC_SPINNER1), &si);
		si.f.MaxValue = 0.50; Spinner_SetInfo(GetDlgItem(hWnd, IDC_SPINNER4), &si);
		si.f.MaxValue = 0.50; Spinner_SetInfo(GetDlgItem(hWnd, IDC_SPINNER7), &si);

		si.f.Increment = 0.5f; si.f.MinValue = 0.0f; si.f.MaxValue = 360.0f;
		si.f.Value =   0.0f; Spinner_SetInfo(GetDlgItem(hWnd, IDC_SPINNER2),  &si);
		si.f.Value = 120.0f; Spinner_SetInfo(GetDlgItem(hWnd, IDC_SPINNER5),  &si);
		si.f.Value = 210.0f; Spinner_SetInfo(GetDlgItem(hWnd, IDC_SPINNER8),  &si);
		si.f.Value =   0.0f; Spinner_SetInfo(GetDlgItem(hWnd, IDC_SPINNER10), &si);

		si.f.Increment = 0.5f; si.f.MinValue = -90.0f; si.f.MaxValue = 90.0f;
		si.f.Value =  45.0f; Spinner_SetInfo(GetDlgItem(hWnd, IDC_SPINNER3), &si);
		si.f.Value = -10.0f; Spinner_SetInfo(GetDlgItem(hWnd, IDC_SPINNER6), &si);
		si.f.Value = -10.0f; Spinner_SetInfo(GetDlgItem(hWnd, IDC_SPINNER9), &si);

		ColorButton_SetColor(GetDlgItem(hWnd, IDC_COLOR1), RGB(255,255,255));
		ColorButton_SetColor(GetDlgItem(hWnd, IDC_COLOR2), RGB( 64, 64,128));
		ColorButton_SetColor(GetDlgItem(hWnd, IDC_COLOR3), RGB( 64, 64,128));
		ColorButton_SetColor(GetDlgItem(hWnd, IDC_COLOR4), RGB(128,128,128));
		ColorButton_SetColor(GetDlgItem(hWnd, IDC_COLOR5), RGB(255,255,255));
		ColorButton_SetColor(GetDlgItem(hWnd, IDC_COLOR6), RGB(  0,  0,  0));
		break;
	}
		
	case WM_COMMAND:
		if (info != NULL && info->engine != NULL)
		{
			WORD code     = HIWORD(wParam);
			WORD id       = LOWORD(wParam);
			HWND hControl = (HWND)lParam;
			if (code == BN_CLICKED)
			{
				switch (id)
				{
					case IDC_CHECK4:
						// Ground checkbox
						info->engine->setGroundVisibility(IsDlgButtonChecked(hWnd, id) == BST_CHECKED);
						break;
					
					case IDC_CHECK3:
						// Wireframe checkbox
						info->engine->setRenderMode( (IsDlgButtonChecked(hWnd, id) == BST_CHECKED) ? RM_WIREFRAME : RM_SOLID );
						break;

					case IDC_CHECK1:
						// Show bones checkbox
						EnableWindow(GetDlgItem(hWnd, IDC_CHECK2), IsDlgButtonChecked(hWnd, id) == BST_CHECKED);
						break;
				}
			}
			else if (code == SN_CHANGE || code == CBN_CHANGE)
			{
				// A spinner or color button has changed
				switch (id)
				{
					case IDC_SPINNER1: case IDC_SPINNER2: case IDC_SPINNER3: case IDC_COLOR1: case IDC_COLOR5:
						info->engine->setLight(LT_SUN, ReadLight(hWnd, IDC_SPINNER1, IDC_SPINNER2, IDC_SPINNER3, IDC_COLOR1, IDC_COLOR5));
						break;

					case IDC_SPINNER4: case IDC_SPINNER5: case IDC_SPINNER6: case IDC_COLOR2:
						info->engine->setLight(LT_FILL1, ReadLight(hWnd, IDC_SPINNER4, IDC_SPINNER5, IDC_SPINNER6, IDC_COLOR2));
						break;

					case IDC_SPINNER7: case IDC_SPINNER8: case IDC_SPINNER9: case IDC_COLOR3:
						info->engine->setLight(LT_FILL2, ReadLight(hWnd, IDC_SPINNER7, IDC_SPINNER8, IDC_SPINNER9, IDC_COLOR3));
						break;

					case IDC_SPINNER10:
					{	// Wind
						float zangle = D3DXToRadian(GetSpinnerFloat(hControl));
						info->engine->setWind(D3DXVECTOR3(cos(zangle), sin(zangle), 0.0f));
						break;
					}

					case IDC_COLOR4:
					{	// Shadow
						COLORREF color = ColorButton_GetColor(hControl);
						info->engine->setShadow(D3DXVECTOR4(GetRValue(color) / 255.0f, GetGValue(color) / 255.0f, GetBValue(color) / 255.0f, 1.0f));
						break;
					}

					case IDC_COLOR6:
					{	// Ambient
						COLORREF color = ColorButton_GetColor(hControl);
						info->engine->setAmbient(D3DXVECTOR4(GetRValue(color) / 255.0f, GetGValue(color) / 255.0f, GetBValue(color) / 255.0f, 1.0f));
						break;
					}
				}
			}
			InvalidateRect(info->hRenderWnd, NULL, FALSE);
			UpdateWindow(info->hRenderWnd);
		}
		break;
	}
	return FALSE;
}

static LRESULT CALLBACK MainWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	ApplicationInfo* info = (ApplicationInfo*)(LONG_PTR)GetWindowLongPtr(hWnd, GWL_USERDATA);

	switch (uMsg)
	{
		case WM_CREATE:
		{
			CREATESTRUCT* pcs   = (CREATESTRUCT*)lParam;
			HFONT         hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

			// Store pointer
			info = (ApplicationInfo*)pcs->lpCreateParams;
			SetWindowLongPtr(hWnd, GWL_USERDATA, (LONG)(LONG_PTR)info);

			// Create settings dialog
			if ((info->hSettings = CreateDialogParam(pcs->hInstance, MAKEINTRESOURCE(IDD_SETTINGS), hWnd, SettingsDialogProc, (LPARAM)info)) == NULL)
			{
				return -1;
			}

			// Create the log window at the bottom
			LoadLibrary("Riched32.dll");
			if ((info->hConsole = CreateWindowEx(WS_EX_CLIENTEDGE, "RICHEDIT", "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE,
				0, 0, 84, 84, hWnd, NULL, pcs->hInstance, NULL)) == NULL)
			{
				return -1;
			}

			// Get the sizes of the components
			RECT client, settings, console;
			GetClientRect(hWnd, &client);
			GetWindowRect(info->hSettings, &settings);
			GetWindowRect(info->hConsole,  &console);
			settings.bottom = settings.bottom - settings.top;
			settings.right  = settings.right  - settings.left;
			console.bottom  = console.bottom  - console.top;
			console.right   = console.right   - console.left;

			// Create the mesh selection list box
			if ((info->hListBox = CreateWindowEx(WS_EX_CLIENTEDGE, "LISTBOX", "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | LBS_NOINTEGRALHEIGHT | LBS_NOTIFY | LBS_HASSTRINGS | LBS_MULTIPLESEL | LBS_DISABLENOSCROLL,
				0, 0, settings.bottom, 0, hWnd, NULL, pcs->hInstance, NULL)) == NULL)
			{
				return -1;
			}
		
			// Create the top bar: Colors
			if ((info->hColorsLabel = CreateWindowEx(0, "STATIC", "Colorization:", WS_CHILD | WS_VISIBLE,
				settings.right + 4, 8, 500, 16, hWnd, NULL, pcs->hInstance, NULL)) == NULL)
			{
				return -1;
			}

			for (int i = 0; i < NUM_COLORS + 1; i++)
			{
				if ((info->hColorBtn[i] = CreateWindowEx(0, "BUTTON", "", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | BS_OWNERDRAW | (i == 0 ? WS_GROUP : 0),
					settings.right + 70 + i * 28, 4, 24, 24, hWnd, NULL, pcs->hInstance, NULL)) == NULL)
				{
					return -1;
				}
			}

			if ((info->hBackgroundLabel = CreateWindowEx(0, "STATIC", "Background: ", WS_CHILD | WS_VISIBLE,
				0, 0, 65, 16, hWnd, NULL, pcs->hInstance, NULL)) == NULL)
			{
				return -1;
			}
			
			if ((info->hBackgroundBtn = CreateWindowEx(0, "ColorButton", "", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_OWNERDRAW,
				0, 0, 24, 24, hWnd, NULL, pcs->hInstance, NULL)) == NULL)
			{
				return -1;
			}

			// Create the bottom bar: Animation controls
			if ((info->hPlayButton = CreateWindowEx(0, "BUTTON", "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_DISABLED | BS_ICON,
				0, 0, 24, 24, hWnd, NULL, pcs->hInstance, NULL)) == NULL)
			{
				return -1;
			}

			if ((info->hTimeSlider = CreateWindowEx(0, TRACKBAR_CLASS, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_DISABLED | TBS_HORZ | TBS_NOTICKS,
				0, 0, 0, 22, hWnd, NULL, pcs->hInstance, NULL)) == NULL)
			{
				return -1;
			}

			// Set the control's initial properties
			SendMessage(info->hListBox,         WM_SETFONT, (WPARAM)hFont, FALSE);
			SendMessage(info->hColorsLabel,     WM_SETFONT, (WPARAM)hFont, FALSE);
			SendMessage(info->hBackgroundLabel, WM_SETFONT, (WPARAM)hFont, FALSE);
			SendMessage(info->hPlayButton,      BM_SETIMAGE, IMAGE_ICON, (LPARAM)LoadIcon(pcs->hInstance, MAKEINTRESOURCE(IDI_ICON_PLAY)));
			SendMessage(info->hConsole, EM_SETBKGNDCOLOR, TRUE, GetSysColor(COLOR_WINDOW));
			SendMessage(info->hConsole, WM_SETFONT, (WPARAM)GetStockObject(ANSI_FIXED_FONT), TRUE);

			// And finally, the render window
			if ((info->hRenderWnd = CreateWindowEx(WS_EX_CLIENTEDGE | WS_EX_ACCEPTFILES, "AloRenderWindow", "", WS_CHILD | WS_VISIBLE,
				settings.right + 8, 40, 1, 1, hWnd, NULL, pcs->hInstance, info)) == NULL)
			{
				return -1;
			}

			ShowWindow(info->hSettings, SW_SHOW);
			break;
		}

		case WM_CLOSE:
			PostQuitMessage(0);
			break;

		case WM_SIZE:
			info->isMinimized = (wParam == SIZE_MINIMIZED);
			if (!info->isMinimized)
			{
				RECT client, settings, console, label;
				GetClientRect(hWnd, &client);
				GetWindowRect(info->hSettings, &settings);
				GetWindowRect(info->hConsole,  &console);
				GetWindowRect(info->hBackgroundLabel, &label);
				settings.bottom = settings.bottom - settings.top;
				settings.right  = settings.right  - settings.left;
				console.bottom = console.bottom - console.top;
				console.right  = console.right  - console.left;
				label.right  = label.right  - label.left;
				label.bottom = label.bottom - label.top;

				MoveWindow(info->hListBox,    0, settings.bottom + 4, settings.right, client.bottom - console.bottom - settings.bottom - 6, TRUE );
				MoveWindow(info->hRenderWnd,  settings.right +  4, 32, client.right - settings.right, client.bottom - console.bottom - 32 - 28, TRUE );
				MoveWindow(info->hPlayButton, settings.right +  4, client.bottom - console.bottom - 26, 24, 24, TRUE );
				MoveWindow(info->hTimeSlider, settings.right + 32, client.bottom - console.bottom - 26, client.right - settings.right - 32, 22, TRUE );
				MoveWindow(info->hConsole,    0, client.bottom - console.bottom, client.right, console.bottom, TRUE );
				MoveWindow(info->hBackgroundBtn,   client.right - 28, 4, 24, 24, TRUE);
				MoveWindow(info->hBackgroundLabel, client.right - 32 - label.right, 4 + (24 - label.bottom) / 2, label.right, label.bottom, TRUE);

				if (info->engine != NULL)
				{
					// When we resize, we need to kill the device and recreate it
					info->textureManager->invalidateAll();
					info->effectManager->invalidateAll();
					info->engine->reinitialize( info->hRenderWnd, client.right - 200, client.bottom );
				}
			}
			break;

		case WM_COMMAND:
			if (info != NULL)
			{
				// Menu and control notifications
				WORD code     = HIWORD(wParam);
				WORD id       = LOWORD(wParam);
				HWND hControl = (HWND)lParam;
				if (lParam == 0)
				{
					// Menu or accelerator
					if (id >= ID_FILE_HISTORY_0 && id < ID_FILE_HISTORY_0 + min(9,NUM_HISTORY_ITEMS))
					{
						// It's a history item
						OpenHistoryFile(info, id - ID_FILE_HISTORY_0);
					}
					else switch (id)
					{
						case ID_FILE_OPEN:
						{
							Model* model = DlgOpenModel( info );
							if (model != NULL)
							{
								onModelLoaded(info, model);
							}
							break;
						}

						case ID_FILE_OPENANIMATION:
							if (info->engine != NULL)
							{
								Animation* anim = DlgOpenAnimation( info, info->engine->getModel() );
								if (anim != NULL)
								{
									onAnimationLoaded(info, anim);
								}
							}
							break;

						case ID_FILE_EXIT:
							PostQuitMessage(0);
							break;

						case ID_MODEL_DETAILS40012:
							DialogBoxParam( info->hInstance, MAKEINTRESOURCE(IDD_DIALOG1), info->hMainWnd, ModelDetailsDlgProc, (LPARAM)info->model );
							break;

						case ID_HELP_ABOUT:
							MessageBox(NULL, "Alamo Object Viewer, v0.7\n\nBy Mike Lankamp", "About", MB_OK );
							break;
					}
				}
				else if (code == CBN_CHANGE && info->engine != NULL)
				{
					if (hControl == info->hBackgroundBtn)
					{
						// The background color has changed
						info->engine->setBackground(ColorButton_GetColor(hControl));
						InvalidateRect(info->hRenderWnd, NULL, FALSE);
						UpdateWindow(info->hRenderWnd);
					}
				}
				else if (code == LBN_SELCHANGE || code == BN_CLICKED)
				{
					if (hControl == info->hListBox && info->engine != NULL)
					{
						// Listbox selection changed, enable meshes appropriately
						unsigned int nItems = (unsigned int)SendMessage(info->hListBox, LB_GETCOUNT, 0, 0);
						for (unsigned int i = 0; i < nItems; i++)
						{
							info->engine->enableMesh(i, SendMessage(info->hListBox, LB_GETSEL, i, 0) > 0);
						}
						InvalidateRect(info->hRenderWnd, NULL, FALSE);
						UpdateWindow(info->hRenderWnd);
					}
					else if (hControl == info->hPlayButton)
					{
						info->playing = !info->playing;
						DWORD resource = (info->playing) ? IDI_ICON_PAUSE : IDI_ICON_PLAY;
						SendMessage(info->hPlayButton, BM_SETIMAGE, IMAGE_ICON, (LPARAM)LoadIcon(info->hInstance, MAKEINTRESOURCE(resource)));
						if (info->playing)
						{
							info->playStartTime  = GetTickCount();
							info->playStartFrame = (unsigned int)SendMessage(info->hTimeSlider, TBM_GETPOS, 0, 0);
						}
					}
				}
			}
			break;

		case WM_MEASUREITEM:
		{
			MEASUREITEMSTRUCT* pdi = (MEASUREITEMSTRUCT*)lParam;
			pdi->itemHeight = 12;
			break;
		}

		case WM_DRAWITEM:
		{
			HWND hBtn           = (HWND)wParam;
			DRAWITEMSTRUCT* dis = (DRAWITEMSTRUCT*)lParam;

			HPEN   hPen;
			HBRUSH hBrush;
			if (dis->hwndItem == info->hBackgroundBtn)
			{
				hBrush = CreateSolidBrush( (info->engine != NULL) ? info->engine->getBackground() : RGB(0,0,0) );
				hPen   = CreatePen(PS_SOLID, 1, RGB(0,0,0));
			}
			else
			{
				// Find the index of the button
				COLORREF color = RGB(0,0,0);
				int i;
				for (i = 0; i < NUM_COLORS; i++)
				{
					if (info->hColorBtn[i] == dis->hwndItem)
					{
						color = PredefinedColors[i];
						break;
					}
				}

				if (dis->itemState & ODS_SELECTED)
				{
					int previous = info->selectedColor;
					info->selectedColor = i;
					if (previous >= 0 && previous <= NUM_COLORS)
					{
						// Clear previous
						InvalidateRect(info->hColorBtn[previous], NULL, TRUE);
						UpdateWindow(info->hColorBtn[previous]);
					}
				}

				if (info->selectedColor == i) {
					hPen = CreatePen(PS_SOLID, 5, GetSysColor(COLOR_HIGHLIGHT) );
				} else {
					hPen = CreatePen(PS_SOLID, 1, RGB(0,0,0) );
				}
				if (i < NUM_COLORS) {
					hBrush = CreateSolidBrush(color);
				} else {
					LOGBRUSH lb = {
						BS_HATCHED,
						RGB(128,128,128),
						HS_BDIAGONAL,
					};
					hBrush = CreateBrushIndirect(&lb);
				}
			}

			SelectObject(dis->hDC, hBrush);
			SelectObject(dis->hDC, hPen);
			Rectangle(dis->hDC, dis->rcItem.left, dis->rcItem.top, dis->rcItem.right, dis->rcItem.bottom);
			DeleteObject(hBrush);
			DeleteObject(hPen);

			break;
		}

		case WM_SIZING:
		{
			const int MIN_WIDTH  = 700;
			const int MIN_HEIGHT = 400;

			RECT* rect = (RECT*)lParam;
			bool left  = (wParam == WMSZ_BOTTOMLEFT) || (wParam == WMSZ_LEFT) || (wParam == WMSZ_TOPLEFT);
			bool top   = (wParam == WMSZ_TOPLEFT)    || (wParam == WMSZ_TOP)  || (wParam == WMSZ_TOPRIGHT);
			if (rect->right - rect->left < MIN_WIDTH)
			{
				if (left) rect->left  = rect->right - MIN_WIDTH;
				else      rect->right = rect->left  + MIN_WIDTH;
			}
			if (rect->bottom - rect->top < MIN_HEIGHT)
			{
				if (top) rect->top    = rect->bottom - MIN_HEIGHT;
				else     rect->bottom = rect->top    + MIN_HEIGHT;
			}
			break;
		}

		case WM_HSCROLL:
			InvalidateRect(hWnd, NULL, FALSE);
			UpdateWindow(hWnd);
			break;
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

static void render(ApplicationInfo* info)
{
	unsigned int frame = 0;
	if (info->anim != NULL)
	{
		unsigned int curFrame = (unsigned int)SendMessage(info->hTimeSlider, TBM_GETPOS, 0, 0);
		if (info->playing)
		{
			frame = ((unsigned int)(info->anim->getFPS() * (GetTickCount() - info->playStartTime) / 1000.0f) + info->playStartFrame) % info->anim->getNumFrames();

			// Update time slider position
			if (curFrame != frame)
			{
				SendMessage(info->hTimeSlider, TBM_SETPOS, TRUE, frame );
			}
		}
		else
		{
			frame = curFrame;
		}
	}

	if (info->engine != NULL)
	{
		RENDERINFO ri;
		ri.showBones     = (IsDlgButtonChecked(info->hSettings, IDC_CHECK1) == BST_CHECKED);
		ri.showBoneNames = (IsDlgButtonChecked(info->hSettings, IDC_CHECK2) == BST_CHECKED);
		ri.useColor      = (info->selectedColor < NUM_COLORS);
		ri.color         = (info->selectedColor < NUM_COLORS) ? PredefinedColors[info->selectedColor] : 0;

		// Render!
		if (!info->engine->render(frame, ri))
		{
			// Device was lost and needs to be reset
			RECT client;
			GetClientRect(info->hMainWnd, &client);
			info->textureManager->invalidateAll();
			info->effectManager->invalidateAll();
			info->engine->reinitialize( info->hRenderWnd, client.right - 200, client.bottom );
			info->engine->render(frame, ri);
		}
	}
}

// Window procedure for the render window (canvas)
static LRESULT CALLBACK RenderWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	ApplicationInfo* info = (ApplicationInfo*)(LONG_PTR)GetWindowLongPtr(hWnd, GWL_USERDATA);

	switch (uMsg)
	{
		case WM_CREATE:
		{
			CREATESTRUCT* pcs = (CREATESTRUCT*)lParam;
			info = (ApplicationInfo*)pcs->lpCreateParams;
			SetWindowLongPtr(hWnd, GWL_USERDATA, (LONG)(LONG_PTR)info);
			break;
		}

		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
			if (info->engine != NULL)
			{
				// Start dragging, remember start settings
				info->startCam = info->engine->getCamera();
				info->xstart   = LOWORD(lParam);
				info->ystart   = HIWORD(lParam);
				info->dragmode = (wParam & MK_CONTROL) ? ApplicationInfo::ZOOM : (uMsg == WM_LBUTTONDOWN) ? ApplicationInfo::MOVE : ApplicationInfo::ROTATE;
				SetCapture(hWnd);
				SetFocus(hWnd);
			}
			break;

		case WM_MOUSEMOVE:
			if (info->engine != NULL && info->dragmode != ApplicationInfo::NONE)
			{
				// Yay, math time!
				long x = (short)LOWORD(lParam) - info->xstart;
				long y = (short)HIWORD(lParam) - info->ystart;

				Camera      camera = info->startCam;
				D3DXVECTOR3 orthVec, diff = info->startCam.Position - info->startCam.Target;
				
				// Get the orthogonal vector
				D3DXVec3Cross( &orthVec, &diff, &camera.Up );
				D3DXVec3Normalize( &orthVec, &orthVec );

				if (info->dragmode == ApplicationInfo::ROTATE)
				{
					// Lets rotate
					D3DXMATRIX rotateXY, rotateZ, rotate;
					D3DXMatrixRotationZ( &rotateZ, -D3DXToRadian(x / 2.0f) );
					D3DXMatrixRotationAxis( &rotateXY, &orthVec, D3DXToRadian(y / 2.0f) );
					D3DXMatrixMultiply( &rotate, &rotateXY, &rotateZ );
					D3DXVec3TransformCoord( &camera.Position, &diff, &rotate );
					camera.Position += camera.Target;
				}
				else if (info->dragmode == ApplicationInfo::MOVE)
				{
					// Lets translate
					D3DXVECTOR3 Up;
					D3DXVec3Cross( &Up, &orthVec, &diff );
					D3DXVec3Normalize( &Up, &Up );
					
					// The distance we move depends on the distance from the object
					// Large distance: move a lot, small distance: move a little
					float multiplier = D3DXVec3Length( &diff ) / 1000;

					camera.Target  += (float)x * multiplier * orthVec;
					camera.Target  += (float)y * multiplier * Up;
					camera.Position = diff + camera.Target;
				}
				else if (info->dragmode == ApplicationInfo::ZOOM)
				{
					// Lets zoom
					// The amount we scroll in and out depends on the distance.
					D3DXVECTOR3 diff = camera.Position - camera.Target;
					float olddist = D3DXVec3Length( &diff );
					float newdist = max(7.0f, olddist - sqrt(olddist) * -y);
					D3DXVec3Scale( &camera.Position, &diff, newdist / olddist );
					camera.Position += camera.Target;
				}

				info->engine->setCamera( camera );
				InvalidateRect(hWnd, NULL, FALSE);
			}
			break;

		case WM_MOUSEWHEEL:
			if (info->engine != NULL && info->dragmode == ApplicationInfo::NONE)
			{
				Camera camera = info->engine->getCamera();

				// The amount we scroll in and out depends on the distance.
				D3DXVECTOR3 diff = camera.Position - camera.Target;
				float olddist = D3DXVec3Length( &diff );
				float newdist = max(10.0f, olddist - sqrt(olddist) * (SHORT)HIWORD(wParam) / WHEEL_DELTA);
				D3DXVec3Scale( &camera.Position, &diff, newdist / olddist );
				camera.Position += camera.Target;

				info->engine->setCamera(camera);
				InvalidateRect(info->hRenderWnd, NULL, FALSE);
			}
			break;

		case WM_LBUTTONUP:
		case WM_RBUTTONUP:
			// Stop dragging
			info->dragmode = ApplicationInfo::NONE;
			ReleaseCapture();
			break;

		case WM_PAINT:
			if (info->engine != NULL)
			{
				render(info);
			}
			else
			{
				// No renderer; just paint it black
				PAINTSTRUCT ps;
				HDC hDC = BeginPaint(hWnd, &ps);
				FillRect(hDC, &ps.rcPaint, (HBRUSH)GetStockObject(BLACK_BRUSH));
				EndPaint(hWnd, &ps);
			}
			break;

		case WM_SETFOCUS:
			// Yes, we want focus please
			return 0;

		case WM_DROPFILES:
		{
			// User dropped a filename on the window
			HDROP hDrop = (HDROP)wParam;
			UINT nFiles = DragQueryFile(hDrop, -1, NULL, 0);
			bool model  = false, animation = false;
			for (UINT i = 0; i < nFiles; i++)
			{
				UINT size = DragQueryFile(hDrop, i, NULL, 0);
				string filename(size,' ');
				DragQueryFile(hDrop, i, (LPSTR)filename.c_str(), size + 1);
				LoadFile(info, filename);
			}
			DragFinish(hDrop);
			break;
		}
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

// Returns the install path by querying the registry, or an empty string when failed.
static void getGamePath_Reg(vector<string>& strings)
{
	const char* paths[] = {
		"Software\\LucasArts\\Star Wars Empire at War Forces of Corruption\\1.0",
		"Software\\LucasArts\\Star Wars Empire at War Forces of Corruption Demo\\1.0",
		"Software\\LucasArts\\Star Wars Empire at War\\1.0",
		""
	};

	for (int i = 0; paths[i][0] != '\0'; i++)
	{
		HKEY hKey;
		if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, paths[i], 0, KEY_QUERY_VALUE, &hKey ) == ERROR_SUCCESS)
		{
			DWORD type, size = MAX_PATH;
			char path[MAX_PATH];
			if (RegQueryValueEx(hKey, "ExePath", NULL, &type, (LPBYTE)path, &size) == ERROR_SUCCESS)
			{
				string str = path;
				size_t pos = str.find_last_of("\\");
				if (pos != string::npos)
				{
					str = str.substr(0, pos);
				}
				strings.push_back(str);
			}
			RegCloseKey(hKey);
		}
	}
}

// Adds the %PROGRAMFILES%\LucasArts\Star Wars Empire at War\GameData paths to the vector
static void getGamePath_Shell(vector<string>& strings)
{
	TCHAR path[MAX_PATH];
	if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_PROGRAM_FILES, NULL, SHGFP_TYPE_CURRENT, path )))
	{
		MessageBox(NULL, path, NULL, MB_OK );
		string str = path;
		if (*str.rbegin() != '\\') str += '\\';
		
		strings.push_back(str + "LucasArts\\Star Wars Empire at War Forces of Corruption");
		strings.push_back(str + "LucasArts\\Star Wars Empire at War Forces of Corruption Demo");
		strings.push_back(str + "LucasArts\\Star Wars Empire at War\\GameData");
	}
}

static FileManager* createFileManager( HWND hWnd, const vector<string>& argv )
{
	// Search for the Empire at War path
	vector<string> EmpireAtWarPaths;
	if (argv.size() > 1)
	{
		// Override on the command line; use that
		for (size_t i = 1; i < argv.size(); i++)
		{
			if (PathIsDirectory(argv[i].c_str()))
			{
				EmpireAtWarPaths.push_back(argv[i]);
			}
		}
	}

	if (EmpireAtWarPaths.empty())
	{
		// First try the registry
		getGamePath_Reg(EmpireAtWarPaths);
		if (EmpireAtWarPaths.empty())
		{
			// Then try the shell
			getGamePath_Shell(EmpireAtWarPaths);
		}
		
	}
	FileManager* fileManager = NULL;

	while (fileManager == NULL)
	{
		for (size_t i = 0; i < EmpireAtWarPaths.size(); i++)
		{
			if (*EmpireAtWarPaths[i].rbegin() != '\\') EmpireAtWarPaths[i] += '\\';
		}

		try
		{
			// Initialize the file manager
			fileManager = new FileManager( EmpireAtWarPaths );
		}
		catch (FileNotFoundException&)
		{
			// This path didn't work; ask the user to select a path
			BROWSEINFO bi;
			bi.hwndOwner      = hWnd;
			bi.pidlRoot       = NULL;
			bi.pszDisplayName = NULL;
			bi.lpszTitle      = "Please select the Star Wars Empire at War or Forces of Corruption folder that contains the Data folder.";
			bi.ulFlags        = BIF_RETURNONLYFSDIRS;
			bi.lpfn           = NULL;
			LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
			if (pidl == NULL)
			{
				fileManager = NULL;
				break;
			}

			TCHAR path[MAX_PATH];
			if (SHGetPathFromIDList( pidl, path ))
			{
				EmpireAtWarPaths.clear();
				EmpireAtWarPaths.push_back(path);
			}
			CoTaskMemFree(pidl);
		}
	}
	return fileManager;
}

void onLogChange(int diff, void* data)
{
	ApplicationInfo* info = (ApplicationInfo*)data;
	const vector<string>& lines = Log::getLines();
	string text;
	for (size_t i = 0; i < lines.size(); i++)
	{
		text += lines[i];
		if (i < lines.size() - 1) text += "\n";
	}
	SetWindowText(info->hConsole, text.c_str());
	SendMessage(info->hConsole, WM_VSCROLL, SB_BOTTOM, 0);
}

void main( ApplicationInfo* info, const vector<string>& argv )
{
	Log::addCallback(onLogChange, info);
	Log::Write("Alamo Object Viewer 0.7, by Mike Lankamp\n");

	FileManager* fileManager = createFileManager( info->hMainWnd, argv );
	if (fileManager == NULL)
	{
		// No file manager, no play
		return;
	}

	try
	{
		// Initialize the other managers and engine
		TextureManager textureManager(fileManager, "Data\\Art\\Textures\\");
		EffectManager  effectManager (fileManager, "Data\\Art\\Shaders\\");

		try
		{
			info->engine = new Engine(info->hRenderWnd, &textureManager, &effectManager);
		}
		catch (Exception& e)
		{
			Log::Write("Unable to initialize Direct3D:\n  %s\n", e.getMessage().c_str());
			info->engine = NULL;
		}

		info->anim           = NULL;
		info->fileManager    = fileManager;
		info->effectManager  = &effectManager;
		info->textureManager = &textureManager;

		if (info->engine != NULL)
		{
			// Initialize the engine with Nature settings
			COLORREF color;
			info->engine->setLight(LT_SUN,   ReadLight(info->hSettings, IDC_SPINNER1, IDC_SPINNER2, IDC_SPINNER3, IDC_COLOR1, IDC_COLOR5));
			info->engine->setLight(LT_FILL1, ReadLight(info->hSettings, IDC_SPINNER4, IDC_SPINNER5, IDC_SPINNER6, IDC_COLOR2));
			info->engine->setLight(LT_FILL2, ReadLight(info->hSettings, IDC_SPINNER7, IDC_SPINNER8, IDC_SPINNER9, IDC_COLOR3));
			color = ColorButton_GetColor(GetDlgItem(info->hSettings, IDC_COLOR4));
			info->engine->setShadow  (D3DXVECTOR4(GetRValue(color) / 255.0f, GetGValue(color) / 255.0f, GetBValue(color) / 255.0f, 1.0f));
			color = ColorButton_GetColor(GetDlgItem(info->hSettings, IDC_COLOR6));
			info->engine->setAmbient (D3DXVECTOR4(GetRValue(color) / 255.0f, GetGValue(color) / 255.0f, GetBValue(color) / 255.0f, 1.0f));

			// See if a file was specified
			for (size_t i = 1; i < argv.size(); i++)
			{
				if (PathFileExists(argv[i].c_str()) && !PathIsDirectory(argv[i].c_str()))
				{
					if (!LoadModel(info, argv[i]))
					{
						MessageBox(NULL, "Unable to open the specified model", NULL, MB_OK | MB_ICONHAND );
					}
					break;
				}
			}
		}

		// Message loop
		ShowWindow(info->hMainWnd, SW_SHOW);
		HACCEL hAccel = LoadAccelerators( info->hInstance, MAKEINTRESOURCE(IDR_ACCELERATOR1));	
		bool quit = false;
		while (!quit)
		{
			MSG msg;
			while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) != 0)
			{
				if (msg.message == WM_QUIT)
				{
					quit = true;
				}
				if (!TranslateAccelerator(info->hMainWnd, hAccel, &msg))
				if (!IsDialogMessage(info->hMainWnd, &msg))
				{
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
			}

			if (!quit)
			{
				if (info->engine != NULL && info->engine->getModel() != NULL && !info->isMinimized)
				{
					render(info);
				}
				else
				{
					WaitMessage();
				}
			}
		}
	}
	catch (...)
	{
		delete fileManager;
		throw;
	}
	delete fileManager;
}

// Create the main window and its child windows
static bool InitializeUI( ApplicationInfo* info )
{
	WNDCLASSEX wcx;
	wcx.cbSize        = sizeof wcx;
    wcx.style		  = CS_HREDRAW | CS_VREDRAW;
    wcx.lpfnWndProc	  = MainWindowProc;
    wcx.cbClsExtra	  = 0;
    wcx.cbWndExtra    = 0;
    wcx.hInstance     = info->hInstance;
    wcx.hIcon         = LoadIcon(info->hInstance, MAKEINTRESOURCE(IDI_LOGO));
    wcx.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wcx.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wcx.lpszMenuName  = MAKEINTRESOURCE(IDR_MENU1);
    wcx.lpszClassName = "AloViewer";
    wcx.hIconSm       = NULL;

	if (RegisterClassEx(&wcx) == 0)
	{
		return false;
	}

    wcx.lpfnWndProc	  = RenderWindowProc;
    wcx.lpszClassName = "AloRenderWindow";
    wcx.hbrBackground = NULL;
	if (RegisterClassEx(&wcx) == 0)
	{
		return false;
	}

	if (!UI_Initialize(info->hInstance))
	{
		return false;
	}

	if ((info->hMainWnd = CreateWindowEx(WS_EX_CONTROLPARENT, "AloViewer", "Alamo Object Viewer", WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, 
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, info->hInstance, info)) == NULL)
	{
		return false;
	}

	AppendHistory(info);
	return true;
}

vector<string> parseCommandLine()
{
	vector<string> argv;
	char* cmdline = GetCommandLine();

	bool quoted = false;
	string arg;
	for (char* p = cmdline; p == cmdline || *(p - 1) != '\0'; p++)
	{
		if (*p == '\0' || (*p == ' ' && !quoted))
		{
			if (arg != "")
			{
				argv.push_back(arg);
				arg = "";
			}
		}
		else if (*p == '"') quoted = !quoted;
		else arg += *p;
	}
	return argv;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)
{
	#ifndef NDEBUG
	AllocConsole();
	freopen("conout$", "w", stdout);
	#endif 

	try
	{
		ApplicationInfo info;
		info.hInstance     = hInstance;
		info.playing       = false;
		info.isMinimized   = false;
		info.selectedColor = NUM_COLORS;
		info.engine        = NULL;

		if (!InitializeUI( &info ))
		{
			throw Exception("Initialization failed");
		}

		main( &info, parseCommandLine() );
	}
	catch (Exception& e)
	{
		MessageBox(NULL, e.getMessage().c_str(), NULL, MB_OK );
	}

	#ifndef NDEBUG
	FreeConsole();
	#endif 

	return 0;
}