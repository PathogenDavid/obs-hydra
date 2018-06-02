#pragma once

class EventHandlerBase
{
public:
    virtual void Dispatch() = 0;
};

template<class TTarget>
class EventHandler : public EventHandlerBase
{
public:
    typedef void (TTarget::*TargetMethodType)();
private:
    TTarget * targetObject;
    TargetMethodType targetMethod;
public:
    EventHandler(TTarget* targetObject, TargetMethodType targetMethod)
        : targetObject(targetObject), targetMethod(targetMethod)
    {
    }

    virtual void Dispatch()
    {
        (targetObject->*targetMethod)();
    }
};
