#include "Win32Exception.h"

#include <sstream>

namespace HydraCore
{
    Win32Exception::Win32Exception() : Win32Exception(GetLastError())
    {
    }

    static std::string MakeExceptionMessage(DWORD errorCode)
    {
        std::ostringstream ret;
        ret << "Win32 Error: " << errorCode;
        return ret.str();
    }

    Win32Exception::Win32Exception(DWORD errorCode) : std::runtime_error(MakeExceptionMessage(errorCode))
    {
    }
}
