#pragma once
#include <stdexcept>
#include <Windows.h>

namespace HydraCore
{
    class Win32Exception : public std::runtime_error
    {
    public:
        Win32Exception();
        Win32Exception(DWORD errorCode);
    };
}
