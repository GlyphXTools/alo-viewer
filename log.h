#ifndef LOG_H
#define LOG_H

#include <vector>
#include <string>

namespace Log
{
	typedef void (*CALLBACK_FUNC)(int, void*);
	
	void Write(const char* format, ...);
	const std::vector<std::string>& getLines();
	void addCallback(CALLBACK_FUNC callback, void* data);
};

#endif