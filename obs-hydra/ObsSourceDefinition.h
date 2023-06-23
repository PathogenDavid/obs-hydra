/*---------------------------------------------------------------------
Copyright (C) 2018  David Maas

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
#pragma once
#include <obs.h>

template<class TSource>
struct ObsSourceMethods
{
    // General source info
    typedef uint32_t (TSource::*GetWidth)();
    typedef uint32_t (TSource::*GetHeight)();

    // Properties
    typedef void (TSource::*Update)(obs_data_t* settings);
    typedef obs_properties_t* (TSource::*GetProperties)();
    typedef void (TSource::*Save)(obs_data_t* settings);
    typedef void (TSource::*Load)(obs_data_t* settings);

    // Source State
    typedef void (TSource::*Activate)();
    typedef void (TSource::*Deactivate)();
    typedef void (TSource::*Show)();
    typedef void (TSource::*Hide)();

    // Video
    typedef void (TSource::*VideoTick)(float seconds);
    typedef void (TSource::*VideoRender)(gs_effect_t* effect);

    // User Interaction
    typedef void (TSource::*Focus)(bool focus);
    typedef void (TSource::*MouseClick)(const obs_mouse_event* event, int32_t type, bool mouseUp, uint32_t clickCount);
    typedef void (TSource::*MouseMove)(const obs_mouse_event* event, bool mouseLeave);
    typedef void (TSource::*MouseWheel)(const obs_mouse_event* event, int xDelta, int yDelta);
    typedef void (TSource::*KeyClick)(const obs_key_event* event, bool keyUp);

    // Child Sources
    typedef void (TSource::*EnumActiveSources)(obs_source_enum_proc_t enumCallback, void* param);
    typedef void (TSource::*EnumAllSources)(obs_source_enum_proc_t enumCallback, void* param);

    // Audio
    typedef bool (TSource::*AudioRender)(uint64_t* tsOut, obs_source_audio_mix* audioOutput, uint32_t mixers, size_t channels, size_t sampleRate);

    // Filters
    typedef obs_source_frame* (TSource::*FilterVideo)(obs_source_frame* frame);
    typedef obs_audio_data* (TSource::*FilterAudio)(obs_audio_data* audio);
    typedef void (TSource::*FilterRemove)(obs_source_t* source);

    // Transitions
    typedef void (TSource::*TransitionStart)();
    typedef void (TSource::*TransitionStop)();

    //-------------------------------------------------------------------------

    // Property Defaults (Non-Instance)
    typedef void(*GetDefaults)(obs_data_t* settings);
};

template<class TSource>
class ObsSourceDefinition
{
private:
    struct ObsSourceInstance
    {
        TSource* source;
        ObsSourceDefinition<TSource>* definition;
    };

    obs_source_info sourceInfo;
    const char* name;
public:
    ObsSourceDefinition(const char* id, const char* name)
    {
        this->name = name;

        sourceInfo = {};
        sourceInfo.id = id;
        sourceInfo.type_data = (void*)this;

        sourceInfo.get_name = get_name;
        sourceInfo.create = create;
        sourceInfo.destroy = destroy;
        sourceInfo.free_type_data = free_type_data;

        sourceInfo.type = OBS_SOURCE_TYPE_INPUT;
        // OBS_SOURCE_COMPOSITE - As far as the definition presented by the documentation is concerned, we are a composite source.
        //  However, from what I can find: All this really does (as of OBS 21.1.2) is enable audio.
        //  I imagine this functionality could be extended in the future (like actually allowing child sources) but for now I'm leaving it off.
        sourceInfo.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_CUSTOM_DRAW | OBS_SOURCE_DO_NOT_DUPLICATE;

        sourceInfo.icon_type = OBS_ICON_TYPE_DESKTOP_CAPTURE;
    }

    ObsSourceDefinition<TSource>* WithType(obs_source_type type)
    {
        sourceInfo.type = type;
        return this;
    }

    ObsSourceDefinition<TSource>* WithOutputFlag(uint32_t outputFlag)
    {
        sourceInfo.output_flags |= outputFlag;
        return this;
    }

    void Register()
    {
        obs_register_source(&sourceInfo);
    }

    //-------------------------------------------------------------------------
    // Trampoline Methods
    //-------------------------------------------------------------------------
private:
    static const char* get_name(void* typeData)
    {
        return ((ObsSourceDefinition<TSource>*)typeData)->name;
    }

    static void* create(obs_data_t* settings, obs_source_t* source)
    {
        ObsSourceInstance* ret = new ObsSourceInstance();
        ret->source = new TSource(settings, source);
        ret->definition = (ObsSourceDefinition<TSource>*)obs_source_get_type_data(source);
        return ret;
    }

    static void destroy(void* data)
    {
        ObsSourceInstance* instance = (ObsSourceInstance*)data;
        delete instance->source;
        delete instance;
    }

    static void free_type_data(void* typeData)
    {
        delete (ObsSourceDefinition<TSource>*)typeData;
    }

    static void get_defaults2(void* typeData, obs_data_t* settings)
    {
        ((ObsSourceDefinition<TSource>*)typeData)->GetDefaults(settings);
    }

    //-------------------------------------------------------------------------

    static uint32_t get_width(void* _data)
    {
        ObsSourceInstance* data = (ObsSourceInstance*)_data;
        return (data->source->*(data->definition->GetWidth))();
    }

    static uint32_t get_height(void* _data)
    {
        ObsSourceInstance* data = (ObsSourceInstance*)_data;
        return (data->source->*(data->definition->GetHeight))();
    }

    static void update(void* _data, obs_data_t* settings)
    {
        ObsSourceInstance* data = (ObsSourceInstance*)_data;
        return (data->source->*(data->definition->Update))(settings);
    }

    static obs_properties_t* get_properties(void* _data)
    {
        ObsSourceInstance* data = (ObsSourceInstance*)_data;
        return (data->source->*(data->definition->GetProperties))();
    }

    static void save(void* _data, obs_data_t* settings)
    {
        ObsSourceInstance* data = (ObsSourceInstance*)_data;
        return (data->source->*(data->definition->Save))(settings);
    }

    static void load(void* _data, obs_data_t* settings)
    {
        ObsSourceInstance* data = (ObsSourceInstance*)_data;
        return (data->source->*(data->definition->Load))(settings);
    }

    static void activate(void* _data)
    {
        ObsSourceInstance* data = (ObsSourceInstance*)_data;
        return (data->source->*(data->definition->Activate))();
    }

    static void deactivate(void* _data)
    {
        ObsSourceInstance* data = (ObsSourceInstance*)_data;
        return (data->source->*(data->definition->Deactivate))();
    }

    static void show(void* _data)
    {
        ObsSourceInstance* data = (ObsSourceInstance*)_data;
        return (data->source->*(data->definition->Show))();
    }

    static void hide(void* _data)
    {
        ObsSourceInstance* data = (ObsSourceInstance*)_data;
        return (data->source->*(data->definition->Hide))();
    }

    static void video_tick(void* _data, float seconds)
    {
        ObsSourceInstance* data = (ObsSourceInstance*)_data;
        return (data->source->*(data->definition->VideoTick))(seconds);
    }

    static void video_render(void* _data, gs_effect_t* effect)
    {
        ObsSourceInstance* data = (ObsSourceInstance*)_data;
        return (data->source->*(data->definition->VideoRender))(effect);
    }

    static void focus(void* _data, bool focus)
    {
        ObsSourceInstance* data = (ObsSourceInstance*)_data;
        return (data->source->*(data->definition->Focus))(focus);
    }

    static void mouse_click(void* _data, const obs_mouse_event* event, int32_t type, bool mouse_up, uint32_t click_count)
    {
        ObsSourceInstance* data = (ObsSourceInstance*)_data;
        return (data->source->*(data->definition->MouseClick))(event, type, mouse_up, click_count);
    }

    static void mouse_move(void* _data, const obs_mouse_event* event, bool mouse_leave)
    {
        ObsSourceInstance* data = (ObsSourceInstance*)_data;
        return (data->source->*(data->definition->MouseMove))(event, mouse_leave);
    }

    static void mouse_wheel(void* _data, const obs_mouse_event* event, int x_delta, int y_delta)
    {
        ObsSourceInstance* data = (ObsSourceInstance*)_data;
        return (data->source->*(data->definition->MouseWheel))(event, x_delta, y_delta);
    }

    static void key_click(void* _data, const obs_key_event* event, bool key_up)
    {
        ObsSourceInstance* data = (ObsSourceInstance*)_data;
        return (data->source->*(data->definition->KeyClick))(event, key_up);
    }

    static void enum_active_sources(void* _data, obs_source_enum_proc_t enum_callback, void* param)
    {
        ObsSourceInstance* data = (ObsSourceInstance*)_data;
        return (data->source->*(data->definition->EnumActiveSources))(enum_callback, param);
    }

    static void enum_all_sources(void* _data, obs_source_enum_proc_t enum_callback, void* param)
    {
        ObsSourceInstance* data = (ObsSourceInstance*)_data;
        return (data->source->*(data->definition->EnumAllSources))(enum_callback, param);
    }

    static bool audio_render(void* _data, uint64_t* ts_out, obs_source_audio_mix* audio_output, uint32_t mixers, size_t channels, size_t sample_rate)
    {
        ObsSourceInstance* data = (ObsSourceInstance*)_data;
        return (data->source->*(data->definition->AudioRender))(ts_out, audio_output, mixers, channels, sample_rate);
    }

    static obs_source_frame* filter_video(void* _data, obs_source_frame* frame)
    {
        ObsSourceInstance* data = (ObsSourceInstance*)_data;
        return (data->source->*(data->definition->FilterVideo))(frame);
    }

    static obs_audio_data* filter_audio(void* _data, obs_audio_data* audio)
    {
        ObsSourceInstance* data = (ObsSourceInstance*)_data;
        return (data->source->*(data->definition->FilterAudio))(audio);
    }

    static void filter_remove(void* _data, obs_source_t* source)
    {
        ObsSourceInstance* data = (ObsSourceInstance*)_data;
        return (data->source->*(data->definition->FilterRemove))(source);
    }

    static void transition_start(void* _data)
    {
        ObsSourceInstance* data = (ObsSourceInstance*)_data;
        return (data->source->*(data->definition->TransitionStart))();
    }

    static void transition_stop(void* _data)
    {
        ObsSourceInstance* data = (ObsSourceInstance*)_data;
        return (data->source->*(data->definition->TransitionStop))();
    }

    //-------------------------------------------------------------------------
    // Registered Method Fields
    //-------------------------------------------------------------------------
private:
    typename ObsSourceMethods<TSource>::GetWidth GetWidth = nullptr;
    typename ObsSourceMethods<TSource>::GetHeight GetHeight = nullptr;
    typename ObsSourceMethods<TSource>::Update Update = nullptr;
    typename ObsSourceMethods<TSource>::GetProperties GetProperties = nullptr;
    typename ObsSourceMethods<TSource>::Save Save = nullptr;
    typename ObsSourceMethods<TSource>::Load Load = nullptr;
    typename ObsSourceMethods<TSource>::Activate Activate = nullptr;
    typename ObsSourceMethods<TSource>::Deactivate Deactivate = nullptr;
    typename ObsSourceMethods<TSource>::Show Show = nullptr;
    typename ObsSourceMethods<TSource>::Hide Hide = nullptr;
    typename ObsSourceMethods<TSource>::VideoTick VideoTick = nullptr;
    typename ObsSourceMethods<TSource>::VideoRender VideoRender = nullptr;
    typename ObsSourceMethods<TSource>::Focus Focus = nullptr;
    typename ObsSourceMethods<TSource>::MouseClick MouseClick = nullptr;
    typename ObsSourceMethods<TSource>::MouseMove MouseMove = nullptr;
    typename ObsSourceMethods<TSource>::MouseWheel MouseWheel = nullptr;
    typename ObsSourceMethods<TSource>::KeyClick KeyClick = nullptr;
    typename ObsSourceMethods<TSource>::EnumActiveSources EnumActiveSources = nullptr;
    typename ObsSourceMethods<TSource>::EnumAllSources EnumAllSources = nullptr;
    typename ObsSourceMethods<TSource>::AudioRender AudioRender = nullptr;
    typename ObsSourceMethods<TSource>::FilterVideo FilterVideo = nullptr;
    typename ObsSourceMethods<TSource>::FilterAudio FilterAudio = nullptr;
    typename ObsSourceMethods<TSource>::FilterRemove FilterRemove = nullptr;
    typename ObsSourceMethods<TSource>::TransitionStart TransitionStart = nullptr;
    typename ObsSourceMethods<TSource>::TransitionStop TransitionStop = nullptr;

    typename ObsSourceMethods<TSource>::GetDefaults GetDefaults = nullptr;

    //-------------------------------------------------------------------------
    // Method Registration
    //-------------------------------------------------------------------------
public:

    ObsSourceDefinition<TSource>* WithGetWidth(typename ObsSourceMethods<TSource>::GetWidth implementation)
    {
        GetWidth = implementation;
        sourceInfo.get_width = get_width;
        return this;
    }

    ObsSourceDefinition<TSource>* WithGetHeight(typename ObsSourceMethods<TSource>::GetHeight implementation)
    {
        GetHeight = implementation;
        sourceInfo.get_height = get_height;
        return this;
    }

    ObsSourceDefinition<TSource>* WithUpdate(typename ObsSourceMethods<TSource>::Update implementation)
    {
        Update = implementation;
        sourceInfo.update = update;
        return this;
    }

    ObsSourceDefinition<TSource>* WithGetProperties(typename ObsSourceMethods<TSource>::GetProperties implementation)
    {
        GetProperties = implementation;
        sourceInfo.get_properties = get_properties;
        return this;
    }

    ObsSourceDefinition<TSource>* WithSave(typename ObsSourceMethods<TSource>::Save implementation)
    {
        Save = implementation;
        sourceInfo.save = save;
        return this;
    }

    ObsSourceDefinition<TSource>* WithLoad(typename ObsSourceMethods<TSource>::Load implementation)
    {
        Load = implementation;
        sourceInfo.load = load;
        return this;
    }

    ObsSourceDefinition<TSource>* WithActivate(typename ObsSourceMethods<TSource>::Activate implementation)
    {
        Activate = implementation;
        sourceInfo.activate = activate;
        return this;
    }

    ObsSourceDefinition<TSource>* WithDeactivate(typename ObsSourceMethods<TSource>::Deactivate implementation)
    {
        Deactivate = implementation;
        sourceInfo.deactivate = deactivate;
        return this;
    }

    ObsSourceDefinition<TSource>* WithShow(typename ObsSourceMethods<TSource>::Show implementation)
    {
        Show = implementation;
        sourceInfo.show = show;
        return this;
    }

    ObsSourceDefinition<TSource>* WithHide(typename ObsSourceMethods<TSource>::Hide implementation)
    {
        Hide = implementation;
        sourceInfo.hide = hide;
        return this;
    }

    ObsSourceDefinition<TSource>* WithVideoTick(typename ObsSourceMethods<TSource>::VideoTick implementation)
    {
        VideoTick = implementation;
        sourceInfo.video_tick = video_tick;
        return this;
    }

    ObsSourceDefinition<TSource>* WithVideoRender(typename ObsSourceMethods<TSource>::VideoRender implementation)
    {
        VideoRender = implementation;
        sourceInfo.video_render = video_render;
        return this;
    }

    ObsSourceDefinition<TSource>* WithFocus(typename ObsSourceMethods<TSource>::Focus implementation)
    {
        Focus = implementation;
        sourceInfo.focus = focus;
        return this;
    }

    ObsSourceDefinition<TSource>* WithMouseClick(typename ObsSourceMethods<TSource>::MouseClick implementation)
    {
        MouseClick = implementation;
        sourceInfo.mouse_click = mouse_click;
        return this;
    }

    ObsSourceDefinition<TSource>* WithMouseMove(typename ObsSourceMethods<TSource>::MouseMove implementation)
    {
        MouseMove = implementation;
        sourceInfo.mouse_move = mouse_move;
        return this;
    }

    ObsSourceDefinition<TSource>* WithMouseWheel(typename ObsSourceMethods<TSource>::MouseWheel implementation)
    {
        MouseWheel = implementation;
        sourceInfo.mouse_wheel = mouse_wheel;
        return this;
    }

    ObsSourceDefinition<TSource>* WithKeyClick(typename ObsSourceMethods<TSource>::KeyClick implementation)
    {
        KeyClick = implementation;
        sourceInfo.key_click = key_click;
        return this;
    }

    ObsSourceDefinition<TSource>* WithEnumActiveSources(typename ObsSourceMethods<TSource>::EnumActiveSources implementation)
    {
        EnumActiveSources = implementation;
        sourceInfo.enum_active_sources = enum_active_sources;
        return this;
    }

    ObsSourceDefinition<TSource>* WithEnumAllSources(typename ObsSourceMethods<TSource>::EnumAllSources implementation)
    {
        EnumAllSources = implementation;
        sourceInfo.enum_all_sources = enum_all_sources;
        return this;
    }

    ObsSourceDefinition<TSource>* WithAudioRender(typename ObsSourceMethods<TSource>::AudioRender implementation)
    {
        AudioRender = implementation;
        sourceInfo.audio_render = audio_render;
        return this;
    }

    ObsSourceDefinition<TSource>* WithFilterVideo(typename ObsSourceMethods<TSource>::FilterVideo implementation)
    {
        FilterVideo = implementation;
        sourceInfo.filter_video = filter_video;
        return this;
    }

    ObsSourceDefinition<TSource>* WithFilterAudio(typename ObsSourceMethods<TSource>::FilterAudio implementation)
    {
        FilterAudio = implementation;
        sourceInfo.filter_audio = filter_audio;
        return this;
    }

    ObsSourceDefinition<TSource>* WithFilterRemove(typename ObsSourceMethods<TSource>::FilterRemove implementation)
    {
        FilterRemove = implementation;
        sourceInfo.filter_remove = filter_remove;
        return this;
    }

    ObsSourceDefinition<TSource>* WithTransitionStart(typename ObsSourceMethods<TSource>::TransitionStart implementation)
    {
        TransitionStart = implementation;
        sourceInfo.transition_start = transition_start;
        return this;
    }

    ObsSourceDefinition<TSource>* WithTransitionStop(typename ObsSourceMethods<TSource>::TransitionStop implementation)
    {
        TransitionStop = implementation;
        sourceInfo.transition_stop = transition_stop;
        return this;
    }

    ObsSourceDefinition<TSource>* WithGetDefaults(typename ObsSourceMethods<TSource>::GetDefaults implementation)
    {
        GetDefaults = implementation;
        sourceInfo.get_defaults2 = get_defaults2;
        return this;
    }
};
