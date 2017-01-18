#ifndef EXCEPTIONS_DS8_H
#define EXCEPTIONS_DS8_H

#include <windows.h>
#include <dxerr.h>

#include "General/Exceptions.h"

namespace Alamo {
namespace DirectSound8 {

class DirectSoundException : public SoundException
{
public:
	DirectSoundException(const DWORD hError) : SoundException(DXGetErrorDescription( hError )) {}
    DirectSoundException(const std::wstring& error) : SoundException(error) {}
};

} }
#endif