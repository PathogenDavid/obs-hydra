#include "ActiveMonitorTracker.h"
#include "Win32Exception.h"

namespace HydraCore
{
    static ActiveMonitorTracker* instance = nullptr;

    ActiveMonitorTracker::ActiveMonitorTracker()
    {
        instance = this;

        // Set the initially active monitor
        HWND activeWindow = GetForegroundWindow();
        if (activeWindow == NULL)
        {
            activeWindow = GetDesktopWindow();
        }
        UpdateActiveMonitor(activeWindow);

        // Start the event processing thread
        threadHandle = CreateThread(NULL, 0, ActiveMonitorTracker::MonitorThreadEntry, this, 0, NULL);

        if (threadHandle == NULL)
        {
            throw Win32Exception();
        }
    }

    DWORD WINAPI ActiveMonitorTracker::MonitorThreadEntry(LPVOID _this)
    {
        ((ActiveMonitorTracker*)_this)->MonitorThreadEntry();
        return 0;
    }

    void ActiveMonitorTracker::UpdateActiveMonitor(HWND activeWindow)
    {
        // Get the active monitor from the active window
        HMONITOR newMonitor = MonitorFromWindow(activeWindow, MONITOR_DEFAULTTONEAREST);

        // Update the active monitor
        ActiveMonitorTracker* tracker = ActiveMonitorTracker::GetInstance();

        if (tracker->activeMonitor == newMonitor)
        {
            return;
        }

        tracker->activeMonitor = newMonitor;
        tracker->activeMonitorChangedEvent.Dispatch();
    }

    void CALLBACK ActiveMonitorTracker::ProcessHookEvent(HWINEVENTHOOK eventHook, DWORD event, HWND activeWindow, LONG idObject, LONG idChild, DWORD eventThread, DWORD eventTime)
    {
        UpdateActiveMonitor(activeWindow);
    }

    void ActiveMonitorTracker::MonitorThreadEntry()
    {
        // Register for events
        HWINEVENTHOOK forgroundWindowChangedEventHook = SetWinEventHook(EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_FOREGROUND, NULL, ProcessHookEvent, 0, 0, WINEVENT_OUTOFCONTEXT);
        if (forgroundWindowChangedEventHook == NULL)
        {
            throw Win32Exception();
        }

        HWINEVENTHOOK windowMovedEventHook = SetWinEventHook(EVENT_SYSTEM_MOVESIZEEND, EVENT_SYSTEM_MOVESIZEEND, NULL, ProcessHookEvent, 0, 0, WINEVENT_OUTOFCONTEXT);
        if (windowMovedEventHook == NULL)
        {
            throw Win32Exception();
        }

        // Process events
        MSG message;
        while (GetMessage(&message, nullptr, 0, 0))
        {
            TranslateMessage(&message);
            DispatchMessage(&message);
        }

        // Stop processing events
        UnhookWinEvent(forgroundWindowChangedEventHook);
        UnhookWinEvent(windowMovedEventHook);
    }

    void ActiveMonitorTracker::UnsubscribeActiveMonitorChanged(EventSubscriptionHandle subscriptionHandle)
    {
        activeMonitorChangedEvent.Unsubscribe(subscriptionHandle);
    }

    HMONITOR ActiveMonitorTracker::GetActiveMonitorHandle()
    {
        return activeMonitor;
    }

    ActiveMonitorTracker* ActiveMonitorTracker::GetInstance()
    {
        if (instance == nullptr)
        {
            new ActiveMonitorTracker();
        }

        return instance;
    }
}
