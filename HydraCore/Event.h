#pragma once
#include "EventHandler.h"

#include <map>
#include <mutex>

namespace HydraCore
{
    typedef uintptr_t EventSubscriptionHandle;

    class Event
    {
    private:
        std::map<EventSubscriptionHandle, EventHandlerBase*> eventHandlers;
        std::mutex eventHandlersMutex;
        EventSubscriptionHandle nextHandle;
    public:
        inline Event()
        {
            nextHandle = 1;
        }

        ~Event();

        template<class TTarget>
        EventSubscriptionHandle Subscribe(TTarget* targetObject, typename EventHandler<TTarget>::TargetMethodType targetMethod)
        {
            std::lock_guard<std::mutex> lock(eventHandlersMutex);

            EventSubscriptionHandle handle = nextHandle;
            nextHandle++;
            eventHandlers[handle] = new EventHandler<TTarget>(targetObject, targetMethod);
            return handle;
        }

        void Unsubscribe(EventSubscriptionHandle subscriptionHandle);
        void Dispatch();
    };
}
