#ifndef NDEBUG
#include <iostream>
#endif
#include <stdarg.h>
#include "log.h"
using namespace std;

namespace Log
{

static vector<string>						strings;
static vector<pair<CALLBACK_FUNC,void*> >	callbacks;

const vector<string>& getLines()
{
	return strings;
}

void addCallback(CALLBACK_FUNC callback, void* data)
{
	callbacks.push_back(make_pair(callback, data));
}

void Write(const char* format, ...)
{
	va_list args;
	va_start(args, format);
	
	// Format string
	size_t size = _vscprintf(format, args);
	string str(size, ' ');
	vsprintf((char*)str.c_str(), format, args);
	
	int diff = 1;

	if (strings.size() == 0)
	{
		strings.push_back("");
	}

	// Add the lines
	for (size_t end = 0, ofs = 0; end != string::npos; ofs = end + 1)
	{
		end = str.find_first_of("\n", ofs);
		string line = (end == string::npos) ? str.substr(ofs) : str.substr(ofs, end - ofs);
#ifndef NDEBUG
		cout << line;
#endif
		strings.back() += line;
		if (end != string::npos)
		{
#ifndef NDEBUG
			cout << endl;
#endif
			strings.push_back("");
			diff++;
		}
	}

	for (vector<pair<CALLBACK_FUNC, void*> >::const_iterator p = callbacks.begin(); p != callbacks.end(); p++)
	{
		p->first(diff, p->second);
	}

	va_end(args);
};

}