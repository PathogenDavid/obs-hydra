#include <ActiveMonitorTracker.h>
#include <stdio.h>
#include <Win32Exception.h>

using namespace HydraCore;

class Test
{
private:
    ActiveMonitorTracker* tracker;

    void HandleNewActiveMonitor()
    {
        HMONITOR monitor = tracker->GetActiveMonitorHandle();
        MONITORINFOEX monitorInfo = {};
        monitorInfo.cbSize = sizeof(monitorInfo);

        if (!GetMonitorInfo(monitor, &monitorInfo))
        {
            throw Win32Exception();
        }

        printf("Active monitor changed: %s @ %d, %d\n", monitorInfo.szDevice, monitorInfo.rcMonitor.left, monitorInfo.rcMonitor.top);
    }
public:
    Test()
    {
        tracker = ActiveMonitorTracker::GetInstance();
        tracker->SubscribeActiveMonitorChanged(this, &Test::HandleNewActiveMonitor);
        HandleNewActiveMonitor(); // Show initial active monitor.
    }
};

int main()
{
    new Test();
    Sleep(INFINITE);
}
