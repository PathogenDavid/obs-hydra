#pragma once
#include "EventHandler.h"

#include <map>

namespace HydraCore
{
    typedef char* EventSubscriptionHandle;

    class Event
    {
    private:
        std::map<EventSubscriptionHandle, EventHandlerBase*> eventHandlers;
        EventSubscriptionHandle nextHandle;
    public:
        Event()
        {
            nextHandle = (EventSubscriptionHandle)1;
        }

        ~Event()
        {
            for (const std::pair<EventSubscriptionHandle, EventHandlerBase*>& eventHandler : eventHandlers)
            {
                delete eventHandler.second;
            }
        }

        template<class TTarget>
        EventSubscriptionHandle Subscribe(TTarget* targetObject, typename EventHandler<TTarget>::TargetMethodType targetMethod)
        {
            EventSubscriptionHandle handle = nextHandle;
            nextHandle++;
            eventHandlers[handle] = new EventHandler<TTarget>(targetObject, targetMethod);
            return handle;
        }

        void Unsubscribe(EventSubscriptionHandle subscriptionHandle)
        {
            eventHandlers.erase(subscriptionHandle);
        }

        void Dispatch()
        {
            for (const std::pair<EventSubscriptionHandle, EventHandlerBase*>& eventHandler : eventHandlers)
            {
                eventHandler.second->Dispatch();
            }
        }
    };
}
