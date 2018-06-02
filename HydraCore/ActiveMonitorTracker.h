#pragma once
#include <Windows.h>

#include "Event.h"

namespace HydraCore
{
    class ActiveMonitorTracker
    {
    private:
        Event activeMonitorChangedEvent;

        HMONITOR activeMonitor;
        HANDLE threadHandle;

        ActiveMonitorTracker();

        static DWORD WINAPI MonitorThreadEntry(LPVOID _this);
        void MonitorThreadEntry();

        static void UpdateActiveMonitor(HWND activeWindow);
        static void CALLBACK ProcessHookEvent(HWINEVENTHOOK eventHook, DWORD event, HWND activeWindow, LONG idObject, LONG idChild, DWORD eventThread, DWORD eventTime);
    public:
        template<class TTarget>
        EventSubscriptionHandle SubscribeActiveMonitorChanged(TTarget* targetObject, typename EventHandler<TTarget>::TargetMethodType targetMethod)
        {
            return activeMonitorChangedEvent.Subscribe(targetObject, targetMethod);
        }

        void UnsubscribeActiveMonitorChanged(EventSubscriptionHandle subscriptionHandle);

        HMONITOR GetActiveMonitorHandle();

        static ActiveMonitorTracker* GetInstance();
    };
}
