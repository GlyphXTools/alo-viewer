#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>
#include "dxerr9.h"
#include "types.h"

class Exception
{
	std::string  filename;
	std::string  message;
	int          line;

public:
	const std::string& getFilename() const
	{
		return filename;
	}

	const std::string& getMessage() const
	{
		return message;
	}

	int getLine() const
	{
		return line;
	}

	Exception( const std::string _filename, int _line, const std::string _message = "")
		: filename(_filename), line(_line), message(_message) {}
	virtual ~Exception() {}
};

class IOException : public Exception
{
public:
	IOException(const std::string filename, int line, const std::string message) : Exception(filename, line, message) {}
	~IOException() {}
};

class OutOfMemoryException : public Exception
{
public:
	OutOfMemoryException(const std::string filename, int line) : Exception(filename, line) {}
	~OutOfMemoryException() {}
};

class LoadException : public Exception
{
public:
	LoadException( const std::string filename, int line, const std::string message = "")
		: Exception(filename, line, message) {}
	~LoadException() {}
};

class D3DException : public Exception
{
public:
	D3DException( const std::string filename, int line, const DWORD hError)
		: Exception(filename, line, DXGetErrorDescription9( hError )) {}
	~D3DException() {}
};

class ParseException : public Exception
{
public:
	ParseException( const std::string filename, int line, const std::string message)
		: Exception(filename, line, message) {}
	~ParseException() {}
};

class FileNotFoundException : public IOException
{
public:
	FileNotFoundException( const std::string filename, int line, const std::string file)
		: IOException(filename, line, "Unable to find file:\n" + file) {}
};

class BadFileException : public IOException
{
public:
	BadFileException( const std::string filename, int line, const std::string file)
		: IOException(filename, line, "Bad or corrupted file:\n" + file) {}
};

//
// Define the instances without file information
//
#define Exception(msg)				(Exception(__FILE__, __LINE__, msg))
#define OutOfMemoryException()		(OutOfMemoryException(__FILE__, __LINE__))
#define IOException(msg)			(IOException(__FILE__, __LINE__, msg))
#define LoadException(msg)			(LoadException(__FILE__, __LINE__, msg))
#define D3DException(err)			(D3DException(__FILE__, __LINE__, err))
#define ParseException(msg)			(ParseException(__FILE__, __LINE__, msg))
#define FileNotFoundException(file)	(FileNotFoundException(__FILE__, __LINE__, file))
#define BadFileException(file)		(BadFileException(__FILE__, __LINE__, file))

#endif