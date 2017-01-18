#ifndef WAVEFILE_H
#define WAVEFILE_H

#include "Assets/Files.h"
#include <windows.h>
#include <mmreg.h>

namespace Alamo {

// Class for reading WAV files
class WaveFile : public IObject
{
    WAVEFORMATEX  m_format;
    char*         m_data;
    unsigned long m_size;

    ~WaveFile();
public:
    WAVEFORMATEX& GetFormat()     { return m_format; }
    const char*   GetData() const { return m_data;   }
    unsigned long GetSize() const { return m_size;   }

    WaveFile(ptr<IFile> file);
};

}

#endif