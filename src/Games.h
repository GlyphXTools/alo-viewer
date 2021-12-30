#ifndef GAMES_H
#define GAMES_H

#include <string>
#include <vector>

// Supported games
enum GameID
{
    GID_UNKNOWN = -1,
    GID_EAW = 0,
    GID_EAW_FOC,
    GID_UAW_EA,
    NUM_GAME_IDS
};

// The GameMod structure identifies a specific mod for a specific game.
// An empty mod string identifies the game itself, unmodded.
struct GameMod
{
    GameID       m_game;
    std::wstring m_mod;
    std::wstring m_steamId;

    // Operators
    bool operator == (const GameMod& rhs) const {
        return m_game == rhs.m_game && m_mod == rhs.m_mod && m_steamId == rhs.m_steamId;
    }

    bool operator != (const GameMod& rhs) const {
        return !(*this == rhs);
    }

    bool operator < (const GameMod& rhs) const {
        if (m_steamId.empty() && rhs.m_steamId.empty()) {
            // Two non-Steam mods are sorted by game and then by mod name
            return (m_game < rhs.m_game) || (m_game == rhs.m_game && m_mod < rhs.m_mod);
        }
        else if (!m_steamId.empty() && !rhs.m_steamId.empty()) {
            // Two Steam mods are sorted by game, then Steam ID, then mod name
            return (m_game < rhs.m_game) || (m_game == rhs.m_game && _wtoi(m_steamId.c_str()) < _wtoi(rhs.m_steamId.c_str())) || (m_steamId == rhs.m_steamId && m_game == rhs.m_game && m_mod < rhs.m_mod);
        }
        else {
            // A Steam mod and a non-Steam mod are sorted by game, then Steam mods first
            return (m_game < rhs.m_game) || (m_game == rhs.m_game && !m_steamId.empty());
        }
    }

    static GameMod GetForFile(const std::wstring& path);
    static void GetAllGameMods(std::vector<GameMod>& gamemods);

    std::wstring GetBaseDir() const;
    void AppendAssetDirs(std::vector<std::wstring>& basedirs) const;
    bool IsValid() const;
    bool IsBaseGame() const;

    // Constructors
    GameMod(GameID game = GID_UNKNOWN) : m_game(game) {}
    GameMod(GameID game, const std::wstring& mod) : m_game(game), m_mod(mod) {}
    GameMod(GameID game, const std::wstring& mod, const std::wstring& steamId) : m_game(game), m_mod(mod), m_steamId(steamId) {}
};

#endif