#ifndef NDEBUG
#include <iostream>
#endif
#include <stdarg.h>
#include <stack>
#include "log.h"
using namespace std;

namespace Log
{

static vector<Line>					      g_lines;
static vector<pair<CALLBACK_FUNC,void*> > g_callbacks;
static stack<size_t>                      g_freeCallbacks;

void Uninitialize()
{
    g_lines.clear();
    g_callbacks.clear();
    while (!g_freeCallbacks.empty()) {
        g_freeCallbacks.pop();
    }
}

size_t RegisterCallback(CALLBACK_FUNC callback, void* data)
{
    if (g_freeCallbacks.empty())
    {
        g_freeCallbacks.push(g_callbacks.size());
        g_callbacks.push_back(make_pair<CALLBACK_FUNC,void*>(NULL,NULL));
    }
    size_t entry = g_freeCallbacks.top();
    g_callbacks[entry].first  = callback;
    g_callbacks[entry].second = data;
    g_freeCallbacks.pop();
    return entry;
}

void UnregisterCallback(size_t callback)
{
    if (g_callbacks[callback].first != NULL)
    {
        g_callbacks[callback].first = NULL;
        g_freeCallbacks.push(callback);
    }
}

static void Write(LineType type, const char* format, va_list args)
{
	// Format string
	size_t size = _vscprintf(format, args);
	string str(size, ' ');
	vsprintf((char*)str.c_str(), format, args);
    
    // Make sure the string ends in a newline
    if (*str.rbegin() != '\n') {
        str += "\n";
    }
	
    Line line;
    line.time = time(NULL);
    line.type = type;

	// Add the lines
	vector<Line> newlines;
	for (size_t end, ofs = 0; (end = str.find_first_of("\n", ofs)) != string::npos; ofs = end + 1)
	{
		line.text = str.substr(ofs, end - ofs);

        g_lines.push_back(line);
        newlines.push_back(line);
#ifndef NDEBUG
		cout << line.text << endl;
#endif
	}

	for (vector<pair<CALLBACK_FUNC, void*> >::const_iterator p = g_callbacks.begin(); p != g_callbacks.end(); p++)
	{
		p->first(newlines, p->second);
	}
};

void WriteInfo(const char* format, ...)
{
	va_list args;
	va_start(args, format);
    Write(LT_INFO, format, args);
	va_end(args);
}

void WriteError(const char* format, ...)
{
	va_list args;
	va_start(args, format);
    Write(LT_ERROR, format, args);
	va_end(args);
}

}