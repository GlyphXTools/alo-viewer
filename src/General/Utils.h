#ifndef UTILS_H
#define UTILS_H

#include <cstdarg>
#include <string>

// General purpose utility functions and classes
namespace Alamo
{

template <typename T>
struct range
{
    T min;
    T max;
    
    void normalize() {
        // Swap min and max if necessary
        if (min > max) {
            T tmp = max;
            max = min;
            min = tmp;
        }
    }

    range(const T& _min, const T& _max) { this->min = _min; this->max = _max; }
    range() {}
};

// Calculates the CRC-32 checksum of the specified block of data
unsigned long crc32(const void* data, size_t size);

// Converts an ANSI string to a wide (UCS-2) string and back
std::wstring AnsiToWide(const char* cstr);
std::string  WideToAnsi(const wchar_t* cstr, const char* defChar = " ");
static std::wstring AnsiToWide(const std::string& str)                             { return AnsiToWide(str.c_str()); }
static std::string  WideToAnsi(const std::wstring& str, const char* defChar = " ") { return WideToAnsi(str.c_str(), defChar); }

std::string  Uppercase(const char* str);
std::wstring Uppercase(const wchar_t* str);
static std::string  Uppercase(const std::string&  str) { return Uppercase(str.c_str()); }
static std::wstring Uppercase(const std::wstring& str) { return Uppercase(str.c_str()); }

std::wstring FormatString(const wchar_t* format, ...);

std::string  Trim(const char* str, const char* delim = " \r\n\t\v\f");
std::wstring Trim(const wchar_t* str, const wchar_t* delim = L" \r\n\t\v\f");
static std::string  Trim(const std::string&  str) { return Trim(str.c_str()); }
static std::wstring Trim(const std::wstring& str) { return Trim(str.c_str()); }

}

#endif