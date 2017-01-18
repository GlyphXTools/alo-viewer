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

    // Operators
    bool operator == (const GameMod& rhs) const {
        return m_game == rhs.m_game && m_mod == rhs.m_mod;
    }

    bool operator != (const GameMod& rhs) const {
        return !(*this == rhs);
    }

    bool operator < (const GameMod& rhs) const {
        return (m_game < rhs.m_game) || (m_game == rhs.m_game && m_mod < rhs.m_mod);
    }

    static GameMod GetForFile(const std::wstring& path);
    static void GetAllGameMods(std::vector<GameMod>& gamemods);

    std::wstring GetBaseDir() const;
    void AppendAssetDirs(std::vector<std::wstring>& basedirs) const;
    bool IsValid() const;

    // Constructors
    GameMod(GameID game = GID_UNKNOWN) : m_game(game) {}
    GameMod(GameID game, const std::wstring& mod) : m_game(game), m_mod(mod) {}
};

#endif