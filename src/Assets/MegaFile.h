#ifndef MEGAFILE_H
#define MEGAFILE_H

#include "Files.h"
#include <map>
#include <vector>

namespace Alamo {

class MegaFile : public IObject
{
    struct FileInfo;
    typedef std::multimap<unsigned long, FileInfo*> FileIndex;
    typedef std::vector<FileInfo> FileList;

    ptr<IFile> m_file;
    FileList   m_files;
    FileIndex  m_fileIndex;

public:
    size_t GetNumFiles() const;
    const std::string& GetFilename(size_t index) const;
    ptr<IFile> GetFile(size_t index) const;
    ptr<IFile> GetFile(const std::string& path) const;

    MegaFile(ptr<IFile> file);
};

}
#endif