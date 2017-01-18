#ifndef LOG_H
#define LOG_H

#include <vector>
#include <string>
#include <ctime>

namespace Log
{
    enum LineType
    {
        LT_INFO,
        LT_ERROR
    };

    struct Line
    {
        time_t      time;
        LineType    type;
        std::string text;
    };
	
	typedef void (*CALLBACK_FUNC)(const std::vector<Line>& newlines, void* data);

	void WriteInfo(const char* format, ...);
	void WriteError(const char* format, ...);
	size_t RegisterCallback(CALLBACK_FUNC callback, void* data);
	void UnregisterCallback(size_t callback);
    void Uninitialize();
};

#endif