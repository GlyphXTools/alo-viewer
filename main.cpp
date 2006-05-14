#include <iostream>
#include <algorithm>
#include <sstream>
#include "model.h"
#include "engine.h"
#include "exceptions.h"
using namespace std;

#include "shlobj.h"

#include <commdlg.h>
#include <commctrl.h>
#include <afxres.h>
#include "resource.h"

class TextureManager : public ITextureManager
{
	typedef map<string,IDirect3DTexture9*> TextureMap;

	TextureMap    textures;
	string        basePath;
	IFileManager* fileManager;

public:
	// Invalidate all entries (i.e. clear them)
	void invalidateAll()
	{
		for (TextureMap::iterator i = textures.begin(); i != textures.end(); i++)
		{
			i->second->Release();
		}
		textures.clear();
	}

	IDirect3DTexture9* getTexture(IDirect3DDevice9* pDevice, string name)
	{
		IDirect3DTexture9* pTexture = NULL;
		transform(name.begin(), name.end(), name.begin(), toupper);
		TextureMap::iterator p = textures.find(name);
		if (p != textures.end())
		{
			// Texture has already been loaded
			pTexture = p->second;
		}
		else
		{
			// Texture has not yet been loaded; load it
			string filename = name;
			if (filename.rfind(".TGA") != string::npos)
			{
				filename = filename.substr(0, filename.rfind(".TGA") ) + ".DDS";
			}
			File* file = fileManager->getFile( basePath + filename );
			if (file != NULL)
			{
				unsigned long size = file->getSize();
				char* data = new char[ size ];
				file->read( (void*)data, size );
				if (D3DXCreateTextureFromFileInMemory( pDevice, (void*)data, size, &pTexture ) == D3D_OK)
				{
					textures.insert(make_pair(name, pTexture));
				}
				delete[] data;
			}
		}
		if (pTexture != NULL)
		{
			pTexture->AddRef();
		}
		return pTexture;
	}

	TextureManager(IFileManager* fileManager, const std::string& basePath)
	{
		this->basePath    = basePath;
		this->fileManager = fileManager;
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

public:

	// Invalidate all entries (i.e. clear them)
	void invalidateAll()
	{
		for (EffectMap::iterator i = effects.begin(); i != effects.end(); i++)
		{
			i->second->Release();
		}
		effects.clear();
	}

	ID3DXEffect* getEffect(IDirect3DDevice9* pDevice, std::string name)
	{
		ID3DXEffect* pEffect = NULL;
		transform(name.begin(), name.end(), name.begin(), toupper);
		EffectMap::iterator p = effects.find(name);
		if (p != effects.end())
		{
			// Effect has already been loaded
			pEffect = p->second;
		}
		else
		{
			// Effect has not yet been loaded; load it
			string filename = name;
			if (filename.rfind(".FX") != string::npos)
			{
				filename = filename.substr(0, filename.rfind(".FX") ) + ".FXO";
			}
			
			File* file = fileManager->getFile( basePath + filename );
			if (file != NULL)
			{
				unsigned long size = file->getSize();
				char* data = new char[ size ];
				file->read( (void*)data, size );
				if (D3DXCreateEffect(pDevice, data, size, NULL, NULL, 0, NULL, &pEffect, NULL) == D3D_OK)
				{
					effects.insert(make_pair(name, pEffect));
				}
				delete[] data;
			}

		}
		if (pEffect != NULL)
		{
			pEffect->AddRef();
		}
		return pEffect;
	}

	EffectManager(IFileManager* fileManager, const std::string& basePath)
	{
		this->basePath    = basePath;
		this->fileManager = fileManager;
	}

	~EffectManager()
	{
		invalidateAll();
	}
};

class ModelManager
{
	typedef map<string,Model*> ModelMap;

	ModelMap      models;
	string        basePath;
	IFileManager* fileManager;

public:

	const Model* getModel(std::string name)
	{
		Model* pModel = NULL;
		transform(name.begin(), name.end(), name.begin(), toupper);
		ModelMap::iterator p = models.find(name);
		if (p != models.end())
		{
			// Model has already been loaded
			pModel = p->second;
		}
		else
		{
			// Model has not yet been loaded; load it
			File* file = new PhysicalFile( name );
			pModel = NULL;
			try
			{
				pModel = new Model(file);
			}
			catch (...) {}
			file->release();

			if (pModel == NULL)
			{
				file = new PhysicalFile( basePath + name );
				try
				{
					pModel = new Model(file);
				}
				catch (...) {}
				file->release();
			}

			if (pModel != NULL)
			{
				models.insert(make_pair(name, pModel));
			}
		}
		return pModel;
	}

	ModelManager(IFileManager* fileManager, const std::string& basePath)
	{
		this->basePath    = basePath;
		this->fileManager = fileManager;
	}

	~ModelManager()
	{
		for (ModelMap::iterator i = models.begin(); i != models.end(); i++)
		{
			delete i->second;
		}
	}
};

// Global info about the applications
struct ApplicationInfo
{
	HINSTANCE hInstance;

	HWND hMainWnd;
	HWND hListBox;
	HWND hRenderWnd;

	Engine*         engine;
	FileManager*    fileManager;
	ModelManager*   modelManager;
	EffectManager*  effectManager;
	TextureManager* textureManager;

	// The current model. This will be NULL when loaded through the ModelManager
	const Model* currentModel;

	// Dragging
	enum { NONE, ROTATE, MOVE } dragmode;
	long           xstart;
	long           ystart;
	Engine::Camera startCam;

	ApplicationInfo()
	{
		hMainWnd     = hListBox = hRenderWnd = NULL;
		dragmode     = NONE;
		currentModel = NULL;
	}

	~ApplicationInfo()
	{
		DestroyWindow( hRenderWnd );
		DestroyWindow( hListBox );
		DestroyWindow( hMainWnd );
	}
};

// Initializes the list box with mesh names when a model has been loaded
static void onModelLoaded( ApplicationInfo* info, const Model* model )
{
	info->engine->clearMeshes();
	for (unsigned int i = 0; i < model->getNumMeshes(); i++)
	{
		D3DXMATRIX transformation;
		model->getTransformation(i, transformation );
		info->engine->addMesh( model->getMesh(i), transformation );
	}

	if (info->currentModel != NULL)
	{
		delete info->currentModel;
		info->currentModel = NULL;
	}

	// Clear the list box
	while (SendMessage(info->hListBox, LB_DELETESTRING, 0, 0) > 0);

	for (unsigned int i = 0; i < model->getNumMeshes(); i++)
	{
		string name = model->getMesh(i)->getName();
		if (name == "")
		{
			name = "<unnamed>";
		}
		SendMessage(info->hListBox, LB_ADDSTRING, 0, (LPARAM)name.c_str() );
	}

	// Enable model menu
	HMENU hMenu = GetMenu(info->hMainWnd);
	EnableMenuItem(hMenu, 1, MF_BYPOSITION | MF_ENABLED);
	DrawMenuBar(info->hMainWnd);

	InvalidateRect(info->hRenderWnd, NULL, FALSE );
}

// Dialog box window procedure for "Select file" dialog
INT_PTR CALLBACK SelectFileDlgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_INITDIALOG:
		{
			HWND hList = GetDlgItem(hWnd, IDC_LIST1);
			MegaFile* file = (MegaFile*)lParam;
			for (unsigned int i = 0; i < file->getNumFiles(); i++)
			{
				string name = file->getFilename(i);
				if (name != "")
				{
					size_t p = name.find_last_of(".");
					if (p != string::npos)
					{
						string ext = name.substr(p + 1);
						transform(ext.begin(), ext.end(), ext.begin(), toupper);
						if (ext == "ALO")
						{
							int index = (int)SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)name.c_str() );
							SendMessage(hList, LB_SETITEMDATA, index, i );
						}
					}
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

// Do the whole "Open File" dialog and load and set
// the model accordingly.
static void menuOpenFile( ApplicationInfo *info )
{
	char filename[MAX_PATH];
	filename[0] = '\0';

	OPENFILENAME ofn;
	memset(&ofn, 0, sizeof(OPENFILENAME));
	ofn.lStructSize  = sizeof(OPENFILENAME);
	ofn.hwndOwner    = info->hMainWnd;
	ofn.hInstance    = info->hInstance;
	ofn.lpstrFilter  = "Alamo Files (*.alo, *.meg)\0*.ALO; *.MEG\0All Files (*.*)\0*.*\0\0";
	ofn.nFilterIndex = 0;
	ofn.lpstrFile    = filename;
	ofn.nMaxFile     = MAX_PATH;
	ofn.Flags        = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
	if (GetOpenFileName( &ofn ) != 0)
	{
		const Model* model = NULL;

		string ext = &filename[ofn.nFileExtension];
		transform(ext.begin(), ext.end(), ext.begin(), toupper);
		if (ext == "MEG")
		{
			// Load it through a MegaFile
			File* file = new PhysicalFile(filename);
			MegaFile megafile( file );
			file->release();
			int index = (int)DialogBoxParam( info->hInstance, MAKEINTRESOURCE(IDD_DIALOG2), info->hMainWnd, SelectFileDlgProc, (LPARAM)&megafile );
			if (index >= 0)
			{
				// We selected a subfile, try to load it
				file = megafile.getFile(index);
				try
				{
					model = new Model( file );
					onModelLoaded( info, model );
					info->currentModel = model;
				}
				catch (...)
				{
					MessageBox(NULL, "Unable to open the specified model", NULL, MB_OK | MB_ICONHAND );
				}
				file->release();
			}
		}
		else
		{
			// Load the model file directly
			model = info->modelManager->getModel( filename );
			if (model == NULL)
			{
				MessageBox(NULL, "Unable to open the specified model", NULL, MB_OK | MB_ICONHAND );
			}
			else
			{
				onModelLoaded( info, model );
				info->currentModel = model;
			}
		}

	}
}

LRESULT CALLBACK MainWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	ApplicationInfo* info = (ApplicationInfo*)(LONG_PTR)GetWindowLongPtr(hWnd, GWL_USERDATA);

	switch (uMsg)
	{
		case WM_COMMAND:
			if (info != NULL)
			{
				// Menu and control notifications
				WORD code = HIWORD(wParam);
				WORD id   = LOWORD(wParam);
				if (code == 0 || (code == 1 && lParam == 0))
				{
					// Menu or accelerator
					switch (id)
					{
						case ID_FILE_OPEN:
							menuOpenFile( info );
							break;

						case ID_FILE_EXIT:
							PostQuitMessage(0);
							break;

						case ID_HELP_ABOUT:
							MessageBox(NULL, "Alamo Object Viewer\nBy Mike Lankamp", "About", MB_OK );
							break;
					}
				}
				else if (code == LBN_SELCHANGE)
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
			}
			break;

		case WM_SIZE:
			if (info != NULL)
			{
				RECT client;
				GetClientRect(info->hMainWnd, &client);
				MoveWindow( info->hListBox, 0, 0, 200, client.bottom, TRUE );
				MoveWindow( info->hRenderWnd, 200, 0, client.right - 200, client.bottom, TRUE );

				// When we resize, we need to kill the device and recreate it
				info->textureManager->invalidateAll();
				info->effectManager->invalidateAll();
				info->engine->reinitialize( info->hRenderWnd, client.right - 200, client.bottom );
			}
			break;

		case WM_CLOSE:
			PostQuitMessage(0);
			break;
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

// Window procedure for the render window (canvas)
LRESULT CALLBACK RenderWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
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
			info->dragmode = (uMsg == WM_LBUTTONDOWN) ? ApplicationInfo::MOVE : ApplicationInfo::ROTATE;
			SetCapture(hWnd);
			SetFocus(hWnd);
			break;

		case WM_MOUSEMOVE:
			if (info->dragmode != ApplicationInfo::NONE)
			{
				// Yay, math time!
				long x = (short)LOWORD(lParam) - info->xstart;
				long y = (short)HIWORD(lParam) - info->ystart;

				Engine::Camera camera = info->startCam;
				D3DXVECTOR3    orthVec, diff = info->startCam.Position - info->startCam.Target;
				
				// Get the orthogonal vector
				D3DXVec3Cross( &orthVec, &diff, &camera.Up );
				D3DXVec3Normalize( &orthVec, &orthVec );

				if (info->dragmode == ApplicationInfo::ROTATE)
				{
					// Lets rotate
					D3DXMATRIX rotateXY, rotateZ, rotate;
					D3DXMatrixRotationZ( &rotateZ, D3DXToRadian(x / 2.0f) );
					D3DXMatrixRotationAxis( &rotateXY, &orthVec, D3DXToRadian(y / 2.0f) );
					D3DXMatrixMultiply( &rotate, &rotateXY, &rotateZ );
					D3DXVec3TransformCoord( &camera.Position, &diff, &rotate );
					camera.Position += camera.Target;
				}
				else
				{
					// Lets translate
					D3DXVECTOR3 Up;
					D3DXVec3Cross( &Up, &orthVec, &diff );
					D3DXVec3Normalize( &Up, &Up );
					
					// The distance we move depends on the distance from the object
					// Large distance: move a lot, small distance: move a little
					float multiplier = D3DXVec3Length( &diff ) / 1000;

					camera.Target  -= (float)x * multiplier * orthVec;
					camera.Target  += (float)y * multiplier * Up;
					camera.Position = diff + camera.Target;
				}

				info->engine->setCamera( camera );
				InvalidateRect(hWnd, NULL, FALSE);
			}
			break;

		case WM_MOUSEWHEEL:
			if (info->dragmode == ApplicationInfo::NONE)
			{
				Engine::Camera camera = info->engine->getCamera();

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
			// Render!
			if (!info->engine->render())
			{
				// Device was lost and needs to be reset
				RECT client;
				GetClientRect(info->hMainWnd, &client);
				info->textureManager->invalidateAll();
				info->effectManager->invalidateAll();
				info->engine->reinitialize( info->hRenderWnd, client.right - 200, client.bottom );
				info->engine->render();
			}
			break;

		case WM_SETFOCUS:
			// Yes, we want focus please
			return 0;
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

// Returns the install path by querying the registry, or an empty string when failed.
std::string getGamePath_Reg()
{
	HKEY hKey;
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Software\\LucasArts\\Star Wars Empire at War\\1.0", 0, KEY_QUERY_VALUE, &hKey ) == ERROR_SUCCESS)
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
			return str;
		}
	}
	return "";
}

// Returns %PROGRAMFILES%\LucasArts\Star Wars Empire at War\GameData, or an empty string when failed.
std::string getGamePath_Shell()
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

void main( ApplicationInfo* info, const vector<string>& argv )
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
			bi.hwndOwner      = info->hMainWnd;
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

	if (fileManager == NULL)
	{
		return;
	}

	try
	{
		// Initialize the other managers and engine
		ModelManager   modelManager  (fileManager, "Data\\Art\\Models\\");
		TextureManager textureManager(fileManager, "Data\\Art\\Textures\\");
		EffectManager  effectManager (fileManager, "Data\\Art\\Shaders\\");
		Engine         engine(info->hRenderWnd, &textureManager, &effectManager);

		info->engine         = &engine;
		info->fileManager    = fileManager;
		info->modelManager   = &modelManager;
		info->effectManager  = &effectManager;
		info->textureManager = &textureManager;

		// Message loop
		ShowWindow(info->hMainWnd, SW_SHOW);
		HACCEL hAccel = LoadAccelerators( info->hInstance, MAKEINTRESOURCE(IDR_ACCELERATOR1));
		MSG msg;
		while (GetMessage(&msg, NULL, 0, 0) != 0)
		{
			if (!TranslateAccelerator(info->hMainWnd, hAccel, &msg))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
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

// Create the main window and its listbox and canvas
static void CreateMainWindow( ApplicationInfo* info )
{
	WNDCLASSEX wcx;
	wcx.cbSize        = sizeof wcx;
    wcx.style		  = CS_HREDRAW | CS_VREDRAW;
    wcx.lpfnWndProc	  = MainWindowProc;
    wcx.cbClsExtra	  = 0;
    wcx.cbWndExtra    = 0;
    wcx.hInstance     = info->hInstance;
    wcx.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    wcx.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wcx.hbrBackground = NULL;
    wcx.lpszMenuName  = MAKEINTRESOURCE(IDR_MENU1);
    wcx.lpszClassName = "AloViewer";
    wcx.hIconSm       = NULL;

	if (RegisterClassEx(&wcx) == 0)
	{
		throw Exception("Unable to register window class");
	}

    wcx.lpfnWndProc	  = RenderWindowProc;
    wcx.lpszClassName = "AloRenderWindow";
	if (RegisterClassEx(&wcx) == 0)
	{
		throw Exception("Unable to register window class");
	}

	info->hMainWnd = CreateWindowEx(0, "AloViewer", "Alamo Object Viewer", WS_OVERLAPPEDWINDOW, 
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, info->hInstance, NULL);
	if (info->hMainWnd == NULL)
	{
		throw Exception("Unable to create main window");
	}

	RECT client;
	GetClientRect(info->hMainWnd, &client);

	info->hListBox = CreateWindowEx(WS_EX_CLIENTEDGE, "LISTBOX", "", WS_CHILD | WS_VISIBLE | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT | LBS_MULTIPLESEL, 0, 0, 200, client.bottom, info->hMainWnd, NULL, info->hInstance, NULL);
	if (info->hListBox == NULL)
	{
		DestroyWindow(info->hMainWnd);
		throw Exception("Unable to create main window");
	}
	SendMessage(info->hListBox, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), FALSE);

	info->hRenderWnd = CreateWindowEx(WS_EX_CLIENTEDGE, "AloRenderWindow", "", WS_CHILD | WS_VISIBLE, 200, 0, client.right - 200, client.bottom, info->hMainWnd, NULL, info->hInstance, NULL);
	if (info->hRenderWnd == NULL)
	{
		DestroyWindow(info->hListBox);
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
	try
	{
		CoInitializeEx(NULL, COINIT_APARTMENTTHREADED );
		ApplicationInfo info;
		info.hInstance = hInstance;
		CreateMainWindow( &info );
		main( &info, parseCommandLine() );
	}
	catch (Exception& e)
	{
		stringstream str;
		str << "Caught exception from " + e.getFilename() << ":" << e.getLine() << endl << endl << e.getMessage() << endl;
		MessageBox(NULL, str.str().c_str(), NULL, MB_OK );
	}

	return 0;
}