#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#include <stdexcept>
#include <string>

namespace Alamo
{

class wexception
{
	std::wstring m_what;
public:
	const wchar_t* what() const { return m_what.c_str(); }
	wexception(const std::wstring& message) : m_what(message) {}
};

class wruntime_error : public wexception
{
public:
	wruntime_error(const std::wstring& message) : wexception(message) {}
};

class IOException : public wruntime_error
{
public:
	IOException(const std::wstring& message) : wruntime_error(message) {}
};

class FileNotFoundException : public IOException
{
public:
	FileNotFoundException(const std::wstring& filename) : IOException(L"Unable to find file:\n" + filename) {}
};

class ReadException : public IOException
{
public:
	ReadException() : IOException(L"Unable to read file") {}
};

class WriteException : public IOException
{
public:
	WriteException() : IOException(L"Unable to write file") {}
};

class BadFileException : public wruntime_error
{
public:
	BadFileException() : wruntime_error(L"Invalid or corrupt file") {}
};

class ParseException : public std::runtime_error
{
public:
	ParseException(const std::string& message) : std::runtime_error(message) {}
};

class GraphicsException : public wruntime_error
{
public:
	GraphicsException(const std::wstring& message) : wruntime_error(message) {}
};

class SoundException : public wruntime_error
{
public:
	SoundException(const std::wstring& message) : wruntime_error(message) {}
};

class MathException : public wruntime_error
{
public:
	MathException(const std::wstring& message) : wruntime_error(message) {}
};

class ArgumentException : public wruntime_error
{
public:
	ArgumentException(const std::wstring& message) : wruntime_error(message) {}
};

#ifndef NDEBUG
// Debug Verify. Prints the which verification failed.
static inline void __Verify(bool expr, const char* filename, int line) {
	if (!expr) {
        fprintf(stderr, "Bad file at %s:%d\n", filename, line);
		throw BadFileException();
	}
}
#define Verify(expr) __Verify(expr, __FILE__, __LINE__);
#else
// Release Verify. Just throws the exception.
static void inline Verify(bool expr) {
	if (!expr) {
		throw BadFileException();
	}
}
#endif


}

#endif