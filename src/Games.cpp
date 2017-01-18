#include "Games.h"
#include <shlwapi.h>
using namespace std;

// Returns the base directory for a game, based on
// 1. Registry
// 2. Default path, expanded
static wstring GetExePath(const wchar_t* regkey, const wchar_t* valname, const wchar_t* defpath = NULL)
{
	// The games are 32-bit games, so make these calls in 32-bit mode
    wstring result;
	
    HKEY hKey;
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, regkey, 0, KEY_QUERY_VALUE | KEY_WOW64_32KEY, &hKey ) == ERROR_SUCCESS)
	{
		DWORD type, size = MAX_PATH;
        TCHAR path[MAX_PATH];
		if (RegQueryValueEx(hKey, valname, NULL, &type, (LPBYTE)path, &size) == ERROR_SUCCESS)
		{
            if (PathRemoveFileSpec(path) && PathIsDirectory(path))
            {
				result = path;
            }
		}
		RegCloseKey(hKey);
	}
    
    if (result.empty() && defpath != NULL)
    {
        // Use the default path
        TCHAR path[MAX_PATH];
        if (ExpandEnvironmentStrings(defpath, path, MAX_PATH) != 0)
        {
            if (PathIsDirectory(path))
            {
                result = path;
            }
        }
    }

    return result;
}

// Returns the base directory for a game
static wstring GetBaseDirForGame(GameID game)
{
    switch (game)
    {
        case GID_EAW_FOC:
            return GetExePath(L"Software\\LucasArts\\Star Wars Empire at War Forces of Corruption\\1.0", L"ExePath", L"%PROGRAMFILES%\\LucasArts\\Star Wars Empire at War Forces of Corruption");

        case GID_EAW:
            return GetExePath(L"Software\\LucasArts\\Star Wars Empire at War\\1.0", L"ExePath", L"%PROGRAMFILES%\\LucasArts\\Star Wars Empire at War\\GameData");

        case GID_UAW_EA:
            return GetExePath(L"Software\\Petroglyph\\UAWEA", L"GameExe", L"%PROGRAMFILES%\\Sega\\Universe At War Earth Assault");
    }
    return L"";
}

// Returns the GameMod identifier for a file
GameMod GameMod::GetForFile(const std::wstring& path)
{
    vector<GameMod> gamemods;
    GetAllGameMods(gamemods);
    for (size_t i = 0; i < gamemods.size(); i++)
    {
        wstring dir = gamemods[i].GetBaseDir();
        if (path.find(dir) == 0)
        {
            return gamemods[i];
        }
    }
    return GameMod();
}

// Returns the base directory for a game or mod
wstring GameMod::GetBaseDir() const
{
    wstring path = GetBaseDirForGame(m_game);
    if (!m_mod.empty())
    {
        // It's a mod, append modpath
        wstring modpath = path + L"\\Mods\\" + m_mod;
        if (PathIsDirectory(modpath.c_str())) {
            return modpath;
        }
    }
    return path;
}

// Appends the asset search directories for this game mod to the list
void GameMod::AppendAssetDirs(std::vector<std::wstring>& basedirs) const
{
    wstring gamepath, modpath;
    switch (m_game)
    {
    case GID_EAW_FOC:
        if (!(gamepath = GetBaseDirForGame(GID_EAW_FOC)).empty()) {
            if (!m_mod.empty()) {
                modpath = gamepath + L"\\Mods\\" + m_mod;
                if (PathIsDirectory(modpath.c_str())) {
                    basedirs.push_back(modpath);
                }
            }
            basedirs.push_back(gamepath);
        }

    case GID_EAW:
        if (!(gamepath = GetBaseDirForGame(GID_EAW)).empty()) {
            if (m_game == GID_EAW) {
                if (!m_mod.empty()) {
                    // Only add modpath if this is not for FoC.
                    modpath = gamepath + L"\\Mods\\" + m_mod;
                    if (PathIsDirectory(modpath.c_str())) {
                        basedirs.push_back(modpath);
                    }
                }
            }
            basedirs.push_back(gamepath);
        }
        break;

    case GID_UAW_EA:
        if (!(gamepath = GetBaseDirForGame(GID_UAW_EA)).empty()) {
            if (!m_mod.empty()) {
                modpath = gamepath + L"\\Mods\\" + m_mod;
                if (PathIsDirectory(modpath.c_str())) {
                    basedirs.push_back(modpath);
                }
            }
            basedirs.push_back(gamepath);
        }
        break;
    }
}

// Returns true if this mod identifier is valid
bool GameMod::IsValid() const
{
    return !GetBaseDir().empty();
}

// Appends the game and the mods for the game to the list
static void GetMods(GameID game, std::vector<GameMod>& gamemods)
{
    wstring gamepath;
    if (!(gamepath = GetBaseDirForGame(game)).empty()) {
        wstring modpath = gamepath + L"\\Mods\\*.*";
       
        WIN32_FIND_DATA wfd;
        HANDLE hFind = FindFirstFile(modpath.c_str(), &wfd);
        if (hFind != INVALID_HANDLE_VALUE)
        {
            do {
                if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY && wcscmp(wfd.cFileName,L".") != 0 && wcscmp(wfd.cFileName,L"..") != 0) {
                    // It's a mod directory
                    gamemods.push_back(GameMod(game, wfd.cFileName));
                }
            } while (FindNextFile(hFind, &wfd));
            FindClose(hFind);
        }
    }

    // At least we have the game itself, without mods.
    // Append it after the mods, so mods match before the game itself.
    // Even if we can't find the game, we still support this mode so the user can select it
    // and put the assets in the local directory.
    gamemods.push_back(GameMod(game));
}

// Return a list of all installed and supported games and game mods.
void GameMod::GetAllGameMods(std::vector<GameMod>& gamemods)
{
    gamemods.clear();
    GetMods(GID_EAW,     gamemods);
    GetMods(GID_EAW_FOC, gamemods);
    GetMods(GID_UAW_EA,  gamemods);
}
