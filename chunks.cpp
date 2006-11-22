#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "chunks.h"
#include "exceptions.h"
using namespace std;

char* Chunk::getData()
{
	char* data = new char[getSize()];
	stream->seek(0);
	if (stream->read(data, getSize()) != getSize())
	{
		delete[] data;
		throw IOException("Unable to read chunk data");
	}
	return data;
}

Chunk::Chunk(File* input)
{
	if (input->read( (char*)&type, sizeof type) != sizeof type ||
		input->read( (char*)&size, sizeof size) != sizeof size)
	{
		throw IOException("Unable to read chunk header");
	}
	type   = letohl(type);
	size   = letohl(size);
	start  = input->tell();
	stream = new SubFile(input, "", start, getSize());
}

Chunk::~Chunk()
{
	stream->release();
}

char* MiniChunk::getData()
{
	char* data = new char[getSize()];
	stream->seek(0);
	if (stream->read(data, getSize()) != getSize())
	{
		delete[] data;
		throw IOException("Unable to read chunk data");
	}
	return data;
}

MiniChunk::MiniChunk(File* input)
{
	if (input->read( (char*)&type, sizeof type) != sizeof type ||
		input->read( (char*)&size, sizeof size) != sizeof size)
	{
		throw IOException("Unable to read chunk header");
	}
	start  = input->tell();
	stream = new SubFile(input, "", start, getSize());
}

MiniChunk::~MiniChunk()
{
	stream->release();
}

const char* MiniChunks::getChunk(unsigned char type, unsigned char& size) const
{
	size = 0;
	for (size_t i = 0; i < chunks.size(); i++)
	{
		if (chunks[i].first.first == type)
		{
			size = chunks[i].first.second;
			return chunks[i].second;
		}
	}
	return NULL;
}

const char* MiniChunks::getChunk(int index, unsigned char& type, unsigned char& size) const
{
	type = chunks[index].first.first;
	size = chunks[index].first.second;
	return chunks[index].second;
}

MiniChunks::MiniChunks(Chunk& input)
{
	data = input.getData();
	unsigned long size = input.getSize();
	for (unsigned long i = 0; i < size; i += 2 + data[i + 1])
	{
		if (i + 2 + data[i + 1] > size)
		{
			// Invalid size
			throw BadFileException(input.getStream()->getName());
		}
		chunks.push_back(make_pair(make_pair(data[i+0], data[i+1]), data + i + 2));
	}
}

MiniChunks::~MiniChunks()
{
	delete[] data;
}
