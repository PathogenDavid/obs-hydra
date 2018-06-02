#pragma once
#include "EventHandler.h"

#include <map>
#include <mutex>

namespace HydraCore
{
    typedef char* EventSubscriptionHandle;

    class Event
    {
    private:
        std::map<EventSubscriptionHandle, EventHandlerBase*> eventHandlers;
        std::mutex eventHandlersMutex;
        EventSubscriptionHandle nextHandle;
    public:
        Event()
        {
            nextHandle = (EventSubscriptionHandle)1;
        }

        ~Event()
        {
            std::lock_guard<std::mutex> lock(eventHandlersMutex);

            for (const std::pair<EventSubscriptionHandle, EventHandlerBase*>& eventHandler : eventHandlers)
            {
                delete eventHandler.second;
            }
        }

        template<class TTarget>
        EventSubscriptionHandle Subscribe(TTarget* targetObject, typename EventHandler<TTarget>::TargetMethodType targetMethod)
        {
            std::lock_guard<std::mutex> lock(eventHandlersMutex);

            EventSubscriptionHandle handle = nextHandle;
            nextHandle++;
            eventHandlers[handle] = new EventHandler<TTarget>(targetObject, targetMethod);
            return handle;
        }

        void Unsubscribe(EventSubscriptionHandle subscriptionHandle)
        {
            std::lock_guard<std::mutex> lock(eventHandlersMutex);

            eventHandlers.erase(subscriptionHandle);
        }

        void Dispatch()
        {
            std::lock_guard<std::mutex> lock(eventHandlersMutex);

            for (const std::pair<EventSubscriptionHandle, EventHandlerBase*>& eventHandler : eventHandlers)
            {
                eventHandler.second->Dispatch();
            }
        }
    };
}
