#include <stdexcept>
#include <vector>
#include <sstream>
#include "UI/UI.h"
#include "RenderEngine/DirectX9/RenderEngine.h"
#include "RenderEngine/LightSources.h"
#include "Sound/DirectSound8/SoundEngine.h"
#include "Sound/AnimationSFXMaps.h"
#include "Effects/SurfaceFX.h"
#include "General/Log.h"
#include "General/GameTime.h"
#include "General/WinUtils.h"
#include "General/XML.h"
#include "Dialogs/Dialogs.h"
#include "RenderWindow.h"
#include "Console.h"
#include <afxres.h>
#include "config.h"
#include <shlwapi.h>
#include <commctrl.h>
using namespace std;
using namespace Alamo;
using namespace Dialogs;

// Indices of items in the menu. Until the compiler properly supports C99,
// don't use designators
static const int INDEX_MENU_VIEW    = 2;
static const int INDEX_MENU_GAME    = 3;
static const int INDEX_MENU_GAMES[] = {
    /*[GID_EAW]     =*/ 0,
    /*[GID_EAW_FOC] =*/ 1,
    /*[GID_UAW_EA]  =*/ 2,
};
static const int NUM_SUPPORTED_GAMES = 3;

static const int INDEX_MENU_MODS_BASE[] = {
    /*[GID_EAW]     =*/ ID_EAW_UNMODDED,
    /*[GID_EAW_FOC] =*/ ID_EAW_FOC_UNMODDED,
    /*[GID_UAW_EA]  =*/ ID_UAW_EA_BETA_UNMODDED,
};

struct GameModInfo
{
    UINT menuitem;
};

typedef map<GameMod,GameModInfo>              GameModList;
typedef map<UINT,GameModList::const_iterator> GameModLookup;

struct ApplicationInfo : public Dialogs::ISelectionCallback
{
    HINSTANCE hInstance;

    HWND hMainWnd;
    HWND hSettings;
    HWND hSelection;
    HWND hRenderWnd;
    HWND hConsoleWnd;
    HWND hTimeSlider;
    HWND hPlayButton;
    HWND hLoopButton;
    HWND hBackgroundLabel; 
    HWND hBackgroundBtn;
    HWND hModelColorBtn;
    HWND hColorsLabel;
    HWND hColorBtn[Config::NUM_PREDEFINED_COLORS];
    bool isMinimized;

    // Games/Mods
    GameModList                 gameMods;
    GameModLookup               gameModLookup;
    GameModList::const_iterator activeGameMod;
    
    // Animation control
    void OnAnimationSelected(ptr<Animation> animation, const wstring& filename, bool loop);

    bool         playing;
    bool         playLoop;
    float        playStartTime;
    unsigned int playStartFrame;
    const AnimationSFXMaps::SFXMap*          m_sfxmap;
    AnimationSFXMaps::SFXMap::const_iterator m_sfxevent;
    
    IRenderEngine*     engine;
    ISoundEngine*      sound;
    ptr<Model>         model;
    ptr<Animation>     animation;
    string             animationName;
    ptr<IRenderObject> object;
    unsigned int       selectedColor;
    vector<COLORREF>   teamColors;
    void LoadTeamColors();

    // File history
    map<ULONGLONG, wstring> history;

    // Current directory at startup
    wstring startupDirectory;

    ApplicationInfo(HINSTANCE hInstance);
    ~ApplicationInfo();
};

static Color ColorRefToColor(COLORREF color)
{
    return Color(
        GetRValue(color) / 255.0f,
        GetGValue(color) / 255.0f,
        GetBValue(color) / 255.0f,
        1.0f);
}

// Adds the Alo Viewer history to the file menu
static bool AppendHistory(HWND hWnd, ApplicationInfo* info)
{
	// Get the history (timestamp, filename pairs)
    Config::GetHistory(info->history);

    HMENU hMenu = GetSubMenu(GetMenu(hWnd), 0);

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
        for (map<ULONGLONG,wstring>::const_reverse_iterator p = info->history.rbegin(); p != info->history.rend() && j < Config::NUM_HISTORY_ITEMS; p++, i++, j++)
		{
			wstring name = p->second.c_str();

			HDC hDC = GetDC(info->hMainWnd);
			SelectObject(hDC, GetStockObject(DEFAULT_GUI_FONT));
			PathCompactPath(hDC, (LPTSTR)name.c_str(), 400);
			ReleaseDC(info->hMainWnd, hDC);

			if (j < 9)
			{
				name = wstring(L"&") + (wchar_t)(L'1' + j) + L" " + name;
			}
			InsertMenu(hMenu, i, MF_BYPOSITION | MF_STRING, ID_FILE_HISTORY_0 + j, name.c_str());
		}

		// Finally, add the seperator
		InsertMenu(hMenu, i, MF_BYPOSITION | MF_SEPARATOR, 0, 0);
	}
	return true;
}

static void InitializeAssets(ApplicationInfo* info)
{
    static GameMod initializedGameMod;
    if (info->activeGameMod == info->gameMods.end() || info->activeGameMod->first != initializedGameMod)
    {
        // Determine search paths for assets
        vector<wstring> basepaths;

        basepaths.push_back(info->startupDirectory);

        if (info->activeGameMod != info->gameMods.end())
        {
            info->activeGameMod->first.AppendAssetDirs(basepaths);
        }
        
        DWORD start = GetTickCount();
        Assets::Initialize(basepaths);

        if (info->activeGameMod != info->gameMods.end())
        {
            initializedGameMod = info->activeGameMod->first;
        }
    }
}

static void SetActiveGameMod(ApplicationInfo* info, GameModList::const_iterator p)
{
    HMENU hMenu  = GetMenu(info->hMainWnd);
    HMENU hGames = GetSubMenu(hMenu, INDEX_MENU_GAME);
    if (info->activeGameMod != info->gameMods.end())
    {
        // Deselect previously active gamemod
        CheckMenuItem(hGames, INDEX_MENU_GAMES[info->activeGameMod->first.m_game], MF_ENABLED | MF_BYPOSITION);
        CheckMenuItem(hGames, info->activeGameMod->second.menuitem, MF_ENABLED | MF_BYCOMMAND);
    }

    // Select new active gamemod
    info->activeGameMod = p;
    GameID game = GID_UNKNOWN;
    if (info->activeGameMod != info->gameMods.end())
    {
        CheckMenuItem(hGames, INDEX_MENU_GAMES[info->activeGameMod->first.m_game], MF_CHECKED | MF_BYPOSITION);
        CheckMenuItem(hGames, info->activeGameMod->second.menuitem, MF_CHECKED | MF_BYCOMMAND);
        game = info->activeGameMod->first.m_game;
    }

    //
    // Switch the engine
    //

    // Uninitialize asset-dependent subsystems
    SFXEvents::Uninitialize();
    AnimationSFXMaps::Uninitialize();
    SurfaceFX::Uninitialize();
    
    // Clean up the old object
    if (info->object != NULL)
    {
        info->object->Destroy();
        SAFE_RELEASE(info->object);
    }
    if (info->sound != NULL)
    {
        info->sound->Clear();
    }
    info->m_sfxmap = NULL;

    // (Re)initialize assets
    InitializeAssets(info);

    // Switch render engine
    SetRenderWindowGameEngine(info->hRenderWnd, game);
    info->engine           = GetRenderEngine(info->hRenderWnd);
    info->LoadTeamColors();
    for (int i = 0; i < Config::NUM_PREDEFINED_COLORS; i++)
    {
        RedrawWindow(info->hColorBtn[i], NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
    }

    if (info->sound != NULL)
    {
        // Initialize SFX events
        SFXEvents::Initialize();
        AnimationSFXMaps::Initialize();
        SurfaceFX::Initialize();
    }
    LightSources::Initialize();

    if (info->engine != NULL && info->model != NULL)
    {
        // Create the new object
        ptr<IObjectTemplate> templ = info->engine->CreateObjectTemplate(info->model);

        // Assign to info->object at the end to prevent callbacks from
        // acting upon the UI state change.
        ptr<IRenderObject> object = info->engine->CreateRenderObject(templ, 0, 9);

        if (info->selectedColor < info->teamColors.size())
        {
            object->SetColorization(ColorRefToColor(info->teamColors[info->selectedColor]));
        }
        else if (info->selectedColor == Config::NUM_PREDEFINED_COLORS) {
            object->SetColorization(ColorButton_GetColor(info->hModelColorBtn));
        }

        Dialogs::Selection_ResetObject(info->hSelection, object);
        info->object = object;
        if (info->animation != NULL)
        {
            // Apply animation again
            object->SetAnimation(info->animation);

            // Reload SFX map
            info->m_sfxmap = AnimationSFXMaps::GetEvents(info->animationName);
            if (info->m_sfxmap != NULL)
            {
                // Set iterator to current position
                unsigned int curFrame = (unsigned int)SendMessage(info->hTimeSlider, TBM_GETPOS, 0, 0);
                float        time     = curFrame / info->animation->GetFPS();

                info->m_sfxevent = info->m_sfxmap->begin();
                while (info->m_sfxevent != info->m_sfxmap->end() && info->m_sfxevent->first <= time)
                {
                    info->m_sfxevent++;
                }
            }
        }
    }
    else
    {
        Dialogs::Selection_SetObject(info->hSelection, NULL, NULL, L"");
    }

    // Set the Shader LOD popup menu depending on the game
    UINT id = IDR_SHADERLOD_MENU_UAW_EA;
    switch (game)
    {
        case GID_EAW:       
        case GID_EAW_FOC:
            id = IDR_SHADERLOD_MENU_EAW;
            break;
    }

    MENUITEMINFO mii = { sizeof(MENUITEMINFO) };
    mii.fMask    = MIIM_SUBMENU;
    mii.hSubMenu = GetSubMenu(LoadMenu(info->hInstance, MAKEINTRESOURCE(id)), 0);
    HMENU hViewMenu = GetSubMenu(GetMenu(info->hMainWnd), INDEX_MENU_VIEW);
    SetMenuItemInfo(hViewMenu, ID_VIEW_SHADERLOD, FALSE, &mii);
}

static void SetActiveGameMod(ApplicationInfo* info, GameMod mod)
{
    GameModList::const_iterator p = info->gameMods.find(mod);
    if (p != info->gameMods.end())
    {
        SetActiveGameMod(info, p);
    }
}

static bool SetActiveGameMod(ApplicationInfo* info, UINT id)
{
    GameModLookup::const_iterator p = info->gameModLookup.find(id);
    if (p != info->gameModLookup.end())
    {
        SetActiveGameMod(info, p->second);
        
        // User selected a different gamemod, set as default
        Config::SetDefaultGameMod(p->second->first);
        return true;
    }
    return false;
}

static void OnModelLoaded(ApplicationInfo* info, ptr<Model> model, ptr<MegaFile> megaFile, const wstring& filename)
{
    // Clean up the old object
    Dialogs::CloseDetailsDialog();
    Dialogs::Selection_SetObject(info->hSelection, NULL, NULL, L"");
    if (info->object != NULL)
    {
        info->object->Destroy();
        SAFE_RELEASE(info->object);
    }
    if (info->sound != NULL)
    {
        info->sound->Clear();
    }
    info->m_sfxmap = NULL;
    SAFE_RELEASE(info->model);
    SAFE_RELEASE(info->animation);
    info->engine->ClearTextureCache();
    ResetGameTime();

    info->model = model;
    if (model != NULL)
    {
        if (info->engine != NULL)
        {
            // Create the new object
            ptr<IObjectTemplate> templ = info->engine->CreateObjectTemplate(model);

            // Assign to info->object at the end to prevent callbacks from
            // acting upon the UI state change.
            ptr<IRenderObject> object = info->engine->CreateRenderObject(templ, 0, 9);
        
            if (info->selectedColor < info->teamColors.size())
            {
                object->SetColorization(ColorRefToColor(info->teamColors[info->selectedColor]));
            }
            else if (info->selectedColor == Config::NUM_PREDEFINED_COLORS) {
                object->SetColorization(ColorButton_GetColor(info->hModelColorBtn));
            }

            Dialogs::Selection_SetObject(info->hSelection, object, megaFile, filename);

            info->object = object;
        }
        
        // Set window title
        wstringstream ss;
        ss << Config::WINDOW_TITLE << L" - [" << filename << "]";
        wstring title = ss.str();
        SetWindowText(info->hMainWnd, title.c_str());
    }

    // Set UI state
    EnableMenuItem(GetMenu(info->hMainWnd), ID_FILE_DETAILS, MF_BYCOMMAND | (model != NULL) ? MF_ENABLED : MF_GRAYED );
    EnableMenuItem(GetMenu(info->hMainWnd), ID_FILE_OPENANIMATION, MF_BYCOMMAND | (model != NULL) ? MF_ENABLED : MF_GRAYED );
    EnableMenuItem(GetMenu(info->hMainWnd), ID_FILE_OPENANIMATIONOVERRIDE, MF_BYCOMMAND | (model != NULL) ? MF_ENABLED : MF_GRAYED );

	// Reset animation controls
	SendMessage(info->hTimeSlider, TBM_SETPOS,   TRUE, 0);
	SendMessage(info->hTimeSlider, TBM_SETRANGE, TRUE, MAKELONG(0,1));
	EnableWindow(info->hTimeSlider, FALSE);
	EnableWindow(info->hPlayButton, FALSE);
    EnableWindow(info->hLoopButton, FALSE);
    
	SendMessage(info->hPlayButton, BM_SETIMAGE, IMAGE_ICON, (LPARAM)LoadIcon(info->hInstance, MAKEINTRESOURCE(IDI_PLAY)));
    SendMessage(info->hLoopButton, BM_SETCHECK, BST_UNCHECKED, 0);  
	info->playing  = false;
    info->playLoop = false;
}

void ApplicationInfo::OnAnimationSelected(ptr<Animation> anim, const wstring& _filename, bool loop)
{
    if (anim != animation)
    {
        // Animation changed, release previous one and set new one
        SAFE_RELEASE(animation);
        if (sound != NULL)
        {
            sound->Clear();
        }
        m_sfxmap = NULL;

        if (object != NULL)
        {
            object->SetAnimation(anim);
            animation = anim;
        }

        if (animation != NULL)
        {
		    // Reset animation controls
		    unsigned int numFrames = animation->GetNumFrames();
		    SendMessage(hTimeSlider, TBM_SETRANGE, TRUE, MAKELONG(0, numFrames - 1));
		    SendMessage(hTimeSlider, TBM_SETPOS,   TRUE, 0);
		    EnableWindow(hTimeSlider, TRUE);
		    EnableWindow(hPlayButton, TRUE);
            EnableWindow(hLoopButton, TRUE);
            
            playing = false;

            //
            // Initialize animation SFX events
            //

            // Get filename without extension
            wstring filename = _filename;
            wstring::size_type ofs = filename.find_last_of(L"/\\");
            if (ofs != wstring::npos) filename = filename.substr(ofs + 1);
            ofs = filename.find_last_of(L".");
            if (ofs != wstring::npos) filename = filename.substr(0, ofs);

            // Get events
            animationName = WideToAnsi(filename);
            m_sfxmap = AnimationSFXMaps::GetEvents(animationName);
        }
        else
        {
		    EnableWindow(hTimeSlider, FALSE);
		    EnableWindow(hPlayButton, FALSE);
            EnableWindow(hLoopButton, FALSE);
        }
    }

    if (animation != NULL)
    {
        if (!playing || !loop)
        {
            // Reset animation
            UpdateGameTime();
            playStartTime  = GetGameTime();
	        playStartFrame = 0;
            playing  = true;
		    SendMessage(hTimeSlider, TBM_SETPOS, TRUE, 0);

            if (object != NULL)
            {
                object->ResetAnimation();
            }

            if (m_sfxmap != NULL) m_sfxevent = m_sfxmap->begin();
            if (sound    != NULL) sound->Play();
        }
        playLoop = loop;
        SendMessage(hPlayButton, BM_SETIMAGE, IMAGE_ICON, (LPARAM)LoadIcon(hInstance, MAKEINTRESOURCE(IDI_PAUSE)));
        SendMessage(hLoopButton, BM_SETCHECK, playLoop ? BST_CHECKED : BST_UNCHECKED, 0);  
    }
}

// Generic file open routine
static void LoadFile(ApplicationInfo* info, wstring filename)
{
	const TCHAR* ext = PathFindExtension(filename.c_str());
	if (*ext != '\0')
	{
        bool model = (_wcsicmp(ext,L".alo") == 0);
        bool valid = (_wcsicmp(ext,L".ala") == 0 || model);

        if (valid)
        {
#ifdef NDEBUG
            // In debug mode the IDE should catch this
	        try
#endif
	        {
                // Open the file
                ptr<IFile>    file;
                ptr<MegaFile> meg;
	            size_t pos = filename.find(L"|");
	            if (pos != string::npos)
	            {
		            // It's an MEG:Filename pair
		            file = new PhysicalFile(filename.substr(0, pos));
		            meg  = new MegaFile(file);

                    file = meg->GetFile(WideToAnsi(filename.substr(pos + 1)));
	            }
                else
                {
                    // It's a regular file
                    file = new PhysicalFile(filename);
                }

                if (model)
                {
                    // Check which game/mod basedir the file is in
                    for (GameModList::const_reverse_iterator p = info->gameMods.rbegin(); p != info->gameMods.rend(); p++)
                    {
                        wstring path = p->first.GetBaseDir();
                        if (!path.empty() && filename.find(path) != wstring::npos)
                        {
                            // This one, set as active
                            SetActiveGameMod(info, p->first);
                            break;
                        }
                    }

		            ptr<Model> model = new Model(file);
		            OnModelLoaded(info, model, meg, filename);
                }
                else if (info->model != NULL)
                {
                    ptr<ANIMATION_INFO> pai = new ANIMATION_INFO;
                    pai->SetAnimation(new Animation(file, *info->model), file);
		            info->OnAnimationSelected(pai->animation, filename, false);
                    Dialogs::AddToAnimationList(info->hSelection, pai, AnimationType::ADDITIONAL_ANIM);
                }

                // Add it to the history
                Config::AddToHistory(filename);
                AppendHistory(info->hMainWnd, info);
            }
#ifdef NDEBUG
            catch (...)
            {
                valid = false;
            }
#endif
        }

        if (!valid)
        {
            wstring error = LoadString(IDS_ERR_UNABLE_TO_OPEN_FILE);
	    	MessageBox(NULL, error.c_str(), NULL, MB_OK | MB_ICONHAND );
        }
	}
}

static void OpenHistoryFile(ApplicationInfo* info, int idx)
{
	// Find the correct entry
	map<ULONGLONG, wstring>::const_reverse_iterator p = info->history.rbegin();
	for (int j = 0; p != info->history.rend() && idx > 0; idx--, p++);
	if (p != info->history.rend())
	{
		LoadFile(info, p->second);
	}
}

static void SetShaderLOD(ApplicationInfo* info, ShaderDetail detail)
{
    if (info->engine != NULL)
    {
        RenderSettings settings = info->engine->GetSettings();
        settings.m_shaderDetail = detail;
        info->engine->SetSettings(settings);
    }
}

static void ToggleRenderSetting(ApplicationInfo* info, bool RenderSettings::*setting )
{
    if (info->engine != NULL)
    {
        RenderSettings settings = info->engine->GetSettings();
        settings.*setting = !(settings.*setting);
        info->engine->SetSettings(settings);
    }
}

static void InitializeGameMenu(ApplicationInfo* info)
{
    // Enable game menu items
    HMENU hGames = GetSubMenu(GetMenu(info->hMainWnd), INDEX_MENU_GAME);
    
    map<GameID, pair<UINT, bool> > menuitems;
    for (GameModList::iterator p = info->gameMods.begin(); p != info->gameMods.end(); p++)
    {
        UINT  id    = INDEX_MENU_GAMES[p->first.m_game];
        HMENU hGame = GetSubMenu(hGames, id);

        pair<map<GameID, pair<UINT, bool> >::iterator, bool> q = menuitems.insert(make_pair(p->first.m_game, make_pair(INDEX_MENU_MODS_BASE[p->first.m_game], false)));
        if (q.second)
        {
            // First time we see this game; add Unmodded lookup
            GameModList::iterator m = info->gameMods.find(GameMod(p->first.m_game));
            id = q.first->second.first++;
            info->gameModLookup[id] = m;
            m->second.menuitem = id;
        }
        
        static bool listedAllSteamMods = true;
        if (!p->first.m_mod.empty())
        {
            // It's a mod
            if (!q.first->second.second)
            {
                // It's the first mod for this game, add separator
                AppendMenu(hGame, MF_SEPARATOR, 0, NULL);
                q.first->second.second = true;
                if (!p->first.m_steamId.empty()) {
                    listedAllSteamMods = false;
                }
            }

            if (!listedAllSteamMods && p->first.m_steamId.empty()) {
                AppendMenu(hGame, MF_SEPARATOR, 0, NULL);
                listedAllSteamMods = true;
            }

            id = q.first->second.first++;
            info->gameModLookup[id] = p;
            p->second.menuitem = id;
            AppendMenu(hGame, MF_ENABLED | MF_STRING, id, p->first.m_mod.c_str());
        }
    }
    
    // Now set the active mod
    SetActiveGameMod(info, info->activeGameMod);
}

static void DoSelectAll(HWND hWnd)
{
    HWND hFocus = GetFocus();
    TCHAR classname[256];
    GetClassName(hFocus, classname, 256);

    if (_wcsicmp(classname, L"Edit") == 0)
    {
        // Select all text
        SendMessage(hFocus, EM_SETSEL, 0, -1);
    }
    else if (_wcsicmp(classname, WC_LISTVIEW) == 0)
    {
        // Select all items
        int count = ListView_GetItemCount(hFocus);
        for (int i = 0; i < count; i++)
        {
            ListView_SetItemState(hFocus, i, LVIS_SELECTED, LVIS_SELECTED );
        }
    }
}

static void DoMenuItem(ApplicationInfo* info, UINT id)
{
	// Menu or accelerator
    if (id >= ID_FILE_HISTORY_0 && id < ID_FILE_HISTORY_0 + min(9,Config::NUM_HISTORY_ITEMS))
	{
		// It's a history item
		OpenHistoryFile(info, id - ID_FILE_HISTORY_0);
	}
    else if (SetActiveGameMod(info, id))
    {
    }
    else switch (id)
	{
		case ID_FILE_OPENMODEL:
		{
            wstring filename;
            ptr<MegaFile> meg;
            ptr<Model> model = Dialogs::ShowOpenModelDialog(info->hMainWnd, info->activeGameMod->first, &filename, meg);
			if (model != NULL)
			{
				OnModelLoaded(info, model, meg, filename);

                // Add it to the history
                Config::AddToHistory(filename);
                AppendHistory(info->hMainWnd, info);
			}
			break;
		}

		case ID_FILE_OPENANIMATION:
			if (info->engine != NULL)
			{
                ptr<ANIMATION_INFO> pai = Dialogs::ShowOpenAnimationDialog(info->hMainWnd, info->activeGameMod->first, info->model);
				if (pai->animation != NULL)
				{
                    info->OnAnimationSelected(pai->animation, pai->filename, false);

                    // Add it to the history
                    Config::AddToHistory(pai->filename);
                    AppendHistory(info->hMainWnd, info);

                    Dialogs::AddToAnimationList(info->hSelection, pai, AnimationType::ADDITIONAL_ANIM);
				}
			}
			break;

		case ID_FILE_OPENANIMATIONOVERRIDE:
			if (info->engine != NULL)
			{
                wstring filename;
                ptr<MegaFile> meg;
                if (Dialogs::ShowOpenAnimationOverrideDialog(info->hMainWnd, info->activeGameMod->first, &filename, meg))
                {
                    Dialogs::LoadModelAnimations(info->hSelection, info->model, meg, filename, AnimationType::OVERRIDE_ANIM);
                }
            }
			break;

		case ID_FILE_EXIT:
			PostQuitMessage(0);
			break;

		case ID_FILE_DETAILS:
            Dialogs::ShowDetailsDialog(info->hMainWnd, *info->model);
			break;

        case ID_EDIT_SELECTALL: DoSelectAll(GetFocus()); break;
        case ID_EDIT_CUT:    SendMessage(GetFocus(), WM_CUT,   0, 0); break;
        case ID_EDIT_COPY:   SendMessage(GetFocus(), WM_COPY,  0, 0); break;
        case ID_EDIT_PASTE:  SendMessage(GetFocus(), WM_PASTE, 0, 0); break;
        case ID_EDIT_DELETE: SendMessage(GetFocus(), WM_CLEAR, 0, 0); break;

        case ID_SHADERLOD_DX9:           SetShaderLOD(info, SHADERDETAIL_DX9); break;
        case ID_SHADERLOD_DX8ATI:        SetShaderLOD(info, SHADERDETAIL_DX8ATI); break;
        case ID_SHADERLOD_DX8:           SetShaderLOD(info, SHADERDETAIL_DX8); break;
        case ID_SHADERLOD_FIXEDFUNCTION: SetShaderLOD(info, SHADERDETAIL_FIXEDFUNCTION); break;
        case ID_VIEW_SOFTSHADOWS:        ToggleRenderSetting(info, &RenderSettings::m_softShadows); break;
        case ID_VIEW_ANTIALIASING:       ToggleRenderSetting(info, &RenderSettings::m_antiAlias); break;
        case ID_VIEW_BLOOM:              ToggleRenderSetting(info, &RenderSettings::m_bloom); break;
        case ID_VIEW_HEATDISTORTIONS:    ToggleRenderSetting(info, &RenderSettings::m_heatDistortion); break;
        case ID_VIEW_HEATDEBUG:          ToggleRenderSetting(info, &RenderSettings::m_heatDebug); break;
        case ID_VIEW_DEBUGSHADOWS:       ToggleRenderSetting(info, &RenderSettings::m_shadowDebug); break;
        case ID_VIEW_TOGGLELOG: {
            ShowConsoleWindow(info->hConsoleWnd, !IsWindowVisible(info->hConsoleWnd));
            break;
        }
        case ID_VIEW_SETCAMERA: {
            if (info->engine != NULL)
            {
                Camera camera = info->engine->GetCamera();
                Dialogs::ShowCameraDialog(info->hMainWnd, camera);
                info->engine->SetCamera(camera);
            }
            break;
        }

		case ID_HELP_ABOUT:
            Dialogs::ShowAboutDialog(info->hMainWnd);
			break;
	}
}

static void DoMenuInit(ApplicationInfo* info, HMENU hMenu)
{
    if (info->engine != NULL)
    {
        const RenderSettings& settings = info->engine->GetSettings();
        CheckMenuItem(hMenu, ID_SHADERLOD_DX9,            MF_BYCOMMAND | (settings.m_shaderDetail == SHADERDETAIL_DX9           ? MF_CHECKED : MF_UNCHECKED));
        CheckMenuItem(hMenu, ID_SHADERLOD_DX8ATI,         MF_BYCOMMAND | (settings.m_shaderDetail == SHADERDETAIL_DX8ATI        ? MF_CHECKED : MF_UNCHECKED));
        CheckMenuItem(hMenu, ID_SHADERLOD_DX8   ,         MF_BYCOMMAND | (settings.m_shaderDetail == SHADERDETAIL_DX8           ? MF_CHECKED : MF_UNCHECKED));
        CheckMenuItem(hMenu, ID_SHADERLOD_FIXEDFUNCTION,  MF_BYCOMMAND | (settings.m_shaderDetail == SHADERDETAIL_FIXEDFUNCTION ? MF_CHECKED : MF_UNCHECKED));
        CheckMenuItem(hMenu, ID_VIEW_SOFTSHADOWS,     MF_BYCOMMAND | (settings.m_softShadows    ? MF_CHECKED : MF_UNCHECKED));
        CheckMenuItem(hMenu, ID_VIEW_BLOOM,           MF_BYCOMMAND | (settings.m_bloom          ? MF_CHECKED : MF_UNCHECKED));
        CheckMenuItem(hMenu, ID_VIEW_HEATDISTORTIONS, MF_BYCOMMAND | (settings.m_heatDistortion ? MF_CHECKED : MF_UNCHECKED));
        CheckMenuItem(hMenu, ID_VIEW_HEATDEBUG,       MF_BYCOMMAND | (settings.m_heatDebug      ? MF_CHECKED : MF_UNCHECKED));
        CheckMenuItem(hMenu, ID_VIEW_DEBUGSHADOWS,    MF_BYCOMMAND | (settings.m_shadowDebug    ? MF_CHECKED : MF_UNCHECKED));
        CheckMenuItem(hMenu, ID_VIEW_ANTIALIASING,    MF_BYCOMMAND | (settings.m_antiAlias      ? MF_CHECKED : MF_UNCHECKED));
    }
    else
    {
        EnableMenuItem(hMenu, ID_SHADERLOD_DX9,            MF_BYCOMMAND | MF_GRAYED);
        EnableMenuItem(hMenu, ID_SHADERLOD_DX8ATI,         MF_BYCOMMAND | MF_GRAYED);
        EnableMenuItem(hMenu, ID_SHADERLOD_DX8   ,         MF_BYCOMMAND | MF_GRAYED);
        EnableMenuItem(hMenu, ID_SHADERLOD_FIXEDFUNCTION,  MF_BYCOMMAND | MF_GRAYED);
        EnableMenuItem(hMenu, ID_VIEW_SOFTSHADOWS,     MF_BYCOMMAND | MF_GRAYED);
        EnableMenuItem(hMenu, ID_VIEW_BLOOM,           MF_BYCOMMAND | MF_GRAYED);
        EnableMenuItem(hMenu, ID_VIEW_HEATDISTORTIONS, MF_BYCOMMAND | MF_GRAYED);
        EnableMenuItem(hMenu, ID_VIEW_HEATDEBUG,       MF_BYCOMMAND | MF_GRAYED);
        EnableMenuItem(hMenu, ID_VIEW_DEBUGSHADOWS,    MF_BYCOMMAND | MF_GRAYED);
        EnableMenuItem(hMenu, ID_VIEW_ANTIALIASING,    MF_BYCOMMAND | MF_GRAYED);
    }

    HWND hFocus = GetFocus();
    TCHAR classname[256];
    GetClassName(hFocus, classname, 256);

    DWORD enabled = (_wcsicmp(classname, L"Edit") == 0) ? MF_ENABLED : MF_GRAYED;
    EnableMenuItem(hMenu, ID_EDIT_CUT,    MF_BYCOMMAND | enabled);
    EnableMenuItem(hMenu, ID_EDIT_COPY,   MF_BYCOMMAND | enabled);
    EnableMenuItem(hMenu, ID_EDIT_PASTE,  MF_BYCOMMAND | enabled);
    EnableMenuItem(hMenu, ID_EDIT_DELETE, MF_BYCOMMAND | enabled);

    enabled = (_wcsicmp(classname, L"Edit")     == 0 ||
               _wcsicmp(classname, WC_LISTVIEW) == 0) ? MF_ENABLED : MF_GRAYED;
    EnableMenuItem(hMenu, ID_EDIT_SELECTALL, MF_BYCOMMAND | enabled);
    
    enabled = (info->engine != NULL) ? MF_ENABLED : MF_GRAYED;
    EnableMenuItem(hMenu, ID_VIEW_SETCAMERA, MF_BYCOMMAND | enabled);
}

static void Update(ApplicationInfo* info)
{
    UpdateGameTime();
    if (info->object != NULL)
    {
        if (info->animation != NULL)
        {
            // Update animation
            unsigned int curFrame = (unsigned int)SendMessage(info->hTimeSlider, TBM_GETPOS, 0, 0);
            float        fps      = info->animation->GetFPS();
            float        time     = curFrame / fps;
            if (info->playing)
            {
                unsigned int frame = 0;
                if (info->animation->GetNumFrames() > 0)
                {
                    time = GetGameTime() - info->playStartTime + info->playStartFrame / fps;
                    if (info->playLoop) {
                        while (time >= info->animation->GetNumFrames() / fps)
                        {
                            // Wrap-around for loop
                            time                -= info->animation->GetNumFrames() / fps;
                            info->playStartTime += info->animation->GetNumFrames() / fps;
                            if (info->m_sfxmap != NULL)
                            {
                                info->m_sfxevent     = info->m_sfxmap->begin();
                            }
                        }
                    }
                    else if (time >= (info->animation->GetNumFrames() - 1) / fps)
                    {
                        time = (info->animation->GetNumFrames() - 1) / fps;
                        info->playing = false;
                        SendMessage(info->hPlayButton, BM_SETIMAGE, IMAGE_ICON, (LPARAM)LoadIcon(info->hInstance, MAKEINTRESOURCE(IDI_PLAY)));
                    }

                    frame = (unsigned int)(time * fps);
                }

                // Update time slider position
                if (curFrame != frame)
                {
                    SendMessage(info->hTimeSlider, TBM_SETPOS, TRUE, frame );
                }

                if (info->m_sfxmap != NULL)
                {
                    // Update SFX events
                    while (info->m_sfxevent != info->m_sfxmap->end() && info->m_sfxevent->first <= time)
                    {
                        const AnimationSFXMaps::Event& e = info->m_sfxevent->second;
                        switch (e.type)
                        {
                            case AnimationSFXMaps::Event::SOUND:
                                if (info->sound != NULL)
                                {
                                    // Play sound
                                    const SFXEvent* sfx = SFXEvents::GetEvent(e.name);
                                    if (sfx != NULL)
                                    {
                                        info->sound->PlaySoundEvent(*sfx);
                                    }
                                }
                                break;

                            case AnimationSFXMaps::Event::SURFACE:
                            {
                                const SurfaceFX::Effect* fx = SurfaceFX::GetEffect(e.name);
                                if (fx != NULL)
                                {
                                    if (info->sound != NULL)
                                    {
                                        // Play sound
                                        const SFXEvent* sfx = SFXEvents::GetEvent(fx->sound);
                                        if (sfx != NULL)
                                        {
                                            info->sound->PlaySoundEvent(*sfx);
                                        }
                                    }
                                }
                                break;
                            }
                        }
                        info->m_sfxevent++;
                    }
                }
            }
            info->object->SetAnimationTime(time);
        }
        info->object->Update();
    }
}

static LRESULT CALLBACK MainWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    ApplicationInfo* info = (ApplicationInfo*)(LONG_PTR)GetWindowLongPtr(hWnd, GWLP_USERDATA);
    switch (uMsg)
    {
        case WM_CREATE:
        {
            // Get the application info from the create parameters
            CREATESTRUCT* pcs = (CREATESTRUCT*)lParam;
            info = (ApplicationInfo*)pcs->lpCreateParams;
            SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG)(LONG_PTR)info);

            //
            // Create the console window before anything else, so errors during
            // creation of the other components show up in the log
            //
            if ((info->hConsoleWnd = CreateConsoleWindow(hWnd, false)) == NULL)
            {
                return -1;
            }

            //
            // Initialize sound system
            //
            try
            {
                info->sound = new DirectSound8::SoundEngine(hWnd);
            }
            catch (wexception& e)
            {
                Log::WriteError("Unable to initialize sound: %ls\n", e.what());
            }

            //
            // Get the persistent environment
            //
            Environment environment = Config::GetDefaultEnvironment();
            Config::GetEnvironment(&environment);

            //
            // Create the settings dialog
            //
            if ((info->hSettings = Dialogs::CreateSettingsDialog(hWnd, environment, info->sound)) == NULL)
            {
                return -1;
            }
            ColorButton_SetColor(info->hBackgroundBtn, environment.m_clearColor);

            // Get size of dialog
            RECT settings;
            GetWindowRect(info->hSettings, &settings);
            settings.right  -= settings.left;
			settings.bottom -= settings.top;

            //
            // Create the render window.
            // Make sure it's at its correct initial size, so we avoid a needless
            // resolution change after the create.
            //
            GameID game = (info->activeGameMod != info->gameMods.end()) ? info->activeGameMod->first.m_game : GID_UNKNOWN;

            RECT client;
            GetClientRect(hWnd, &client);
            if (IsWindowVisible(info->hConsoleWnd))
            {
                RECT console;
                GetWindowRect(info->hConsoleWnd, &console);
                client.bottom -= (console.bottom - console.top);
            }

            if ((info->hRenderWnd = CreateRenderWindow(hWnd, game, environment,
                    settings.right + 4, 32, client.right - settings.right - 4, client.bottom - 24 - 34)) == NULL)
	        {
		        return -1;
	        }
            SetRenderWindowOptions(info->hRenderWnd, Dialogs::Settings_GetRenderOptions(info->hSettings));
            info->engine           = GetRenderEngine(info->hRenderWnd);
            info->LoadTeamColors();

            //
            // Create the child windows
            //

            // Create the selection dialog
            if ((info->hSelection = Dialogs::CreateSelectionDialog(hWnd, *info)) == NULL)
            {
                return -1;
            }
            
            // Create the top bar: Colorization
            wstring label = LoadString(IDS_INTERFACE_COLORIZATION);
			if ((info->hColorsLabel = CreateWindowEx(0, L"STATIC", label.c_str(), WS_CHILD | WS_VISIBLE,
				settings.right + 4, 8, 500, 16, hWnd, NULL, pcs->hInstance, NULL)) == NULL)
			{
				return -1;
			}

			for (int i = 0; i < Config::NUM_PREDEFINED_COLORS; i++)
			{
				if ((info->hColorBtn[i] = CreateWindowEx(0, L"BUTTON", NULL, WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | BS_OWNERDRAW | (i == 0 ? WS_GROUP : 0),
					settings.right + 70 + i * 28, 4, 24, 24, hWnd, NULL, pcs->hInstance, NULL)) == NULL)
				{
					return -1;
				}
			}

            if ((info->hModelColorBtn = CreateWindowEx(0, L"ColorButton", NULL, WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_OWNERDRAW,
                settings.right + 70 + Config::NUM_PREDEFINED_COLORS * 28, 4, 24, 24, hWnd, NULL, pcs->hInstance, NULL)) == NULL)
            {
                return -1;
            }
            RenderSettings renderSettings = info->engine->GetSettings();
            ColorButton_SetColor(info->hModelColorBtn, renderSettings.m_modelColor);

            label = LoadString(IDS_INTERFACE_BACKGROUND);
            if ((info->hBackgroundLabel = CreateWindowEx(0, L"STATIC", label.c_str(), WS_CHILD | WS_VISIBLE,
				0, 0, 65, 16, hWnd, NULL, pcs->hInstance, NULL)) == NULL)
			{
				return -1;
			}
			
			if ((info->hBackgroundBtn = CreateWindowEx(0, L"ColorButton", NULL, WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_OWNERDRAW,
				0, 0, 24, 24, hWnd, NULL, pcs->hInstance, NULL)) == NULL)
			{
				return -1;
			}
            ColorButton_SetColor(info->hBackgroundBtn, environment.m_clearColor);

			// Create the bottom bar: Animation controls
			if ((info->hPlayButton = CreateWindowEx(0, L"BUTTON", NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_DISABLED | BS_ICON,
				0, 0, 24, 24, hWnd, NULL, pcs->hInstance, NULL)) == NULL)
			{
				return -1;
			}

			if ((info->hTimeSlider = CreateWindowEx(0, TRACKBAR_CLASS, NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_DISABLED | TBS_HORZ | TBS_NOTICKS,
				0, 0, 0, 22, hWnd, NULL, pcs->hInstance, NULL)) == NULL)
			{
				return -1;
			}

			if ((info->hLoopButton = CreateWindowEx(0, L"BUTTON", NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_DISABLED | BS_ICON | BS_AUTOCHECKBOX | BS_PUSHLIKE,
				0, 0, 24, 24, hWnd, NULL, pcs->hInstance, NULL)) == NULL)
			{
				return -1;
			}

			// Set the control's initial properties
            HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
			SendMessage(info->hColorsLabel,     WM_SETFONT, (WPARAM)hFont, FALSE);
			SendMessage(info->hBackgroundLabel, WM_SETFONT, (WPARAM)hFont, FALSE);
			SendMessage(info->hPlayButton,      BM_SETIMAGE, IMAGE_ICON, (LPARAM)LoadIcon(pcs->hInstance, MAKEINTRESOURCE(IDI_PLAY)));
            SendMessage(info->hLoopButton,      BM_SETIMAGE, IMAGE_ICON, (LPARAM)LoadIcon(pcs->hInstance, MAKEINTRESOURCE(IDI_LOOP)));
            ShowWindow(info->hSettings,  SW_SHOWNORMAL);
            ShowWindow(info->hSelection, SW_SHOWNORMAL);

            // Read the file history and append it
            AppendHistory(hWnd, info);
        }
        break;

        case WM_DESTROY:
            DestroyWindow(info->hSettings);
            DestroyWindow(info->hRenderWnd);
            SAFE_RELEASE(info->sound);
            break;

        case WM_TIMER:
            Update(info);
            if (info->engine != NULL)
            {
                RedrawWindow(info->hRenderWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
            }
            break;

        case WM_DRAWITEM:
		{
			HWND hBtn           = (HWND)wParam;
			DRAWITEMSTRUCT* dis = (DRAWITEMSTRUCT*)lParam;
	
            // Find the index of the button
			COLORREF color = RGB(0,0,0);
            int i = -1;
            if (info->teamColors.size() == Config::NUM_PREDEFINED_COLORS)
            {
                for (i = 0; i < Config::NUM_PREDEFINED_COLORS - 1; i++)
			    {
				    if (info->hColorBtn[i] == dis->hwndItem)
				    {
                        color = info->teamColors[i];
					    break;
				    }
			    }

			    if (dis->itemState & ODS_SELECTED)
			    {
				    int previous = info->selectedColor;
				    info->selectedColor = i;
				    if (previous >= 0 && previous < Config::NUM_PREDEFINED_COLORS)
				    {
					    // Clear previous
					    InvalidateRect(info->hColorBtn[previous], NULL, TRUE);
					    UpdateWindow(info->hColorBtn[previous]);
                    }
                    else {
                        ColorButton_SetSelected(info->hModelColorBtn, false);
                    }
			    }
            }

			HPEN hPen;
			if (info->selectedColor == i) {
				hPen = CreatePen(PS_SOLID, 5, GetSysColor(COLOR_HIGHLIGHT) );
			} else {
				hPen = CreatePen(PS_SOLID, 1, RGB(0,0,0) );
			}
			
			HBRUSH hBrush;
            if (i < Config::NUM_PREDEFINED_COLORS - 1) {
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

		case WM_SIZING:
		{
            // Restrict the size to at least the configured minimum sizes
			RECT* rect = (RECT*)lParam;
			bool left  = (wParam == WMSZ_BOTTOMLEFT) || (wParam == WMSZ_LEFT) || (wParam == WMSZ_TOPLEFT);
			bool top   = (wParam == WMSZ_TOPLEFT)    || (wParam == WMSZ_TOP)  || (wParam == WMSZ_TOPRIGHT);
            if (rect->right - rect->left < Config::MIN_APPLICATION_WIDTH)
			{
				if (left) rect->left  = rect->right - Config::MIN_APPLICATION_WIDTH;
				else      rect->right = rect->left  + Config::MIN_APPLICATION_WIDTH;
			}
            if (rect->bottom - rect->top < Config::MIN_APPLICATION_HEIGHT)
			{
				if (top) rect->top    = rect->bottom - Config::MIN_APPLICATION_HEIGHT;
				else     rect->bottom = rect->top    + Config::MIN_APPLICATION_HEIGHT;
			}
			break;
		}

        case WM_SIZE:
        {
            // Keep track of whether we're minimized or not
            info->isMinimized = (wParam == SIZE_MINIMIZED);
            if (!info->isMinimized)
            {
                // Not minimized, adjust all controls
                unsigned int width  = LOWORD(lParam);
                unsigned int height = HIWORD(lParam);

                RECT settings, label, playbtn, console;
                GetWindowRect(info->hSettings,        &settings);
                GetWindowRect(info->hBackgroundLabel, &label);
                GetWindowRect(info->hPlayButton,      &playbtn);
                GetWindowRect(info->hConsoleWnd,      &console);
                settings.right  -= settings.left;
                settings.bottom -= settings.top;
                label.right     -= label.left;
                label.bottom    -= label.top;
                playbtn.right   -= playbtn.left;
                playbtn.bottom  -= playbtn.top;
                console.right   -= console.left;
                console.bottom  -= console.top;

                MoveWindow(info->hConsoleWnd, 0, height - console.bottom, width, console.bottom, TRUE);
                if (IsWindowVisible(info->hConsoleWnd))
                {
                    // Everything else goes above the console
                    height -= console.bottom;
                }

                MoveWindow(info->hBackgroundBtn,   width - 28, 4, 24, 24, TRUE);
				MoveWindow(info->hBackgroundLabel, width - 32 - label.right, 4 + (24 - label.bottom) / 2, label.right, label.bottom, TRUE);
                MoveWindow(info->hPlayButton, settings.right  + 4,                 height - playbtn.bottom + 0, playbtn.right, playbtn.bottom, TRUE);
                MoveWindow(info->hTimeSlider, settings.right  + 4 + playbtn.right, height - playbtn.bottom - 1, width - settings.right - 2 * (4 + playbtn.right), playbtn.bottom + 2, TRUE);
                MoveWindow(info->hLoopButton, width - playbtn.right,           height - playbtn.bottom + 0, playbtn.right, playbtn.bottom, TRUE);
                MoveWindow(info->hRenderWnd,  settings.right  + 4, 32, width - settings.right - 4, height - playbtn.bottom - 34, TRUE);
                MoveWindow(info->hSelection,  0, settings.bottom + 2, settings.right, height - settings.bottom - 2, TRUE);
            }
        }
        break;

        case WM_CLOSE:
            // When closing the main window, we exit the program
            PostQuitMessage(0);
            return 0;

		case WM_INITMENU:
			DoMenuInit(info, (HMENU)wParam);
			break;

		case WM_COMMAND:
		{
			// Menu and control notifications
			WORD code     = HIWORD(wParam);
			WORD id       = LOWORD(wParam);
			HWND hControl = (HWND)lParam;
			if (lParam == 0)
			{
                // Menu or accelerator
                DoMenuItem(info, id);
			}
            else if (code == SN_CHANGE && info->engine != NULL)
            {
                // Environment has changed, redraw
                info->engine->SetEnvironment(Dialogs::Settings_GetEnvironment(hControl));
                RedrawWindow(info->hRenderWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
            }
			else if (code == CBN_CHANGE && info->engine != NULL)
			{
				if (hControl == info->hBackgroundBtn)
				{
					// The background color has changed
                    Environment& environment = Dialogs::Settings_GetEnvironment(info->hSettings);
                    environment.m_clearColor = ColorButton_GetColor(hControl);
                    info->engine->SetEnvironment(environment);
                    RedrawWindow(info->hRenderWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
				}
                else if (hControl == info->hModelColorBtn)
				{
					// The model color has changed
                    if (info->selectedColor < Config::NUM_PREDEFINED_COLORS) {
                        // Deselect the previously-selected color button
                        HDC hDC = GetDC(info->hColorBtn[info->selectedColor]);
                        RECT rc;
                        GetClientRect(info->hColorBtn[info->selectedColor], &rc);
                        HPEN hPen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
                        HBRUSH hBrush;
                        if (info->selectedColor < Config::NUM_PREDEFINED_COLORS - 1) {
                            hBrush = CreateSolidBrush(info->teamColors[info->selectedColor]);
                        }
                        else {
                            LOGBRUSH lb = {
                                BS_HATCHED,
                                RGB(128,128,128),
                                HS_BDIAGONAL,
                            };
                            hBrush = CreateBrushIndirect(&lb);
                        }

                        SelectObject(hDC, hBrush);
                        SelectObject(hDC, hPen);
                        Rectangle(hDC, rc.left, rc.top, rc.right, rc.bottom);
                        DeleteObject(hBrush);
                        DeleteObject(hPen);

                        info->selectedColor = Config::NUM_PREDEFINED_COLORS;

                        // Draw the selection box around this color button
                        ColorButton_SetSelected(hControl, true);
                    }
                    Color modelColor = ColorButton_GetColor(hControl);
                    if (info->object != NULL) {
                        info->object->SetColorization(modelColor);
                        RedrawWindow(info->hRenderWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
                    }
                    RenderSettings settings = info->engine->GetSettings();
                    settings.m_modelColor = modelColor;
                    info->engine->SetSettings(settings);
                }
			}
			else if (code == LBN_SELCHANGE || code == BN_CLICKED)
			{
                if (hControl == info->hSettings)
                {
                    // Render options have changed
                    SetRenderWindowOptions(info->hRenderWnd, Dialogs::Settings_GetRenderOptions(info->hSettings));
                }
                else if (hControl == info->hPlayButton)
				{
					info->playing  = !info->playing;
					DWORD resource = (info->playing) ? IDI_PAUSE : IDI_PLAY;
					SendMessage(info->hPlayButton, BM_SETIMAGE, IMAGE_ICON, (LPARAM)LoadIcon(info->hInstance, MAKEINTRESOURCE(resource)));
					if (info->playing)
					{
						info->playStartTime  = GetGameTime();
						info->playStartFrame = (unsigned int)SendMessage(info->hTimeSlider, TBM_GETPOS, 0, 0);
                        if (info->playStartFrame == info->animation->GetNumFrames() - 1)
                        {
                            // Reset animation
                            info->playStartFrame = 0;
                            if (info->m_sfxmap != NULL)
                            {
                                info->m_sfxevent = info->m_sfxmap->begin();
                            }
                        }
                        SendMessage(info->hTimeSlider, TBM_SETPOS, TRUE, 0);
                        if (info->sound != NULL)
                        {
                            info->sound->Play();
                        }
					}
                    else if (info->sound != NULL)
                    {
                        info->sound->Pause();
                    }
				}
                else if (hControl == info->hLoopButton)
				{
					info->playLoop = !info->playLoop;
				}
                else if (info->object != NULL && info->teamColors.size() >= Config::NUM_PREDEFINED_COLORS)
                {
                    for (int i = 0; i < Config::NUM_PREDEFINED_COLORS; i++)
                    {
                        if (info->hColorBtn[i] == hControl)
                        {
                            info->object->SetColorization(ColorRefToColor(info->teamColors[i]));
                            RedrawWindow(info->hRenderWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
                            break;
                        }
                    }
                }
			}
		}
		break;

        case WM_DROPFILES:
        {
		    // User dropped a filename on the renderwindow.
            // We extract a single ALO and ALA file from the drop operation.
            HDROP hDrop = (HDROP)wParam;
		    UINT nFiles = DragQueryFile(hDrop, -1, NULL, 0);
            wstring alofile, alafile;
		    for (UINT i = 0; i < nFiles; i++)
		    {
			    UINT size = DragQueryFile(hDrop, i, NULL, 0);
                TCHAR* filename = new TCHAR[size + 1];
			    DragQueryFile(hDrop, i, filename, size + 1);
                const TCHAR* ext = PathFindExtension(filename);
                     if (_wcsicmp(ext, L".alo") == 0) alofile = filename;
                else if (_wcsicmp(ext, L".ala") == 0) alafile = filename;
                delete[] filename;
		    }
		    DragFinish(hDrop);

            // First load the ALO file, then the ALA file
            if (!alofile.empty()) LoadFile(info, alofile);
            if (!alafile.empty()) LoadFile(info, alafile);
            break;
        }
    }
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
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
    wcx.lpszMenuName  = MAKEINTRESOURCE(IDR_MAINMENU);
    wcx.lpszClassName = L"AloViewer";
    wcx.hIconSm       = NULL;

	if (RegisterClassEx(&wcx) == 0)
	{
		return false;
	}

    if (!RegisterRenderWindow(info->hInstance))
    {
        return false;
    }

    if (!RegisterConsoleWindow(info->hInstance))
    {
        UnregisterRenderWindow(info->hInstance);
        return false;
    }

	if (!UI_Initialize(info->hInstance))
	{
        UnregisterConsoleWindow(info->hInstance);
        UnregisterRenderWindow(info->hInstance);
		return false;
	}

    if ((info->hMainWnd = CreateWindowEx(WS_EX_CONTROLPARENT, L"AloViewer", Config::WINDOW_TITLE, WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, 
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, info->hInstance, info)) == NULL)
	{
        UI_Uninitialize(info->hInstance);
        UnregisterConsoleWindow(info->hInstance);
        UnregisterRenderWindow(info->hInstance);
		return false;
	}
    InitializeGameMenu(info);
	return true;
}

// Parse the command line into a argv-style vector
static vector<wstring> ParseCommandLine()
{
	int nArgs;
	vector<wstring> argv;

	LPTSTR *args = CommandLineToArgvW(GetCommandLineW(), &nArgs);
	if (args != NULL)
	{
		for (int i = 0; i < nArgs; i++)
		{
			argv.push_back(args[i]);
		}
		LocalFree(args);
	}
	return argv;
}

ApplicationInfo::ApplicationInfo(HINSTANCE hInstance)
{
    this->hInstance = hInstance;
	engine          = NULL;
    sound           = NULL;
    isMinimized     = false;
    playing         = false; 
    playLoop        = false;
    selectedColor   = Config::NUM_PREDEFINED_COLORS - 1;

    TCHAR buffer[MAX_PATH];
    GetCurrentDirectory(MAX_PATH, buffer);
    startupDirectory = buffer;

    // See what games and mods are installed
    vector<GameMod> mods;
    GameMod::GetAllGameMods(mods);
    for (size_t i = 0; i < mods.size(); i++)
    {
        GameModInfo info;
        info.menuitem = 0;
        gameMods[mods[i]] = info;
    }

    // Get default game mode from configuration
    GameMod def = Config::GetDefaultGameMod();
    if (def.m_game == GID_UNKNOWN)
    {
        // No default, see what games are installed, start at the latest
        for (int i = NUM_GAME_IDS - 1; i >= 0; i--)
        {
            GameMod mod = GameMod((GameID)i);
            if (gameMods.find(mod) != gameMods.end())
            {
                // Use this one
                def = mod;
                Config::SetDefaultGameMod(def);
                break;
            }
        }
    }
    activeGameMod = gameMods.find(def);
}

ApplicationInfo::~ApplicationInfo()
{
    // Release created objects
    if (object != NULL)
    {
        object->Destroy();
        SAFE_RELEASE(object);
    }
    SAFE_RELEASE(sound);
    SAFE_RELEASE(model);
    SAFE_RELEASE(animation);

    // Clean up UI
    DestroyWindow(hMainWnd);
    UI_Uninitialize(hInstance);
    UnregisterConsoleWindow(hInstance);
    UnregisterRenderWindow(hInstance);
    UnregisterClass(L"AloViewer", hInstance);

    // Clean up subsystems
    LightSources::Uninitialize();
    SurfaceFX::Uninitialize();
    AnimationSFXMaps::Uninitialize();
    SFXEvents::Uninitialize();
    Assets::Uninitialize();
}

void ApplicationInfo::LoadTeamColors() {
    teamColors.clear();
    if (activeGameMod == gameMods.end()) {
        return;
    }
    GameMod mod = activeGameMod->first;

    // First, add the default colors
    switch (mod.m_game)
    {
        case GID_EAW:
        case GID_EAW_FOC: teamColors.insert(teamColors.begin(), Config::PREDEFINED_COLORS_EAW, Config::PREDEFINED_COLORS_EAW + Config::NUM_PREDEFINED_COLORS); break;
        case GID_UAW_EA:  teamColors.insert(teamColors.begin(), Config::PREDEFINED_COLORS_UAW, Config::PREDEFINED_COLORS_UAW + Config::NUM_PREDEFINED_COLORS); break;
    }

    // Then, if we are loading a mod, replace the defaults with the ones defined in GameConstants.xml, if it exists
    // Any colors not defined in GameConstants.xml will remain the default color, including our white/no color entry
    if (!mod.IsBaseGame()) {
        XMLTree xml;
        ptr<IFile> gameConstantsFile = Assets::LoadFile("GameConstants.xml", "Data/XML/");
        if (gameConstantsFile != NULL) {
            try {
                xml.parse(gameConstantsFile);
                const XMLNode* root = xml.getRoot();
                for (size_t i = 0; i < root->getNumChildren(); i++)
                {
                    const XMLNode* ent = root->getChild(i);
                    for (int j = 0; j < _countof(Config::MP_COLOR_NAMES); j++) {
                        wstring name = ent->getName();
                        if (name == Config::MP_COLOR_NAMES[j]) {
                            COLORREF color = Config::ParseMPColor(ent->getData());
                            teamColors[j] = color;
                        }
                    }
                }
            }
            catch (ParseException&) {}
        }
    }
    return;
}

// Initialize all modules
static void Initialize(ApplicationInfo* info)
{
    // Initialize assets
    InitializeAssets(info);

    // Initialize the UI
    if (!InitializeUI( info ))
    {
	    throw exception("User Interface Initialization failed");
    }

    if (info->sound != NULL)
    {
        // Initialize SFX events
        SFXEvents::Initialize();
        AnimationSFXMaps::Initialize();
        SurfaceFX::Initialize();
    }
    LightSources::Initialize();
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)
{
#ifndef NDEBUG
    // In debug mode we create a console and turn on memory checking
 	AllocConsole();
	freopen("conout$", "wb", stdout);
	freopen("conout$", "wb", stderr);
    _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

#ifdef NDEBUG
    // Only catch exceptions in release mode.
    // In debug mode, the IDE will jump to the source.
    try
#endif
    {
		ApplicationInfo info(hInstance);
        vector<wstring> args = ParseCommandLine();

        Initialize(&info);

        // Parse arguments
        if (args.size() > 1)
        {
            LoadFile(&info, args[1]);
        }
      
        // Main message processing loop
        ShowWindow(info.hMainWnd, SW_SHOWNORMAL);
        HACCEL hAccel = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCELERATOR1));	

        // Create the update timer; it will keep sending WM_TIMER messages to keep
        // the viewer busy, even when in menus and dialogs.
        SetTimer(info.hMainWnd, 0, USER_TIMER_MINIMUM, NULL);

        MSG msg;
        while (GetMessage(&msg, NULL, 0, 0))
        {
			if (!TranslateAccelerator(info.hMainWnd, hAccel, &msg))
			if (!IsDialogMessage(info.hMainWnd, &msg))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }

        // Kill the update timer
        KillTimer(info.hMainWnd, 0);
    }
#ifdef NDEBUG
    catch (exception& e)
    {
		MessageBoxA(NULL, e.what(), NULL, MB_OK );
    }
    catch (wexception& e)
    {
		MessageBoxW(NULL, e.what(), NULL, MB_OK );
    }
#endif
    Log::Uninitialize();

#ifndef NDEBUG
	FreeConsole();
#endif 

    return 0;
}