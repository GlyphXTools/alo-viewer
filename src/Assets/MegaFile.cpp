#include "Assets/MegaFile.h"
#include "General/ExactTypes.h"
#include "General/Exceptions.h"
#include "General/Utils.h"
#include <vector>
using namespace Alamo;
using namespace std;

// MegaFile parsers
#pragma pack(1)
struct MEGHEADER
{
	uint32_t numStrings;
	uint32_t numFiles;
};

struct MEGFILEINFO
{
	uint32_t crc;
	uint32_t index;
	uint32_t size;
	uint32_t start;
	uint32_t nameIndex;
};
#pragma pack()

// Represents a file in a MegaFile
struct MegaFile::FileInfo
{
    string        m_name;
	unsigned long m_start;
	unsigned long m_size;
};

size_t MegaFile::GetNumFiles() const
{
    return m_files.size();
}

const string& MegaFile::GetFilename(size_t index) const
{
    return m_files[index].m_name;
}

ptr<IFile> MegaFile::GetFile(size_t index) const
{
    return new SubFile(m_file, m_files[index].m_name, m_files[index].m_start, m_files[index].m_size);
}

ptr<IFile> MegaFile::GetFile(const std::string& path) const
{
    // Uppercase and replace slash with backslash
    string name = Uppercase(path);
    string::size_type ofs = 0;
    while ((ofs = name.find_first_of("/", ofs)) != string::npos)
    {
        name[ofs] = '\\';
    }
    
    unsigned long crc = crc32(name.c_str(), name.length());
    FileIndex::const_iterator p = m_fileIndex.lower_bound(crc);
    while (p != m_fileIndex.end() && p->first == crc)
    {
        if (p->second->m_name == name)
        {
            // Found it
            return new SubFile(m_file, p->second->m_name, p->second->m_start, p->second->m_size);
        }
        p++;
    }

    // File not found
    return NULL;
}

MegaFile::MegaFile(ptr<IFile> file)
    : m_file(file)
{
	MEGHEADER header;
	if (file->read(&header, sizeof(header)) != sizeof(header))
	{
		throw ReadException();
	}

	//
	// Read filenames
	//
	unsigned long numStrings = letohl(header.numStrings);
	vector<string> filenames(numStrings);
	size_t pos = 0;
	for (unsigned long i = 0; i < numStrings; i++)
	{
		uint16_t length;
		if (file->read(&length, sizeof(uint16_t)) != sizeof(uint16_t))
		{
			throw ReadException();
		}

		Buffer<char> data(length);
		if (file->read(data, length) != length)
		{
			throw ReadException();
		}
		filenames[i] = string(data, length);
	}

	//
	// Read master index table
	//
	unsigned long numFiles  = letohl(header.numFiles);
	Buffer<MEGFILEINFO> info(numFiles);
	if (file->read(info, numFiles * sizeof(MEGFILEINFO)) != numFiles * sizeof(MEGFILEINFO))
	{
		throw ReadException();
	}

    // Unpack and store
    m_files.resize(numFiles);
	for (unsigned long i = 0; i < numFiles; i++)
	{
		FileInfo& fi = m_files[i];
        fi.m_name  = filenames[letohl(info[i].nameIndex)];
		fi.m_start = letohl(info[i].start);
		fi.m_size  = letohl(info[i].size);

        // Not found, add it
        m_fileIndex.insert(make_pair(letohl(info[i].crc), &fi));
	}
}
