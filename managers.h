#ifndef MANAGERS_H
#define MANAGERS_H

#include <string>
#include <map>
#include <vector>
#include "files.h"
#include "types.h"

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
	std::vector<std::string> basepaths;
	std::vector<MegaFile*>	 megafiles;

public:
	File* getFile(const std::string& path);
	FileManager(const std::vector<std::string>& basepaths);
	~FileManager();
};

#endif