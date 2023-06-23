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
        std::string interfaceId;
        std::string name;
        std::string description;
        Rectangle rectangle;
        bool isPrimary;
    public:
        Monitor(uint32_t id, HMONITOR handle, LPRECT rectangle);

        inline HMONITOR GetHandle()
        {
            return handle;
        }

        inline uint32_t GetId()
        {
            return id;
        }
        
        inline std::string GetInterfaceId()
        {
            return interfaceId;
        }

        inline std::string GetName()
        {
            return name;
        }

        inline std::string GetDescription()
        {
            return description;
        }

        inline Rectangle GetRectangle()
        {
            return rectangle;
        }

        inline uint32_t GetWidth()
        {
            return rectangle.Width;
        }

        inline uint32_t GetHeight()
        {
            return rectangle.Height;
        }

        inline bool IsPrimary()
        {
            return isPrimary;
        }

        static std::vector<Monitor> GetAllMonitors(bool sortLeftToRight = false);
        static Monitor GetPrimaryMonitor();
        static Monitor GetPrimaryMonitor(std::vector<Monitor>& monitors);
    };
}
