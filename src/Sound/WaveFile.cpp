#include "General/ExactTypes.h"
#include "General/Exceptions.h"
#include "Sound/WaveFile.h"
using namespace std;

namespace Alamo {

#pragma pack(1)
struct ROOTCHUNK
{
    uint32_t ID;
    uint32_t size;
    uint32_t type;
};

struct CHUNK
{
    uint32_t ID;
    uint32_t size;
};

struct WAVEFORMATEX_LE
{
    uint16_t wFormatTag; 
    uint16_t nChannels; 
    uint32_t nSamplesPerSec; 
    uint32_t nAvgBytesPerSec; 
    uint16_t nBlockAlign; 
    uint16_t wBitsPerSample; 
}; 
#pragma pack()

static const unsigned long RIFF_ID = 0x46464952;    // "RIFF"
static const unsigned long WAVE_ID = 0x45564157;    // "WAVE"
static const unsigned long FMT_ID  = 0x20746D66;    // "fmt "
static const unsigned long DATA_ID = 0x61746164;    // "data"

WaveFile::WaveFile(ptr<IFile> file)
{
    // Read and check root RIFF chunk
    ROOTCHUNK root;
    if (file->read(&root, sizeof root) != sizeof root)
    {
        throw ReadException();
    }
    
    if (letohl(root.ID) != RIFF_ID && letohl(root.size) != file->size() - sizeof root && letohl(root.type) != WAVE_ID)
    {
        throw BadFileException();
    }

    // Find format chunk
    CHUNK chunk = {0,0};
    do
    {
        if (file->read(&chunk, sizeof chunk) != sizeof chunk)
        {
            throw ReadException();
        }

        if (letohl(chunk.ID) != FMT_ID)
        {
            file->skip(letohl(chunk.size));
        }
    } while (letohl(chunk.ID) != FMT_ID);

    // Read format chunk
    WAVEFORMATEX_LE fmt;
    if (letohl(chunk.size) < sizeof(WAVEFORMATEX_LE))
    {
        throw BadFileException();
    }

    if (file->read(&fmt, sizeof(WAVEFORMATEX_LE)) < sizeof(WAVEFORMATEX_LE))
    {
        throw ReadException();
    }
    file->skip(letohl(chunk.size) - sizeof(WAVEFORMATEX_LE));

    m_format.wFormatTag      = letohs(fmt.wFormatTag);
    m_format.nChannels       = letohs(fmt.nChannels);
    m_format.nSamplesPerSec  = letohl(fmt.nSamplesPerSec);
    m_format.nAvgBytesPerSec = letohl(fmt.nAvgBytesPerSec);
    m_format.nBlockAlign     = letohs(fmt.nBlockAlign);
    m_format.wBitsPerSample  = letohs(fmt.wBitsPerSample);
    m_format.cbSize          = 0;

    // Find data chunk
    do
    {
        if (file->read(&chunk, sizeof chunk) != sizeof chunk)
        {
            throw ReadException();
        }
    
        if (letohl(chunk.ID) != DATA_ID)
        {
            file->skip(letohl(chunk.size));
        }
    } while (letohl(chunk.ID) != DATA_ID);

    // Read data chunk
    m_size = letohl(chunk.size);
    m_data = new char[m_size];
    if (file->read(m_data, m_size) != m_size)
    {
        delete[] m_data;
        throw ReadException();
    }
}

WaveFile::~WaveFile()
{
    delete[] m_data;
}

}