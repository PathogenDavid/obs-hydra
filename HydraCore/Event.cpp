#include "Event.h"

namespace HydraCore
{
    Event::~Event()
    {
        std::lock_guard<std::mutex> lock(eventHandlersMutex);

        for (const std::pair<EventSubscriptionHandle, EventHandlerBase*>& eventHandler : eventHandlers)
        {
            delete eventHandler.second;
        }
    }

    void Event::Unsubscribe(EventSubscriptionHandle subscriptionHandle)
    {
        std::lock_guard<std::mutex> lock(eventHandlersMutex);
        eventHandlers.erase(subscriptionHandle);
    }

    void Event::Dispatch()
    {
        std::lock_guard<std::mutex> lock(eventHandlersMutex);

        for (const std::pair<EventSubscriptionHandle, EventHandlerBase*>& eventHandler : eventHandlers)
        {
            eventHandler.second->Dispatch();
        }
    }
}
