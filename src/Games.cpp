#include "Games.h"
#include <shlwapi.h>
#include <array>
#include <cctype>
#include <fstream>
#include <optional>
#include <filesystem>
#include <sstream>
#include <unordered_map>
#include <variant>

namespace {
    class VdfParser {
    public:
        struct KeyValue;
        using KeyValues = std::vector<KeyValue>;
        struct KeyValue {
            std::string key;
            std::variant<std::string, KeyValues> value;
        };

        explicit VdfParser(const std::filesystem::path& path) {
            file.exceptions(std::ios::badbit | std::ios::failbit);
            file.open(path);
        }

        std::optional<KeyValue> Parse() {
            if (auto key = next()) {
                if (*key == "}") {
                    // We're done
                    return {};
                }

                if (auto value = next()) {
                    KeyValue result;
                    if (*value == "{") {
                        KeyValues values;
                        while (auto kv = Parse()) {
                            values.push_back(std::move(*kv));
                        }
                        result.value = std::move(values);
                    }
                    else {
                        result.value = std::move(*value);
                    }
                    result.key = std::move(*key);
                    return result;
                }
            }
            return {};
        }

    private:
        std::optional<std::pair<std::string, std::string>> next_pair() {
            if (auto key = next()) {
                if (auto value = next()) {
                    return std::make_pair<std::string, std::string>(std::move(*key), std::move(*value));
                }
            }
            return {};
        }

        std::optional<std::string> next() {
            int ch;
            while (std::isspace(ch = file.get())) {}
            if (ch == -1) {
                return {};
            }

            std::stringstream token;
            if (ch == '"') {
                while ((ch = file.get()) != '"') {
                    if (ch == '\\') {
                        if ((ch = file.get()) == -1) {
                            return {};
                        }
                    }
                    token << (char)ch;
                }
            }
            else {
                token << (char)ch;
                while (!std::isspace(ch = file.get())) {
                    if (ch == -1) {
                        return {};
                    }
                    token << (char)ch;
                }
            }
            return token.str();
        }

        std::ifstream file;
    };

    std::optional<std::string> FindValue(const VdfParser::KeyValues& key_values, const std::string& key)
    {
        for (const auto& kv: key_values) {
            if (kv.key == key) {
                if (const auto* val = std::get_if<std::string>(&kv.value)) {
                    return *val;
                }
            }
        }
        return {};
    }

    // Returns the base directory for a game, based on
    // 1. Registry
    // 2. Default path, expanded
    std::optional<std::filesystem::path> GetRegistryPath(const wchar_t* regkey, const wchar_t* valname)
    {
        // The games are 32-bit games, so make these calls in 32-bit mode
        HKEY hKey;
        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, regkey, 0, KEY_QUERY_VALUE | KEY_WOW64_32KEY, &hKey) == ERROR_SUCCESS)
        {
            DWORD type, size = MAX_PATH;
            TCHAR path[MAX_PATH];
            if (RegQueryValueEx(hKey, valname, NULL, &type, (LPBYTE)path, &size) == ERROR_SUCCESS)
            {
                RegCloseKey(hKey);
                return path;
            }
            RegCloseKey(hKey);
        }
        return {};
    }

    std::optional<std::filesystem::path> GetDirectoryPath(std::optional<std::filesystem::path> path)
    {
        if (path) {
            path->remove_filename();
            if (!std::filesystem::is_directory(*path)) {
                return {};
            }
        }
        return path;
    }

    std::optional<std::filesystem::path> GetEnvironmentPath(const wchar_t* envpath) {
        TCHAR path[MAX_PATH];
        if (ExpandEnvironmentStrings(envpath, path, MAX_PATH) != 0)
        {
            if (PathIsDirectory(path))
            {
                return path;
            }
        }
        return {};
    }

    std::vector<std::filesystem::path> GetSteamLibraryPaths() {
        std::vector<std::filesystem::path> library_paths;
        if (const auto steam_path = GetRegistryPath(L"Software\\Valve\\Steam", L"InstallPath")) {
            if (const auto root = VdfParser(*steam_path / "steamapps" / "libraryfolders.vdf").Parse()) {
                if (root->key == "libraryfolders") {
                    if (const auto* folders = std::get_if<VdfParser::KeyValues>(&root->value)) {
                        for (const auto& folder : *folders) {
                            if (const auto* folder_items = std::get_if<VdfParser::KeyValues>(&folder.value)) {
                                if (auto path = FindValue(*folder_items, "path")) {
                                    if (std::filesystem::is_directory(*path)) {
                                        library_paths.push_back(std::move(*path));
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        return library_paths;
    }

    std::optional<std::filesystem::path> GetSteamAppPath(unsigned int app_id, const wchar_t* subdir = nullptr) {
        try {
            for (const auto& library_path : GetSteamLibraryPaths()) {
                try {
                    if (const auto root = VdfParser(library_path / "steamapps" / ("appmanifest_" + std::to_string(app_id) + ".acf")).Parse()) {
                        if (root->key == "AppState") {
                            if (const auto* app_items = std::get_if<VdfParser::KeyValues>(&root->value)) {
                                if (const auto dirname = FindValue(*app_items, "installdir")) {
                                    auto app_path = library_path / "steamapps" / "common" / *dirname;
                                    if (subdir) {
                                        app_path /= subdir;
                                    }
                                    if (std::filesystem::is_directory(app_path)) {
                                        return app_path;
                                    }
                                }
                            }
                        }
                    }
                }
                catch (const std::exception&) {
                }
            }
        }
        catch (const std::exception&) {
        }
        return {};
    }

    // Calls every provider in order and returns the first path found. Returns empty string if no paths are found.
    template <typename... PathProviders>
    std::wstring FindFirstPath(PathProviders ...path_providers)
    {
        try {
            ([&] {
                if (auto path = path_providers()) {
                    throw std::move(*path);
                }
                }(), ...);
        }
        catch (const std::filesystem::path& path) {
            return path.wstring();
        }
        return {};
    }

    unsigned int SteamAppId_EmpireAtWarGoldPack = 32470;

    // Returns the base directory for a game
    std::wstring GetBaseDirForGame(GameID game)
    {
        switch (game)
        {
        case GID_EAW_FOC:
            return FindFirstPath(
                [] { return GetDirectoryPath(GetRegistryPath(L"Software\\LucasArts\\Star Wars Empire at War Forces of Corruption\\1.0", L"ExePath")); },
                [] { return GetSteamAppPath(SteamAppId_EmpireAtWarGoldPack, L"corruption"); },
                [] { return GetEnvironmentPath(L"%PROGRAMFILES%\\LucasArts\\Star Wars Empire at War Forces of Corruption"); }
            );

        case GID_EAW:
            return FindFirstPath(
                [] { return GetDirectoryPath(GetRegistryPath(L"Software\\LucasArts\\Star Wars Empire at War\\1.0", L"ExePath")); },
                [] { return GetSteamAppPath(SteamAppId_EmpireAtWarGoldPack, L"GameData"); },
                [] { return GetEnvironmentPath(L"%PROGRAMFILES%\\LucasArts\\Star Wars Empire at War\\GameData"); }
            );

        case GID_UAW_EA:
            return FindFirstPath(
                [] { return GetDirectoryPath(GetRegistryPath(L"Software\\Petroglyph\\UAWEA", L"GameExe")); },
                [] { return GetEnvironmentPath(L"%PROGRAMFILES%\\Sega\\Universe At War Earth Assault"); }
            );
        }
        return L"";
    }
}

// Returns the GameMod identifier for a file
GameMod GameMod::GetForFile(const std::wstring& path)
{
    std::vector<GameMod> gamemods;
    GetAllGameMods(gamemods);
    for (size_t i = 0; i < gamemods.size(); i++)
    {
        std::wstring dir = gamemods[i].GetBaseDir();
        if (path.find(dir) == 0)
        {
            return gamemods[i];
        }
    }
    return GameMod();
}

// Returns the base directory for a game or mod
std::wstring GameMod::GetBaseDir() const
{
    std::wstring path = GetBaseDirForGame(m_game);
    if (!m_mod.empty())
    {
        // It's a mod, append modpath
        std::wstring modpath = path + L"\\Mods\\" + m_mod;
        if (PathIsDirectory(modpath.c_str())) {
            return modpath;
        }
    }
    return path;
}

// Appends the asset search directories for this game mod to the list
void GameMod::AppendAssetDirs(std::vector<std::wstring>& basedirs) const
{
    std::wstring gamepath, modpath;
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
    std::wstring gamepath;
    if (!(gamepath = GetBaseDirForGame(game)).empty()) {
        std::wstring modpath = gamepath + L"\\Mods\\*.*";
       
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
