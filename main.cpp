#include <iostream>
#include <algorithm>
#include <sstream>
#include "model.h"
#include "mesh.h"
#include "engine.h"
#include "exceptions.h"
#include "dialogs.h"
using namespace std;

#include <shlobj.h>
#include <afxres.h>
#include "resource.h"

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

	TextureManager(IFileManager* fileManager, const std::string& basePath)
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

class EffectManager : public IEffectManager
{
	typedef map<string,ID3DXEffect*> EffectMap;

	EffectMap     effects;
	string        basePath;
	IFileManager* fileManager;
	ID3DXEffect*  pDefaultEffect;

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
			if (D3DXCreateEffect(pDevice, data, size, NULL, NULL, 0, NULL, &pEffect, NULL) != D3D_OK)
			{
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

	EffectManager(IFileManager* fileManager, const std::string& basePath)
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

// Initializes the list box with mesh names when a model has been loaded
static void onModelLoaded( ApplicationInfo* info, Model* model )
{
	// Clear the list box
	SendMessage(info->hListBox, LB_RESETCONTENT, 0, 0);

	delete info->engine->getModel();
	info->engine->setModel( model );

	if (model != NULL)
	{
		// Add meshes to listbox
		for (unsigned int i = 0; i < model->getNumMeshes(); i++)
		{
			const Mesh* pMesh = model->getMesh(i);
			SendMessage( info->hListBox, LB_ADDSTRING, 0, (LPARAM)pMesh->getName().c_str() );
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
	info->anim = anim;
	info->engine->applyAnimation(anim);

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

static LRESULT CALLBACK MainWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	ApplicationInfo* info = (ApplicationInfo*)(LONG_PTR)GetWindowLongPtr(hWnd, GWL_USERDATA);

	switch (uMsg)
	{
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
					switch (id)
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
						{
							Animation* anim = DlgOpenAnimation( info, info->engine->getModel() );
							if (anim != NULL)
							{
								onAnimationLoaded(info, anim);
							}
							break;
						}

						case ID_FILE_EXIT:
							PostQuitMessage(0);
							break;

						case ID_MODEL_DETAILS40012:
							DialogBoxParam( info->hInstance, MAKEINTRESOURCE(IDD_DIALOG1), info->hMainWnd, ModelDetailsDlgProc, (LPARAM)info->engine->getModel() );
							break;

						case ID_HELP_ABOUT:
							MessageBox(NULL, "Alamo Object Viewer, v0.4\n\nBy Mike Lankamp", "About", MB_OK );
							break;
					}
				}
				else if (code == LBN_SELCHANGE || code == BN_CLICKED)
				{
					if (hControl == info->hListBox)
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
					else if (hControl == info->hWireframeCheckbox)
					{
						// Redraw window
						info->engine->setRenderMode( (SendMessage(hControl, BM_GETCHECK, 0, 0) == BST_CHECKED) ? RM_WIREFRAME : RM_SOLID );
						InvalidateRect(info->hRenderWnd, NULL, FALSE);
						UpdateWindow(info->hRenderWnd);
					}
					else if (hControl == info->hBonesCheckbox || hControl == info->hNamesCheckbox)
					{
						if (hControl == info->hBonesCheckbox)
						{
							bool showBones = (SendMessage(info->hBonesCheckbox, BM_GETCHECK, 0, 0) == BST_CHECKED);
							EnableWindow(info->hNamesCheckbox, showBones);
						}
						// Redraw window
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

			HPEN hPen;
			if (info->selectedColor == i) {
				hPen = CreatePen(PS_SOLID, 5, GetSysColor(COLOR_HIGHLIGHT) );
			} else {
				hPen = CreatePen(PS_SOLID, 1, RGB(0,0,0) );
			}
			HBRUSH hBrush;
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
			SelectObject(dis->hDC, hBrush);
			SelectObject(dis->hDC, hPen);
			Rectangle(dis->hDC, dis->rcItem.left, dis->rcItem.top, dis->rcItem.right, dis->rcItem.bottom);
			DeleteObject(hBrush);
			DeleteObject(hPen);

			break;
		}

		case WM_SIZE:
			if (info != NULL)
			{
				info->isMinimized = (wParam == SIZE_MINIMIZED);
				if (!info->isMinimized)
				{
					RECT client;
					GetClientRect(info->hMainWnd, &client);
					MoveWindow( info->hListBox,      0, 58, 200, client.bottom - 58, TRUE );
					MoveWindow( info->hRenderWnd,  200, 40, client.right - 200, client.bottom - 70, TRUE );
					MoveWindow( info->hTimeSlider, 240, client.bottom - 26, client.right - 240, 22, TRUE );
					MoveWindow( info->hPlayButton, 208, client.bottom - 26, 24, 24, TRUE );

					// When we resize, we need to kill the device and recreate it
					info->textureManager->invalidateAll();
					info->effectManager->invalidateAll();
					info->engine->reinitialize( info->hRenderWnd, client.right - 200, client.bottom );
				}
			}
			break;

		case WM_SIZING:
		{
			const int MIN_WIDTH  = 300;
			const int MIN_HEIGHT = 200;

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

		case WM_CLOSE:
			PostQuitMessage(0);
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

	RENDERINFO ri;
	ri.showBones     = (SendMessage(info->hBonesCheckbox, BM_GETCHECK, 0, 0) == BST_CHECKED);
	ri.showBoneNames = (SendMessage(info->hNamesCheckbox, BM_GETCHECK, 0, 0) == BST_CHECKED);
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

// Window procedure for the render window (canvas)
static LRESULT CALLBACK RenderWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	ApplicationInfo* info = (ApplicationInfo*)(LONG_PTR)GetWindowLongPtr(hWnd, GWL_USERDATA);

	switch (uMsg)
	{
		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
			// Start dragging, remember start settings
			info->startCam = info->engine->getCamera();
			info->xstart   = LOWORD(lParam);
			info->ystart   = HIWORD(lParam);
			info->dragmode = (wParam & MK_CONTROL) ? ApplicationInfo::ZOOM : (uMsg == WM_LBUTTONDOWN) ? ApplicationInfo::MOVE : ApplicationInfo::ROTATE;
			SetCapture(hWnd);
			SetFocus(hWnd);
			break;

		case WM_MOUSEMOVE:
			if (info->dragmode != ApplicationInfo::NONE)
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
			if (info->dragmode == ApplicationInfo::NONE)
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
			render(info);
			break;

		case WM_SETFOCUS:
			// Yes, we want focus please
			return 0;
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

// Returns the install path by querying the registry, or an empty string when failed.
static string getGamePath_Reg()
{
	string str;
	HKEY   hKey;
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Software\\LucasArts\\Star Wars Empire at War\\1.0", 0, KEY_QUERY_VALUE, &hKey ) == ERROR_SUCCESS)
	{
		DWORD type, size = MAX_PATH;
		char path[MAX_PATH];
		if (RegQueryValueEx(hKey, "ExePath", NULL, &type, (LPBYTE)path, &size) == ERROR_SUCCESS)
		{
			str = path;
			size_t pos = str.find_last_of("\\");
			if (pos != string::npos)
			{
				str = str.substr(0, pos);
			}
		}
		RegCloseKey(hKey);
	}
	return str;
}

// Returns %PROGRAMFILES%\LucasArts\Star Wars Empire at War\GameData, or an empty string when failed.
static string getGamePath_Shell()
{
	TCHAR path[MAX_PATH];
	if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_PROGRAM_FILES, NULL, SHGFP_TYPE_CURRENT, path )))
	{
		MessageBox(NULL, path, NULL, MB_OK );
		string str = path;
		if (*str.rbegin() != '\\') str += '\\';
		return str + "LucasArts\\Star Wars Empire at War\\GameData";
	}
	return "";
}

static FileManager* createFileManager( HWND hWnd, const vector<string>& argv )
{
	// Search for the Empire at War path
	string EmpireAtWarPath;
	if (argv.size() > 1)
	{
		// Override on the command line; use that
		EmpireAtWarPath = argv[1];
	}
	else
	{
		// First try the registry
		if ((EmpireAtWarPath = getGamePath_Reg()) == "")
		{
			// Then try the shell
			EmpireAtWarPath = getGamePath_Shell();
		}
		
	}
	FileManager* fileManager = NULL;

	while (fileManager == NULL)
	{
		if (*EmpireAtWarPath.rbegin() != '\\') EmpireAtWarPath += '\\';

		try
		{
			// Initialize the file manager
			fileManager = new FileManager( EmpireAtWarPath );
		}
		catch (FileNotFoundException&)
		{
			// This path didn't work; ask the user to select a path
			BROWSEINFO bi;
			bi.hwndOwner      = hWnd;
			bi.pidlRoot       = NULL;
			bi.pszDisplayName = NULL;
			bi.lpszTitle      = "Please select the GameData folder in the Star Wars Empire at War installation directory.";
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
				EmpireAtWarPath = path;
			}
			CoTaskMemFree(pidl);
		}
	}
	return fileManager;
}

void main( ApplicationInfo* info, const vector<string>& argv )
{
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
		Engine         engine(info->hRenderWnd, &textureManager, &effectManager);

		info->anim           = NULL;
		info->engine         = &engine;
		info->fileManager    = fileManager;
		info->effectManager  = &effectManager;
		info->textureManager = &textureManager;

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
				{
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
			}

			if (!quit)
			{
				if (info->engine->getModel() != NULL && !info->isMinimized)
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
static void CreateMainWindow( ApplicationInfo* info )
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
		throw Exception("Unable to register window class");
	}

    wcx.lpfnWndProc	  = RenderWindowProc;
    wcx.lpszClassName = "AloRenderWindow";
    wcx.hbrBackground = NULL;
	if (RegisterClassEx(&wcx) == 0)
	{
		throw Exception("Unable to register window class");
	}

	try
	{
		info->hMainWnd = CreateWindowEx(0, "AloViewer", "Alamo Object Viewer", WS_OVERLAPPEDWINDOW, 
			CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, info->hInstance, NULL);
		if (info->hMainWnd == NULL) throw 0;

		RECT client;
		GetClientRect(info->hMainWnd, &client);
		HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

		info->hBonesCheckbox = CreateWindow("BUTTON", "Show bones", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
			8,  4, 184, 12, info->hMainWnd, NULL, info->hInstance, NULL);
		if (info->hBonesCheckbox == NULL) throw 0;
		SendMessage(info->hBonesCheckbox, WM_SETFONT, (WPARAM)hFont, FALSE);

		info->hNamesCheckbox = CreateWindow("BUTTON", "Show bone names", WS_CHILD | WS_VISIBLE | WS_DISABLED | BS_AUTOCHECKBOX,
			8, 22, 184, 12, info->hMainWnd, NULL, info->hInstance, NULL);
		if (info->hNamesCheckbox == NULL) throw 0;
		SendMessage(info->hNamesCheckbox, WM_SETFONT, (WPARAM)hFont, FALSE);

		info->hWireframeCheckbox = CreateWindow("BUTTON", "Show wireframe", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
			8, 40, 184, 12, info->hMainWnd, NULL, info->hInstance, NULL);
		if (info->hWireframeCheckbox == NULL) throw 0;
		SendMessage(info->hWireframeCheckbox, WM_SETFONT, (WPARAM)hFont, FALSE);

		info->hListBox = CreateWindowEx(WS_EX_CLIENTEDGE, "LISTBOX", "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOINTEGRALHEIGHT | LBS_NOTIFY | LBS_HASSTRINGS | LBS_MULTIPLESEL | LBS_DISABLENOSCROLL,
			0, 58, 200, client.bottom - 58, info->hMainWnd, NULL, info->hInstance, NULL);
		if (info->hListBox == NULL) throw 0;
		SendMessage(info->hListBox, WM_SETFONT, (WPARAM)hFont, FALSE);
		
		info->hColorsLabel = CreateWindowEx(0, "STATIC", "Colorization:", WS_CHILD | WS_VISIBLE,
			200, 8, 500, 32, info->hMainWnd, NULL, info->hInstance, NULL);
		if (info->hColorsLabel == NULL) throw 0;
		SendMessage(info->hColorsLabel, WM_SETFONT, (WPARAM)hFont, FALSE);

		for (int i = 0; i < NUM_COLORS + 1; i++)
		{
			info->hColorBtn[i] = CreateWindowEx(0, "BUTTON", "", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | BS_OWNERDRAW | (i == 0 ? WS_GROUP : 0),
				260 + i * 28, 4, 24, 24, info->hMainWnd, NULL, info->hInstance, NULL);
			if (info->hColorBtn[i] == NULL) throw 0;
		}

		info->hRenderWnd = CreateWindowEx(WS_EX_CLIENTEDGE, "AloRenderWindow", "", WS_CHILD | WS_VISIBLE,
			200, 40, client.right - 200, client.bottom - 70, info->hMainWnd, NULL, info->hInstance, NULL);
		if (info->hRenderWnd == NULL) throw 0;

		info->hTimeSlider = CreateWindowEx(0, TRACKBAR_CLASS, "", WS_CHILD | WS_VISIBLE | WS_DISABLED | TBS_HORZ | TBS_NOTICKS,
			240, client.bottom - 26, client.right - 240, 22, info->hMainWnd, NULL, info->hInstance, NULL);
		if (info->hTimeSlider == NULL) throw 0;

		info->hPlayButton = CreateWindowEx(0, "BUTTON", "", WS_CHILD | WS_VISIBLE | WS_DISABLED | BS_ICON,
			208, client.bottom - 26, 24, 24, info->hMainWnd, NULL, info->hInstance, NULL);
		if (info->hPlayButton == NULL) throw 0;
		SendMessage(info->hPlayButton, BM_SETIMAGE, IMAGE_ICON, (LPARAM)LoadIcon(info->hInstance, MAKEINTRESOURCE(IDI_ICON_PLAY)));
	}
	catch (...)
	{
		DestroyWindow(info->hMainWnd);
		throw Exception("Unable to create main window");
	}

	SetWindowLongPtr(info->hMainWnd,   GWL_USERDATA, (LONG)(LONG_PTR)info );
	SetWindowLongPtr(info->hRenderWnd, GWL_USERDATA, (LONG)(LONG_PTR)info );
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
		CoInitializeEx(NULL, COINIT_APARTMENTTHREADED );
		InitCommonControls();
		ApplicationInfo info;
		info.hInstance     = hInstance;
		info.playing       = false;
		info.isMinimized   = false;
		info.selectedColor = NUM_COLORS;

		CreateMainWindow( &info );
		main( &info, parseCommandLine() );
	}
	catch (Exception& e)
	{
		stringstream str;
		str << "Caught exception from " + e.getFilename() << ":" << e.getLine() << endl << endl << e.getMessage() << endl;
		MessageBox(NULL, str.str().c_str(), NULL, MB_OK );
	}

	#ifndef NDEBUG
	FreeConsole();
	#endif 

	return 0;
}