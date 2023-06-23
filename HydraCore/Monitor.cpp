#include "Monitor.h"
#include "Win32Exception.h"

#include <algorithm>
#include <stdexcept>
#include <sstream>

namespace HydraCore
{
    Monitor::Monitor(uint32_t id, HMONITOR handle, LPRECT rectangle)
    {
        this->id = id;
        this->handle = handle;
        
        // Compute the virtual display rectangle
        this->rectangle.Left = rectangle->left;
        this->rectangle.Top = rectangle->top;
        this->rectangle.Width = rectangle->right - rectangle->left;
        this->rectangle.Height = rectangle->bottom - rectangle->top;

        // Get extended monitor info
        MONITORINFOEXA monitorInfo = {};
        monitorInfo.cbSize = sizeof(monitorInfo);
        if (!GetMonitorInfoA(handle, &monitorInfo))
        {
            throw Win32Exception();
        }

        name = std::string(monitorInfo.szDevice);
        isPrimary = monitorInfo.dwFlags == MONITORINFOF_PRIMARY;

        // Get the interface ID of the monitor
        DISPLAY_DEVICEA deviceInfo = {};
        deviceInfo.cb = sizeof(deviceInfo);
        if (!EnumDisplayDevicesA(monitorInfo.szDevice, 0, &deviceInfo, EDD_GET_DEVICE_INTERFACE_NAME))
        {
            // EnumDisplayDevices doesn't set last error but is documented as only failing with bad parameters
            SetLastError(ERROR_INVALID_PARAMETER);
            throw Win32Exception();
        }

        interfaceId = std::string(deviceInfo.DeviceID);

        // Create the monitor description
        std::ostringstream descriptionBuilder;
#if 0
        descriptionBuilder << "[" << interfaceId << "]";
        descriptionBuilder << "[" << name << "]";
#else
        descriptionBuilder << "Display " << id << ": " << this->rectangle.Width << "x" << this->rectangle.Height << " @ " << this->rectangle.Left << "," << this->rectangle.Top;
        if (isPrimary)
        {
            descriptionBuilder << " (Primary)";
        }
#endif
        description = descriptionBuilder.str();
    }

    static BOOL CALLBACK GetAllMonitorsEnumerator(HMONITOR handle, HDC deviceCOntext, LPRECT rectangle, LPARAM settings)
    {
        auto list = (std::vector<Monitor>*)settings;
        list->push_back(Monitor((uint32_t)list->size(), handle, rectangle));
        return TRUE;
    }

    std::vector<Monitor> Monitor::GetAllMonitors(bool sortLeftToRight)
    {
        std::vector<Monitor> ret;

        if (!EnumDisplayMonitors(NULL, NULL, GetAllMonitorsEnumerator, (LPARAM)&ret))
        {
            throw Win32Exception();
        }

        if (sortLeftToRight)
        {
            std::sort(ret.begin(), ret.end(), [](Monitor& a, Monitor& b) { return a.GetRectangle().Left < b.GetRectangle().Left; });
        }

        return ret;
    }

    Monitor Monitor::GetPrimaryMonitor(std::vector<Monitor>& monitors)
    {
        for (Monitor& monitor : monitors)
        {
            if (monitor.IsPrimary())
            {
                return monitor;
            }
        }

        // If we got this far, for some reason Windows has nothing marked as the primary monitor
        if (monitors.size() > 0)
        {
            return monitors[0];
        }

        // If we got this far, for some reason we didn't enumerate any monitors
        throw new std::runtime_error("This system has no monitors attached.");
    }

    Monitor Monitor::GetPrimaryMonitor()
    {
        std::vector<Monitor> monitors = GetAllMonitors();
        return GetPrimaryMonitor(monitors);
    }
}
