#include "config.h"
#include <shlwapi.h>
using namespace Alamo;
using namespace std;

// =========================================
// Utility functions
// =========================================
static void ReadColor(HKEY hKey, LPCTSTR name, Color* c)
{
    float vals[4];
    DWORD type, size = 4 * sizeof(float);
    if (RegQueryValueEx(hKey, name, NULL, &type, (BYTE*)&vals, &size) == ERROR_SUCCESS && type == REG_BINARY)
	{
        *c = Color(vals[0], vals[1], vals[2], vals[3]);
    }
}

static int ReadInteger(HKEY hKey, LPCTSTR name, int def)
{
    DWORD val, type, size = sizeof(int);
    if (hKey != NULL && RegQueryValueEx(hKey, name, NULL, &type, (BYTE*)&val, &size) == ERROR_SUCCESS && type == REG_DWORD)
	{
        return val;
    }
    return def;
}

static void ReadVector3(HKEY hKey, LPCTSTR name, Vector3* v)
{
    float vals[3];
    DWORD type, size = 3 * sizeof(float);
    if (RegQueryValueEx(hKey, name, NULL, &type, (BYTE*)&vals, &size) == ERROR_SUCCESS && type == REG_BINARY)
	{
        *v = Vector3(vals[0], vals[1], vals[2]);
    }
}

static wstring ReadString(HKEY hKey, LPCTSTR name)
{
    DWORD type, size = 0;
    if (RegQueryValueEx(hKey, name, NULL, &type, NULL, &size) == ERROR_SUCCESS && (type == REG_SZ || type == REG_MULTI_SZ || type == REG_EXPAND_SZ))
	{
        wchar_t* buf = new wchar_t[size / sizeof(wchar_t) + 1];
        if (RegQueryValueEx(hKey, name, NULL, &type, (BYTE*)buf, &size) == ERROR_SUCCESS && (type == REG_SZ || type == REG_MULTI_SZ || type == REG_EXPAND_SZ))
        {
            buf[size / sizeof(wchar_t)] = L'\0';
            wstring str = wstring(buf);
            delete[] buf;
            return str;
        }
    }
    return L"";
}

static void WriteColor(HKEY hKey, LPCTSTR name, const Color& c)
{
    const float vals[4] = {c.r, c.g, c.b, c.a};
    RegSetValueEx(hKey, name, NULL, REG_BINARY, (BYTE*)&vals, 4 * sizeof(float));
}

static void WriteInteger(HKEY hKey, LPCTSTR name, DWORD val)
{
    RegSetValueEx(hKey, name, NULL, REG_DWORD, (BYTE*)&val, sizeof(DWORD));
}

static void WriteVector3(HKEY hKey, LPCTSTR name, const Vector3& v)
{
    const float vals[3] = {v.x, v.y, v.z};
    RegSetValueEx(hKey, name, NULL, REG_BINARY, (BYTE*)&vals, 3 * sizeof(float));
}

static void WriteString(HKEY hKey, LPCTSTR name, const wstring& str)
{
    RegSetValueEx(hKey, name, NULL, REG_SZ, (BYTE*)str.c_str(), (int)str.size() * sizeof(wstring::size_type));
}

// =========================================
// Main Functions
// =========================================

COLORREF Config::ParseMPColor(const wstring& name) {
    vector<wstring> colors = {L"", L"", L""};
    size_t currentColor = 0;
    bool numberDone = false;
    for (wstring::const_iterator it = name.begin(); it < name.end(); it++) {
        if (isdigit(*it) && colors[currentColor].size() < 3) {
            colors[currentColor].push_back(*it);
            numberDone = false;
        }
        else if (currentColor < colors.size() - 1 && !numberDone) {
            ++currentColor;
            numberDone = true;
        }
    }

    return RGB(_wtoi(colors[0].c_str()), _wtoi(colors[1].c_str()), _wtoi(colors[2].c_str()));
}

RenderSettings Config::GetRenderSettings()
{
	HKEY hKey = NULL;
    const wstring path = wstring(REGISTRY_BASE_PATH) + L"\\Settings";
	RegOpenKeyEx(HKEY_CURRENT_USER, path.c_str(), 0, KEY_READ, &hKey);

    RenderSettings settings;
    settings.m_softShadows    = ReadInteger(hKey, L"SoftShadows",    true) != 0;
    settings.m_antiAlias      = ReadInteger(hKey, L"AntiAliasing",   true) != 0;
    settings.m_shaderDetail   = ReadInteger(hKey, L"ShaderDetail",   SHADERDETAIL_DX9);
    settings.m_bloom          = ReadInteger(hKey, L"Bloom",          false) != 0;
    settings.m_heatDistortion = ReadInteger(hKey, L"HeatDistortion", true ) != 0;
    settings.m_heatDebug      = ReadInteger(hKey, L"HeatDebug",      false) != 0;
    settings.m_shadowDebug    = ReadInteger(hKey, L"ShadowDebug",    false) != 0;
    ReadColor(hKey, L"ModelColor", &settings.m_modelColor);
    RegCloseKey(hKey);
    return settings;
}

void Config::SetRenderSettings(const RenderSettings& settings)
{
	HKEY hKey;
    const wstring path = wstring(REGISTRY_BASE_PATH) + L"\\Settings";
	if (RegCreateKeyEx(HKEY_CURRENT_USER, path.c_str(), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS)
	{
        WriteInteger(hKey, L"SoftShadows",    settings.m_softShadows);
        WriteInteger(hKey, L"AntiAliasing",   settings.m_antiAlias);
        WriteInteger(hKey, L"ShaderDetail",   settings.m_shaderDetail);
        WriteInteger(hKey, L"Bloom",          settings.m_bloom);
        WriteInteger(hKey, L"HeatDistortion", settings.m_heatDistortion);
        WriteInteger(hKey, L"HeatDebug",      settings.m_heatDebug);
        WriteInteger(hKey, L"ShadowDebug",    settings.m_shadowDebug);
        WriteColor(hKey,   L"ModelColor",     settings.m_modelColor);
    }
}

// Get the environment
void Config::GetEnvironment(Environment* env)
{
	HKEY hKey;
    const wstring path = wstring(REGISTRY_BASE_PATH) + L"\\Settings";
	if (RegOpenKeyEx(HKEY_CURRENT_USER, path.c_str(), 0, KEY_READ, &hKey) == ERROR_SUCCESS)
	{
        Vector3 wind = Vector3(env->m_wind.heading, 0.0f);
        ReadColor(hKey, L"Ambient",    &env->m_ambient);
        ReadColor(hKey, L"Background", &env->m_clearColor);
        ReadColor(hKey, L"Shadow",     &env->m_shadow);
        ReadColor(hKey, L"Specular",   &env->m_specular);
        ReadColor(hKey, L"Light0Diffuse", &env->m_lights[0].m_color);
        ReadColor(hKey, L"Light1Diffuse", &env->m_lights[1].m_color);
        ReadColor(hKey, L"Light2Diffuse", &env->m_lights[2].m_color);
        ReadVector3(hKey, L"Light0Dir", &env->m_lights[0].m_direction);
        ReadVector3(hKey, L"Light1Dir", &env->m_lights[1].m_direction);
        ReadVector3(hKey, L"Light2Dir", &env->m_lights[2].m_direction);
        ReadVector3(hKey, L"Wind",      &wind);
        env->m_wind.heading = wind.zAngle();
        RegCloseKey(hKey);
    }
}

// Set the environment
void Config::SetEnvironment(const Environment& env)
{
	HKEY hKey;
    const wstring path = wstring(REGISTRY_BASE_PATH) + L"\\Settings";
	if (RegCreateKeyEx(HKEY_CURRENT_USER, path.c_str(), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS)
	{
        Vector3 wind = Vector3(env.m_wind.heading, 0.0f);

        WriteColor(hKey, L"Ambient",    env.m_ambient);
        WriteColor(hKey, L"Background", env.m_clearColor);
        WriteColor(hKey, L"Shadow",     env.m_shadow);
        WriteColor(hKey, L"Specular",   env.m_specular);
        WriteColor(hKey, L"Light0Diffuse", env.m_lights[0].m_color);
        WriteColor(hKey, L"Light1Diffuse", env.m_lights[1].m_color);
        WriteColor(hKey, L"Light2Diffuse", env.m_lights[2].m_color);
        WriteVector3(hKey, L"Light0Dir", env.m_lights[0].m_direction);
        WriteVector3(hKey, L"Light1Dir", env.m_lights[1].m_direction);
        WriteVector3(hKey, L"Light2Dir", env.m_lights[2].m_direction);
        WriteVector3(hKey, L"Wind",      wind);
        RegCloseKey(hKey);
    }
}

void Config::GetHistory(map<ULONGLONG, wstring>& history)
{
	history.clear();

	HKEY hKey;
	if (RegOpenKeyEx(HKEY_CURRENT_USER, REGISTRY_BASE_PATH, 0, KEY_READ | KEY_WRITE, &hKey) == ERROR_SUCCESS)
	{
		LONG error;
		for (int i = 0;; i++)
		{
			wchar_t name[256] = {L'\0'};
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
			map<ULONGLONG,wstring>::const_reverse_iterator p = history.rbegin();
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

// Adds this filename to the history
void Config::AddToHistory(const wstring& name)
{
	// Get the current date & time
	FILETIME   filetime;
	SYSTEMTIME systime;
	GetSystemTime(&systime);
	SystemTimeToFileTime(&systime, &filetime);

	HKEY hKey;
	if (RegCreateKeyEx(HKEY_CURRENT_USER, REGISTRY_BASE_PATH, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS)
	{
		RegSetValueEx(hKey, name.c_str(), 0, REG_BINARY, (BYTE*)&filetime, sizeof(FILETIME));
		RegCloseKey(hKey);
	}
}

Environment Config::GetDefaultEnvironment()
{
    Environment env;
    env.m_ambient      = Color(0.1f, 0.1f, 0.1f, 1);
	env.m_specular     = Color(1.0f, 1.0f, 1.0f, 1);
	env.m_shadow       = Color(0.5f, 0.5f, 0.5f, 1);
	env.m_wind.speed   = 1.0f;
    env.m_wind.heading = 0.0f;
    env.m_clearColor   = Color(0.0f, 0.0f, 0.0f,1);
    env.m_lights[LT_SUN].  m_color     = Color(1.00f, 1.00f, 1.0f, 0.5f);
    env.m_lights[LT_FILL1].m_color     = Color(0.25f, 0.25f, 0.5f, 0.5f);
    env.m_lights[LT_FILL2].m_color     = Color(0.25f, 0.25f, 0.5f, 0.5f);
    env.m_lights[LT_SUN].  m_direction = -Vector3(ToRadians(  0 - 90), ToRadians( 45));
    env.m_lights[LT_FILL1].m_direction = -Vector3(ToRadians(210 - 90), ToRadians(-10));
    env.m_lights[LT_FILL2].m_direction = -Vector3(ToRadians(120 - 90), ToRadians(-10));
    env.m_gravity = Vector3(0,0,-1);
    return env;
}

GameMod Config::GetDefaultGameMod()
{
	HKEY hKey;
    GameMod gm;
    const wstring path = wstring(REGISTRY_BASE_PATH) + L"\\Settings";
	if (RegOpenKeyEx(HKEY_CURRENT_USER, path.c_str(), 0, KEY_READ, &hKey) == ERROR_SUCCESS)
	{
        gm.m_game = (GameID)ReadInteger(hKey, L"GameMod_Game", GID_UNKNOWN);
        gm.m_mod  = ReadString(hKey, L"GameMod_Mod");
        gm.m_steamId  = ReadString(hKey, L"GameMod_SteamId");
        RegCloseKey(hKey);
    }
    return gm;
}

void Config::SetDefaultGameMod(const GameMod& gm)
{
	HKEY hKey;
    const wstring path = wstring(REGISTRY_BASE_PATH) + L"\\Settings";
	if (RegOpenKeyEx(HKEY_CURRENT_USER, path.c_str(), 0, KEY_WRITE, &hKey) == ERROR_SUCCESS)
	{
        WriteInteger(hKey, L"GameMod_Game", gm.m_game);
        WriteString(hKey, L"GameMod_Mod", gm.m_mod);
        WriteString(hKey, L"GameMod_SteamId", gm.m_steamId);
        RegCloseKey(hKey);
    }
}
