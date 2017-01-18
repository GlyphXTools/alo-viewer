#ifndef CHUNKFILE_H
#define CHUNKFILE_H

#include "Assets/Files.h"
#include "General/3DTypes.h"
#include <string>
#include <utility>

namespace Alamo
{

typedef long ChunkType;

struct ChunkHeader
{
	unsigned long type;
	unsigned long size;
};

struct MiniChunkHeader
{
	unsigned char type;
	unsigned char size;
};

class ChunkReader
{
	static const int MAX_CHUNK_DEPTH = 256;

	ptr<IFile>  m_file;
	long        m_position;
	long        m_size;
	long        m_offsets[ MAX_CHUNK_DEPTH ];
	long        m_miniSize;
	long        m_miniOffset;
	int         m_curDepth;

public:
	ChunkType   next();
	ChunkType   nextMini();
	void        skip();
	size_t      size();
	size_t      read(void* buffer, size_t size, bool check = true);
    size_t      tell() { return m_position; }
    bool        group() const { return m_size < 0; }

	float			readFloat();
	unsigned char   readByte();
	unsigned short	readShort();
	unsigned long	readInteger();
	std::string		readString();
	std::wstring	readWideString();
    Vector3         readVector3();
    Vector4         readVector4();
    Color           readColorRGB();
    Color           readColorRGBA();

	ChunkReader(ptr<IFile> file);
};

//
// TemplateReader is a class to allow for the reading
// of values from ChunkReaders by templates.
//
template <typename T> 
struct TemplateReader {
    static T read(ChunkReader& reader);
};
template <> struct TemplateReader<float>   { static float   read(ChunkReader& reader) { return reader.readFloat();     } };
template <> struct TemplateReader<Color>   { static Color   read(ChunkReader& reader) { return reader.readColorRGBA(); } };
template <> struct TemplateReader<Vector3> { static Vector3 read(ChunkReader& reader) { return reader.readVector3();   } };
template <> struct TemplateReader<Vector4> { static Vector4 read(ChunkReader& reader) { return reader.readVector4();   } };

template <typename T1, typename T2> 
struct TemplateReader< std::pair<T1,T2> > {
    static std::pair<T1,T2> read(ChunkReader& reader) {
        const T1 t1 = TemplateReader<T1>::read(reader);
        const T2 t2 = TemplateReader<T2>::read(reader);
        return std::make_pair(t1,t2);
    }
};

class ChunkWriter
{
	static const int MAX_CHUNK_DEPTH = 256;
	
	template <typename HdrType>
	struct ChunkInfo
	{
		HdrType       hdr;
		unsigned long offset;
	};

	IFile*                     m_file;
	ChunkInfo<ChunkHeader>     m_chunks[ MAX_CHUNK_DEPTH ];
	ChunkInfo<MiniChunkHeader> m_miniChunk;
	int                        m_curDepth;

public:
	void beginChunk(ChunkType type);
	void beginMiniChunk(ChunkType type);
	void endChunk();
	void write(const void* buffer, size_t size);
	void writeString(const std::string& str);

	ChunkWriter(ptr<IFile> file);
};

}
#endif