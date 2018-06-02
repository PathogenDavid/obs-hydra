/*---------------------------------------------------------------------
Copyright (C) 2018  Pathogen Studios <opensource@pathogenstudios.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
---------------------------------------------------------------------*/
#include "ActiveMonitorSource.h"
#include "ObsSourceDefinition.h"

#include <algorithm>
#include <Monitor.h>
#include <obs.h>
#include <vector>

#define SHOW_CURSOR_PROPERTY "showCursor"
#define USE_PRIMARY_FOR_SIZE_PROPERTY "usePrimaryMonitorForSize"
#define WIDTH_PROPERTY "width"
#define HEIGHT_PROPERTY "height"

#define MONITOR_CAPTURE_ID_PROPERTY "monitor"
#define MONITOR_CAPTURE_CURSOR_PROPERTY "capture_cursor"

class MonitorSource
{
private:
    obs_source_t* source;
    obs_data_t* settings;
    HydraCore::Monitor monitor;
    bool showCursor;
    bool isEnabled;
public:
    MonitorSource(HydraCore::Monitor& monitor, bool showCursor)
        : monitor(monitor), showCursor(showCursor)
    {
        this->showCursor = showCursor;
        isEnabled = true;

        // Create a data collection to hold the source's settings
        // We can't share this between sources because OBS will use it internally for the source, meaning each source will have the same settings data.
        // (See obs.c:1808 - obs_data_newref is used, only adding a reference - not cloning the settings.)
        settings = obs_data_create();
        obs_data_set_int(settings, MONITOR_CAPTURE_ID_PROPERTY, monitor.GetId());
        obs_data_set_bool(settings, MONITOR_CAPTURE_CURSOR_PROPERTY, showCursor);

        source = obs_source_create_private("monitor_capture", monitor.GetDescription().c_str(), settings);
    }

    void SetShowCursor(bool showCursor)
    {
        if (this->showCursor == showCursor)
        { return; }

        this->showCursor = showCursor;
        obs_data_set_bool(settings, MONITOR_CAPTURE_CURSOR_PROPERTY, showCursor);

        obs_source_update(source, settings);
    }

    std::string GetMonitorName()
    {
        return monitor.GetName();
    }

    obs_source_t* GetSource()
    {
        return source;
    }

    bool IsEnabled()
    {
        return isEnabled;
    }

    void SetIsEnabled(bool enabled)
    {
        isEnabled = enabled;
    }

    ~MonitorSource()
    {
        obs_source_release(source);
        obs_data_release(settings);
    }
};

class ActiveMonitorSource
{
private:
    obs_source_t* source;
    std::vector<MonitorSource*> monitorSources;
    
    bool showCursor;

    uint32_t width;
    uint32_t height;

public:
    ActiveMonitorSource(obs_data_t* settings, obs_source_t* source)
    {
        this->source = source;

        // Create all monitor sources that we might need
        // Instead of dnymaically creating/destroying them, we just create them all at once.
        // OBS does not seem to properly handle the monitor set changing anyway
        // Additionally, not creating sources ahead of time causes some weird behavior in the preview window.
        // I suspect this is because OBS does not reenumerate our child sources when we are in preview mode, but I did not investigate very far.
        showCursor = obs_data_get_bool(settings, SHOW_CURSOR_PROPERTY);

        for (HydraCore::Monitor& monitor : HydraCore::Monitor::GetAllMonitors(true))
        {
            monitorSources.push_back(new MonitorSource(monitor, showCursor));
        }

        // Perform initial update
        Update(settings);
    }

    ~ActiveMonitorSource()
    {
        for (MonitorSource* monitorSource : monitorSources)
        {
            delete monitorSource;
        }
    }

private:
    static bool UsePrimaryForSizePropertyModified(obs_properties_t* properties, obs_property_t* property, obs_data_t* settings)
    {
        bool enableSizeProperties = !obs_data_get_bool(settings, USE_PRIMARY_FOR_SIZE_PROPERTY);
        obs_property_set_enabled(obs_properties_get(properties, WIDTH_PROPERTY), enableSizeProperties);
        obs_property_set_enabled(obs_properties_get(properties, HEIGHT_PROPERTY), enableSizeProperties);
        return true;
    }

    obs_properties_t* GetProperties()
    {
        obs_properties_t* ret = obs_properties_create();

        obs_properties_add_bool(ret, SHOW_CURSOR_PROPERTY, "Show Cursor");
        
        // Source size settings
        obs_property_t* usePrimaryForSize = obs_properties_add_bool(ret, USE_PRIMARY_FOR_SIZE_PROPERTY, "Use primary monitor size");
        obs_property_set_modified_callback(usePrimaryForSize, UsePrimaryForSizePropertyModified);
        
        obs_properties_add_int(ret, WIDTH_PROPERTY, "Width", 1, 4096, 1);
        obs_properties_add_int(ret, HEIGHT_PROPERTY, "Height", 1, 4096, 1);

        // Monitor selectors
        std::vector<HydraCore::Monitor> monitors = HydraCore::Monitor::GetAllMonitors(true);
        for (HydraCore::Monitor& monitor : monitors)
        {
            obs_properties_add_bool(ret, monitor.GetName().c_str(), monitor.GetDescription().c_str());
        }

        return ret;
    }

    static void GetDefaults(obs_data_t* settings)
    {
        std::vector<HydraCore::Monitor> monitors = HydraCore::Monitor::GetAllMonitors();
        HydraCore::Monitor primaryMonitor = HydraCore::Monitor::GetPrimaryMonitor(monitors);
        
        obs_data_set_default_bool(settings, SHOW_CURSOR_PROPERTY, true);
        obs_data_set_default_bool(settings, USE_PRIMARY_FOR_SIZE_PROPERTY, true);

        obs_data_set_default_int(settings, WIDTH_PROPERTY, primaryMonitor.GetWidth());
        obs_data_set_default_int(settings, HEIGHT_PROPERTY, primaryMonitor.GetHeight());

        for (HydraCore::Monitor& monitor : monitors)
        {
            obs_data_set_default_bool(settings, monitor.GetName().c_str(), true);
        }
    }

    void Update(obs_data_t* settings)
    {
        // Update showCursor
        bool newShowCursor = obs_data_get_bool(settings, SHOW_CURSOR_PROPERTY);
        if (newShowCursor != showCursor)
        {
            showCursor = newShowCursor;

            for (MonitorSource* monitorSource : monitorSources)
            {
                monitorSource->SetShowCursor(showCursor);
            }
        }

        // Get the size
        if (obs_data_get_bool(settings, USE_PRIMARY_FOR_SIZE_PROPERTY))
        {
            HydraCore::Monitor primaryMonitor = HydraCore::Monitor::GetPrimaryMonitor();
            width = primaryMonitor.GetWidth();
            height = primaryMonitor.GetHeight();
        }
        else
        {
            width = (uint32_t)obs_data_get_int(settings, WIDTH_PROPERTY);
            height = (uint32_t)obs_data_get_int(settings, HEIGHT_PROPERTY);
        }

        // Update which monitors are enabled
        for (MonitorSource* monitorSource : monitorSources)
        {
            bool isEnabled = obs_data_get_bool(settings, monitorSource->GetMonitorName().c_str());
            monitorSource->SetIsEnabled(isEnabled);
        }
    }

    uint32_t GetWidth()
    {
        return width;
    }

    uint32_t GetHeight()
    {
        return height;
    }

    void VideoRender(gs_effect_t* effect)
    {
        int i = 0;
        float monitorCount = (float)std::count_if(monitorSources.begin(), monitorSources.end(), [](MonitorSource* monitor) { return monitor->IsEnabled(); });
        float monitorSize = ((float)GetWidth()) / monitorCount;
        for (MonitorSource* monitorSource : monitorSources)
        {
            if (!monitorSource->IsEnabled())
            {
                continue;
            }

            gs_matrix_push();
            gs_matrix_translate3f(monitorSize * (float)i, 0.f, 0.f);
            gs_matrix_scale3f(1.f / monitorCount, 1.f, 1.f);

            obs_source_video_render(monitorSource->GetSource());

            gs_matrix_pop();
            i++;
        }
    }

    void EnumSources(obs_source_enum_proc_t enumCallback, void* param, bool activeOnly)
    {
        for (MonitorSource* monitorSource : monitorSources)
        {
            if (activeOnly && !monitorSource->IsEnabled())
            {
                continue;
            }

            enumCallback(source, monitorSource->GetSource(), param);
        }
    }

    void EnumActiveSources(obs_source_enum_proc_t enumCallback, void* param)
    {
        EnumSources(enumCallback, param, true);
    }

    void EnumAllSources(obs_source_enum_proc_t enumCallback, void* param)
    {
        EnumSources(enumCallback, param, false);
    }

public:
    static void Register()
    {
        (new ObsSourceDefinition<ActiveMonitorSource>("obs-hydra-active-monitor-source", "Active Monitor Source"))
            // General configuration
            ->WithType(OBS_SOURCE_TYPE_INPUT)
            ->WithOutputFlag(OBS_SOURCE_VIDEO)
            ->WithOutputFlag(OBS_SOURCE_CUSTOM_DRAW)
            ->WithOutputFlag(OBS_SOURCE_DO_NOT_DUPLICATE)
            //->WithOutputFlag(OBS_SOURCE_COMPOSITE)
            // As far as the definition presented by the documentation is concerned, we are a composite source.
            //  However, from what I can find: All this really does (as of OBS 21.1.2) is enable audio.
            //  I imagine this functionality could be extended in the future (like actually allowing child sources) but for now I'm leaving it off.
            // Properties
            ->WithGetDefaults(&GetDefaults)
            ->WithGetProperties(&GetProperties)
            ->WithUpdate(&Update)
            // Rendering
            ->WithGetWidth(&GetWidth)
            ->WithGetHeight(&GetHeight)
            ->WithVideoRender(&VideoRender)
            // Source enumeration
            ->WithEnumActiveSources(&EnumActiveSources)
            ->WithEnumAllSources(&EnumAllSources)
            // Register
            ->Register();
    }
};

void RegisterActiveMonitorSource()
{
    ActiveMonitorSource::Register();
}
