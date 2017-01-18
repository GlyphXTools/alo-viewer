#ifndef EXCEPTIONS_DX9_H
#define EXCEPTIONS_DX9_H

#include <windows.h>
#include <dxerr.h>

#include "General/Exceptions.h"

namespace Alamo {
namespace DirectX9 {

class DirectXException : public GraphicsException
{
public:
	DirectXException(const DWORD hError) : GraphicsException(DXGetErrorDescription( hError )) {}
    DirectXException(const std::wstring& error) : GraphicsException(error) {}
};

} }
#endif