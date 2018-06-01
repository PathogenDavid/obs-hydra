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

#include <obs.h>

#define MONITOR_COUNT 3

class ActiveMonitorSource
{
private:
    obs_source_t* source;
    obs_source_t* monitors[MONITOR_COUNT];

public:
    ActiveMonitorSource(obs_data_t* settings, obs_source_t* source)
    {
        this->source = source;

        obs_data_t* captureSettings = obs_data_create();
        obs_data_set_bool(captureSettings, "capture_cursor", true);
        for (int i = 0; i < MONITOR_COUNT; i++)
        {
            obs_data_set_int(captureSettings, "monitor", i);
            monitors[i] = obs_source_create_private("monitor_capture", nullptr, captureSettings);
        }
        obs_data_release(captureSettings);
    }

    ~ActiveMonitorSource()
    {
        for (int i = 0; i < MONITOR_COUNT; i++)
        {
            obs_source_release(monitors[i]);
        }
    }

private:
    uint32_t GetWidth()
    {
        return 1920;
    }

    uint32_t GetHeight()
    {
        return 1080;
    }

    void VideoRender(gs_effect_t* effect)
    {
        for (int i = 0; i < MONITOR_COUNT; i++)
        {
            gs_matrix_push();
            gs_matrix_translate3f((1920.f / ((float)MONITOR_COUNT)) * ((float)i), 0.f, 0.f);
            gs_matrix_scale3f(1.f / ((float)MONITOR_COUNT), 1.f, 1.f);

            obs_source_video_render(monitors[i]);

            gs_matrix_pop();
        }
    }

    void EnumSources(obs_source_enum_proc_t enumCallback, void* param, bool activeOnly)
    {
        for (int i = 0; i < MONITOR_COUNT; i++)
        {
            enumCallback(source, monitors[i], param);
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
            ->WithType(OBS_SOURCE_TYPE_INPUT)
            ->WithOutputFlag(OBS_SOURCE_VIDEO)
            ->WithOutputFlag(OBS_SOURCE_CUSTOM_DRAW)
            ->WithOutputFlag(OBS_SOURCE_DO_NOT_DUPLICATE)
            //->WithOutputFlag(OBS_SOURCE_COMPOSITE)
            // As far as the definition presented by the documentation is concerned, we are a composite source.
            //  However, from what I can find: All this really does (as of OBS 21.1.2) is enable audio.
            //  I imagine this functionality could be extended in the future (like actually allowing child sources) but for now I'm leaving it off.
            ->WithGetWidth(&GetWidth)
            ->WithGetHeight(&GetHeight)
            ->WithVideoRender(&VideoRender)
            ->WithEnumActiveSources(&EnumActiveSources)
            ->WithEnumAllSources(&EnumAllSources)
            ->Register();
    }
};

void RegisterActiveMonitorSource()
{
    ActiveMonitorSource::Register();
}
