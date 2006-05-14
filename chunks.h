#ifndef CHUNKS_H
#define CHUNKS_H

#include <vector>
#include "types.h"
#include "files.h"

// This represents a Chunk
class Chunk
{
    uint32_t type;
	uint32_t size;
	unsigned long start;

	File* stream;

public:
	bool          isGroup()   const { return (size & 0x80000000) != 0; }
	unsigned long getSize()   const { return size & 0x7FFFFFFF; }
	unsigned long getType()   const { return type;   }
	unsigned long getStart()  const { return start;  }
	File*         getStream() const { return stream; }

	char* getData();
	Chunk(File* input);
	~Chunk();
};

class MiniChunk
{
    uint8_t type;
	uint8_t size;
	unsigned long start;
	File* stream;

public:
	unsigned char getSize()   const { return size; }
	unsigned char getType()   const { return type; }
	unsigned long getStart()  const { return start; }
	File*         getStream() const { return stream; }

	char* getData();
	MiniChunk(File* input);
	~MiniChunk();
};

class MiniChunks
{
	std::vector<std::pair<std::pair<unsigned char, unsigned char>, char*> > chunks;
	char* data;

public:
	unsigned long getNumChunks() const
	{
		return (unsigned long)chunks.size();
	}

	const char* getChunk(unsigned char type, unsigned char& size) const;
	const char* getChunk(int index, unsigned char& type, unsigned char& size) const;
	MiniChunks(Chunk& input);
	~MiniChunks();
};

#endif