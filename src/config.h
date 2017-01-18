#ifndef CONFIG_H
#define CONFIG_H

#include "General/GameTypes.h"
#include "Games.h"
#include <map>

namespace Config
{
// Application version
static const int      VERSION_MAJOR = 1;
static const int      VERSION_MINOR = 2;

// Title of the main window
static const wchar_t* WINDOW_TITLE  = L"Alamo Object Viewer";

// Base path in HKEY_CURRENT_USER for this application
static const wchar_t* REGISTRY_BASE_PATH = L"Software\\AloViewer";

// Minimum dimension for the application window
static const int MIN_APPLICATION_WIDTH  = 700;
static const int MIN_APPLICATION_HEIGHT = 550;

// There are eight predefined colors, copied from GameConstants.xml
// The ninth one is white; no colorization
static const int NUM_PREDEFINED_COLORS = 9;
static const COLORREF PREDEFINED_COLORS_EAW[NUM_PREDEFINED_COLORS] = {
	RGB( 78,150,237), RGB(237, 78, 78), RGB(119,237, 78), RGB(237,149, 78),
	RGB(111,217,224), RGB(224,111,209), RGB(224,215,111), RGB(109, 75,  5),
    RGB(255,255,255)
};

static const COLORREF PREDEFINED_COLORS_UAW[NUM_PREDEFINED_COLORS] = {
	RGB( 78,150,255), RGB(255, 23, 23), RGB(119,255, 78), RGB(255,149, 23),
    RGB(111,217,224), RGB(255,111,209), RGB(224,223, 45), RGB(150,150,150),
    RGB(255,255,255)
};

// Show up to this amount of files in the File menu
static const int NUM_HISTORY_ITEMS = 9;

// Get and set stored render settings
Alamo::RenderSettings GetRenderSettings();
void SetRenderSettings(const Alamo::RenderSettings& settings);

// Get and set the environment
void GetEnvironment(Alamo::Environment* env);
Alamo::Environment GetDefaultEnvironment();
void SetEnvironment(const Alamo::Environment& env);

// Get and set the file history
void GetHistory(std::map<ULONGLONG, std::wstring>& history);
void AddToHistory(const std::wstring& name);

// Get and set the default game mod
GameMod GetDefaultGameMod();
void SetDefaultGameMod(const GameMod& mode);
}
#endif