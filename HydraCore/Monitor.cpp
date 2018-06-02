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
        MONITORINFOEX monitorInfo = {};
        monitorInfo.cbSize = sizeof(monitorInfo);
        if (!GetMonitorInfo(handle, &monitorInfo))
        {
            throw Win32Exception();
        }

        name = std::string(monitorInfo.szDevice);
        isPrimary = monitorInfo.dwFlags == MONITORINFOF_PRIMARY;

        // Create the monitor description
        std::ostringstream descriptionBuilder;
        descriptionBuilder << "Display " << id << ": " << this->rectangle.Width << "x" << this->rectangle.Height << " @ " << this->rectangle.Left << "," << this->rectangle.Top;
        if (isPrimary)
        {
            descriptionBuilder << " (Primary)";
        }
        description = descriptionBuilder.str();
    }

    HMONITOR Monitor::GetHandle()
    {
        return handle;
    }

    uint32_t Monitor::GetId()
    {
        return id;
    }

    std::string Monitor::GetName()
    {
        return name;
    }

    std::string Monitor::GetDescription()
    {
        return description;
    }

    Rectangle Monitor::GetRectangle()
    {
        return rectangle;
    }

    uint32_t Monitor::GetWidth()
    {
        return rectangle.Width;
    }

    uint32_t Monitor::GetHeight()
    {
        return rectangle.Height;
    }

    bool Monitor::IsPrimary()
    {
        return isPrimary;
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
