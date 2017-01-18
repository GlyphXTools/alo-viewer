#include <windows.h>
#include "General/Exceptions.h"
#include "General/Utils.h"
#include "Assets/Files.h"
using namespace Alamo;
using namespace std;

/*
 * IFile class
 */
IFile::IFile(const std::wstring& name)
    : m_name(name)
{
}

/*
 * PhysicalFile class
 */
bool PhysicalFile::eof() const
{
	return tell() == size();
}

size_t PhysicalFile::size() const
{
	return GetFileSize(m_hFile, NULL);
}

unsigned long PhysicalFile::tell() const
{
	return m_offset;
}

unsigned long PhysicalFile::seek(unsigned long pos)
{
	return m_offset = pos;
}

unsigned long PhysicalFile::skip(long count)
{
	return m_offset = min(max(m_offset + count, 0), (unsigned long)size() - 1);
}

size_t PhysicalFile::read(void* buffer, size_t size)
{
	DWORD read;
	SetFilePointer(m_hFile, m_offset, NULL, FILE_BEGIN);
	if (!ReadFile(m_hFile, buffer, (DWORD)size, &read, NULL))
	{
		throw ReadException();
	}
	m_offset += read;
	return read;
}

size_t PhysicalFile::write(const void* buffer, size_t size)
{
	DWORD written;
	SetFilePointer(m_hFile, m_offset, NULL, FILE_BEGIN);
	if (!WriteFile(m_hFile, buffer, (DWORD)size, &written, NULL))
	{
		throw WriteException();
	}
	m_offset += written;
	return written;
}

PhysicalFile::PhysicalFile(const wstring& filename)
    : IFile(filename)
{
	m_hFile = CreateFile(filename.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (m_hFile == INVALID_HANDLE_VALUE)
	{
		DWORD error = GetLastError();
		if (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND)
		{
			throw FileNotFoundException(filename);
		}
		throw IOException(L"Unable to open file:\n" + filename);
	}
	m_offset = 0;
}

PhysicalFile::~PhysicalFile()
{
	CloseHandle(m_hFile);
}

/*
 * SubFile class
 */
bool SubFile::eof() const
{
	return tell() == size();
}

size_t SubFile::size() const
{
	return m_size;
}

unsigned long SubFile::tell() const
{
	return m_offset;
}

unsigned long SubFile::seek(unsigned long pos)
{
	return m_offset = pos;
}

unsigned long SubFile::skip(long count)
{
	return m_offset = min(max(m_offset + count, 0), (unsigned long)size() - 1);
}

size_t SubFile::read(void* buffer, size_t size)
{
	m_file->seek(m_start + m_offset);
    size = min(size, m_size - m_offset);
	size_t read = m_file->read(buffer, size);
	m_offset += (unsigned long)read;
	return read;
}

size_t SubFile::write(const void* buffer, size_t size)
{
	m_file->seek(m_start + m_offset);
	size_t written = m_file->write(buffer, size);
	m_offset += (unsigned long)written;
	return written;
}

SubFile::SubFile(IFile* file, const std::string& subfilename, unsigned long start, unsigned long size)
    : IFile(file->name() + L"|" + AnsiToWide(subfilename))
{
	m_file   = file;
	m_start  = start;
	m_size   = size;
	m_offset = 0;
	m_file->AddRef();
}

SubFile::~SubFile()
{
	SAFE_RELEASE(m_file);
}
