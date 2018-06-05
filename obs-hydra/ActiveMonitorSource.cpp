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
#include <ActiveMonitorTracker.h>
#include <Monitor.h>
#include <obs.h>
#include <LinearAnimation.h>
#include <vector>

#define SHOW_CURSOR_PROPERTY "showCursor"
#define USE_PRIMARY_FOR_SIZE_PROPERTY "usePrimaryMonitorForSize"
#define WIDTH_PROPERTY "width"
#define HEIGHT_PROPERTY "height"

#define ANIMATION_ENABLED_PROPERTY "animationEnabled"
#define ANIMATION_SPEED_PROPERTY "animationSpeed"

#define OVERVIEW_MODE_PROPERTY "overviewMode"
#define OVERVIEW_OUTLINE_ENABLED_PROPERTY "overviewOutlineEnabled"
#define OVERVIEW_OUTLINE_THICKNESS_PROPERTY "overviewOutlineThickness"
#define OVERVIEW_OUTLINE_COLOR_PROPERTY "overviewOutlineColor"

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
    int physicalIndex;
public:
    MonitorSource(HydraCore::Monitor& monitor, bool showCursor)
        : monitor(monitor), showCursor(showCursor)
    {
        this->showCursor = showCursor;
        isEnabled = true;
        this->physicalIndex = 0;

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

    HMONITOR GetMonitorHandle()
    {
        return monitor.GetHandle();
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

    int GetPhysicalIndex()
    {
        return physicalIndex;
    }

    void SetPhysicalIndex(int physicalIndex)
    {
        this->physicalIndex = physicalIndex;
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

    bool overviewMode;
    bool overviewOutlineEnabled;
    int overviewOutlineThickness;
    vec4 overviewOutlineColor;

    uint32_t activeMonitorCount;

    HydraCore::ActiveMonitorTracker* tracker;
    HydraCore::EventSubscriptionHandle trackerEventSubscription;
    HMONITOR activeMonitorHandle;
    MonitorSource* activeMonitor;

    HydraCore::LinearAnimation animation;
    bool animationEnabled;

    gs_effect_t* solidEffect;
    gs_eparam_t* solidEffectColor;
    gs_technique_t* solidEffectTechnique;

    void ActiveMonitorChanged()
    {
        activeMonitorHandle = tracker->GetActiveMonitorHandle();

        // It is intentional that activeMonitor is not changed in the event the handle is not found in the sources collection
        // This makes it so the last known visible monitor is the one that is visible.
        for (MonitorSource* monitorSource : monitorSources)
        {
            if (monitorSource->GetMonitorHandle() == activeMonitorHandle && monitorSource->IsEnabled())
            {
                activeMonitor = monitorSource;
            }
        }

        if (animationEnabled)
        {
            animation.SetTargetPosition((float)(activeMonitor->GetPhysicalIndex() * width));
        }
    }

public:
    ActiveMonitorSource(obs_data_t* settings, obs_source_t* source)
    {
        this->source = source;

        // Initialize active monitor tracker
        tracker = HydraCore::ActiveMonitorTracker::GetInstance();
        trackerEventSubscription = tracker->SubscribeActiveMonitorChanged(this, &ActiveMonitorSource::ActiveMonitorChanged);

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

        activeMonitor = monitorSources[0];

        // Get solid effect for overview mode
        solidEffect = obs_get_base_effect(OBS_EFFECT_SOLID);
        solidEffectColor = gs_effect_get_param_by_name(solidEffect, "color");
        solidEffectTechnique = gs_effect_get_technique(solidEffect, "Solid");

        // Perform initial update
        Update(settings);

        // Jump animation to active monitor
        animation.JumpToPosition((float)(activeMonitor->GetPhysicalIndex() * width));
    }

    ~ActiveMonitorSource()
    {
        tracker->UnsubscribeActiveMonitorChanged(trackerEventSubscription);

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

        // Overview mode
        obs_properties_add_bool(ret, OVERVIEW_MODE_PROPERTY, "Overview Mode");
        obs_properties_add_bool(ret, OVERVIEW_OUTLINE_ENABLED_PROPERTY, "Overview Outline");
        obs_properties_add_int_slider(ret, OVERVIEW_OUTLINE_THICKNESS_PROPERTY, "Overview Outline Thickness", 0, 1'000, 1);
        obs_properties_add_color(ret, OVERVIEW_OUTLINE_COLOR_PROPERTY, "Overview Outline Color");

        // Animation
        obs_properties_add_bool(ret, ANIMATION_ENABLED_PROPERTY, "Enable Animation");
        obs_properties_add_float_slider(ret, ANIMATION_SPEED_PROPERTY, "Animation Speed", 0.0, 100'000.0, 1.0);

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

        obs_data_set_default_bool(settings, OVERVIEW_MODE_PROPERTY, false);
        obs_data_set_default_bool(settings, OVERVIEW_OUTLINE_ENABLED_PROPERTY, true);
        obs_data_set_default_int(settings, OVERVIEW_OUTLINE_THICKNESS_PROPERTY, 10);
        obs_data_set_default_int(settings, OVERVIEW_OUTLINE_COLOR_PROPERTY, 0xffff386d);

        obs_data_set_default_bool(settings, ANIMATION_ENABLED_PROPERTY, true);
        obs_data_set_default_double(settings, ANIMATION_SPEED_PROPERTY, 1920.0 * 4.0);
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
        activeMonitorCount = 0;
        for (MonitorSource* monitorSource : monitorSources)
        {
            bool isEnabled = obs_data_get_bool(settings, monitorSource->GetMonitorName().c_str());
            monitorSource->SetIsEnabled(isEnabled);

            if (isEnabled)
            {
                monitorSource->SetPhysicalIndex(activeMonitorCount);
                activeMonitorCount++;
            }
        }

        // Pretend that the active monitor changed in case the active monitor just became enabled
        // (Note that we don't bother changing off of the current monitor if it became disabled.)
        ActiveMonitorChanged();

        // Update overview mode
        overviewMode = obs_data_get_bool(settings, OVERVIEW_MODE_PROPERTY);
        overviewOutlineEnabled = obs_data_get_bool(settings, OVERVIEW_OUTLINE_ENABLED_PROPERTY);
        overviewOutlineThickness = (int)obs_data_get_int(settings, OVERVIEW_OUTLINE_THICKNESS_PROPERTY);
        vec4_from_rgba(&overviewOutlineColor, (uint32_t)obs_data_get_int(settings, OVERVIEW_OUTLINE_COLOR_PROPERTY));

        // Update animation
        animationEnabled = obs_data_get_bool(settings, ANIMATION_ENABLED_PROPERTY);
        animation.SetVelocity((float)obs_data_get_double(settings, ANIMATION_SPEED_PROPERTY));
    }

    uint32_t GetWidth()
    {
        if (overviewMode)
        {
            return width * activeMonitorCount;
        }

        return width;
    }

    uint32_t GetHeight()
    {
        return height;
    }

    void RenderSourceNormalized(obs_source_t* source)
    {
        gs_matrix_push();

        float sourceWidth = (float)obs_source_get_width(source);
        float sourceHeight = (float)obs_source_get_height(source);

        gs_matrix_scale3f((float)width / sourceWidth, (float)height / sourceHeight, 1.f);

        obs_source_video_render(source);

        gs_matrix_pop();
    }

    void RenderSourceNormalized(MonitorSource* source)
    {
        RenderSourceNormalized(source->GetSource());
    }

    void RenderOverviewMode()
    {
        gs_matrix_push();

        for (MonitorSource* monitorSource : monitorSources)
        {
            if (!monitorSource->IsEnabled())
            {
                continue;
            }

            RenderSourceNormalized(monitorSource);

            gs_matrix_translate3f((float)width, 0.f, 0.f);
        }

        gs_matrix_pop();

        if (overviewOutlineEnabled)
        {
            gs_matrix_push();

            gs_matrix_translate3f(animation.GetCurrentPosition(), 0.f, 0.f);

            gs_effect_set_vec4(solidEffectColor, &overviewOutlineColor);

            gs_technique_begin(solidEffectTechnique);
            gs_technique_begin_pass(solidEffectTechnique, 0);

            // Top
            gs_draw_sprite(nullptr, 0, width, overviewOutlineThickness);

            // Left
            gs_draw_sprite(nullptr, 0, overviewOutlineThickness, height);
            
            // Right
            gs_matrix_push();
            gs_matrix_translate3f((float)(width - overviewOutlineThickness), 0.f, 0.f);
            gs_draw_sprite(nullptr, 0, overviewOutlineThickness, height);
            gs_matrix_pop();

            // Bottom
            gs_matrix_translate3f(0.f, (float)(height - overviewOutlineThickness), 0.f);
            gs_draw_sprite(nullptr, 0, width, overviewOutlineThickness);

            gs_technique_end_pass(solidEffectTechnique);
            gs_technique_end(solidEffectTechnique);

            gs_matrix_pop();
        }
    }

    void RenderNormalMode()
    {
        if (!animation.IsAnimating())
        {
            RenderSourceNormalized(activeMonitor);
            return;
        }
        
        gs_matrix_push();

        gs_matrix_translate3f(-animation.GetCurrentPosition(), 0.f, 0.f);

        for (MonitorSource* monitorSource : monitorSources)
        {
            if (!monitorSource->IsEnabled())
            {
                continue;
            }

            RenderSourceNormalized(monitorSource);

            gs_matrix_translate3f((float)width, 0.f, 0.f);
        }

        gs_matrix_pop();
    }

    void VideoRender(gs_effect_t* effect)
    {
        if (overviewMode)
        {
            RenderOverviewMode();
        }
        else
        {
            RenderNormalMode();
        }
    }

    void VideoTick(float deltaTime)
    {
        animation.Update(deltaTime);
    }

    void EnumSources(obs_source_enum_proc_t enumCallback, void* param, bool activeOnly)
    {
        // If we're only enumerating active monitors, aren't in overview mode, and aren't animating; then just report the active monitor.
#if false // For some reason this isn't working as expected. I think it's OBS's fault.
        if (activeOnly && !overviewMode && !animation.IsAnimating())
        {
            enumCallback(source, activeMonitor->GetSource(), param);
            return;
        }
#endif

        for (MonitorSource* monitorSource : monitorSources)
        {
            // Skip disabled monitors
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
            ->WithVideoTick(&VideoTick)
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
