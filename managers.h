#ifndef MANAGERS_H
#define MANAGERS_H

#include <string>
#include <map>
#include <vector>
#include "types.h"

class File
{
	unsigned long refcount;
	std::string   name;

protected:
	virtual ~File() {}

public:
	void addRef();
	void release();

	const std::string getName() const { return name; }
	virtual bool              eof() = 0;
	virtual unsigned long     getSize() = 0;
	virtual void              seek(unsigned long offset) = 0;
	virtual unsigned long     tell() = 0;
	virtual unsigned long     read( void* buffer, unsigned long count ) = 0;

	File(const std::string& name);
};

class PhysicalFile : public File
{
	struct Handle
	{
		HANDLE        hFile;
		unsigned long refcount;
	};

	Handle*       handle;
	unsigned long offset;
	std::string   name;

	~PhysicalFile();
public:
	const std::string& getName() const { return name; }
	bool               eof();
	unsigned long      getSize();
	void               seek(unsigned long offset);
	unsigned long      tell();
	unsigned long      read( void* buffer, unsigned long count );

	PhysicalFile(PhysicalFile& file);
	PhysicalFile(const std::string& filename);
	PhysicalFile();
};

class SubFile : public File
{
	File*         file;
	unsigned long offset;
	unsigned long start;
	unsigned long size;

	~SubFile();
public:
	bool          eof();
	unsigned long getSize();
	void          seek(unsigned long offset);
	unsigned long tell();
	unsigned long read( void* buffer, unsigned long count );

	SubFile(File* file, unsigned long start, unsigned long size);
};

class MegaFile
{
	struct FileInfo
	{
		uint32_t crc;
		uint32_t index;
		uint32_t size;
		uint32_t start;
		uint32_t nameIndex;
	};

	File* file;
	std::vector<FileInfo>      files;
	std::vector<std::string>   filenames;

public:
	File*              getFile(std::string path) const;
	File*              getFile(int index) const;
	const std::string& getFilename(int index) const;
	unsigned int       getNumFiles() const { return (unsigned int)files.size(); }

	MegaFile(File* file);
	~MegaFile();
};

//
// Manager interfaces
//
class IFileManager
{
public:
	virtual File* getFile(const std::string& path) = 0;
};

class ITextureManager
{
public:
	virtual IDirect3DTexture9* getTexture(IDirect3DDevice9* pDevice, std::string name) = 0;
};

class IEffectManager
{
public:
	virtual ID3DXEffect* getEffect(IDirect3DDevice9* pDevice, std::string name) = 0;
};

//
// File Manager
//
class FileManager : public IFileManager
{
	std::string                     basepath;
	std::map<std::string,MegaFile*> megafiles;

public:
	File* getFile(std::string megafile, const std::string& path);
	File* getFile(const std::string& path);
	FileManager(const std::string& basepath = ".");
	~FileManager();
};

#endif