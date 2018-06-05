#include "LinearAnimation.h"

#include <cmath>

namespace HydraCore
{
    LinearAnimation::LinearAnimation()
        : Animation()
    {
        JumpToPosition(0.f);
    }

    void LinearAnimation::JumpToPosition(float targetPosition)
    {
        isAnimating = false;
        currentPosition = targetPosition;
        this->targetPosition = targetPosition;
    }

    void LinearAnimation::Update(float deltaTime)
    {
        // Do nothing if we aren't currently animating
        if (!isAnimating)
        {
            return;
        }

        float distance = targetPosition - currentPosition;
        float direction = distance < 0.f ? -1.f : 1.f;
        distance = abs(distance);

        float deltaPosition = velocity * deltaTime;

        if (distance < (velocity * deltaTime))
        {
            currentPosition = targetPosition;
            isAnimating = false;
            return;
        }

        currentPosition += deltaPosition * direction;
    }

    void LinearAnimation::SetTargetPosition(float targetPosition)
    {
        this->targetPosition = targetPosition;
        isAnimating = currentPosition != targetPosition;
    }

    void LinearAnimation::SetVelocity(float velocity)
    {
        this->velocity = velocity;
    }
}
