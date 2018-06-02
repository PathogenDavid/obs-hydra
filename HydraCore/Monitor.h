#pragma once
#include <string>
#include <vector>
#include <Windows.h>

#include "Rectangle.h"

namespace HydraCore
{
    class Monitor
    {
    private:
        HMONITOR handle;
        uint32_t id;
        std::string name;
        std::string description;
        Rectangle rectangle;
        bool isPrimary;
    public:
        Monitor(uint32_t id, HMONITOR handle, LPRECT rectangle);
        uint32_t GetId();
        std::string GetName();
        std::string GetDescription();
        Rectangle GetRectangle();
        uint32_t GetWidth();
        uint32_t GetHeight();
        bool IsPrimary();

        static std::vector<Monitor> GetAllMonitors(bool sortLeftToRight = false);
        static Monitor GetPrimaryMonitor();
        static Monitor GetPrimaryMonitor(std::vector<Monitor>& monitors);
    };
}
