#include <windows.h>
#include <algorithm>
#include "General/Utils.h"
using namespace std;

namespace Alamo
{

unsigned long crc32(const void *data, size_t size)
{
    static unsigned long lookupTable[256];
    static bool          validTable = false;

	if (!validTable)
	{
        // Initialize table
	    for (int i = 0; i < 256; i++)
        {
		    unsigned long crc = i;
            for (int j = 0; j < 8; j++)
		    {
			    crc = (crc & 1) ? (crc >> 1) ^ 0xEDB88320 : (crc >> 1);
		    }
            lookupTable[i] = crc & 0xFFFFFFFF;
	    }
	    validTable = true;
	}

	unsigned long crc = 0xFFFFFFFF;
	for (size_t j = 0; j < size; j++)
	{
		crc = ((crc >> 8) & 0x00FFFFFF) ^ lookupTable[ (crc ^ ((const char*)data)[j]) & 0xFF ];
	}
	return crc ^ 0xFFFFFFFF;
}

// Convert an ANSI string to a wide (UCS-2) string
wstring AnsiToWide(const char* cstr)
{
	int size = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, cstr, -1, NULL, 0);
	WCHAR* wstr = new WCHAR[size];
	try
	{
		MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, cstr, -1, wstr, size);
		wstring result(wstr);
		delete[] wstr;
		return result;
	}
	catch (...)
	{
		delete[] wstr;
		throw;
	}
}

// Convert  a wide (UCS-2) string to an an ANSI string
string WideToAnsi(const wchar_t* cstr, const char* defChar)
{
	int size = WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_NO_BEST_FIT_CHARS | WC_DEFAULTCHAR, cstr, -1, NULL, 0, defChar, NULL);
	CHAR* str = new CHAR[size];
	try
	{
		WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_NO_BEST_FIT_CHARS | WC_DEFAULTCHAR, cstr, -1, str, size, defChar, NULL);
		string result(str);
		delete[] str;
		return result;
	}
	catch (...)
	{
		delete[] str;
		throw;
	}
}

// Uppercase strings
string Uppercase(const char* str)
{
	string result = str;
	transform(result.begin(), result.end(), result.begin(), toupper);
	return result;
}

wstring Uppercase(const wchar_t* str)
{
	wstring result = str;
	transform(result.begin(), result.end(), result.begin(), toupper);
	return result;
}

string Trim(const char* str, const char* delim)
{
    string result = str;
    string::size_type p = result.find_first_not_of(delim);
    if (p != string::npos)
    {
        string::size_type q = result.find_last_not_of(delim);
        return result.substr(p, q - p + 1);
    }
    return "";
}

wstring Trim(const wchar_t* str, const wchar_t* delim)
{
    wstring result = str;
    wstring::size_type p = result.find_first_not_of(delim);
    if (p != wstring::npos)
    {
        wstring::size_type q = result.find_last_not_of(delim);
        return result.substr(p, q - p + 1);
    }
    return L"";
}

wstring GetWindowText(HWND hWnd)
{
    wstring str(L"\0", GetWindowTextLength(hWnd) + 1);
    GetWindowText(hWnd, (LPTSTR)str.c_str(), (int)str.length());
    return str;
}

void GetParentRect(HWND hWnd, RECT* pRect)
{
    HWND hParent = GetParent(hWnd);
    GetWindowRect(hWnd, pRect);
    POINT topleft     = {pRect->left,  pRect->top};
    POINT bottomright = {pRect->right, pRect->bottom};
    ScreenToClient(hParent, &topleft);
    ScreenToClient(hParent, &bottomright);
    pRect->left   = topleft.x;
    pRect->top    = topleft.y;
    pRect->right  = bottomright.x;
    pRect->bottom = bottomright.y;
}

static wstring FormatString(const wchar_t* format, va_list args)
{
    int      n   = _vscwprintf(format, args);
    wchar_t* buf = new wchar_t[n + 1];
    try
    {
        vswprintf(buf, n + 1, format, args);
        wstring str = buf;
        delete[] buf;
        return str;
    }
    catch (...)
    {
        delete[] buf;
        throw;
    }
}

wstring FormatString(const wchar_t* format, ...)
{
    va_list args;
    va_start(args, format);
    wstring str = FormatString(format, args);
    va_end(args);
    return str;
}

wstring LoadString(UINT id, ...)
{
    int len = 256;
    TCHAR* buf = new TCHAR[len];
    try
    {
        while (::LoadString(NULL, id, buf, len) >= len - 1)
        {
            delete[] buf;
            buf = NULL;
            buf = new TCHAR[len *= 2];
        }
        va_list args;
        va_start(args, id);
        wstring str = FormatString(buf, args);
        va_end(args);
        delete[] buf;
        return str;
    }
    catch (...)
    {
        delete[] buf;
        throw;
    }
}

BOOL ScreenToClient(HWND hWnd, LPRECT rect)
{
    POINT a = {rect->left,  rect->top};
    POINT b = {rect->right, rect->bottom};
    if (!ScreenToClient(hWnd, &a)) return FALSE;
    if (!ScreenToClient(hWnd, &b)) return FALSE;
    rect->left  = a.x; rect->top    = a.y;
    rect->right = b.x; rect->bottom = b.y;
    return TRUE;
}

}