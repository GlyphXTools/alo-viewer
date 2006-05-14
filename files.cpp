#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "files.h"
#include "exceptions.h"
using namespace std;

struct PhysicalFile::Handle
{
	HANDLE        hFile;
	unsigned long refcount;
};

//
// File class
//
File::File(const std::string& name)
{
	this->refcount = 1;
	this->name     = name;
}

void File::addRef()
{
	refcount++;
}

void File::release()
{
	if (--refcount == 0)
	{
		delete this;
	}
}

//
// PhysicalFile class
//
bool PhysicalFile::eof()
{
	return offset == getSize();
}

unsigned long PhysicalFile::getSize()
{
	return GetFileSize(handle->hFile, NULL);
}

void PhysicalFile::seek(unsigned long offset)
{
	this->offset = min(offset, getSize());
}

unsigned long PhysicalFile::tell()
{
	return offset;
}

unsigned long PhysicalFile::read( void* buffer, unsigned long count )
{
	DWORD read;
	SetFilePointer( handle->hFile, offset, NULL, FILE_BEGIN );
	if (!ReadFile(handle->hFile, buffer, count, &read, NULL))
	{
		read = 0;
	}
	offset += read;
	return read;
}

PhysicalFile::PhysicalFile(PhysicalFile& file) : File(file.getName())
{
	handle = file.handle;
	handle->refcount++;
}

PhysicalFile::PhysicalFile(const string& filename) : File(filename)
{
	offset = 0;
	handle = new Handle;

	handle->refcount = 1;
	handle->hFile    = CreateFile(filename.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (handle->hFile == INVALID_HANDLE_VALUE)
	{
		DWORD error = GetLastError();
		delete handle;
		if (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND)
		{
			throw FileNotFoundException(filename);
		}
		throw IOException("Unable to open file" + filename);
	}
}

PhysicalFile::~PhysicalFile()
{
	if (--handle->refcount == 0)
	{
		CloseHandle(handle->hFile);
		delete handle;
	}
}

//
// SubFile class
//
bool SubFile::eof()
{
	return offset == getSize();
}

unsigned long SubFile::getSize()
{
	return size;
}

void SubFile::seek(unsigned long offset)
{
	this->offset = min(offset, getSize());
}

unsigned long SubFile::tell()
{
	return offset;
}

unsigned long SubFile::read( void* buffer, unsigned long count )
{
	count = min(count, size);
	file->seek( start + offset );
	count = file->read(buffer, count);
	offset += count;
	return count;
}

SubFile::SubFile(File* file, unsigned long start, unsigned long size) : File(file->getName())
{
	this->file   = file;
	this->offset = 0;
	this->start  = start;
	this->size   = size;
	file->addRef();
}

SubFile::~SubFile()
{
	file->release();
}
